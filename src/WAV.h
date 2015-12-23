/* Helper functions for using WAV files. */

#ifndef __WAV_H__
#define __WAV_H__

struct WAV_Format
{
	unsigned short numberOfChannels;
	unsigned long sampleRate;
	unsigned short significantBitsPerSample;
};

struct RAW_Format
{
	unsigned long size;
	signed char *data;
};

struct WAV_ChunkPrototype
{
	char ID[ 4 ];
	unsigned long dataSize;
};

struct WAV_Header
{
	char ID[ 4 ];
	unsigned long dataSize;
	char RIFFtype[ 4 ];
};

struct WAV_FormatChunk
{
	char ID[ 4 ];
	unsigned long dataSize;
	unsigned short compressionCode;
	unsigned short numberOfChannels;
	unsigned long sampleRate;
	unsigned long bytesPerSecond;
	unsigned short blockAlign;
	unsigned short significantBitsPerSample;
};

struct WAV_DataChunk
{
	char ID[ 4 ];
	unsigned long dataSize;
	signed char data[0];
};

extern struct WAV_Format gWAVFormat, gOutputWAVFormat;
extern struct RAW_Format gRAWFormat;
struct RAW_Format WAV_GetRAW( unsigned char *wavData, bool invert );
void WAV_CreateWAV( char *filename, signed char *wavBuffer, int *data, int size, bool invert );
void WAV_Output( char *filename, int *data, unsigned long size, bool invert );
void WAV_CreateHeader( struct WAV_Header *header );
void WAV_CreateFormatChunk( struct WAV_FormatChunk *formatChunk );
void WAV_CreateDataChunk( struct WAV_DataChunk *dataChunk );
int WAV_CreateData( signed char *wavBuffer, int *data, int size, bool invert );
void WAV_Normalise( struct RAW_Format format );

#endif
