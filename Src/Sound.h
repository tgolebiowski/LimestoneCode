void InitSound( const void* platformSpecificThing, int32 SamplesPerSecond, int32 buffersize );
void PushInfoToSoundCard();

struct SoundRenderBuffer {
	int32 samplesPerSecond;
	int32 samplesToWrite;
	int16* samples;
};

void OutputTestTone( SoundRenderBuffer* srb, int hz = 440 ) {
	const int volume = 3000;
	const int WavePeriod = srb->samplesPerSecond / hz;
	static float tSine = 0.0f;
	const float tSineStep = 2.0f * PI / (float)WavePeriod;

	int32 samplesToWrite = srb->samplesToWrite / 2;

	for( int32 sampleIndex = 0; sampleIndex < samplesToWrite; ++sampleIndex ) {
		tSine += tSineStep;
		if( tSine > ( 2.0f * PI ) ) {
			tSine -= ( 2.0f * PI );
		}

		int16 sampleValue = volume * sinf( tSine );
		int32 i = sampleIndex * 2;
		srb->samples[ i ] = sampleValue; //Left Channel
		srb->samples[ i + 1 ] = sampleValue; //Right Channel
	}
}

#ifdef WIN32_ENTRY
#include <mmsystem.h>
#include <dsound.h>
#include <comdef.h>

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name( LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter )
typedef DIRECT_SOUND_CREATE( direct_sound_create );

static struct {
	int32 writeBufferSize;
	int32 safetySampleBytes;
	int32 expectedBytesPerFrame;
	uint8 bytesPerSample;
	LPDIRECTSOUNDBUFFER writeBuffer;

	uint64 runningSampleIndex;
	DWORD bytesToWrite, byteToLock;

	SoundRenderBuffer srb;
} SoundRendererStorage;

void PrepAudio() {
	DWORD playCursorPosition, writeCursorPosition;
	if( SUCCEEDED( SoundRendererStorage.writeBuffer->GetCurrentPosition( &playCursorPosition, &writeCursorPosition) ) ) {
		static bool firstTime = true;
		if( firstTime ) {
			SoundRendererStorage.runningSampleIndex = writeCursorPosition / SoundRendererStorage.bytesPerSample;
			firstTime = false;
		}

	    //Pick up where we left off
		SoundRendererStorage.byteToLock = ( SoundRendererStorage.runningSampleIndex * SoundRendererStorage.bytesPerSample * 2 ) % SoundRendererStorage.writeBufferSize;

	    //Calculate how much to write
		SoundRendererStorage.safetySampleBytes;
		DWORD ExpectedFrameBoundaryByte = playCursorPosition + SoundRendererStorage.expectedBytesPerFrame;

		//DSound can be latent sometimes when reporting the current writeCursor, so this is 
		//the farthest ahead that the cursor could possibly be
		DWORD safeWriteCursor = writeCursorPosition;
		if( safeWriteCursor < playCursorPosition ) {
			safeWriteCursor += SoundRendererStorage.writeBufferSize;
		}
		safeWriteCursor += SoundRendererStorage.safetySampleBytes;

		bool AudioCardIsLowLatency = safeWriteCursor < ExpectedFrameBoundaryByte;

		//Determine up to which byte we should write
		DWORD targetCursor = 0;
		if( AudioCardIsLowLatency ) {
			targetCursor = ( ExpectedFrameBoundaryByte + SoundRendererStorage.expectedBytesPerFrame );
		} else {
			targetCursor = ( writeCursorPosition + SoundRendererStorage.expectedBytesPerFrame + SoundRendererStorage.safetySampleBytes );
		}
		targetCursor = targetCursor % SoundRendererStorage.writeBufferSize;

		//Wrap up on math, how many bytes do we actually write
		if( SoundRendererStorage.byteToLock > targetCursor ) {
			SoundRendererStorage.bytesToWrite = SoundRendererStorage.writeBufferSize - SoundRendererStorage.byteToLock;
			SoundRendererStorage.bytesToWrite += targetCursor;
		} else {
			SoundRendererStorage.bytesToWrite = targetCursor - SoundRendererStorage.byteToLock;
		}

	    //Save number of samples that can be written to platform independent struct
		SoundRendererStorage.srb.samplesToWrite = SoundRendererStorage.bytesToWrite / SoundRendererStorage.bytesPerSample;
	} else {
		printf("couldn't get cursor\n");
		return;
	}

}

void PushAudioToSoundCard() {
	VOID* region0;
	DWORD region0Size;
	VOID* region1;
	DWORD region1Size;

	HRESULT result = SoundRendererStorage.writeBuffer->Lock( SoundRendererStorage.byteToLock, SoundRendererStorage.bytesToWrite,
		&region0, &region0Size,
		&region1, &region1Size,
		0 );
	if( !SUCCEEDED( result ) ) {
		_com_error error( result );
		LPCTSTR errMessage = error.ErrorMessage();
		printf( "Couldn't Lock Sound Buffer\n" );
		printf( "%s \n", errMessage );
		printf( "BTL:%lu BTW:%lu r0:%lu r0s:%lu r1:%lu r1s:%lu\n", SoundRendererStorage.byteToLock, 
			SoundRendererStorage.bytesToWrite, region0, region0Size, region1, region1Size );
		printf( "SoundRenderBuffer Info: Samples--%d\n", SoundRendererStorage.srb.samplesToWrite );
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

void InitSound( const void* platformSpecificThing, int targetGameHZ ) {
	const int32 SamplesPerSecond = 48000;
	const int32 BufferSize = SamplesPerSecond * sizeof( int16 ) * 2;
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
			bufferDescription.dwBufferBytes = BufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			HRESULT result = directSound->CreateSoundBuffer( &bufferDescription, &SoundRendererStorage.writeBuffer, 0 );
			if( !SUCCEEDED( result ) ) {
				printf("Couldn't secure a writable buffer\n");
			}

			SoundRendererStorage.byteToLock = 0;
			SoundRendererStorage.bytesToWrite = 0;
			SoundRendererStorage.writeBufferSize = BufferSize;
			SoundRendererStorage.bytesPerSample = sizeof( int16 );
 			SoundRendererStorage.safetySampleBytes = ( ( SamplesPerSecond / targetGameHZ ) / 2 ) * SoundRendererStorage.bytesPerSample;
 			SoundRendererStorage.expectedBytesPerFrame = ( 48000 * SoundRendererStorage.bytesPerSample * 2 ) / targetGameHZ;
			SoundRendererStorage.runningSampleIndex = 0;

			SoundRendererStorage.srb.samplesPerSecond = SamplesPerSecond;
			SoundRendererStorage.srb.samplesToWrite = BufferSize / sizeof( int16 );
			SoundRendererStorage.srb.samples = (int16*)malloc( BufferSize );
			memset( SoundRendererStorage.srb.samples, 0, BufferSize );

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

#endif //WIN32 specific implementation