/* Handler for CBM tapes. */

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "UberCassette.h"
#include "WAV.h"
#include "Sample.h"
#include "CBM.h"

// Normal CBM tapes measure pulse length on crossing the zero boundary from + to -.

void CBM_BuildTapeBlocks( struct RAW_Format format );
struct CBM_CassetteBlock *CBM_FindTapeBlock( struct RAW_Format format, double *pointer );


struct CBM_CassetteBlock *gCBM_CassetteBlockList = NULL;
const float cMinimumCurveGradient = 0.02f;



unsigned char *CBM_ParseWAV( unsigned char *wavData )
{
	unsigned char *tParsedData = NULL;
	struct RAW_Format tRawData;

	// First off, extract the RAW data.
	tRawData = WAV_GetRAW( wavData, false );

    if ( tRawData.data == NULL )
        return NULL;

	// First we scan the entire file for the wavelength of the header.
    printf( "Building wavelength table.\n" );
	BuildWavelengthTable( tRawData, 0, -1, gWaveLengthTable );

	// Work out our thresholds....
    printf( "Building thresholds.\n" );
	BuildThresholds( tRawData, 3, &gThresholds[ 0 ], gWaveLengthTable, gSample_ValueTable );

	if ( gOutputType == TAP || gOutputType == WAV )
	{
		if ( gAlgorithmType == TRIGGER )
			BuildRawWaveList( tRawData, 0, NULL, -1, false );
		else 
			BuildRawWaveList_Cleaned( tRawData, 0, NULL, -1, false );
		return (unsigned char *)1; // We have data ... but it's somewhere else.
	}

	// Now we have lovely 8-bit mono raw data.
	CBM_BuildTapeBlocks( tRawData );

	return tParsedData;
}


void CBM_BuildTapeBlocks( struct RAW_Format format )
{
	struct CBM_CassetteBlock *tBlock, *tLastBlock;
	double tPointer = 1;

	gCBM_CassetteBlockList = NULL;

	// Now we look for our data.
	while ( tPointer && (tBlock = CBM_FindTapeBlock( format, &tPointer )) > 0 )
	{
		if ( gCBM_CassetteBlockList == NULL )
		{
			gCBM_CassetteBlockList = (struct CBM_CassetteBlock *)malloc( sizeof( struct CBM_CassetteBlock ) );
			*gCBM_CassetteBlockList = *tBlock;
			gCBM_CassetteBlockList->next = NULL;
			tLastBlock = gCBM_CassetteBlockList;
		}
		else
		{
			tLastBlock->next = (struct CBM_CassetteBlock *)malloc( sizeof( struct CBM_CassetteBlock ) );
			tLastBlock = tLastBlock->next;
			*tLastBlock = *tBlock;
			tLastBlock->next = NULL;
		}
	}
}

void CBM_ReadData( unsigned char *data, int length, struct RAW_Format format, double *pointer )
{
	double tPointer = *pointer;
	bool tValidWave = true, tDoneSync = true;
	double tWaveLength;
	unsigned char tLastByte = 0, tChecksum = 0;
	int tBytes = 0, tBits = 0, tCycle = 0, tLastCycle = 0, tParity = 0, tBlockNumber = 0;
	unsigned char *tData[ 2 ];
	int tLoggedByte = 0;

	tData[ 0 ] = (unsigned char *)malloc( length );
	tData[ 1 ] = (unsigned char *)malloc( length );

	while ( tBlockNumber < 2 )
	{
		tPointer = CBM_FindHeader( format, tPointer );
		tBytes = -9;

		if ( tPointer == 0 )
		{
			*pointer = 0;
			free( tData[ 0 ] );
			free( tData[ 1 ] );
			return;
		}
		//printf( "\nReading block at %ld.\n", tPointer );

//		printf( "0000:  " );

		tCycle = 0; tDoneSync = true; tBits = 0; tParity = 0; tChecksum = 0; tLastByte = 0; tLoggedByte = 0;

		while ( tValidWave && tBytes < length )
		{
			tChecksum = 0;
			tWaveLength = FindWavelength( format, &tPointer, false, false, false, NULL );

			if ( gSample_ValueTable[ (int)tWaveLength ] != -1 )
			{
				if ( gSample_ValueTable[ (int)tWaveLength ] == 2 )
				{
					//printf( "Sync byte %d at %d.   ", tBytes, tPointer );
					if ( tBits < 8 && tBits != 0 )
					{
						//printf( "*** ERROR *** Sync byte out of place.\n" );
					}
					tLastByte = 0;
					tBits = 0;
					tParity = 0;
					tDoneSync = true;
					tCycle = 0;
					//printf( "         %04x:  ", tBytes );
				}
				else if ( tDoneSync == true )
				{
					tDoneSync = false;
				}
				else
				{
					if ( tCycle == 0 )
					{
						tLastCycle = gSample_ValueTable[ (int)tWaveLength ];
	//					if ( tBits < 8 )
	//						printf( "%d", tLastCycle );
					}
					else
					{
						if ( tBits == 8 )
						{
	//						printf( " = %02x   ", tLastByte );
							// Check parity!
							if ( tLastCycle == 0 )
							{
								if ( gSample_ValueTable[ (int)tWaveLength ] == 1 )
								{
									if ( tParity == 0 )
										;//printf( "\n*** Parity error on byte %ld ***.\n", tPointer );
								}
							}
							else
							{
								if ( gSample_ValueTable[ (int)tWaveLength ] == 0  )
								{
									if ( tParity == 1 )
										;//printf( "\n*** Parity error on byte %ld ***.\n", tPointer );
								}
							}

							if ( tBytes >= 0 )
							{
								tData[ tBlockNumber ][ tBytes ] = tLastByte;
/*								if ( tBytes < 256 )
								{
									printf( " %02x", tLastByte );
									tLoggedByte++;
									if ( tLoggedByte == 16 )
									{
										printf( "\n%04x:  ", tBytes+1 );
										tLoggedByte = 0;								
									}
								}*/
								tChecksum ^= tLastByte;
							}
							tBytes++;
							tBits = 9;
						}
						else
						{
	//						printf( "%d", gSample_ValueTable[ tWaveLength ] );

							if ( tLastCycle == 0 )
							{
								if ( gSample_ValueTable[ (int)tWaveLength ] == 1 )
									tLastByte |= 0;
								else
								{
									//printf( "*** ERROR *** found two short cycles." );
								}
							}
							else
							{
								if ( gSample_ValueTable[ (int)tWaveLength ] == 0 )
								{
									tLastByte |= 1 << tBits;
									tParity = 1 - tParity;
								}
								else
								{
									//printf( "*** ERROR *** found two medium cycles." );
								}
							}
							tBits++;
							if ( tBits > 8 )
							{
								//printf( "*** ERROR *** long byte.\n" );
							}
						}	
					}
					//printf( "%d", gSample_ValueTable[ tWaveLength ] );
					tCycle = 1 - tCycle;
				}
			}
			else
				tValidWave = false;
		}

		if ( tValidWave == false )
			break;

		tBlockNumber++;
		tLastCycle = 0;
	}

	for ( tBytes = 0 ; tBytes < length ; tBytes++ )
	{
		if ( tData[ 0 ][ tBytes ] != tData[ 1 ][ tBytes ] )
			;//printf( "Data error on byte %d: is %02x and %02x.\n", tBytes, tData[ 0 ][ tBytes ], tData[ 1 ][ tBytes ] );

		data[ tBytes ] = tData[ 0 ][ tBytes ];		
	}

	free( tData[ 0 ] );
	free( tData[ 1 ] );

	// Skip the trailing zeroes.
	tPointer = CBM_FindHeader( format, tPointer);

	*pointer = tPointer;
}


