/* The WAV handler for CBM tapes. */

#include "WAV.h"

struct CBM_CassetteBlock
{
	unsigned char c64sFileType;
	unsigned char cbm1541FileType;
	unsigned short startAddress;
	unsigned short endAddress;
	unsigned char filename[ 128 ];
	unsigned long offset;
	unsigned char *data;
	struct CBM_CassetteBlock *next;
};

struct CBMHeader
{
	unsigned short startAddress;
	unsigned short endAddress;
	char name[ 128 ];
};

extern struct CBM_CassetteBlock *gCBM_CassetteBlockList;

unsigned char *CBM_ParseWAV( unsigned char *rawData );
double CBM_FindHeader( struct RAW_Format format, double pointer );
