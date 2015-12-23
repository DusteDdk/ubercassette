// Tool functions for getting the data ready to parse (if necessary).

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "UberCassette.h"
#include "Sample.h"
#include "WAV.h"


#define SAMPLE_ENTRIES 20

void BuildRawWaveList( struct RAW_Format format, unsigned long offset, double *wavelengthTable, int blocks, bool halfWaves )
{
	double tPointer = offset, tWaveStart;
	bool tFirstWave = true;
	int tBlocks = blocks;

	tWaveStart = tPointer;
	tPointer = FindNextWavelengthStart( format, tPointer );
	AddRawCycle( (double) tPointer - tWaveStart );

    printf( "Building RAW Wave list.\n" );

	while ( tBlocks != 0 && tPointer < format.size )
	{
		if ( FindWavelength( format, &tPointer, tFirstWave, false, halfWaves, wavelengthTable ) > 2000.0f )
			tBlocks--;

		tFirstWave = !tFirstWave;
	}
	printf( "RAW Wave list done.\n" );
}

double FindNextWavelengthStart( struct RAW_Format format, double pointer )
{
	// Find the beginning of a waveform.
	unsigned long tPointer = (unsigned long)pointer;

	while ( format.data[ tPointer ] < gPreferences.minimumSignalLevel && tPointer < format.size )
		tPointer++;

	if ( tPointer == format.size )
		return -1;	// No more waveforms here.

	while ( format.data[ tPointer ] > 0 && tPointer < format.size )
		tPointer++;

	if ( tPointer == format.size )
		return -1;	// Still no more waveforms.

	return (double)tPointer;
}


void AddCurveSample( signed char *curve, unsigned char sample )
{
	int i;

	for ( i = 0 ; i < SAMPLE_ENTRIES-1 ; i++ )
		curve[ i ] = curve[ i + 1 ];
	curve[ i ] = sample;
}

signed char GetCurveAverage( signed char *curve )
{
	signed char tAverage;
	tAverage = ((int)curve[ SAMPLE_ENTRIES-5 ] + (int)curve[ SAMPLE_ENTRIES-4 ] + (int)curve[ SAMPLE_ENTRIES-3 ] + (int)curve[ SAMPLE_ENTRIES-2 ] + (int)curve[ SAMPLE_ENTRIES-1 ]) / 5;
	return tAverage;
}

signed char GetCurvePeak( signed char *curve )
{
	signed char tPeak=0;
	int i;

	for ( i=0 ; i<SAMPLE_ENTRIES ; i++ )
	{
		if ( tPeak < curve[ i ] )
			tPeak = curve[ i ];
	}
	return tPeak;
}

signed char GetCurveTrough( signed char *curve )
{
	signed char tTrough=0;
	int i;

	for ( i=0 ; i<SAMPLE_ENTRIES ; i++ )
	{
		if ( tTrough > curve[ i ] )
			tTrough = curve[ i ];
	}
	return tTrough;
}

int HasStartedCurve( signed char *curve )
{
	signed char tTrough = GetCurveTrough( curve );

	if ( tTrough > -gPreferences.minimumSignalLevel )
		return false;

	if ( curve[ SAMPLE_ENTRIES-1 ] > curve[ SAMPLE_ENTRIES-2 ] )
		return false;	// We're going up-hill.

	return true;
}

void AddWave( double *list, int amount, double cycles )
{
	int tCounter = 0;
	double *tListCounter;

	for ( tCounter = 0 ; tCounter < amount ; tCounter++ )
	{
		if ( list )
		{
			tListCounter = &list[ 0 ];
			*tListCounter += 1.0f;
			list[ (int)*tListCounter ] = cycles;

			if ( cycles > 24 && cycles < 36 )
			{
				bool test=true;
			}
		}
		else
			AddRawCycle( cycles );

		if ( cycles < 0.0f )
		{
			printf( "ERROR in amount of cycles.\n" );
		}
		if ( cycles > 1000 )
		{
			bool test=true;
		}
	}
}

