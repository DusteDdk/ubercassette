#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>

#include "UberCassette.h"

#include "CBM.h"
#include "Acorn.h"
#include "Spectrum.h"
#include "Amstrad.h"

#include "T64.h"
#include "TAP.h"
#include "UEF.h"
#include "WAV.h"
#include "TZX.h"
#include "CSW.h"

#include "Sample.h"

#define VERSION_STRING "V0.04"

void ShowHelp();
unsigned char *LoadData( char *filename, enum MediumType inputType );
void SaveData( unsigned char *data, char *outputFilename, enum MediumType outputType );
bool IsBigEndian();

struct MachineData gMachineData;

#include "Machines.h"

int gRawLength = 0;
int gRawCycles[ 1024 * 1024 * 4 ];
double gWaveCycles[ 1024 * 1024 * 4 ];
double gTotalCycles = 0;
enum MediumType gInputType = WAV, gOutputType = TAP;

enum MediumType GetMediumType( char *filename );
bool ParseParameters( int argc, char *argv[] );
char gInputFilename[ 1024 ], gOutputFilename[ 1024 ];
enum AlgorithmType gAlgorithmType;
enum VideoType gVideoType;
double gThresholdDivisor[ 2 ];
struct Preferences gPreferences;
unsigned int gWaveLengthTable[ NUMBER_OF_THRESHOLDS ];
signed int gSample_ValueTable[ NUMBER_OF_THRESHOLDS ];
int gThresholds[ 3 ];

bool gBigEndian = false;

int main( int argc, char *argv[] )
{
	unsigned char *tData;

    gRAWFormat.data = (char *)NULL;

	if ( argc < 3 )
	{
		ShowHelp();
		exit( 0 );
	}

    gBigEndian = IsBigEndian();

	if ( ParseParameters( argc, argv ) )
	{
		tData = LoadData( gInputFilename, gInputType );
		if ( tData )
    		SaveData( tData, gOutputFilename, gOutputType );
	}
}

