/* Handler for Spectrum tapes. */

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "UberCassette.h"
#include "WAV.h"
#include "Sample.h"
#include "Spectrum.h"

#define MAX_WAVELENGTH 20000

void Spectrum_BuildTapeBlocks( struct RAW_Format format );
struct Spectrum_CassetteBlock *Spectrum_FindTapeBlock( struct RAW_Format format, unsigned long *pointer );

struct Spectrum_CassetteBlock *gSpectrum_CassetteBlockList = NULL;

unsigned char *Spectrum_ParseWAV( unsigned char *wavData )
{
	unsigned char *tParsedData = NULL;
	struct RAW_Format tRawData;

	// First off, extract the RAW data.
	tRawData = WAV_GetRAW( wavData, false );

    if ( tRawData.data == NULL )
        return NULL;

	// First we scan the entire file for the wavelength of the header.
	if ( gAlgorithmType == TRIGGER )
		BuildRawWaveList( tRawData, 0, NULL, -1, true );
	else 
		BuildRawWaveList_Cleaned( tRawData, 0, NULL, -1, true );

	if ( gOutputType != CSW )
		Spectrum_BuildTapeBlocks( tRawData );

	return (unsigned char *)1; // We have data ... but it's somewhere else.
}


void Spectrum_BuildTapeBlocks( struct RAW_Format format )
{
#define SBTB_RETRIES 20

	int tPointer = 0, tDataStartPointer;
	int *tRawPointer = &gRawCycles[ tPointer ], *tDataStartRawPointer;
	double *tWavePointer = &gWaveCycles[ tPointer ], *tDataStartWavePointer;
	int tBlockThresholds[ 3 ]={0,0,0}, tBlockWavelengths[ NUMBER_OF_THRESHOLDS ], tValueTable[ NUMBER_OF_THRESHOLDS ], tCPUThresholds[ 3 ];
	double tOffset = 0.0f, tDataStartOffset;
	int tDelayCounter = 0;
	int tHeaderCycles = 0;
	enum Stage { SS_HEADER, SS_SYNC1, SS_SYNC2, SS_DATA } tStage = SS_HEADER;
	unsigned char tData[ 128 * 1024 ];
	int tDataSize = 0, tBit, tPulse;
	unsigned char tXORByte = 0;
	double tDivisors[ 2 ];
	int tRetry = SBTB_RETRIES;
	bool tDataInvalid = true;
	unsigned short tDataBlockLength = 0;
	struct LoaderStruct tLoader;

	tDivisors[ 0 ] = gThresholdDivisor[ 0 ];
	tDivisors[ 1 ] = gThresholdDivisor[ 1 ];

	tLoader.id = LOADER_ROM; 
	tLoader.bitsInLastByte = 8;

	while ( tPointer < gRawLength && tOffset < gRAWFormat.size && *tRawPointer > 0 && *tWavePointer > 0 )
	{
		// What waves are used in this block?
		if ( *tRawPointer >= MAX_WAVELENGTH || BuildWavelengthTable( format, tOffset, 1, tBlockWavelengths ) == false )
		{
			// Silence is golden.
			tDelayCounter += *tRawPointer++;
			tOffset += *tWavePointer++;
		}
		else
		{
			printf( "Adding data at %f.\n", tOffset );

			if ( tDelayCounter > 0 )
			{
				if ( tDataSize )
				{
					if ( tLoader.id == LOADER_ROM )
						Spectrum_AddStandardDataBlock( format, tDataSize, (char *)tData, tDelayCounter );
					else if ( tLoader.id == LOADER_SPEEDLOCK )
						Spectrum_AddPureDataBlock( tDataSize, (char *)tData, tDelayCounter, tLoader );
					else
						Spectrum_AddTurboDataBlock( format, tLoader, tDataSize, (char *)tData, tDelayCounter );
				}
				else
					Spectrum_AddPauseBlock( format, tDelayCounter );
				tDelayCounter = 0;
			}

			tDataInvalid = true;
			tRetry = SBTB_RETRIES;
			tDataStartOffset = tOffset;
			tDataStartRawPointer = tRawPointer;
			tDataStartPointer = tPointer;
			tDataStartWavePointer = tWavePointer;

		if ( tOffset == 1720361 )
		{
			bool test=true;
		}

			while ( tRetry > 0 && tDataInvalid )
			{
				tXORByte = 0x00;
				tDataSize = 0; tBit = 0; *tData = 0; tPulse = 0; tStage = SS_HEADER;

				// Work out our thresholds....
				BuildThresholds( format, 3, &tBlockThresholds[ 0 ], tBlockWavelengths, tValueTable );

				tCPUThresholds[ 0 ] = ConvertToCycles( tBlockThresholds[ 0 ]); tCPUThresholds[ 1 ] = ConvertToCycles( tBlockThresholds[ 1 ] ); tCPUThresholds[ 2 ] = ConvertToCycles( tBlockThresholds[ 2 ] );

				if ( tCPUThresholds[ 0 ] < 1400 && tCPUThresholds[ 1 ] < 2300 && tCPUThresholds[ 2 ] > 3900 ) 
				{
					// Work out our protection scheme here.
					tLoader.id = LOADER_SPEEDLOCK;
					tLoader.bitsInLastByte = 6;	// First 6 bits are ID.
					tLoader.zeroPulse = /*0x0234;*/ tCPUThresholds[ 0 ] / 2;
					tLoader.onePulse = /*0x0469;*/tCPUThresholds[ 1 ] / 2;
					tLoader.pilotLength = 0;
					tLoader.sync1 = 3500;
					tLoader.sync2 = 0;
					tLoader.pilotPulse = /*0x0875*/2168;
				}
				if ( tBlockThresholds[ 2 ] == -1 )
				{
					bool test=true;
				}

				// Find our header sync length

				while ( tPointer < gRawLength && *tRawPointer < MAX_WAVELENGTH )
				{
					if ( tStage == SS_HEADER )
					{
	//					if ( tValueTable[ *tRawPointer * 2 ] == 2 )
						if ( fabs(*tRawPointer - 2168.0f) < 500.0f )
						{
							// We're in standard header timing.
							tHeaderCycles++;
							tLoader.pilotLength++;
						}
						else if ( tHeaderCycles > 10 )
						{
							tStage = SS_SYNC1;
							printf( "Starting data with pulse lengths %d (%ld) and %d(%ld).\n", tBlockThresholds[ 0 ], ConvertToCycles( tBlockThresholds[ 0 ] ), tBlockThresholds[ 1 ], ConvertToCycles( tBlockThresholds [ 1 ]) );

							continue; // Go back and check this cycle.
						}
					}
					else if ( tStage == SS_SYNC1 )
					{
						if ( fabs(*tRawPointer - 667.0f) < 800.0f )
						{
							if ( tLoader.id == LOADER_ROM )
								tStage = SS_SYNC2;
							else if ( tLoader.id == LOADER_SPEEDLOCK )
							{
								tStage = SS_HEADER;
								printf( "Header blip at %f. Cycles: %d\n", tOffset, tLoader.pilotLength );
								Spectrum_AddPureToneBlock( /*0x0875*/ tCPUThresholds[ 2 ] / 2, tLoader.pilotLength );
								Spectrum_AddPulseBlock( 2, /*0x02CA*/667 );
								tHeaderCycles = 0;
								tLoader.pilotLength = 1;
							}
						}
						else
						{
							if ( tLoader.id == LOADER_SPEEDLOCK )
							{
								if ( *tRawPointer > 0x0C51/*3500*/ )
								{
									tLoader.pilotLength--;
									Spectrum_AddPureToneBlock( /*0x0875*/ tCPUThresholds[ 2 ] / 2, tLoader.pilotLength );
									Spectrum_AddPulseBlock( 2, 0x0C51 /*3663*/ );
									tStage = SS_DATA;
									tDataBlockLength = 1; // Just 6 bits, in fact.
								}
							}
							else
							{
								tStage = SS_DATA;
							}
						}
					}
					else if ( tStage == SS_SYNC2 )
					{
						if ( fabs(*tRawPointer - 735.0f) < 800.0f )
						{
							tStage = SS_DATA;
						}
						else
						{
							tStage = SS_DATA;
						}
					}
					else if ( tStage == SS_DATA )
					{
						if ( tPulse == 0 )
							tPulse = *tRawPointer;
						else
						{
							tPulse += *tRawPointer;
							tData[ tDataSize ] <<= 1;
							if ( tValueTable[ tPulse ] == 1 )
								tData[ tDataSize ] |= 1;
							else if ( tValueTable[ tPulse ] == 0 )
								tData[ tDataSize ] |= 0;
							tBit++;
							if ( tBit == 8 || (tDataSize == (tDataBlockLength-1) && tBit == tLoader.bitsInLastByte) )
							{
								if ( tDataSize < tDataBlockLength )
								{
									tXORByte ^= tData[ tDataSize ];
								}
								tData[ tDataSize ] <<= (8 - tLoader.bitsInLastByte);
								tDataSize++;
								tData[ tDataSize ] = 0;
								tBit = 0;
								if ( tLoader.id == LOADER_SPEEDLOCK && tDataSize == tDataBlockLength )
								{
									Spectrum_AddPureDataBlock( tDataSize, (char *)tData, 0x0000, tLoader );
									tLoader.bitsInLastByte = 8;
									tDataSize = 0;
								}
								tDataBlockLength = 0;	// We've done all that data.
							}
							tPulse = 0;
						}
						tDelayCounter = 0;
					}

					tOffset += *tWavePointer;
					tPointer++;
					tRawPointer++;
					tWavePointer++;
				}
				if ( tData[ 0 ] == 0x00 )
				{
					// This is a header.
					tDataBlockLength = tData[ 11 ] + tData[ 12 ] * 256;
				}
				if ( tLoader.id == LOADER_ROM )
				{
					if ( tXORByte != 0x00 )
					{
						printf(" WARNING! Invalid XOR checksum.\n" );
		
						if ( --tRetry )
						{
							double tThresholdMod;
							printf(" Retrying...\n" );
							tThresholdMod = 0.025f * (float)((SBTB_RETRIES - (tRetry - 1) ) / 2) * ( (tRetry & 1) ? +1 : -1 );
							gThresholdDivisor[ 0 ] = tDivisors[ 0 ] + tThresholdMod;
							gThresholdDivisor[ 1 ] = tDivisors[ 1 ] + tThresholdMod;
							tOffset = tDataStartOffset;
							tRawPointer = tDataStartRawPointer ;
							tPointer = tDataStartPointer;
							tWavePointer = tDataStartWavePointer;
						}
						else
							printf( "Giving up.\n" );
					}
					else
						tDataInvalid = false;
				}
				else
					tDataInvalid = false;
			}
		}
	}
	if ( tDataSize )
	{
		if ( tLoader.id == LOADER_ROM )
			Spectrum_AddStandardDataBlock( format, tDataSize, (char *)tData, tDelayCounter );
		else if ( tLoader.id == LOADER_SPEEDLOCK )
			Spectrum_AddPureDataBlock( tDataSize, (char *)tData, tDelayCounter, tLoader );
		else
			Spectrum_AddTurboDataBlock( format, tLoader, tDataSize, (char *)tData, tDelayCounter );
	}

	gThresholdDivisor[ 0 ] = tDivisors[ 0 ];
	gThresholdDivisor[ 1 ] = tDivisors[ 1 ];
}

