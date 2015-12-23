#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "UberCassette.h"
#include "WAV.h"
#include "Sample.h"
#include "Acorn.h"

struct Acorn_CassetteBlock *gAcorn_CassetteBlockList = NULL;

const unsigned char cDummyByte[] = { 0,0,0, 1,1, 0,0,1,0,0,1,0,0,1,0,0,1,-1 };

#define MAX_WAVELENGTH 2000.0f

unsigned char *Acorn_ParseWAV( unsigned char *wavData )
{
	unsigned char *tParsedData = NULL;
	struct RAW_Format tRawData;

	// First off, extract the RAW data.
	tRawData = WAV_GetRAW( wavData, true );

	// First we scan the entire file for the wavelength of the header.
	BuildWavelengthTable( tRawData, 0, -1, gWaveLengthTable );

	// Work out our thresholds....
	BuildThresholds( tRawData, 2, &gThresholds[ 0 ], gWaveLengthTable, gSample_ValueTable );

	if ( gAlgorithmType == TRIGGER )
		BuildRawWaveList( tRawData, 0, NULL, -1, false );
	else 
		BuildRawWaveList_Cleaned( tRawData, 0, NULL, -1, false );

	// Now we have lovely 8-bit mono raw data.
	Acorn_BuildTapeBlocks( tRawData );

	return (unsigned char *)1;
}

void Acorn_BuildTapeBlocks( struct RAW_Format format )
{
	int tPointer = 0;
	int *tRawPointer = &gRawCycles[ tPointer ];
	int tHeaderCycles = -1, tHeaderStart = -1, tDummyCycles = -1;
	enum State { STATE_NOSIGNAL, STATE_HEADERSTART, STATE_DUMMYBYTE, STATE_HEADER, STATE_TRAILER, STATE_DATA, STATE_DATA_WAITING_START_BIT, STATE_DATA_WAITING_STOP_BIT } tState = STATE_NOSIGNAL;
	unsigned char tBytes[ 128 * 1024 ];
	signed long tValue;
	char *tBytePointer = &tBytes[ 0 ];
	int tBits = 0, tHalfBits = 0, tParity = 0;
	int tDelayCycles = 0;

	*tBytePointer = 0;

    //printf( "Building Acorn blocks. Size is %ld.\n", gRawLength );

	while ( tPointer < gRawLength )
	{
		if ( tPointer == 172 )
		{
			bool test=true;
		}

		if ( *tRawPointer < NUMBER_OF_THRESHOLDS )
			tValue = gSample_ValueTable[ (int)*tRawPointer ];
		else
			tValue = -1; // Invalid wave length.

		switch ( tState )
		{
		case STATE_NOSIGNAL:
		{
			tDelayCycles += (int)*tRawPointer;
			if ( tValue == 0 )
			{
				tHeaderCycles++;
				if ( tHeaderCycles > 1 )	// The first wave is rarely formed correctly.
				{
					tState = STATE_HEADERSTART;
					tHeaderStart = tPointer - 3;
					tDummyCycles = 3;
					//tDelayCycles -= (int)*(tRawPointer-1);
					//tDelayCycles -= (int)*(tRawPointer-2);
				}
			}
			else
			{
				if ( tDelayCycles < 0 )
				{
					printf( "Error in Acorn cycles.\n" );
				}
				tHeaderCycles = 0;
				tDummyCycles = 0;
//				printf( " Adding %d cycles (%f Seconds).\n", tDelayCycles, (float)tDelayCycles / (float)gWAVFormat.sampleRate );
			}
			break;
		}
		case STATE_HEADERSTART:
		{
			if ( tValue == cDummyByte[ tDummyCycles ] )
			{
				tDummyCycles++;
				if ( tValue != 0 )
					tHeaderCycles = 0;
			}
			else if ( tValue == 0 )
			{
				tHeaderCycles++;
				tDummyCycles = 0;
			}
			else
			{
				tState = STATE_NOSIGNAL;
				break;
			}

			if ( tDummyCycles > 16 )
			{
				tState = STATE_DUMMYBYTE;
				Acorn_AddPause( tDelayCycles );
				tDelayCycles = 0;		
			}
			else if ( tHeaderCycles > 7 )
			{
				int *tHeaderPointer = tRawPointer - 8;
				// Valid header!
				tState = STATE_HEADER;
				tHeaderStart = tPointer - 8;

				Acorn_AddPause( tDelayCycles );
				tDelayCycles = 0;		
			}
			break;
		}
		case STATE_DUMMYBYTE:
		case STATE_HEADER:
		{
			if ( tValue == 0 )
			{
				tHeaderCycles++;
			}
			else
			{
				if ( tState == STATE_DUMMYBYTE && tValue == 1 )
				{
					if ( tHeaderCycles < 8 )
					{
						// It's a dummy byte. Takes a while to settle down.
						tHeaderCycles = 0;
						break;
					}
				}
				// End of the header!
				if ( tState == STATE_DUMMYBYTE )
					Acorn_AddHeaderCycles( tHeaderCycles, true );
				else
					Acorn_AddHeaderCycles( tHeaderCycles, false );
					
				if ( tValue == 1 )
					tState = STATE_DATA;
				else
					tState = STATE_NOSIGNAL;

				tHeaderCycles = 0;
				tBits = 0;
				tHalfBits = 0;
			}
			break;
		}
		case STATE_DATA:
		{
			// We've done the start bit (0).
			if ( tValue == 0 )
			{
				// 1 bit.
				tHalfBits = 1 - tHalfBits;
				if ( tHalfBits == 0 )
				{
					*tBytePointer |= ( 1 << tBits );
					tBits++;
					tParity = 1 - tParity;
				}
			}
			else
			{
				tHalfBits = 0;
				tBits++;
			}

			if ( tBits == 8 )
			{
				tBytePointer++;
				*tBytePointer = 0;
				tBits = 0;
				tState = STATE_DATA_WAITING_STOP_BIT;
			}
			break;
		}
		case STATE_DATA_WAITING_START_BIT:
		{
			if ( tValue == 0 )
			{
				if ( tBytePointer != (char *)tBytes )
				{
					Acorn_AddDataBlock( tBytePointer - (char *)tBytes, tBytes );
					tHeaderCycles = 0;
					tState = STATE_TRAILER;
					tBytePointer = tBytes;
				}
				else
					tState = STATE_NOSIGNAL;
			}
			else
				tState = STATE_DATA;
			break;
		}
		case STATE_TRAILER:
		{
			if ( tValue == -1 || *tRawPointer > MAX_WAVELENGTH )//*(tRawPointer+1) > MAX_WAVELENGTH || *(tRawPointer+2) > MAX_WAVELENGTH || *(tRawPointer+3) > MAX_WAVELENGTH || *(tRawPointer+4) > MAX_WAVELENGTH || *(tRawPointer+5) > MAX_WAVELENGTH)
			{
				tState = STATE_NOSIGNAL;
				Acorn_AddHeaderCycles( tHeaderCycles, false );
				tHeaderCycles = 0;
				tDelayCycles = 0;
			}
			else if ( tValue == 0 )
			{
				tHeaderCycles++;
			}
			else if ( tValue == 1 )
			{
				Acorn_AddHeaderCycles( tHeaderCycles, false );
				tHeaderCycles = 0;	

				if ( tValue == 1 )
					tState = STATE_DATA;
			}
			break;
		}
		case STATE_DATA_WAITING_STOP_BIT:
		{
			if ( tValue == 0 )
			{
				tHalfBits = 1 - tHalfBits;
				if ( tHalfBits == 0 )
				{
					tState = STATE_DATA_WAITING_START_BIT;
				}
			}
			else
				tState = STATE_NOSIGNAL;
			break;
		}
		default:
			break;
		}

		tRawPointer++;
		tPointer++;
	}
	
	printf( "Done.\n" );
}