bool ParseParameters( int argc, char *argv[] )
{
	int tParameterNumber = 0;
	char *tParameter;

	gInputFilename[ 0 ] = 0x00;
	gOutputFilename[ 0 ] = 0x00;

	gMachineData = MACHINE_C64;
	gAlgorithmType = TRIGGER;
	gPreferences.audioChannel = AUDIO_BOTH;
	gPreferences.normalise = true;
	gPreferences.loader = LOADER_AUTODETECT;
	gPreferences.minimumSignalLevel = 0x08;

	gThresholdDivisor[ 0 ] = gThresholdDivisor[ 1 ];

	for ( tParameterNumber = 1 ; tParameterNumber < argc ; tParameterNumber++ )
	{
		tParameter = argv[ tParameterNumber ];
		if ( *tParameter == '-' )
		{
			if ( !strnicmp( tParameter, "-machine", 8 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();
					return false;
				}
				if ( !strnicmp( tParameter, "c64", 3 ) )
				{
					gMachineData = MACHINE_C64;
				}
				else if ( !strnicmp( tParameter, "vic20", 3 ) )
				{
					gMachineData = MACHINE_VIC;
				}
				else if ( !strnicmp( tParameter, "c16", 3 ) )
				{
					gMachineData = MACHINE_C16;
				}
				else if ( !strnicmp( tParameter, "bbc", 3 ) )
				{
					gMachineData = MACHINE_BBC;
				}
				else if ( !strnicmp( tParameter, "electron", 8 ) )
				{
					gMachineData = MACHINE_ELECTRON;
				}
				else if ( !strnicmp( tParameter, "spectrum", 8 ) )
				{
					gMachineData = MACHINE_SPECTRUM;
				}
				else if ( !strnicmp( tParameter, "amstrad", 7 ) )
				{
					gMachineData = MACHINE_AMSTRAD;
				}
			}
			else if ( !strnicmp( tParameter, "-algorithm", 10 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				if ( !strnicmp( tParameter, "trigger", 7 ) )
					gAlgorithmType = TRIGGER;
				else if ( !strnicmp( tParameter, "wave", 4 ) )
					gAlgorithmType = WAVE;
				else if ( !strnicmp( tParameter, "saw", 3 ) )
					gAlgorithmType = SAW;
			}
			else if ( !strnicmp( tParameter, "-video", 6 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				gVideoType = PAL;
				if ( !strnicmp( tParameter, "ntsc2", 5 ) )
					gAlgorithmType = NTSC2;
				else if ( !strnicmp( tParameter, "ntsc", 4 ) )
					gAlgorithmType = NTSC;
			}
			else if ( !strnicmp( tParameter, "-normalise", 10 ) || !strnicmp( tParameter, "-normalize", 10 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				if ( !strnicmp( tParameter, "OFF", 3 ) )
					gPreferences.normalise = false;
			}
			else if ( !strnicmp( tParameter, "-audio", 6 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				gPreferences.audioChannel = AUDIO_BOTH;
				if ( !strnicmp( tParameter, "left", 4 ) )
					gPreferences.audioChannel = AUDIO_LEFT;
				else if ( !strnicmp( tParameter, "right", 5 ) )
					gPreferences.audioChannel = AUDIO_RIGHT;
			}
			else if ( !strnicmp( tParameter, "-minsignal", 9 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				if ( (atof( tParameter )) > 0.0f )
					gPreferences.minimumSignalLevel = atoi( tParameter );
			}
			else if ( !strnicmp( tParameter, "-threshold1", 11 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				if ( (atof( tParameter )) > 0.0f )
					gThresholdDivisor[ 0 ] = (double)atof( tParameter );
			}
			else if ( !strnicmp( tParameter, "-threshold2", 11 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				if ( (atof( tParameter )) > 0.0f )
					gThresholdDivisor[ 1 ] = (double)atof( tParameter );
			}
			else if ( !strnicmp( tParameter, "-thresholds", 11 ) )
			{
				tParameter = argv[ ++tParameterNumber ];
				if ( tParameterNumber == argc )
				{
					ShowHelp();				
					return false;
				}
				if ( (atof( tParameter )) > 0.0f )
					gThresholdDivisor[ 1 ] = gThresholdDivisor[ 0 ] = (double)atof( tParameter );
			}
		}
		else
		{
			if ( *gInputFilename )
			{
				if ( *gOutputFilename == 0x00 )
					strcpy( gOutputFilename, tParameter );
			}
			else
				strcpy( gInputFilename, tParameter );
		}
	}

	gInputType = GetMediumType( gInputFilename );
	gOutputType = GetMediumType( gOutputFilename );

	if ( gThresholdDivisor[ 0 ] == 0.0f )
		gThresholdDivisor[ 0 ] = gMachineData.divisors[ 0 ];
	if ( gThresholdDivisor[ 1 ] == 0.0f )
		gThresholdDivisor[ 1 ] = gMachineData.divisors[ 1 ];

	return true;
}

enum MediumType GetMediumType( char *filename )
{
	char *tExtension;

	tExtension = strrchr( filename, '.' );
	if ( tExtension == NULL )
		return WAV;

	if ( !strnicmp( tExtension, ".WAV", 4 ) )
		return WAV;
	if ( !strnicmp( tExtension, ".TAP", 4 ) )
		return TAP;
	if ( !strnicmp( tExtension, ".T64", 4 ) )
		return T64;
	if ( !strnicmp( tExtension, ".UEF", 4 ) )
		return UEF;
	if ( !strnicmp( tExtension, ".TZX", 4 ) )
		return TZX;
	if ( !strnicmp( tExtension, ".CDT", 4 ) )
		return CDT;
	if ( !strnicmp( tExtension, ".CSW", 4 ) )
		return CSW;

	// Default is WAV.
	return WAV;
}


void AddRawCycle( double length )
{
	int tCycles;
	if ( length < 0.0f )
	{
		bool test=true;
	}
	if ( length > 1000 )
	{
		bool test=true;
	}
	if ( length >= 79 && length <= 86 )
	{
		bool test=true;
	}
	gWaveCycles[ gRawLength ] = length;
	tCycles = gMachineData.converter( length );
	gTotalCycles += length;
	gRawCycles[ gRawLength++ ] = tCycles;
}

void AddMachineCycle( int cycles )
{
	gWaveCycles[ gRawLength ] = ConvertFromCycles( cycles );
	gRawCycles[ gRawLength++ ] = cycles;
}

unsigned char *LoadData( char *filename, enum MediumType inputType )
{
	struct stat tFileStat;
	int tFileLength = 0, tBytesRead = 0;
	unsigned char *tSourceData, *tParsedData = NULL; 
	FILE *tFile;

	stat( filename, &tFileStat );
	tFileLength = tFileStat.st_size;
	tSourceData = (unsigned char *)malloc( tFileLength );

	tFile = fopen( filename, "rb" );
	if ( tFile == NULL )
	{
		printf( "Couldn't open input file.\n" );
		return NULL;
	}
	tBytesRead = fread( tSourceData, 1, tFileLength, tFile );
	fclose( tFile );

    if ( tBytesRead == 0 )
    {
        printf("Error! No bytes read.\n" );
        free( tSourceData );
        return NULL;
    }
    else
    {
//        printf("Read %d bytes of WAV.\nBytes are: %02x %02x %02x %02x.\n", tSourceData[ 0 ], tSourceData[ 1 ], tSourceData[ 2 ], tSourceData[ 3 ] );
    }

	switch ( inputType )
	{
	case WAV:
	{
		switch ( gMachineData.type )
		{
		case VIC20:
		case C16:
		case C64:
			tParsedData = CBM_ParseWAV( tSourceData );
			break;
		case ELECTRON:
		case BBC:
			tParsedData = Acorn_ParseWAV( tSourceData );
			break;
		case SPECTRUM:
			tParsedData = Spectrum_ParseWAV( tSourceData );
			break;
		case AMSTRAD:
			tParsedData = Amstrad_ParseWAV( tSourceData );
			break;
		default:
			break;
		}
		break;
	}
	case TAP:
		tParsedData = TAP_Parse( tSourceData, tBytesRead );
		break;
	case CSW:
		tParsedData = CSW_Parse( tSourceData, tBytesRead );
		break;
	default:
		break;
	}

    free( tSourceData );
    free( gRAWFormat.data );

	return tParsedData;
}

void SaveData( unsigned char *data, char *outputFilename, enum MediumType outputType )
{
	switch ( outputType )
	{
	case T64:
		T64_Output( gCBM_CassetteBlockList, outputFilename );
		break;
	case TAP:
		TAP_Output( gCBM_CassetteBlockList, outputFilename );
		break;
	case UEF:
		UEF_Output( gAcorn_CassetteBlockList, outputFilename );
		break;
	case WAV:
		WAV_Output( outputFilename, gRawCycles, gRawLength, gMachineData.invertWave );
		break;
	case CDT:
	case TZX:
		TZX_Output( gSpectrum_CassetteBlockList, outputFilename );
		break;
	case CSW:
		CSW_Output( gSpectrum_CassetteBlockList, outputFilename );
		break;
	default:
		break;
	}
}

void ShowHelp()
{
	printf( "UberCassette %s\n", VERSION_STRING );
	printf( "Usage:\n" );
	printf( "UberCassette <input filename> <output filename> [-<parameter> <value>]\n" );
	printf( "File type detection depends on file extension. More than one algorithm may\nbe available, if one doesn't work, try the other. Possible extensions are:\n" );
	printf( "Input:\t\t.WAV .TAP .CSW\nOutput:\t\t.T64 .TAP .UEF .TZX .CSW .WAV\n" );
	printf( "Parameters available are:\n" );
	printf( "-machine:\tC64 (default)  C16  VIC20  BBC  ELECTRON  SPECTRUM\n" );
	printf( "-algorithm:\tTRIGGER(default)  WAVE\n" );
	printf( "-video:\t\tPAL (default)  NTSC\n" );
	printf( "-threshold1:\tCut-off Distance (0.0 - 1.0) from short bit to large bit\n" );
	printf( "-threshold2:\tCut-off Distance (0.0 - 1.0) from large bit to very large bit\n" );
	printf( "-audio:\t\tBOTH (default)  LEFT  RIGHT\tWhich channel of a stereo file to use.\n" );
	printf( "-normalise:\tON (default)  OFF\tNormalise the input waveform.\n" );
	printf( "-minsignal:\t<integer> is minimum amplitude for a signal. Default: 8.\n" );
	return;
}

bool IsBigEndian()
{
    unsigned char tTestBytes[ 2 ];
    *((unsigned short *)&tTestBytes[ 0 ]) = 0x1234;
    
    if ( tTestBytes[ 0 ] == 0x12 )
        return true;
    
    return false;
}

unsigned long CorrectEndianLong( unsigned long source )
{
    unsigned long tValue;
    
    if ( gBigEndian == false )
        return source;
    
    tValue  = (source & 0x000000FF) << 24;
    tValue |= (source & 0x0000FF00) <<  8;
    tValue |= (source & 0x00FF0000) >>  8;
    tValue |= (source & 0xFF000000) >> 24;

    return tValue;    
}

unsigned short CorrectEndianShort( unsigned short source )
{
    unsigned short tValue;
    
    if ( gBigEndian == false )
        return source;
        
    tValue  = (source & 0x00FF) << 8;
    tValue |= (source & 0xFF00) >> 8;

    return tValue;
}

unsigned long ConvertTo8087Float( double value )
{
/* assume that the doubleing point number 'Value' is to be stored */
	double mantissa;
	int exponent;
	unsigned char tdouble[4];
	unsigned long IMantissa;

	/* sign bit */
	if(value < 0)
	{
		value = -value;
		tdouble[3] = 0x80;
	}
	else
		tdouble[3] = 0;

	/* decode mantissa and exponent */
	mantissa = (double)frexp(value, &exponent);
	exponent += 126;

	/* store mantissa */
	IMantissa = (unsigned long)(mantissa * (1 << 24));
	tdouble[0] = IMantissa&0xff;
	tdouble[1] = (IMantissa >> 8)&0xff;
	tdouble[2] = (IMantissa >> 16)&0x7f;

	/* store exponent */
	tdouble[3] |= exponent >> 1;
	tdouble[2] |= (exponent&1) << 7;
	
	return *((unsigned long *)&tdouble[0]);
}
