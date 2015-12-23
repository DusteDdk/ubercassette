#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UberCassette.h"

#include "CBM.h"

struct T64_TapeInfo
{
	unsigned short version;
	unsigned short maxEntries;
	unsigned short usedEntries;
	unsigned char pad[ 2 ];
	char name[ 25 ];	// Terminating zero on the end.
};

struct T64_DirectoryEntry
{
	unsigned char fileType;
	unsigned char diskFileType;
	unsigned short startAddress;
	unsigned short endAddress;
	unsigned char pad[ 2 ];
	unsigned long offset;
	unsigned char pad2[ 4 ];
	unsigned char filename[ 16 ];
};

void T64_WriteHeader( FILE *file, struct CBM_CassetteBlock *list )
{
	int tEntries = 0, i;
	struct CBM_CassetteBlock *tNode = list;
	char tBuffer[ 33 ];
	struct T64_TapeInfo *tTapeInfo;
	struct T64_DirectoryEntry *tDirectoryEntry;
	unsigned long tOffset = 0;

	while ( tNode )
	{
		tNode = tNode->next;
		tEntries++;
	}

	memset( tBuffer, 0, 32 );
	sprintf( tBuffer, "C64S tape image file" );
	fwrite( tBuffer, 32, 1, file );

	memset( tBuffer, 0, 32 );
	tTapeInfo = ( struct T64_TapeInfo *)tBuffer;
	tTapeInfo->version = 0x0101;
	tTapeInfo->maxEntries = CorrectEndianShort( tEntries );
	tTapeInfo->usedEntries = CorrectEndianShort( tEntries );
	sprintf( tTapeInfo->name, "DUMPED TAPE             " );
	fwrite( tBuffer, 32, 1, file );

	tOffset = 0x40 + 0x20 * tEntries;
	tDirectoryEntry = (struct T64_DirectoryEntry *)tBuffer;

	tNode = list;

	while ( tNode )
	{
		memset( tBuffer, 0x20, 32 );
		tDirectoryEntry->fileType = tNode->c64sFileType;
		tDirectoryEntry->diskFileType = tNode->cbm1541FileType;
		tDirectoryEntry->startAddress = CorrectEndianShort( tNode->startAddress );
		tDirectoryEntry->endAddress = CorrectEndianShort( tNode->endAddress );
		tDirectoryEntry->offset = CorrectEndianLong( tOffset );
		tOffset += tNode->endAddress - tNode->startAddress + 1;
		for ( i=0 ; i<16 ; i++ )
			tDirectoryEntry->filename[ i ] = tNode->filename[ i ];

		fwrite( tBuffer, 32, 1, file );
		tNode = tNode->next;
	}
}

void T64_WriteBlocks( FILE *file, struct CBM_CassetteBlock *list )
{
	struct CBM_CassetteBlock *tNode = list;

	while ( tNode )
	{
		fwrite( tNode->data, 1, tNode->endAddress - tNode->startAddress + 1, file );
        free( tNode->data );
		tNode = tNode->next;
	}
}

void T64_Output( struct CBM_CassetteBlock *list, char *filename )
{
	FILE *tOutputFile = NULL;
	struct CBM_CassetteBlock *tBlock = list;

	tOutputFile = fopen( filename, "wb" );
	if ( tOutputFile == NULL )
	{
		printf( "Couldn't open output file.\n" );
		return;
	}

	T64_WriteHeader( tOutputFile, gCBM_CassetteBlockList );
	T64_WriteBlocks( tOutputFile, gCBM_CassetteBlockList );			

	fclose( tOutputFile );
}