void TriggerWaveAddition( struct RAW_Format format, double *wavelengthTable, bool halfWaves, double *peak, double *trough, double *waveLength, unsigned long *pointer, double *curveStart, double *lastWave, double *curveEnd, bool *loggedWave )
{
	double tOldTrough = 0.0f, tPeak = *peak, tTrough = *trough, tCurveStart = *curveStart, tLastWave = *lastWave, tCurveEnd = *curveEnd;
	bool tLoggedWave = *loggedWave;
	double tWaveLength = *waveLength;
	unsigned long tPointer = *pointer;
	double tCurveHalf = 0.0f;
	double tTargetAmplitude, tHalfWaveLength = 0.0f;
	double tClosestPoint, tClosestHalfPoint;		
	double tStartOfCycle = tCurveStart;
	int tTroughAmplitude=0,	tPeakAmplitude=0, tOldTroughAmplitude=0;
	static int tCounter = 0;	
	bool tEnableCounter = false;

	if ( tPointer > 1708089 )
	{
		tEnableCounter = true;	
	}

	tOldTrough = tTrough;
	tTrough = tPointer - 1;

	if ( tTrough && tPeak )
	{
		tTroughAmplitude = format.data[ (int)tTrough ]; tPeakAmplitude = format.data[ (int)tPeak ];					
		if ( tOldTrough )
			tOldTroughAmplitude = format.data[ (int)tOldTrough ];
	}

	// Use this if we don't have a full wave's data.
	if ( tOldTrough )
		tWaveLength = (double)(tPointer - tOldTrough);

	if ( tPeak && tTrough )
	{
		if ( gAlgorithmType == SAW )
		{
			if ( halfWaves )
			{
				tCurveHalf = tOldTrough + (tPeak - tOldTrough) / 2.0f;
				tHalfWaveLength = tCurveHalf - tStartOfCycle;
				tCurveStart = tPeak + (tTrough - tPeak) / 2.0f;
			}
		}
		else
		{
			int i;
			int tX1, tX2;
			double tGradient, tTargetY;
			tTargetAmplitude = (tPeakAmplitude + tTroughAmplitude) / 2.0f;
			tClosestPoint = tTrough;
			for ( i = (int)tPeak ; i < (int)tTrough ; i++ )
			{
				if ( fabs( (double)format.data[ i ] - tTargetAmplitude ) < fabs((double)format.data[ (int)tClosestPoint ] - tTargetAmplitude))
					tClosestPoint = (double) i;
			}
			if ( tOldTrough && halfWaves )
			{
				tClosestHalfPoint = (double)tPeak;
				for ( i = (int)tPeak ; i > (int)tOldTrough ; i-- )
				{
					if ( fabs( (double)format.data[ i ] - tTargetAmplitude ) < (double)format.data[ (int)tClosestHalfPoint ]- tTargetAmplitude )
						tClosestHalfPoint = (double) i;
				}
			}
			
			// Extrapolate the actual curve trigger.
			if ( format.data[ (int)tClosestPoint ] > tTargetAmplitude )
			{
				tX1 = (int)tClosestPoint; tX2 = (int)tClosestPoint+1;
			}
			else
			{
				tX1 = (int)tClosestPoint-1; tX2 = (int)tClosestPoint;
			}

			// y = mx + c, right? So....
			tTargetY = tTargetAmplitude - (double)format.data[ (int)tX1 ];
			tGradient = (double)format.data[ tX1 ] - (double)format.data[ tX2 ];
			if (tGradient == 0.0f )
				tClosestPoint = (double)tX1 + 0.5f;
			else
				tClosestPoint = tX1 - tTargetY / tGradient;	
			tCurveStart = tClosestPoint;	// Set the end of the curve, beginning of the next.
			if ( tCurveStart > tPointer )
			{
				printf( "** WARNING ** Invalid curve length.\n" );
			}

			if ( halfWaves )
			{
				// Extrapolate the actual curve trigger.
				if ( format.data[ (int)tClosestHalfPoint ] > tTargetAmplitude )
				{
					tX1 = (int)tClosestHalfPoint-1; tX2 = (int)tClosestHalfPoint;
				}
				else
				{
					tX1 = (int)tClosestHalfPoint; tX2 = (int)tClosestHalfPoint+1;
				}

				// y = mx + c, right? So....
				tGradient = (double)format.data[ tX1 ] - (double)format.data[ tX2 ];
				if (tGradient == 0.0f )
					tClosestHalfPoint = (double)tX1 + 0.5f;
				else
					tClosestHalfPoint = tX1 - tTargetY / tGradient;	
				tCurveHalf = tClosestHalfPoint;
				if ( tCurveHalf - tStartOfCycle > 0 )
					tHalfWaveLength = tCurveHalf - tStartOfCycle;
			}
		}
	}
	if ( tStartOfCycle )
	{
		if ( tCurveHalf )
			tWaveLength = tCurveStart - tCurveHalf;
		else
			tWaveLength = tCurveStart - tStartOfCycle;
	}
//	if ( tLog )
		;//printf( "Bottom of curve at %d. OldStart: %f  CurveStart: %f  Wavelength: %f.\n", tPointer, tStartOfCycle, tCurveStart, tWaveLength );
	if ( tWaveLength )
	{
		if ( tStartOfCycle - tLastWave > .00001f /*&& tLastWave > 0.0f*/ )
		{
			AddWave( wavelengthTable, 1, tStartOfCycle - tLastWave );
		}
		if ( tHalfWaveLength > 0.001f )
		{
			if ( tEnableCounter  )
				tCounter++;
			AddWave( wavelengthTable, 1, tHalfWaveLength );
		}
		if ( tEnableCounter  )
			tCounter++;
		AddWave( wavelengthTable, 1, tWaveLength );
		tCurveEnd = tCurveStart;
		tLastWave = (double)tPointer;
		tLoggedWave = true;
	}

	*peak = tPeak; *trough = tTrough; *waveLength = tWaveLength; *pointer = tPointer; *curveStart = tCurveStart; *lastWave = tLastWave;
	*curveEnd = tCurveEnd; *loggedWave = tLoggedWave;
}

