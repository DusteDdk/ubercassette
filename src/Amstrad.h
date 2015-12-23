#ifndef __AMSTRAD_H__
#define __AMSTRAD_H__

unsigned char *Amstrad_ParseWAV( unsigned char *wavData );
void Amstrad_BuildTapeBlocks( struct RAW_Format format );

#endif
