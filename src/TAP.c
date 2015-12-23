#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UberCassette.h"

#include "CBM.h"

struct TAP_TapeInfo
{
	char identifier[ 12 ];
	unsigned char version;
	unsigned char machineID;
	unsigned char videoType;
	unsigned char pad[ 1 ];
	unsigned long length;
	unsigned char data[0];
};


void TAP_WriteHeader( FILE *file, int *data, unsigned long length )
{
	char tBuffer[ 20 ];
	struct TAP_TapeInfo *tTapeInfo = (struct TAP_TapeInfo *)tBuffer;
	int tByteLength = 0;
	unsigned long tPointer = 0;
	unsigned long tCycles;

	while ( tPointer < length )
	{
		tCycles = data[ tPointer ];
		if ( (tCycles / 8) > 0xFF )
			tByteLength += 3;
		tByteLength++;
		tPointer++;
	}

	memset( tTapeInfo, 0, 20 );
	sprintf( tTapeInfo->identifier, "%s-TAPE-RAW", gMachineData.string );

	tTapeInfo->version = (gMachineData.halfWave ? 2 : 1); // Half-waves for C16.
	tTapeInfo->machineID = gMachineData.machineID;
	tTapeInfo->length = tByteLength;
	tTapeInfo->videoType = gVideoType;

	fwrite( tTapeInfo, 20, 1, file );
}

void TAP_WriteBlocks( FILE *file, int *data, unsigned long length )
{
	unsigned long tPointer = 0;
	unsigned long tCycles;
	unsigned char tBuffer[ 4 ];
	//signed char tDistances[ 3 ];

	while ( tPointer < length )
	{
		tCycles = data[ tPointer ];
		if ( (tCycles / 8) > 0xFF )
		{
			tBuffer[ 0 ] = 0x00;
			tBuffer[ 1 ]= tCycles & 0xFF;
			tBuffer[ 2 ]= (unsigned char )((tCycles & 0xFF00) >> 8);
			tBuffer[ 3 ]= (unsigned char )((tCycles & 0xFF0000) >> 16);
			fwrite( tBuffer, 4, 1, file );
		}
		else
		{
/*			tCycles /= 8;
			tDistances[ 0 ] = (signed char )abs(tCycles - 0x2C); 
			tDistances[ 1 ] = (signed char )abs(tCycles - 0x3F); 
			tDistances[ 2 ] = (signed char )abs(tCycles - 0x55); 

			if ( tDistances[ 0 ] <= tDistances[ 1 ] && tDistances[ 0 ] <= tDistances[ 2 ] )
				tBuffer[ 0 ] = 0x2C;
			else if ( tDistances[ 1 ] <= tDistances[ 0 ] && tDistances[ 1 ] <= tDistances[ 2 ] )
				tBuffer[ 0 ] = 0x3F;
			else if ( tDistances[ 2 ] <= tDistances[ 0 ] && tDistances[ 2 ] <= tDistances[ 1 ] )
				tBuffer[ 0 ] = 0x55;
*/

			if ( tCycles >= 8 )
			{
				tBuffer[ 0 ] = (unsigned char )(tCycles / 8);
				fwrite( tBuffer, 1, 1, file );
			}
		}
		tPointer++;
	}
}

void TAP_Output( struct CBM_CassetteBlock *list, char *filename )
{
	FILE *tOutputFile = NULL;
	struct CBM_CassetteBlock *tBlock = list;

	unsigned long tLength = gRawLength;

    printf( "Writing TAP file.\n" );

	tOutputFile = fopen( filename, "wb" );
	if ( tOutputFile == NULL )
	{
		printf( "Couldn't open output file.\n" );
		return;
	}

	TAP_WriteHeader( tOutputFile, gRawCycles, tLength );
	TAP_WriteBlocks( tOutputFile, gRawCycles, tLength );			

	fclose( tOutputFile );
	
	printf( "Written TAP file.\n" );
}

unsigned char *TAP_Parse( unsigned char *data, int size )
{
	int tIndex = 0;
	unsigned char *tPointer = data;

	tIndex = sizeof( struct TAP_TapeInfo );
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
