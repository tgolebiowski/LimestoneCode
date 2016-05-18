void InitSound( const void* platformSpecificThing, int32 SamplesPerSecond, int32 buffersize );
void UpdateSound();
void OutputSound( int16 sampleOutputCount, int16* soundBuffer );

struct SoundOutputBuffer {
	int samplesPerSecond;
	int sampleCount;
	int16* samples;
};

void OutputDebugTone( SoundOutputBuffer* soundOutputBuffer ) {
	const int hz = 440;
	const int volume = 3000;
	const int WavePeriod = soundOutputBuffer->samplesPerSecond / hz;

	int16* sampleOut = soundOutputBuffer->samples;
	for( int sampleIndex = 0; sampleIndex < soundOutputBuffer->sampleCount; ++sampleIndex ) {
		float t = 2.0f * PI * (float)sampleIndex / (float)WavePeriod;
		int16 sampleValue = volume * sinf( t );
		*sampleOut++ = sampleValue; //Left channel
		*sampleOut++ = sampleValue; //Right channel
	}
}

#ifdef WIN32_ENTRY
#include <mmsystem.h>
#include <dsound.h>

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name( LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter )
typedef DIRECT_SOUND_CREATE( direct_sound_create );

static struct {
	int32 samplesPerSecond;
	int32 writeBufferSize;
	uint8 bytesPerSample;
	LPDIRECTSOUNDBUFFER writeBuffer;

	uint32 lastSampleWrite;
} SoundRendererStorage;

static SoundOutputBuffer soundOutputBuffer;

void Win32FillSoundBuffer( DWORD byteToLock, DWORD bytesToWrite ) {
	VOID* region0;
	DWORD region0Size;
	VOID* region1;
	DWORD region1Size;

	const int hz = 440;
	const int volume = 3000;
	const int WavePeriod = SoundRendererStorage.samplesPerSecond / hz;

	HRESULT result = SoundRendererStorage.writeBuffer->Lock( byteToLock, bytesToWrite,
		&region0, &region0Size,
		&region1, &region1Size,
		0 );
	if( !SUCCEEDED( result ) ) {
		printf( "Couldn't Lock Sound Buffer\n" );
	}

	int16* sampleSrc = soundOutputBuffer.samples;
	int16* sampleDest = (int16*)region0;
	DWORD region0SampleCount = region0Size / ( SoundRendererStorage.bytesPerSample * 2 );
	for( DWORD sampleIndex = 0; sampleIndex < region0SampleCount; ++sampleIndex ) {
		*sampleDest++ = *sampleSrc++;
		*sampleDest++ = *sampleSrc++;
		++SoundRendererStorage.lastSampleWrite;
	}
	sampleDest = (int16*)region1;
	DWORD region1SampleCount = region1Size / ( SoundRendererStorage.bytesPerSample * 2 );
	for( DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex ) {
		*sampleDest++ = *sampleSrc++;
		*sampleDest++ = *sampleSrc++;
		++SoundRendererStorage.lastSampleWrite;
	}

	result = SoundRendererStorage.writeBuffer->Unlock( region0, region0Size, region1, region1Size );
	if( !SUCCEEDED( result ) ) {
		printf("Couldn't Unlock\n");
	}
}

void InitSound( const void* platformSpecificThing, int32 SamplesPerSecond, int32 buffersize) {
	HMODULE DirectSoundDLL = LoadLibraryA( "dsound.dll" );

	//TODO: logging on all potential failure points
	if( DirectSoundDLL ) {
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress( DirectSoundDLL, "DirectSoundCreate" );

		LPDIRECTSOUND directSound;
		if( DirectSoundCreate && SUCCEEDED( DirectSoundCreate( 0, &directSound, 0 ) ) ) {

			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = SamplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if( SUCCEEDED( directSound->SetCooperativeLevel( (HWND)platformSpecificThing, DSSCL_PRIORITY ) ) ) {

				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof( bufferDescription );
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER primaryBuffer;
				if( SUCCEEDED( directSound->CreateSoundBuffer( &bufferDescription, &primaryBuffer, 0 ) ) ) {
					if( !SUCCEEDED( primaryBuffer->SetFormat( &waveFormat ) ) ) {
						printf("Couldn't set format primary buffer\n");
					}
				} else {
					printf("Couldn't secure primary buffer\n");
				}
			} else {
				printf("couldn't set CooperativeLevel\n");
			}

			//For the secondary buffer (why do we need that again?)
			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof( bufferDescription );
			bufferDescription.dwFlags = 0; //DSBCAPS_PRIMARYBUFFER;
			bufferDescription.dwBufferBytes = buffersize;
			bufferDescription.lpwfxFormat = &waveFormat;
			HRESULT result = directSound->CreateSoundBuffer( &bufferDescription, &SoundRendererStorage.writeBuffer, 0 );
			if( !SUCCEEDED( result ) ) {
				printf("Couldn't secure a writable buffer\n");
			}

			SoundRendererStorage.samplesPerSecond = SamplesPerSecond;
			SoundRendererStorage.writeBufferSize = buffersize;
			SoundRendererStorage.bytesPerSample = sizeof( int16 );
			SoundRendererStorage.lastSampleWrite = 0;
			
			soundOutputBuffer.samplesPerSecond = SamplesPerSecond;
			soundOutputBuffer.sampleCount = 2 * ( SamplesPerSecond / 30 );
			soundOutputBuffer.samples = (int16*)malloc( sizeof( int16 ) * 2 * ( SamplesPerSecond / 30 ) );
			memset( soundOutputBuffer.samples, 0, sizeof( int16 ) * 2 * ( SamplesPerSecond / 30 ) );

			Win32FillSoundBuffer( 0, buffersize );

			HRESULT playResult = SoundRendererStorage.writeBuffer->Play( 0, 0, DSBPLAY_LOOPING );
			if( !SUCCEEDED( playResult ) ) {
				printf("failed to play\n");
			}

		} else {
			printf("couldn't create direct sound object\n");
		}
	} else {
		printf("couldn't create direct sound object\n");
	}
}

void UpdateSound() {
	DWORD playCursorPosition, writeCursorPosition;
	if( !SUCCEEDED( SoundRendererStorage.writeBuffer->GetCurrentPosition( &playCursorPosition, &writeCursorPosition) ) ){
		printf("couldn't get cursor\n");
		return;
	}

	DWORD bytesToWrite;
	DWORD byteToLock = ( SoundRendererStorage.lastSampleWrite * SoundRendererStorage.bytesPerSample * 2 ) % SoundRendererStorage.writeBufferSize;
	if( byteToLock > playCursorPosition ) {
		bytesToWrite = SoundRendererStorage.writeBufferSize - byteToLock;
		bytesToWrite += playCursorPosition;
	} else {
		bytesToWrite = playCursorPosition - byteToLock;
	}

	OutputDebugTone( &soundOutputBuffer );
	Win32FillSoundBuffer( byteToLock, bytesToWrite );
}

#endif //WIN32 specific implementation