void BuildRawWaveList_Cleaned( struct RAW_Format format, unsigned long offset, double *wavelengthTable, int blocks, bool halfWaves )
{
	unsigned long tPointer = 0;
	signed char  tCurve[ SAMPLE_ENTRIES ];
	double tCurveAverage;
	double tCurveStart = 0, tWaveLength, tCurveEnd = 0;
	enum CurveState { NO_CURVE, CURVE_UP, CURVE_DOWN } tState;
	int i, tBlocks = blocks;
	double tPeak, tTrough;
	unsigned long tWaves = 0;
	bool tLog = false, tLoggedWave = false;
	double tLastWave = 0.0f;

	for ( i=0 ; i < SAMPLE_ENTRIES ; i++ )
		tCurve[ i ] = 0;

	tState = NO_CURVE;

	tPointer = offset;

	while ( tBlocks != 0 && tPointer < format.size - 1 )
	{
		if ( tPointer >= 1707995 && !tLog )
		{
			tLog = true;
		}
		AddCurveSample( tCurve, format.data[ tPointer ] );

		if ( abs( GetCurvePeak( tCurve ) - GetCurveTrough( tCurve )) < gPreferences.minimumSignalLevel ) 
		{
			if ( tLoggedWave == false )
			{
				// It's just noise. Forget it.
				tState = NO_CURVE;
			}
			else
			{
				if ( tState != NO_CURVE )
				{
					int tLastByte = SAMPLE_ENTRIES-1;
					for ( tLastByte = SAMPLE_ENTRIES-1 ; tLastByte > 0 ; tLastByte-- )
					{
						if ( abs( tCurve[ tLastByte ] ) < 10 )
							break;
					}
					if ( tPointer < tLastWave )
					{
						printf(" ERROR in curve.\n" );
					}
//					AddWave( 1, (double)((double)tPointer - tLastWave) );
					if ( tPointer - tLastByte > tCurveEnd )
					{
						AddWave( wavelengthTable, 1, (double)((double)tPointer - tLastByte - tCurveEnd));
						tCurveEnd = (double)(tPointer - tLastByte);
						tLastWave = (double)tPointer;
					}
					tBlocks--;
					printf( "Block ends at %ld.\n", tPointer );
				}

				tCurveStart = 0;
				tPeak = tTrough = tWaves = 0;
				tWaveLength = 0.0f;
				tState = NO_CURVE;
			}
		}

		switch ( tState )
		{
		case NO_CURVE:
			if ( HasStartedCurve( tCurve ) )
			{
				tState = CURVE_DOWN;
				if ( tCurveEnd )
				{
//					AddWave( 1, (double)(tPointer - tLastWave) );
					//tCurveEnd = 0;
				}
				tLoggedWave = false;
				tCurveStart = (double)(tPointer - 0);	// was 4
				tCurveAverage = GetCurveAverage( tCurve );
				printf( "Curve found at %f.\n", tCurveStart );
				tPeak = tTrough = tWaves = 0;
				tWaveLength = 0.0f;
			}
			break;
		case CURVE_DOWN:
			if ( format.data[ tPointer ] > format.data[ tPointer - 1 ] && format.data[ tPointer + 1 ] >= format.data[ tPointer ] )
			{
				tState = CURVE_UP;

				if( tPointer == 13265165 )
				{
					bool test=true;
				}
				TriggerWaveAddition( format, wavelengthTable, halfWaves, &tPeak, &tTrough, &tWaveLength, &tPointer, &tCurveStart, &tLastWave, &tCurveEnd, &tLoggedWave );
				if ( tLog )
				{
					static int tCounter = 0;
//					printf( "Wave added at %ld. Counter is %d.\n", tPointer, ++tCounter );
					if ( tPointer > 1714369 )
					{
						bool test=true;
					}
				}
			}
			break;
		case CURVE_UP:
			if ( format.data[ tPointer ] < format.data[ tPointer - 1 ] && format.data[ tPointer + 1 ] <= format.data[ tPointer ] )
			{
				tState = CURVE_DOWN;
				if ( tPeak > 0 )
				{
					tWaveLength = (double)((tPointer-1) - tPeak);
					tPeak = tPointer - 0;
				}
				else
				{
					tPeak = tPointer - 0;
					tWaveLength = (double)((tPeak - tTrough) * 2);
				}
				tWaves++;
			}
			break;
		default:
			break;
		}

		tPointer++;
	}

	// Last curve.
	TriggerWaveAddition( format, wavelengthTable, halfWaves, &tPeak, &tTrough, &tWaveLength, &tPointer, &tCurveStart, &tLastWave, &tCurveEnd, &tLoggedWave );
}

