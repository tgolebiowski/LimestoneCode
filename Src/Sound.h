void InitSound( const void* platformSpecificThing, int32 SamplesPerSecond, int32 buffersize );
void PushInfoToSoundCard();

struct SoundRenderBuffer {
	int32 samplesToPush;

	int32 samplesPerSecond;
	int32 sampleCount;
	int16* samples;
};

void OutputTestTone( SoundRenderBuffer* srb, int32 samplesToWrite, int hz = 440 ) {
	const int volume = 3000;
	const int WavePeriod = srb->samplesPerSecond / hz;
	static float tSine = 0;

	samplesToWrite /= 2;

	for( int32 sampleIndex = 0; sampleIndex < samplesToWrite; ++sampleIndex ) {
		tSine += 2.0f * PI / (float)WavePeriod;
		int16 sampleValue = volume * sinf( tSine );
		int32 i = sampleIndex * 2;
		srb->samples[ i ] = sampleValue; //Left Channel
		srb->samples[ i + 1 ] = sampleValue; //Right Channel
		++srb->samplesToPush;
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

	uint32 runningSampleIndex;
	SoundRenderBuffer srb;
} SoundRendererStorage;


void Win32FillSoundBuffer( DWORD byteToLock, DWORD bytesToWrite ) {
	VOID* region0;
	DWORD region0Size;
	VOID* region1;
	DWORD region1Size;

	HRESULT result = SoundRendererStorage.writeBuffer->Lock( byteToLock, bytesToWrite,
		&region0, &region0Size,
		&region1, &region1Size,
		0 );
	if( !SUCCEEDED( result ) ) {
		printf( "Couldn't Lock Sound Buffer\n" );
		return;
	}

	int16* sampleSrc = SoundRendererStorage.srb.samples;
	int16* sampleDest = (int16*)region0;
	DWORD region0SampleCount = region0Size / ( SoundRendererStorage.bytesPerSample * 2 );
	for( DWORD sampleIndex = 0; sampleIndex < region0SampleCount; ++sampleIndex ) {
		*sampleDest++ = *sampleSrc++;
		*sampleDest++ = *sampleSrc++;
		++SoundRendererStorage.runningSampleIndex;
	}
	sampleDest = (int16*)region1;
	DWORD region1SampleCount = region1Size / ( SoundRendererStorage.bytesPerSample * 2 );
	for( DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex ) {
		*sampleDest++ = *sampleSrc++;
		*sampleDest++ = *sampleSrc++;
		++SoundRendererStorage.runningSampleIndex;
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
			SoundRendererStorage.runningSampleIndex = 0;

			SoundRendererStorage.srb.samplesToPush = 0;
			SoundRendererStorage.srb.samplesPerSecond = SamplesPerSecond;
			SoundRendererStorage.srb.sampleCount = buffersize / sizeof( int16 );
			SoundRendererStorage.srb.samples = (int16*)malloc( buffersize );
			memset( SoundRendererStorage.srb.samples, 0, buffersize );

			OutputTestTone( &SoundRendererStorage.srb, SoundRendererStorage.srb.sampleCount );
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

void PushInfoToSoundCard() {
	DWORD playCursorPosition, writeCursorPosition;
	if( !SUCCEEDED( SoundRendererStorage.writeBuffer->GetCurrentPosition( &playCursorPosition, &writeCursorPosition) ) ){
		printf("couldn't get cursor\n");
		return;
	}

	DWORD bytesToWrite;
	DWORD byteToLock = ( SoundRendererStorage.runningSampleIndex * SoundRendererStorage.bytesPerSample * 2 ) % SoundRendererStorage.writeBufferSize;
	if( byteToLock > playCursorPosition ) {
		bytesToWrite = SoundRendererStorage.writeBufferSize - byteToLock;
		bytesToWrite += playCursorPosition;
	} else {
		bytesToWrite = playCursorPosition - byteToLock;
	}

	if( IsKeyDown( 'h' ) ) {
		OutputTestTone( &SoundRendererStorage.srb, bytesToWrite / sizeof( int16 ), 880 );
	} else {
		OutputTestTone( &SoundRendererStorage.srb, bytesToWrite / sizeof( int16 ) );
	}
	Win32FillSoundBuffer( byteToLock, bytesToWrite );
}

#endif //WIN32 specific implementation