void Spectrum_ReadData( unsigned char *data, int length, struct RAW_Format format, unsigned long *pointer )
{
}


struct SpectrumHeader Spectrum_ReadHeader( struct RAW_Format format, unsigned long *pointer )
{
	struct SpectrumHeader tHeader;
	tHeader.startAddress = 0;
	tHeader.endAddress = 0;
	*tHeader.name = 0;
	return tHeader;	
}

struct Spectrum_CassetteBlock *Spectrum_FindTapeBlock( struct RAW_Format format, unsigned long *pointer )
{
	return NULL;

}

unsigned long Spectrum_FindHeader( struct RAW_Format format, unsigned long pointer )
{
	return 0;
}

void Spectrum_AddBlock( enum Spectrum_CassetteBlockType type, int cycles, char *data, int pause, struct LoaderStruct *loader )
{
	struct Spectrum_CassetteBlock *tNode = gSpectrum_CassetteBlockList;

	while ( tNode )
	{
		if ( tNode->next )
			tNode = tNode->next;
		else
			break;
	}

	if ( tNode && tNode->type == SCBT_PULSE && type == SCBT_PULSE )
	{
		bool test=true;
	}


	if ( gSpectrum_CassetteBlockList == NULL )
		tNode = gSpectrum_CassetteBlockList = (struct Spectrum_CassetteBlock *)malloc( sizeof( struct Spectrum_CassetteBlock ) );
	else
	{
		tNode->next = (struct Spectrum_CassetteBlock *)malloc( sizeof( struct Spectrum_CassetteBlock ) );
		tNode = tNode->next;
	}


	tNode->type = type;
	tNode->size = cycles;
	tNode->data = (unsigned char *)data;
	tNode->pause = pause;
	if ( loader )
	{
		tNode->loader = (struct LoaderStruct *)malloc( sizeof( struct LoaderStruct ));
		if ( tNode->loader == NULL )
		{
			printf( "WARNING: Out of memory copying loader info.\n" );
		}
		else
			memcpy( tNode->loader, loader, sizeof( struct LoaderStruct ));
	}
	tNode->next = NULL;
}

