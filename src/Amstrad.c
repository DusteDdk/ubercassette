#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "UberCassette.h"
#include "WAV.h"
#include "Sample.h"
#include "Spectrum.h"
#include "Amstrad.h"

#define MAX_WAVELENGTH 20000

unsigned char *Amstrad_ParseWAV( unsigned char *wavData )
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
		Amstrad_BuildTapeBlocks( tRawData );

	return (unsigned char *)1; // We have data ... but it's somewhere else.
}

void Amstrad_BuildTapeBlocks( struct RAW_Format format )
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

	memset( &tLoader, 0, sizeof( struct LoaderStruct ) );
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
			tPointer++;
		}
		else
		{
			printf( "Adding data at %f.\n", tOffset );

			if ( tDelayCounter > 0 )
			{
				if ( tDataSize )
				{
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
				BuildThresholds( format, 2, &tBlockThresholds[ 0 ], tBlockWavelengths, tValueTable );

				tCPUThresholds[ 0 ] = ConvertToCycles( tBlockThresholds[ 0 ]); tCPUThresholds[ 1 ] = ConvertToCycles( tBlockThresholds[ 1 ] ); tCPUThresholds[ 2 ] = ConvertToCycles( tBlockThresholds[ 2 ] );

				tLoader.onePulse = tCPUThresholds[ 1 ] / 2; tLoader.zeroPulse = tCPUThresholds[ 0 ] / 2;
				tLoader.pilotPulse = tLoader.onePulse; tLoader.sync1 = tLoader.zeroPulse;
				tLoader.sync2 = tLoader.zeroPulse; tLoader.pilotLength = 0;

				if ( tBlockThresholds[ 1 ] == -1 )
				{
					while ( tPointer < gRawLength && *tRawPointer < MAX_WAVELENGTH )
					{
						tOffset += *tWavePointer;
						tPointer++;
						tRawPointer++;
						tWavePointer++;
					}
					tLoader.pilotLength = 0;
					break;
				}

				// Find our header sync length
				while ( tPointer < gRawLength && *tRawPointer < MAX_WAVELENGTH )
				{
					if ( tStage == SS_HEADER )
					{
						if ( tValueTable[ *tRawPointer * 2 ] == 1 )
						{
							// We're in standard header timing.
							tHeaderCycles++;
							tLoader.pilotLength++;
						}
						else if ( tLoader.pilotLength > 100 )
						{
							tStage = SS_SYNC1;
							printf( "Starting data with pulse lengths %d (%ld) and %d(%ld).\n", tBlockThresholds[ 0 ], ConvertToCycles( tBlockThresholds[ 0 ] ), tBlockThresholds[ 1 ], ConvertToCycles( tBlockThresholds [ 1 ]) );

							continue; // Go back and check this cycle.
						}
					}
					else if ( tStage == SS_SYNC1 )
					{
						if ( tValueTable[ *tRawPointer * 2 ] == 0 )
						{
							if ( tLoader.id == LOADER_ROM )
								tStage = SS_SYNC2;
						}
						else
						{
							tStage = SS_HEADER;	// Wot no sync pulses?
							printf(" WARNING: Invalid sync pulse.\n" );
						}
					}
					else if ( tStage == SS_SYNC2 )
					{
						if ( tValueTable[ *tRawPointer * 2 ] == 0 )
						{
							tStage = SS_DATA;
						}
						else
						{
							tStage = SS_HEADER;	// Wot no sync pulses?
							printf(" WARNING: Invalid sync pulse.\n" );
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
					if ( 0 )
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
		Spectrum_AddTurboDataBlock( format, tLoader, tDataSize, (char *)tData, tDelayCounter );
	}

	gThresholdDivisor[ 0 ] = tDivisors[ 0 ];
	gThresholdDivisor[ 1 ] = tDivisors[ 1 ];
}
