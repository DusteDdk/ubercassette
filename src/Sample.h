// Tool functions for getting the data ready to parse (if necessary).

#include "WAV.h"

void BuildRawWaveList( struct RAW_Format format, unsigned long offset, double *wavelengthTable, int blocks, bool halfWaves );
double FindNextWavelengthStart( struct RAW_Format format, double pointer );
bool BuildWavelengthTable( struct RAW_Format format, double offset, int blocks, int *wavelengthTable );
double FindWavelength( struct RAW_Format format, double *pointer, bool firstWave, bool preParse, bool fullWave, double *wavelengthTable );
void BuildThresholds( struct RAW_Format format, int thresholds, int *thresholdStore, int *wavelengthTable, int *valueTable );
void BuildRawWaveList_Cleaned( struct RAW_Format format, unsigned long offset, double *wavelengthTable, int blocks,  bool halfWaves );
void AddWave( double *list, int amount, double cycles );
unsigned long ConvertToCycles( double samples );
double ConvertFromCycles( int cycles );
double ConvertFromCyclesToSeconds( int cycles );

#define NUMBER_OF_THRESHOLDS (20 * 1024)

extern unsigned int gWaveLengthTable[ NUMBER_OF_THRESHOLDS ];
extern signed int gSample_ValueTable[ NUMBER_OF_THRESHOLDS ];
extern int gThresholds[ 3 ];
static const int gHeaderCheckSize = 60;