void Spectrum_AddPauseBlock( struct RAW_Format format, int cycles )
{
	Spectrum_AddBlock( SCBT_PAUSE, cycles, NULL, 0, NULL );
}

void Spectrum_AddStandardDataBlock( struct RAW_Format format, int length, char *data, int pause )
{
	unsigned char *tData = (unsigned char *)malloc(length);
	memcpy( tData, data, length );
	Spectrum_AddBlock( SCBT_DATA, length, (char *)tData, pause, NULL );
}

void Spectrum_AddTurboDataBlock( struct RAW_Format format, struct LoaderStruct loader, int length, char *data, int pause )
{
	unsigned char *tData = (unsigned char *)malloc(length);
	memcpy( tData, data, length );

	Spectrum_AddBlock( SCBT_TURBODATA, length, (char *)tData, pause, &loader );

}

void Spectrum_AddPureToneBlock( int pulseLength, int count )
{
	Spectrum_AddBlock( SCBT_TONE, count, NULL, pulseLength, NULL );
}

void Spectrum_AddPulseBlock( int pulseLength, int count )
{
	Spectrum_AddBlock( SCBT_PULSE, count, NULL, pulseLength, NULL );
}

void Spectrum_AddPureDataBlock( int length, char *data, int pause, struct LoaderStruct loader )
{
	unsigned char *tData = (unsigned char *)malloc(length);
	memcpy( tData, data, length );

	Spectrum_AddBlock( SCBT_PUREDATA, length, (char *)tData, pause, &loader );
}
