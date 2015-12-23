enum Acorn_CassetteBlockType { ACBT_HEADER, ACBT_DUMMYBYTE, ACBT_DATA, ACBT_PAUSE };

struct Acorn_CassetteBlock
{
	enum Acorn_CassetteBlockType type;
	union
	{
		int size;
		float seconds;
	} sizeUnion;
	unsigned char *data;
	struct Acorn_CassetteBlock *next;
};


extern struct Acorn_CassetteBlock *gAcorn_CassetteBlockList;

unsigned char *Acorn_ParseWAV( unsigned char *rawData );
void Acorn_BuildTapeBlocks( struct RAW_Format format );
void Acorn_AddHeaderCycles( int cycles, bool dummy );
void Acorn_AddBlock( enum Acorn_CassetteBlockType type, int cycles, unsigned char *data );
void Acorn_AddDataBlock( int size, char *data );
void Acorn_AddPause( int size );