struct CBMHeader CBM_ReadHeader( struct RAW_Format format, double *pointer )
{
	struct CBMHeader tHeader;
	unsigned char tData[ 192 ];
	int i;

//	printf( "\n %04x:  ", tBytes );

	CBM_ReadData( &tData[ 0 ], 192, format, pointer );

	if ( *pointer == 0 )
	{
		tHeader.startAddress = tHeader.endAddress = 0;
		return tHeader;
	}
	tHeader.startAddress = tData[ 0x01 ] + tData[ 0x02 ] * 0x0100;
	tHeader.endAddress = tData[ 0x03 ] + tData[ 0x04 ] * 0x0100;
	for ( i=0 ; i < 16 ; i++ )
		tHeader.name[ i ] = tData[ 0x05 + i ];

	printf( "Found header for block %16s, start address is 0x%04x, length is 0x%04x.\n", tHeader.name, tHeader.startAddress, tHeader.endAddress - tHeader.startAddress + 1 );

	return tHeader;	
}

struct CBM_CassetteBlock *CBM_FindTapeBlock( struct RAW_Format format, double *pointer )
{
	double tPointer = *pointer;
	struct CBM_CassetteBlock *tBlock = NULL;
	struct CBMHeader tHeader;
	int i;

	// We have a header! Do stuff!
	tBlock = (struct CBM_CassetteBlock *)malloc( sizeof( struct CBM_CassetteBlock ) );
	tHeader = CBM_ReadHeader( format, &tPointer );
	if ( tPointer == 0 )
		return NULL;
	tBlock->startAddress = CorrectEndianShort( tHeader.startAddress );
	tBlock->endAddress = CorrectEndianShort( tHeader.endAddress );
	tBlock->c64sFileType = 0x01;
	tBlock->cbm1541FileType = 0x82;

	for ( i=0 ; i<16 ; i++ )
		tBlock->filename[ i ] = tHeader.name[ i ];

	tBlock->data = (unsigned char *)malloc( tBlock->endAddress - tBlock->startAddress + 1 );
	CBM_ReadData( tBlock->data, tBlock->endAddress - tBlock->startAddress + 1, format, &tPointer );

	*pointer = tPointer;
	return tBlock;
}

double CBM_FindHeader( struct RAW_Format format, double pointer )
{
	double tPointer = pointer;
	bool tNoHeader = true;
	int tZeroCounter = 0;
	double tWaveLength = 0;

	while ( tNoHeader )
	{
		tPointer = FindNextWavelengthStart( format, tPointer );

		if ( tPointer == -1 )
			return 0;	// No headers here, move along please.

		// Find a valid blip.
		tWaveLength = FindWavelength( format, &tPointer, false, false, false, NULL );
		AddWave( NULL, 1, tWaveLength );

		if ( tPointer == -1 )
			return 0;	// No headers here, move along please.

		if ( tWaveLength == 0 || gSample_ValueTable[ (int)tWaveLength ] != 0 )
			continue;	// Not a header wave after all. Darn it.

		// We've got a zero!
		tZeroCounter = 0;
		while ( gSample_ValueTable[ (int)tWaveLength ] == 0 && tPointer < format.size )
		{
			tZeroCounter++;
			tPointer++;

			if ( tPointer == format.size )
				return 0;

			tWaveLength = FindWavelength( format, &tPointer, false, false, false, NULL );
//			printf( " %d ", tPointer );
		}

		if ( tZeroCounter >= gHeaderCheckSize )
		{
			tNoHeader = false;
		}
	}

	// We have found a header!

	return tPointer;
}

