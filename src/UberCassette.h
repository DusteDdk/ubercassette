#include <string.h>

#ifndef snprintf
#  define snprintf sprintf_s
#endif
#ifdef LINUX
#  define strnicmp strncasecmp
#endif

#ifndef bool
#  define bool int
#endif

#ifndef false
#  define false 0
#  define true 1
#endif



typedef unsigned long (*CycleConverter)( double samples );

enum MachineType { VIC20, C16, C64, BBC, ELECTRON, SPECTRUM, AMSTRAD };
enum VideoType { PAL, NTSC, NTSC2 };
enum AudioChannel { AUDIO_LEFT, AUDIO_RIGHT, AUDIO_BOTH };
enum Loader { LOADER_AUTODETECT, LOADER_ROM, LOADER_SPEEDLOCK, LOADER_UNKNOWN };

struct MachineData
{
	char string[ 12 ];
	char machineID;
	enum MachineType type;
	unsigned long speed_PAL;
	unsigned long speed_NTSC;
	bool halfWave;
	bool invertWave;
	double divisors[ 2 ];
	CycleConverter converter;
};

struct Preferences
{
	bool normalise;
	enum AudioChannel audioChannel;
	enum Loader loader;
	int minimumSignalLevel;
};

extern struct MachineData gMachineData;
extern int gRawLength;
extern double gThresholdDivisor[], gWaveCycles[];
extern int gRawCycles[];

enum MediumType { WAV, T64, TAP, UEF, TZX, CSW, CDT };
enum AlgorithmType { TRIGGER, WAVE, SAW };
extern enum MediumType gInputType, gOutputType;
extern enum AlgorithmType gAlgorithmType;
extern enum VideoType gVideoType;
extern struct Preferences gPreferences;


void AddRawCycle( double length );
void AddMachineCycle( int cycles );
unsigned long CorrectEndianLong( unsigned long source );
unsigned short CorrectEndianShort( unsigned short source );
unsigned long ConvertTo8087Float( double value );

#define PI 3.1415926535897932f