void Acorn_AddHeaderCycles( int cycles, bool dummy )
{
//	printf( "Found header of %d cycles.\n", cycles );
	if ( dummy )
		Acorn_AddBlock( ACBT_DUMMYBYTE, cycles, NULL );
	else
		Acorn_AddBlock( ACBT_HEADER, cycles, NULL );
}

void Acorn_AddDataBlock( int size, char *data )
{
	unsigned char *tBuffer;

	tBuffer = (unsigned char *)malloc( size );
	if ( tBuffer == NULL )
    {
        printf( "Out of memory in Acorn_AddDataBlock.\n" );
        return;
	}
    memcpy( tBuffer, data, size );

	Acorn_AddBlock( ACBT_DATA, size, tBuffer );
}

void Acorn_AddBlock( enum Acorn_CassetteBlockType type, int size, unsigned char *data )
{
	struct Acorn_CassetteBlock *tNode = gAcorn_CassetteBlockList;

	while ( tNode )
	{
		if ( tNode->next )
			tNode = tNode->next;
		else
			break;
	}

	if ( gAcorn_CassetteBlockList == NULL )
		tNode = gAcorn_CassetteBlockList = (struct Acorn_CassetteBlock *)malloc( sizeof( struct Acorn_CassetteBlock ) );
	else
	{
		tNode->next = (struct Acorn_CassetteBlock *)malloc( sizeof( struct Acorn_CassetteBlock ) );
		tNode = tNode->next;
	}

	tNode->type = type;
	tNode->sizeUnion.size = size;
	tNode->data = data;
	tNode->next = NULL;
}

void Acorn_AddPause( int size )
{
	float tLengthInSeconds = (float)(ConvertFromCyclesToSeconds( size ));

	Acorn_AddBlock( ACBT_PAUSE, *((int*)&tLengthInSeconds), NULL );

}
