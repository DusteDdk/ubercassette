/* The WAV handler for Spectrum tapes. */

#ifndef __SPECTRUM_H__
#define __SPECTRUM_H__

#include "WAV.h"

enum Spectrum_CassetteBlockType { SCBT_HEADER, SCBT_DUMMYBYTE, SCBT_DATA, SCBT_TURBODATA, SCBT_PAUSE, SCBT_TONE, SCBT_PULSE, SCBT_PUREDATA };

struct LoaderStruct
{
	enum Loader id;
	int pilotPulse;
	int zeroPulse;
	int onePulse;
	int sync1;
	int sync2;
	int pilotLength;
	int bitsInLastByte;
};

struct Spectrum_CassetteBlock
{
	enum Spectrum_CassetteBlockType type;
	unsigned char *data;
	int size;
	int pause;	// Delay after this block in mS.
	struct LoaderStruct *loader;
	struct Spectrum_CassetteBlock *next;
};

struct SpectrumHeader
{
	unsigned short startAddress;
	unsigned short endAddress;
	char name[ 128 ];
};



extern struct Spectrum_CassetteBlock *gSpectrum_CassetteBlockList;

unsigned char *Spectrum_ParseWAV( unsigned char *rawData );
unsigned long Spectrum_FindHeader( struct RAW_Format format, unsigned long pointer );
void Spectrum_AddPauseBlock( struct RAW_Format format, int cycles );
void Spectrum_AddStandardDataBlock( struct RAW_Format format, int length, char *data, int pause );
void Spectrum_AddPureDataBlock( int length, char *data, int pause, struct LoaderStruct loader );
void Spectrum_AddTurboDataBlock( struct RAW_Format format, struct LoaderStruct loader, int length, char *data, int pause );
void Spectrum_AddBlock( enum Spectrum_CassetteBlockType type, int cycles, char *data, int pause, struct LoaderStruct *loader );
void Spectrum_AddPureToneBlock( int pulseLength, int count );
void Spectrum_AddPulseBlock( int pulseLength, int count );

#endif