void BuildThresholds( struct RAW_Format format, int thresholds, int *thresholdStore, int *wavelengthTable, int *valueTable )
{
	// Find the most common wavelength.
	#define BT_NUMBER_OF_CURVES 20
	#define BT_MINIMUM_WAVES 2
	#define BT_MINIMUM_CURVE_WIDTH 3

	struct BT_Curve
	{
		int start, end, size, peak;
	} tCurves[ BT_NUMBER_OF_CURVES ];

	unsigned char tMostCommon[ 3 ]={-1,-1,-1}, tNextPeakUp = 0, tNextPeakDown = 0;
	int i, tValue, tThresholds[ 2 ];
	bool tHasGoneLow = true;
	int tWaveCounter = 0, tSize=0;

	int tCurveNumber = 0;

	for ( i = 0 ; i < BT_NUMBER_OF_CURVES ; i++ )
	{
		tCurves[ i ].start = tCurves[ i ].end = -1;
		tCurves[ i ].size = 0; tCurves[ i ].peak = 0;
	}
	for ( i = 2 ; i < NUMBER_OF_THRESHOLDS-2 && tCurveNumber < BT_NUMBER_OF_CURVES ; i++ )
	{
		if ( wavelengthTable[ i ] != -1 )
			printf( "%d: %d. ", i, wavelengthTable[ i ] );
//		if ( wavelengthTable[ i ] < wavelengthTable[ i+1 ] && wavelengthTable[ i +1] < wavelengthTable[ i+2 ] && tCurves[ tCurveNumber ].start == -1 )
		if ( wavelengthTable[ i ] >= BT_MINIMUM_WAVES && tCurves[ tCurveNumber ].start == -1 )
			tCurves[ tCurveNumber ].start = i;

		if ( tCurves[ tCurveNumber ].start != -1 )
		{
			if ( wavelengthTable[ tCurves[ tCurveNumber ].peak ] < wavelengthTable[ i ] )
				tCurves[ tCurveNumber ].peak = (tCurves[ tCurveNumber ].start + i) / 2;
			tCurves[ tCurveNumber ].size += wavelengthTable[ i ];
//			if ( wavelengthTable[ i + 1 ] < wavelengthTable[ i ] && wavelengthTable[ i + 2 ] < wavelengthTable[ i+1 ] )
			if ( fabs( i - tCurves[ tCurveNumber ].peak ) > BT_MINIMUM_CURVE_WIDTH && 
				(wavelengthTable[ i ] < BT_MINIMUM_WAVES || (wavelengthTable[ i + 1 ] >= wavelengthTable[ i ] && wavelengthTable[ i + 2 ] >= wavelengthTable[ i + 1 ] && wavelengthTable[ i ] <= wavelengthTable[ i -1 ] && wavelengthTable[ i - 1 ] <= wavelengthTable[ i - 2 ] ))) 
			{
				tCurves[ tCurveNumber ].end = i;
				tCurveNumber++;
			}
		}
	}

	
	for ( i = 0 ; i < BT_NUMBER_OF_CURVES ; i++ )
	{
		if ( tMostCommon[ 0 ] == 255 || tCurves[ i ].size > tCurves[ tMostCommon[ 0 ] ].size )
		{
			if ( thresholds == 3 )
				tMostCommon[ 2 ] = tMostCommon[ 1 ];
			tMostCommon[ 1 ] = tMostCommon[ 0 ];
			tMostCommon[ 0 ] = i;
			printf(" More common than 0. Now %d, %d, %d.\n", tMostCommon[ 0 ], tMostCommon[ 1 ], tMostCommon[ 2 ] );
		}
		else if ( tMostCommon[ 1 ] == 255 || tCurves[ i ].size > tCurves[ tMostCommon[ 1 ] ].size )
		{
			if ( thresholds == 3 )
				tMostCommon[ 2 ] = tMostCommon[ 1 ];
			tMostCommon[ 1 ] = i;
			printf(" More common than 1. Now %d, %d, %d.\n", tMostCommon[ 0 ], tMostCommon[ 1 ], tMostCommon[ 2 ] );
		}
		else if ( tMostCommon[ 2 ] == 255 || tCurves[ i ].size  > tCurves[ tMostCommon[ 2 ] ].size )
		{
			if ( thresholds == 3 )
				tMostCommon[ 2 ] = i;
		}
	}

	printf( "Most Common are %d, %d, %d.\n", tMostCommon[ 0 ], tMostCommon[ 1 ], tMostCommon[ 2 ] );

	tValue = 0;
	for ( i = 0 ; i < NUMBER_OF_THRESHOLDS ; i++ )
	{
		if ( i == tMostCommon[ 0 ] || i == tMostCommon[ 1 ] || (thresholds > 2 && i == tMostCommon[ 2 ]) )
		{
			thresholdStore[ tValue++ ] = tCurves[ i ].start + (tCurves[ i ].end - tCurves[ i ].start) / 2;
		}
	}

	tThresholds[ 0 ] = gMachineData.converter( (double)(thresholdStore[ 0 ] + ( (double)thresholdStore[ 1 ] - (double)thresholdStore[ 0 ]) * gThresholdDivisor[ 0 ]));
	if ( thresholds > 2 )
		tThresholds[ 1 ] = gMachineData.converter( (double)(thresholdStore[ 1 ] + ( (double)thresholdStore[ 2 ] - (double)thresholdStore[ 1 ]) * gThresholdDivisor[ 1 ]));
	else
		tThresholds[ 1 ] = gMachineData.converter( (double)(thresholdStore[ 1 ] + ( (double)NUMBER_OF_THRESHOLDS-1 - (double)thresholdStore[ 1 ]) * gThresholdDivisor[ 1 ]));

	printf( "Thresholds: %d (%d), %d (%d).\n", thresholdStore[ 0 ], tThresholds[ 0 ], thresholdStore[ 1 ], tThresholds[ 1 ] );

	for ( i = 0 ; i < NUMBER_OF_THRESHOLDS ; i ++ )
		valueTable[ i ] = -1;
	for ( i = (int)gMachineData.converter((double)( thresholdStore[ 0 ]) / 2) ; i < tThresholds[ 0 ] ; i++ )	// was 3/4, and 1/4 below.
		valueTable[ i ] = 0;

	if ( thresholds > 2 )
	{
		for ( i = tThresholds[ 0 ] ; i < tThresholds[ 1 ] ; i++ )
			valueTable[ i ] = 1;
		for ( i = tThresholds[ 1 ]  ; i < NUMBER_OF_THRESHOLDS - 1 - (tThresholds[ 1 ] / 2) ; i++ )
			valueTable[ i ] = 2;
	}
	else
	{
		for ( i = tThresholds[ 0 ]  ; i < NUMBER_OF_THRESHOLDS - 1 - (tThresholds[ 0 ] / 2) ; i++ )
			valueTable[ i ] = 1;
	}
}

