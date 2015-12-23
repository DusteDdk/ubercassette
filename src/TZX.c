#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UberCassette.h"
#include "Sample.h"

#include "Spectrum.h"

#define SAMPLE_RATE 44100 * 4

struct TZX_TZXInfo
{
	char identifier[ 8 ];
	unsigned char majorRevision;
	unsigned char minorRevision;
};

struct TZX_CSWInfo
{
	unsigned long length;
	unsigned short pause;
	unsigned char sampleRate[ 3 ];
	unsigned char compression;
	unsigned long pulses;
};

void TZX_WriteHeader( FILE *file, int *data, unsigned long length )
{
	struct TZX_TZXInfo tTZXInfo;
	int tByteLength = 0;
	unsigned long tPointer = 0;

	sprintf( tTZXInfo.identifier, "ZXTape!", gMachineData.string );
	tTZXInfo.identifier[ 7 ] = 0x1A;

	tTZXInfo.majorRevision = 1;
	tTZXInfo.minorRevision = 20;

	fwrite( &tTZXInfo, sizeof( struct TZX_TZXInfo ), 1, file );
}

void TZX_AddDataBlock( struct Spectrum_CassetteBlock *node, FILE *file )
{
	unsigned char tBuffer[ 7 ];
	unsigned short tPauseLength, tDataLength, tBlockLength;
	unsigned char *tPointer = tBuffer;

	tPauseLength = (unsigned short)(((float)node->pause / (float)gMachineData.speed_PAL) * 1000.0f);
	tDataLength = node->size;
	tBlockLength = 4 + tDataLength;

	*tPointer++ = 0x10; // Data.
//	tBuffer[ 1 ] = tBlockLength & 0xFF;
//	tBuffer[ 2 ] = (tBlockLength & 0xFF00) >> 8;
	*tPointer++ = tPauseLength & 0xFF;
	*tPointer++ = (tPauseLength & 0xFF00) >> 8;
	*tPointer++ = tDataLength & 0xFF;
	*tPointer++ = (tDataLength & 0xFF00) >> 8;

	fwrite( tBuffer, tPointer - tBuffer, 1, file );
	fwrite( node->data, tDataLength, 1, file );
	free( node->data );
}

void TZX_AddPureDataBlock( struct Spectrum_CassetteBlock *node, FILE *file )
{
	unsigned char tBuffer[ 12 ];
	unsigned short tPauseLength, tDataLength, tBlockLength;
	unsigned char *tPointer = tBuffer;
	struct LoaderStruct *tLoader = node->loader;

	tPauseLength = (unsigned short)(((float)node->pause / (float)gMachineData.speed_PAL) * 1000.0f);
	tDataLength = node->size;
	tBlockLength = 10 + tDataLength;

	*tPointer++ = 0x14; // Data.
	*tPointer++ = tLoader->zeroPulse & 0xFF;
	*tPointer++ = (tLoader->zeroPulse & 0xFF00) >> 8;
	*tPointer++ = tLoader->onePulse & 0xFF;
	*tPointer++ = (tLoader->onePulse & 0xFF00) >> 8;
	*tPointer++ = tLoader->bitsInLastByte & 0xFF;
	*tPointer++ = tPauseLength & 0xFF;
	*tPointer++ = (tPauseLength & 0xFF00) >> 8;
	*tPointer++ = tDataLength & 0xFF;
	*tPointer++ = (tDataLength & 0xFF00) >> 8;
	*tPointer++ = (tDataLength & 0xFF0000) >> 16;

	fwrite( tBuffer, tPointer - tBuffer, 1, file );
	fwrite( node->data, tDataLength, 1, file );
	if ( node->loader )
		free( node->loader );
	free( node->data );
}


void TZX_AddTurboDataBlock( struct Spectrum_CassetteBlock *node, FILE *file )
{
	unsigned char tBuffer[ 20 ];
	unsigned short tPauseLength, tBlockLength;
	int tDataLength;
	unsigned char *tPointer = tBuffer;
	struct LoaderStruct *tLoader = node->loader;

	tPauseLength = (unsigned short)(((float)node->pause / (float)gMachineData.speed_PAL) * 1000.0f);
	tDataLength = node->size;
	tBlockLength = 0x12 + tDataLength;

	*tPointer++ = 0x11; // Data.
	*tPointer++ = tLoader->pilotPulse & 0xFF;
	*tPointer++ = (tLoader->pilotPulse & 0xFF00) >> 8;
	*tPointer++ = tLoader->sync1 & 0xFF;
	*tPointer++ = (tLoader->sync1 & 0xFF00) >> 8;
	*tPointer++ = tLoader->sync2 & 0xFF;
	*tPointer++ = (tLoader->sync2 & 0xFF00) >> 8;
	*tPointer++ = tLoader->zeroPulse & 0xFF;
	*tPointer++ = (tLoader->zeroPulse & 0xFF00) >> 8;
	*tPointer++ = tLoader->onePulse & 0xFF;
	*tPointer++ = (tLoader->onePulse & 0xFF00) >> 8;
	*tPointer++ = tLoader->pilotLength & 0xFF;
	*tPointer++ = (tLoader->pilotLength & 0xFF00) >> 8;
	*tPointer++ = tLoader->bitsInLastByte & 0xFF;
	*tPointer++ = tPauseLength & 0xFF;
	*tPointer++ = (tPauseLength & 0xFF00) >> 8;
	*tPointer++ = tDataLength & 0xFF;
	*tPointer++ = (tDataLength & 0xFF00) >> 8;
	*tPointer++ = (tDataLength & 0xFF0000) >> 16;

	fwrite( tBuffer, tPointer - tBuffer, 1, file );
	fwrite( node->data, tDataLength, 1, file );
	if ( node->loader )
		free( node->loader );
	free( node->data );
}


