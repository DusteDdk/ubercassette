#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UberCassette.h"
#include "Sample.h"

#include "Spectrum.h"
#include "CBM.h"
#include "CSW.h"

#define SAMPLE_RATE (44100)

struct CSW_CSWInfo
{
	char identifier[ 23 ];
	unsigned char majorRevision;
	unsigned char minorRevision;
	unsigned char sampleRate[ 4 ];
	unsigned char pulses[ 4 ];
	unsigned char compression;
	unsigned char flags;
	unsigned char header;
	char encoder[ 16 ];
};

void CSW_WriteHeader( FILE *file, unsigned long pulses )
{
	struct CSW_CSWInfo tCSWInfo;
	int tByteLength = 0;
	unsigned long tPointer = 0;

	sprintf( tCSWInfo.identifier, "Compressed Square Wave%c", 0x1A );
	tCSWInfo.majorRevision = 2;
	tCSWInfo.minorRevision = 0;
	*((unsigned long *)&tCSWInfo.sampleRate[ 0 ]) = CorrectEndianLong( SAMPLE_RATE );
	*((unsigned long *)&tCSWInfo.pulses[ 0 ]) = CorrectEndianLong( pulses );
	tCSWInfo.compression = 1;
	tCSWInfo.flags = 0;
	tCSWInfo.header = 0;
	sprintf( tCSWInfo.encoder, "UberCassette" );

	fwrite( &tCSWInfo, sizeof( struct CSW_CSWInfo ), 1, file );
}

void CSW_WriteBlocks( FILE *file, int *data, unsigned long length )
{
	unsigned long tPointer = 0;
	unsigned long tCycles;
	int tDataLength = 0;
	//signed char tDistances[ 3 ];
	unsigned char *tBuffer;

	tBuffer = (unsigned char *)malloc( 10 * 1024 * 1024 );
	if (tBuffer == NULL )
	{
		printf( "Couldn't allocate CSW workspace.\n" );
		return;
	}
	while ( tPointer < length )
	{
		tCycles = (unsigned long)(ConvertFromCyclesToSeconds( data[ tPointer ] ) * (float)SAMPLE_RATE);
		if ( tCycles > 0xFF )
		{
			tBuffer[ tDataLength++ ] = 0x00;
			tBuffer[ tDataLength++ ]= tCycles & 0xFF;
			tBuffer[ tDataLength++ ]= (unsigned char )((tCycles & 0xFF00) >> 8);
			tBuffer[ tDataLength++ ]= (unsigned char )((tCycles & 0xFF0000) >> 16);
			tBuffer[ tDataLength++ ]= (unsigned char )((tCycles & 0xFF000000) >> 24);
		}
		else
		{
			tBuffer[ tDataLength++ ] = (unsigned char )tCycles;
		}
		tPointer++;
	}

	CSW_WriteHeader( file, length );
	fwrite( tBuffer, tDataLength, 1, file );

	free( tBuffer );
}

void CSW_Output( struct Spectrum_CassetteBlock *list, char *filename )
{
	FILE *tOutputFile = NULL;
	struct Spectrum_CassetteBlock *tBlock = list;

	unsigned long tLength = gRawLength;

    printf( "Writing CSW file.\n" );

	tOutputFile = fopen( filename, "wb" );
	if ( tOutputFile == NULL )
	{
		printf( "Couldn't open output file.\n" );
		return;
	}

	CSW_WriteBlocks( tOutputFile, gRawCycles, tLength );			

	fclose( tOutputFile );
	
	printf( "Written CSW file.\n" );
}

unsigned char *CSW_Parse( unsigned char *data, int size )
{
	int tIndex = 0;
	unsigned char *tPointer = data;
	struct CSW_CSWInfo *tInfo;
	unsigned long tSampleRate;

	tInfo = (struct CSW_CSWInfo *)data;
	tIndex = sizeof( struct CSW_CSWInfo );
	tPointer = &data[ tIndex ];

	tSampleRate = tInfo->sampleRate[ 0 ] + (tInfo->sampleRate[ 1 ] << 8) + (tInfo->sampleRate[ 2 ] << 16) + (tInfo->sampleRate[ 3 ] << 24);
	gWAVFormat.sampleRate = tSampleRate;

	for ( tIndex = 0 ; tIndex < size ; tIndex++ )
	{
		if ( *tPointer == 0x00 )
		{
			int tDelay = 0x00000000;

			tDelay = *(tPointer+1);
			tDelay += *(tPointer+2) << 8;
			tDelay += *(tPointer+3) << 16;
			tDelay += *(tPointer+4) << 24;
			tPointer += 4;

			AddRawCycle( (float)tDelay );
		}
		else
		{
			AddRawCycle( (float)*tPointer );	
		}

		tPointer++;
	}
	return (unsigned char *)1;
}