bool BuildWavelengthTable( struct RAW_Format format, double offset, int blocks, int *wavelengthTable )
{
#define BWT_MAX_WAVELENGTH 200
	double tPointer = offset;
	double tWavelength = 0.0f;
	int i, tBlocks = blocks, tWaveCounter = 0;
	double *tWaveCycles, *tNumberOfCycles;

	tWaveCycles = (double *)malloc( 32 * 1024 * 1024 );
	if ( tWaveCycles == NULL )
	{
		printf(" Out of memory building wavelength table.\n" );
		return false;
	}
	tWaveCycles[ 0 ] = 0.0f;
	tNumberOfCycles = &tWaveCycles[ 0 ];

	for ( i=0 ; i < NUMBER_OF_THRESHOLDS ; i++ )
		wavelengthTable[ i ] = -1;

	if ( gAlgorithmType == WAVE )
		BuildRawWaveList_Cleaned( format, (unsigned long)offset, tWaveCycles, blocks, false ); 
	else
		BuildRawWaveList( format, (unsigned long)offset, tWaveCycles, blocks, false ); 

	if ( (int)*tNumberOfCycles < 50 )
	{
		// This is no wave!
//		for ( i=0 ; i < NUMBER_OF_THRESHOLDS ; i++ )
//			wavelengthTable[ i ] = -1;
		printf(" Not enough waves: only %d found.\n", tWaveCounter );
		free( tWaveCycles );
		return false;
	}


	for ( i = 0 ; i < (int)*tNumberOfCycles ; i++ )
	{
		if ( tWaveCycles[ i+1 ] < NUMBER_OF_THRESHOLDS )
			wavelengthTable[ (int)tWaveCycles[ i+1 ] ]++;
	}

		/*
	while ( tPointer >= 0 && tPointer < format.size )
	{
		tOldPointer = tPointer;
		printf( "Searching for next wave from %f.\n", tPointer );
		tPointer = (double)FindNextWavelengthStart( format, tPointer );

		if ( tPointer < 0 )
		{
			printf( "No wavelength found.\n" );	
			return false;
		}

		// Now we're pointing to the beginning of a potential wave.
		tWavelength = 128;
		while ( tWavelength > 0 && tWavelength < BWT_MAX_WAVELENGTH )
		{
			double tOldPointer = tPointer;
			static bool tLog = false;

			if ( tPointer > 6850258 && tLog == false )
			{
				tLog = true;
			}

			tWavelength = FindWavelength( format, &tPointer, false, true, true);

			if ( tWavelength > 0 && tWavelength < NUMBER_OF_THRESHOLDS )
			{
				// We have a valid wavelength!
	if ( (int)tWavelength >= 36 && (int)tWavelength <= 38 )
	{
		bool test=true;
	}

				wavelengthTable[ (int)tWavelength ]++;
			}
			tWaveCounter++;
		}
		tBlocks--;
		if ( tBlocks == 0 )
		{
			printf( "End of block found at %f.\n", tPointer );
			break;
		}
	}
*/

#if 1 && defined(_DEBUG)
	// Print out the wavelengths.
	{
		int i;
		for ( i=0 ; i<NUMBER_OF_THRESHOLDS ; i++ )
			if ( wavelengthTable[ i ] > 0 )
				printf( "%d: %d ", i, wavelengthTable[ i ] );
		printf( "\n" );
	}
#endif

	printf("Found wave at %f.\n", tPointer );


	free( tWaveCycles );
	return true;
}

