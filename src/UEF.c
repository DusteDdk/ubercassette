#include <stdio.h>
#include <stdlib.h>

#include "UberCassette.h"
#include "WAV.h"

#include "Acorn.h"
#include "UEF.h"

struct UEF_Header
{
	char string[ 10 ];
	unsigned char minorVersion;
	unsigned char majorVersion;
};

void UEF_WriteHeader( struct Acorn_CassetteBlock *node, FILE *file )
{
	unsigned char tData[ 20 ];

	tData[ 0 ] = 0x10;
	tData[ 1 ] = 0x01;
	tData[ 2 ] = 0x02;
	tData[ 3 ] = 0x00;
	tData[ 4 ] = 0x00;
	tData[ 5 ] = 0x00;
	tData[ 6 ] = (node->sizeUnion.size) & 0xFF;
	tData[ 7 ] = (node->sizeUnion.size >> 8) & 0xFF;
	fwrite( tData, 8, 1, file );
}

void UEF_WriteDummy( struct Acorn_CassetteBlock *node, FILE *file )
{
	unsigned char tData[ 20 ];

	tData[ 0 ] = 0x11;
	tData[ 1 ] = 0x01;
	tData[ 2 ] = 0x04;
	tData[ 3 ] = 0x00;
	tData[ 4 ] = 0x00;
	tData[ 5 ] = 0x00;
	tData[ 6 ] = 0x03;
	tData[ 7 ] = 0x00;
	tData[ 8 ] = (node->sizeUnion.size ) & 0xFF;
	tData[ 9 ] = (node->sizeUnion.size >> 8) & 0xFF;
	fwrite( tData, 10, 1, file );
}


void UEF_WritePause( struct Acorn_CassetteBlock *node, FILE *file )
{
	unsigned char tData[ 20 ];
    unsigned long *tLengthBytes = (unsigned long *)&tData[ 6 ];

    *tLengthBytes = ConvertTo8087Float( node->sizeUnion.seconds );

	tData[ 0 ] = 0x16;
	tData[ 1 ] = 0x01;
	tData[ 2 ] = 0x04;
	tData[ 3 ] = 0x00;
	tData[ 4 ] = 0x00;
	tData[ 5 ] = 0x00;
	fwrite( tData, 10, 1, file );
}

void UEF_WriteData( struct Acorn_CassetteBlock *node, FILE *file )
{
	unsigned char tData[ 20 ];

 //   printf( "Writing UEF data node of size %ld.\n", node->size );

	tData[ 0 ] = 0x00;
	tData[ 1 ] = 0x01;
	*((int*)&tData[ 2 ]) = CorrectEndianLong( node->sizeUnion.size );
	fwrite( tData, 6, 1, file );
	fwrite( node->data, 1, node->sizeUnion.size, file );
	free( node->data );
}

void UEF_Output( struct Acorn_CassetteBlock *list, char *filename )
{
	unsigned char tData[ 20 ];
	struct UEF_Header *tHeader = (struct UEF_Header *)tData;
	FILE *tFile;
	struct Acorn_CassetteBlock *tNode = gAcorn_CassetteBlockList, *tOldNode;

	tFile = fopen( filename, "wb" );
	if ( tFile == NULL )
		return;

	sprintf( tHeader->string, "UEF File!" );
	tHeader->minorVersion = 10;
	tHeader->majorVersion = 0;

	fwrite( tHeader, sizeof( struct UEF_Header ), 1, tFile );

	while ( tNode != NULL )
	{
		switch ( tNode->type )
		{
		case ACBT_HEADER:
			UEF_WriteHeader( tNode, tFile );
			break;
		case ACBT_DUMMYBYTE:
			UEF_WriteDummy( tNode, tFile );
			break;
		case ACBT_DATA:
			UEF_WriteData( tNode, tFile );
			break;
		case ACBT_PAUSE:
			UEF_WritePause( tNode, tFile );
			break;
		}
		tOldNode = tNode;
		tNode = tNode->next;
        free( tOldNode );
	}

	fclose( tFile );
}