void TZX_AddPauseBlock( struct Spectrum_CassetteBlock *node, FILE *file )
{
	unsigned char tBuffer[ 6 ], *tPointer;
	unsigned short tLength, tBlockLength;

	tPointer = tBuffer;
	tLength = (unsigned short)(((float)node->size / (float)gMachineData.speed_PAL) * 1000.0f);
	tBlockLength = 2;

	*tPointer++ = 0x20; // Pause.
	*tPointer++ = tLength & 0xFF;
	*tPointer++ = (tLength & 0xFF00) >> 8;

	fwrite( tBuffer, tPointer - tBuffer, 1, file );
	
}

void TZX_AddPulseBlock( struct Spectrum_CassetteBlock *node, FILE *file )
{
	unsigned char tBuffer[ 6 ], *tPointer;
	unsigned short tLength, tBlockLength, tPulseLength;
	int i;

	tPointer = tBuffer;
	tLength = node->pause;
	tPulseLength = node->size;

	tBlockLength = 1 + tLength * 2;

	*tPointer++ = 0x13; // Pause.
	*tPointer++ = tLength & 0xFF;

	fwrite( tBuffer, tPointer - tBuffer, 1, file );

	for ( i=0 ; i<tLength ; i++ )
	{
		tPointer = &tBuffer[ 0 ];
		*tPointer++ = tPulseLength & 0xFF;
		*tPointer++ = (tPulseLength & 0xFF00) >> 8;
		fwrite( tBuffer, tPointer - tBuffer, 1, file );
	}
}	

void TZX_AddToneBlock( struct Spectrum_CassetteBlock *node, FILE *file )
{
	unsigned char tBuffer[ 6 ], *tPointer;
	unsigned short tLength, tBlockLength, tPulseLength;

	tPointer = tBuffer;
	tLength = node->size;
	tPulseLength = node->pause;

	tBlockLength = 1 + tLength * 2;

	*tPointer++ = 0x12; // Pause.
	*tPointer++ = tPulseLength & 0xFF;
	*tPointer++ = (tPulseLength & 0xFF00) >> 8;
	*tPointer++ = tLength & 0xFF;
	*tPointer++ = (tLength & 0xFF00) >> 8;

	fwrite( tBuffer, tPointer - tBuffer, 1, file );
}	


void TZX_WriteBlocks( struct Spectrum_CassetteBlock *node, FILE *file )
{
	struct Spectrum_CassetteBlock *tNode = node;

	while ( tNode )
	{
		switch ( tNode->type )
		{
		case SCBT_PULSE:
			TZX_AddPulseBlock( tNode, file );
			break;
		case SCBT_TONE:
			TZX_AddToneBlock( tNode, file );
			break;
		case SCBT_PAUSE:
			TZX_AddPauseBlock( tNode, file );
			break;
		case SCBT_DATA:
			TZX_AddDataBlock( tNode, file );
			break;
		case SCBT_TURBODATA:
			TZX_AddTurboDataBlock( tNode, file );
			break;
		case SCBT_PUREDATA:
			TZX_AddPureDataBlock( tNode, file );
			break;
		default:
			break;
		}

		tNode = tNode->next;
	}
}

void TZX_Output( struct Spectrum_CassetteBlock *list, char *filename )
{
	FILE *tOutputFile = NULL;
	struct Spectrum_CassetteBlock *tBlock = list;

	unsigned long tLength = gRawLength;

    printf( "Writing TZX file.\n" );

	tOutputFile = fopen( filename, "wb" );
	if ( tOutputFile == NULL )
	{
		printf( "Couldn't open output file.\n" );
		return;
	}

	TZX_WriteHeader( tOutputFile, gRawCycles, tLength );
	TZX_WriteBlocks( gSpectrum_CassetteBlockList, tOutputFile );			

	fclose( tOutputFile );
	
	printf( "Written TZX file.\n" );
}

unsigned char *TZX_Parse( unsigned char *data, int size )
{
	int tIndex = 0;
	unsigned char *tPointer = data;

	tIndex = sizeof( struct TZX_TZXInfo );
	tPointer = &data[ tIndex ];

	for ( tIndex = 0 ; tIndex < size ; tIndex++ )
	{
		if ( *tPointer == 0x00 )
		{
			int tDelay = 0x00000000;

			tDelay = *(tPointer+1);
			tDelay += *(tPointer+2) << 8;
			tDelay += *(tPointer+3) << 16;
			tPointer += 3;

			AddMachineCycle( tDelay );
		}
		else
		{
			AddMachineCycle( *tPointer * 8 );	
		}

		tPointer++;
	}
	return (unsigned char *)1;
}