double FindWavelength( struct RAW_Format format, double *pointer, bool firstHalfWave, bool preParse, bool halfWaves, double *wavelengthTable )
{
	// Find out how long before we're going under again.
	double tPointer = *pointer;
	bool tHasBeenValid = false;
	double tWavelength = 0, tUpWaveLength = 0;
	bool tWarn = false;
	int tAmplitude = 0;
	double tStartSearch = *pointer;
	bool tCurveIsNegative = false;

	while( tPointer < format.size && tHasBeenValid == false )
	{
		if ( halfWaves )
		{
			bool tCorrectWave = false;
			// Half-waves mean use amplitude on the -ve part too, so we use 0 as a reference point.
			tAmplitude = 0;
			while ( tCorrectWave == false && tPointer < format.size )
			{
				while ( tPointer < format.size && format.data[ (int)tPointer ] == 0 )
					tPointer++;
				if ( (/*firstHalfWave && */format.data[ (int)tPointer ] < 0) || (/*!firstHalfWave &&*/ format.data[ (int)tPointer ] > 0) )
 					tCorrectWave = true;
				else
					tPointer++;
			}
			tCurveIsNegative = (format.data[ (int)tPointer ] < 0) /*firstHalfWave*/;
		}
		else
		{
			if ( format.data[ (int)tPointer ] < tAmplitude )
				tAmplitude = format.data[ (int)tPointer ];
		}
		tPointer++;

		if ( tPointer >= format.size )
		{
			*pointer = tPointer;
			return 0;
		}

		while ( tPointer < format.size && format.data[ (int)tPointer ] == 0 )
			tPointer++;

		if ( tCurveIsNegative )
		{
			while ( tPointer < format.size && format.data[ (int)tPointer ] < 0 )
			{
				if ( (abs(format.data[ (int)tPointer ]) - tAmplitude ) >= gPreferences.minimumSignalLevel / 2 )
					tHasBeenValid = true;
				tPointer++;
			}
		}
		else
		{
			while ( tPointer < format.size && format.data[ (int)tPointer ] > 0 )
			{
				if ( (format.data[ (int)tPointer ] - tAmplitude ) >= gPreferences.minimumSignalLevel / (halfWaves ? 2 : 1) )
					tHasBeenValid = true;
				tPointer++;
			}
		}

		if ( tPointer == format.size )
		{
			*pointer = tPointer;
			return 0;
		}
	}

	tWavelength = (tPointer - *pointer);

	// It's valid! Yay!
	*pointer = tPointer;

	if ( preParse == false )
	{
		if ( tPointer - tWavelength != tStartSearch )
			AddWave( wavelengthTable, 1, (double)(tPointer - tWavelength - tStartSearch) );
		AddWave( wavelengthTable, 1, tWavelength );
	}

	return tWavelength;
}

