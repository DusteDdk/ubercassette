/* Helper functions for using WAV files. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "UberCassette.h"
#include "WAV.h"
#include "Sample.h"

struct WAV_ChunkPrototype;



struct WAV_Format gWAVFormat, gOutputWAVFormat;
struct RAW_Format gRAWFormat;

void WAV_ParseBlock_FMT( struct WAV_FormatChunk *chunk )
{
	gWAVFormat.numberOfChannels = CorrectEndianShort( chunk->numberOfChannels );
	gWAVFormat.sampleRate = CorrectEndianLong( chunk->sampleRate );
	gWAVFormat.significantBitsPerSample = CorrectEndianShort( chunk->significantBitsPerSample );
	return;
}

void WAV_ParseBlock_DATA( struct WAV_DataChunk *chunk, bool invert )
{
	int tRawSize, tChannel = 0;
	unsigned long tInputByte;
	signed char *tOutputPtr;
    unsigned long tDataSize;

	// Convert the data to 8-bit signed, 44KHz.
	tRawSize = CorrectEndianLong( chunk->dataSize );
	if ( gWAVFormat.numberOfChannels == 2 )
		tRawSize /= 2;
	if ( gWAVFormat.significantBitsPerSample != 8 )
		tRawSize /= (gWAVFormat.significantBitsPerSample / 8);

	gRAWFormat.size = tRawSize;
	tOutputPtr = gRAWFormat.data = (signed char *)malloc( tRawSize );
    if  (tOutputPtr == NULL )
    {
        printf( "Out of memory allocating WAV data.\n" );
        return;
    }

    tDataSize = CorrectEndianLong( chunk->dataSize );

	for ( tInputByte = 0 ; tInputByte < tDataSize ; )
	{
		if ( tChannel == 0 )
			*tOutputPtr = 0;

		if ( tChannel == 1 && gPreferences.audioChannel == AUDIO_LEFT 
			|| tChannel == 0 && gPreferences.audioChannel == AUDIO_RIGHT )
		{
			// Skip the right channel.
			tInputByte += gWAVFormat.significantBitsPerSample / 8;
			tChannel++;
			if ( tChannel > (gWAVFormat.numberOfChannels-1))
			{
				tChannel = 0;
				tOutputPtr++;
			}
			continue;
		}

		tInputByte += (gWAVFormat.significantBitsPerSample / 8) - 1;

		if ( gWAVFormat.significantBitsPerSample == 8 )
			*tOutputPtr += (unsigned char)((128 + (int)(chunk->data[ tInputByte++ ])) * (invert ? -1 : 1 ));
		else
			*tOutputPtr += (signed char)((double)(chunk->data[ tInputByte++ ] * (invert ? -1 : 1 )));

		tChannel++;

		if ( tChannel > (gWAVFormat.numberOfChannels-1))
		{
			tChannel = 0;
			tOutputPtr++;
		}
	}

	if ( gPreferences.normalise )
		WAV_Normalise( gRAWFormat );

	return;
}

void WAV_Normalise( struct RAW_Format format )
{
	signed char *tPointer = format.data;
	double tAmplify = 0.0f;
	double tTestAmp = 0.0f;

	while ( tPointer < format.data + format.size )
	{
		if ( *tPointer == -128 || *tPointer == 127 )
		{
			// No amplification necessary, ta.
			return;
		}

		tTestAmp = (double)(abs( *tPointer )) / 127.0f;
		if ( tTestAmp > tAmplify )
			tAmplify = tTestAmp;

		tPointer++;
	}

	if ( tAmplify < 0.00001f )
		return;

	tPointer = format.data;
	while ( tPointer < format.data + format.size )
	{
		*tPointer++ = (signed char)((double)(*tPointer) / tAmplify);
	}
}

unsigned long WAV_ParseBlock( struct WAV_ChunkPrototype *chunk, bool invert )
{
	if (!strnicmp( ((struct WAV_ChunkPrototype *)chunk)->ID, "fmt ", 4))
	{
		WAV_ParseBlock_FMT( (struct WAV_FormatChunk *)chunk );
	}
	if ( !strnicmp( ((struct WAV_ChunkPrototype *)chunk)->ID, "data", 4 ))
	{
		WAV_ParseBlock_DATA( (struct WAV_DataChunk *)chunk, invert );
	}

	return CorrectEndianLong( chunk->dataSize ) + 8;
}

struct RAW_Format WAV_GetRAW( unsigned char *wavData, bool invert )
{
	struct WAV_Header *tHeader = (struct WAV_Header *)wavData;
	int tFileSize = 0;
	struct WAV_ChunkPrototype *tChunk = NULL;
	unsigned char *tAddress;
	unsigned long tChunkLength;

    printf( "WAV_GetRaw.\n" );

	gRAWFormat.data = NULL;
	gRAWFormat.size = 0;

	if ( strnicmp( (char *)tHeader->ID, "RIFF", 4 ) )
	{
		// Not a valid RIFF file.
		printf("WAV is not a valid RIFF file.\n" );
		return gRAWFormat;
	}

	if ( strnicmp( (char *)tHeader->RIFFtype, "WAVE", 4 ) )
	{
		// Not a valid WAVE file.
		printf("WAV is not a valid WAVE file.\n" );
		return gRAWFormat;
	}

	// We have a valid file!
	tFileSize = CorrectEndianLong( tHeader->dataSize ) - 8;

	// Loop through the chunks till we get useful data.
	tChunk = (struct WAV_ChunkPrototype *)((char *)tHeader + sizeof( struct WAV_Header ));
	tAddress = (unsigned char *)tChunk;
	printf( "tFileSize is %d.\n", tFileSize );

	while ( ((unsigned char *)tChunk - wavData) < tFileSize )
	{
		// Add on the length of the chunk.
		tChunkLength = WAV_ParseBlock( tChunk, invert );
		tAddress += tChunkLength;
		tChunk = (struct WAV_ChunkPrototype *)tAddress;
	}

    if ( gRAWFormat.data == NULL )
        return gRAWFormat;

#if defined(DUMP_RAW)
	{
		FILE *tFile;
        printf( "Dumping RAW file of size %ld bytes.\n", gRAWFormat.size );
#if defined(WIN32)
		tFile = fopen( "c:\\temp\\temp.raw", "wb" );
#else
		tFile = fopen( "temp.raw", "wb" );
#endif
		if ( tFile )
		{
			fwrite( gRAWFormat.data, gRAWFormat.size, 1, tFile );
			fclose( tFile );
		}
        printf( "Done.\n" );
	}
#endif

	return gRAWFormat;
}

void WAV_Output( char *filename, int *data, unsigned long size, bool invert )
{
	signed char *tWAVSignal = NULL;

	tWAVSignal = (signed char *)malloc(32 * 1024 * 1024 );
	if ( tWAVSignal == NULL )
	{
		printf( "Can't allocate memory for output WAV file.\n" );
		return;
	}

	gOutputWAVFormat.numberOfChannels = 1;
	gOutputWAVFormat.sampleRate = 44100;
	gOutputWAVFormat.significantBitsPerSample = 8;

	// We have a memory space for our WAV now.
	WAV_CreateWAV( filename, tWAVSignal, data, size, invert );

	free( tWAVSignal );

	return;
}

void WAV_CreateWAV( char *filename, signed char *wavBuffer, int *data, int size, bool invert )
{
	struct WAV_Header tHeader;
	struct WAV_FormatChunk tFormatChunk;
	struct WAV_DataChunk tDataChunk;
	int tLength = 0;
	FILE *tOutputFile = NULL;

	tOutputFile = fopen( filename, "wb" );
	if ( tOutputFile == NULL )
	{
		printf( "Couldn't open output file.\n" );
		return;
	}
	WAV_CreateHeader( &tHeader );
	WAV_CreateFormatChunk( &tFormatChunk );
	WAV_CreateDataChunk( &tDataChunk );

	tLength = WAV_CreateData( wavBuffer, data, size, invert );

	tHeader.dataSize = CorrectEndianLong( 24 + 8 + tLength );
	tDataChunk.dataSize = CorrectEndianLong( tLength );

	fwrite( &tHeader, sizeof( struct WAV_Header ), 1, tOutputFile );
	fwrite( &tFormatChunk, sizeof( struct WAV_FormatChunk ), 1, tOutputFile );
	fwrite( &tDataChunk, sizeof( struct WAV_DataChunk ), 1, tOutputFile );
	fwrite( wavBuffer, tLength, 1, tOutputFile );

	fclose( tOutputFile );
}

void WAV_CreateHeader( struct WAV_Header *header )
{
	header->ID[ 0 ] = 'R'; header->ID[ 1 ] = 'I'; header->ID[ 2 ] = 'F'; header->ID[ 3 ] = 'F';
	header->RIFFtype[ 0 ] = 'W'; header->RIFFtype[ 1 ] = 'A'; header->RIFFtype[ 2 ] = 'V'; header->RIFFtype[ 3 ] = 'E';
}

void WAV_CreateFormatChunk( struct WAV_FormatChunk *formatChunk )
{
	formatChunk->ID[ 0 ] = 'f'; formatChunk->ID[ 1 ] = 'm'; formatChunk->ID[ 2 ] = 't'; formatChunk->ID[ 3 ] = ' ';
	formatChunk->dataSize = CorrectEndianLong( 16 );
	formatChunk->compressionCode = CorrectEndianShort( 1 );
	formatChunk->numberOfChannels = CorrectEndianShort( gOutputWAVFormat.numberOfChannels );
	formatChunk->sampleRate = CorrectEndianLong( gOutputWAVFormat.sampleRate );
	formatChunk->bytesPerSecond = CorrectEndianLong( gOutputWAVFormat.sampleRate * gOutputWAVFormat.numberOfChannels * (gOutputWAVFormat.significantBitsPerSample / 8) );
	formatChunk->blockAlign = CorrectEndianShort( gOutputWAVFormat.numberOfChannels * ( gOutputWAVFormat.significantBitsPerSample / 8 ) );
	formatChunk->significantBitsPerSample = CorrectEndianShort( gOutputWAVFormat.significantBitsPerSample );
}

void WAV_CreateDataChunk( struct WAV_DataChunk *dataChunk )
{
	sprintf( dataChunk->ID, "data" );
}

int WAV_CreateData( signed char *wavBuffer, int *data, int size, bool invert )
{
	int tIndex = 0;
	double tYFactor = 0.0f;
	double tCycleLength = 0.0f;
	double tWaveIndex = 0;
	signed char *tWAVPointer = wavBuffer;
	double tXMod = 0.0f;
	double tXPos = 0.0f;
	bool tPhase = false;

//	for ( tIndex = 0 ; tIndex < (int)gOutputWAVFormat.sampleRate * gOutputWAVFormat.numberOfChannels * gOutputWAVFormat.significantBitsPerSample ; tIndex++ )
//		*tWAVPointer++ = (gOutputWAVFormat.significantBitsPerSample == 8 ? 0x80 : 0x00 );

	tIndex = 0;

	while ( tIndex < size )
	{
		if ( data[ tIndex ] > 20000 )
		{
			// This is likely to be blank space.
			tYFactor = 0.0f;
		}
		else
		{
			// We're in a signal.
			tYFactor = 0.8f;	// Not a full wave, but nice and large.
			if ( tPhase )
				tYFactor = -tYFactor;
		}

		tCycleLength = ConvertFromCycles( data[ tIndex ] );
		tXMod = ((gMachineData.halfWave ? 1 : 2) * PI) / tCycleLength;

		while ( tWaveIndex < tCycleLength )
		{
			tXPos = tXMod * tWaveIndex;
			*tWAVPointer = ((signed char )((tYFactor * sin( tXPos ) * 127.0f)) + 0x80);
			tWaveIndex += 1.0f;
			tWAVPointer++;
		}

		if ( gMachineData.halfWave )
			tPhase = !tPhase;
		tWaveIndex -= tCycleLength;
		tIndex++;
	}
	
	for ( tIndex = 0 ; tIndex < (int)gOutputWAVFormat.sampleRate * gOutputWAVFormat.numberOfChannels * gOutputWAVFormat.significantBitsPerSample ; tIndex++ )
		*tWAVPointer++ = (gOutputWAVFormat.significantBitsPerSample == 8 ? 0x80 : 0x00 );

	return (tWAVPointer - wavBuffer);
}