unsigned long ConvertToCycles( double samples )
{
	double tLengthInSeconds;
	unsigned long tCycles;
	unsigned long tSpeed;

	tLengthInSeconds = samples / gWAVFormat.sampleRate;
	if ( gVideoType == PAL )
		tSpeed = gMachineData.speed_PAL;
	else
		tSpeed = gMachineData.speed_NTSC;
	tCycles = (unsigned long)(((double)tSpeed) * tLengthInSeconds);
	
	return tCycles;
}

double ConvertFromCycles( int cycles )
{
	double tLengthInSeconds;
	double tSamples;
	unsigned long tSpeed;

	if ( gVideoType == PAL )
		tSpeed = gMachineData.speed_PAL;
	else
		tSpeed = gMachineData.speed_NTSC;
	tLengthInSeconds = (double)cycles / (double)tSpeed;
	tSamples = (double)(tLengthInSeconds * (double)gOutputWAVFormat.sampleRate );
	
	return tSamples;
}

double ConvertFromCyclesToSeconds( int cycles )
{
	double tLengthInSeconds;
	unsigned long tSpeed;

	if ( gVideoType == PAL )
		tSpeed = gMachineData.speed_PAL;
	else
		tSpeed = gMachineData.speed_NTSC;
	tLengthInSeconds = (double)cycles / (double)tSpeed;
	
	return tLengthInSeconds;
}
