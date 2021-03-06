#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <XInput.h>
#include <assert.h>

#include <GL/gl.h>
#include "OpenGL/wglext.h"
#include "OpenGL/glext.h"

#include "App.h"

struct App {
	HMODULE gameCodeHandle;
	gameInit* GameInit;
	updateAndRender* UpdateAndRender;
	mixSound* MixSound;
};

struct DynamicFileTracking {
	char* writeHead;
	int numTrackedFiles;
	char stringBuffer [ KILOBYTES(4) ];
	char* fileNames[4];
	FILETIME fileTimes[4];
};

global_variable HDC deviceContext;
global_variable HGLRC openglRenderContext;
global_variable App gameapp = { };

global_variable bool appIsRunning;
global_variable DynamicFileTracking fileTracking;

#define KEY_HISTORY_LEN 24
global_variable char keypressHistory[ KEY_HISTORY_LEN ];
global_variable int keypressHistoryIndex;

/*-----------------------------------------------------------------------------------------
	                            Ancillary Helpers
------------------------------------------------------------------------------------------*/

//Copy-Pasted from altdevblog
void EnableCrashingOnCrashes()
{
	typedef BOOL (WINAPI *tGetPolicy)(LPDWORD lpFlags);
	typedef BOOL (WINAPI *tSetPolicy)(DWORD dwFlags);
	const DWORD EXCEPTION_SWALLOWING = 0x1;

	HMODULE kernel32 = LoadLibraryA( "kernel32.dll" );
	tGetPolicy pGetPolicy = (tGetPolicy)GetProcAddress(
		kernel32,
		"GetProcessUserModeExceptionPolicy"
	);
	tSetPolicy pSetPolicy = (tSetPolicy)GetProcAddress(
		kernel32,
		"SetProcessUserModeExceptionPolicy"
	);
	if( pGetPolicy && pSetPolicy )
	{
		DWORD dwFlags;
		if( pGetPolicy( &dwFlags ) )
		{
            // Turn off the filter
			pSetPolicy(dwFlags & ~EXCEPTION_SWALLOWING);
		}
	}
	FreeLibrary( kernel32 );
}

static bool HasFocus() {
	return GetFocus() != NULL;
}

static void QueryInput( 
	int windowWidth, 
	int windowHeight, 
	int windowPosX, 
	int windowPosY,
	InputState* stateStorage
) {
	if( !HasFocus() ) {
		return;
	}

	memset( &stateStorage->romanCharKeys, 0, sizeof(bool) * 32 );
	for( int keyIndex = 0; keyIndex < 26; ++keyIndex ) {
		const int asciiStart = (int)'a';
		int keyChar = asciiStart + keyIndex;

		if( keyChar >= 'a' && keyChar <= 'z' ) {
			keyChar -= ( 'a' - 'A' );
		}

		short keystate = GetAsyncKeyState( keyChar );
		stateStorage->romanCharKeys[ keyIndex ] = ( 1 << 16 ) & keystate;
	}

	memset( &stateStorage->spcKeys, 0, sizeof(bool) * 8 );
	stateStorage->spcKeys[ InputState::CTRL ] = 
	( 1 << 16 ) & GetAsyncKeyState( VK_CONTROL );
	stateStorage->spcKeys[ InputState::BACKSPACE ] = 
	( 1 << 16 ) & GetAsyncKeyState( VK_BACK );
	stateStorage->spcKeys[ InputState::TAB ] = 
	( 1 << 16 ) & GetAsyncKeyState( VK_TAB );
	stateStorage->spcKeys[ InputState::DEL ] =
	( 1 << 16 ) & GetAsyncKeyState( VK_DELETE );
	stateStorage->spcKeys[ InputState::SPACE ] = 
	( 1 << 16 ) & GetAsyncKeyState( VK_SPACE );

	memcpy( 
		&stateStorage->keysPressedSinceLastUpdate, 
		keypressHistory,
		24
	);
	memset( &keypressHistory, 0, KEY_HISTORY_LEN );
	keypressHistoryIndex = 0;

	POINT mousePosition;
	GetCursorPos( &mousePosition );
	stateStorage->mouseX = ( (float)(mousePosition.x - windowPosX)) / 
	    ( (float)windowWidth / 2.0f ) - 1.0f;
	stateStorage->mouseY = ( ( (float)(mousePosition.y - windowPosY)) / 
		( (float)windowHeight / 2.0f ) - 1.0f) * -1.0f;

	stateStorage->mouseButtons[ InputState::MOUSE_BUTTONS::LEFT ] = 
	( 1 << 16 ) & GetAsyncKeyState( VK_LBUTTON );
	stateStorage->mouseButtons[ InputState::MOUSE_BUTTONS::MIDDLE ] = 
	( 1 << 16 ) & GetAsyncKeyState( VK_MBUTTON );
	stateStorage->mouseButtons[ InputState::MOUSE_BUTTONS::RIGHT ] = 
	( 1 << 16 ) & GetAsyncKeyState( VK_RBUTTON );

	XINPUT_STATE state;
	DWORD queryResult;
	memset( &state, 0, sizeof( XINPUT_STATE ) ) ;
	queryResult = XInputGetState( 0, &state );
	if( queryResult == ERROR_SUCCESS ) {
		//Note: polling of the sticks results in the range not quite reaching 1.0 in the positive direction
		//it is like this to avoid branching on greater or less than 0.0
		stateStorage->leftStick_x = ((float)state.Gamepad.sThumbLX / 32768.0f );
		stateStorage->leftStick_y = ((float)state.Gamepad.sThumbLY / 32768.0f );
		stateStorage->rightStick_x = ((float)state.Gamepad.sThumbRX / 32768.0f );
		stateStorage->rightStick_y = ((float)state.Gamepad.sThumbRY / 32768.0f );
		stateStorage->leftTrigger = ((float)state.Gamepad.bLeftTrigger / 255.0f );
		stateStorage->rightTrigger = ((float)state.Gamepad.bRightTrigger / 255.0f );
		stateStorage->leftBumper = state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
		stateStorage->rightBumper = state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
		stateStorage->button1 = state.Gamepad.wButtons & XINPUT_GAMEPAD_A;
		stateStorage->button2 = state.Gamepad.wButtons & XINPUT_GAMEPAD_B;
		stateStorage->button3 = state.Gamepad.wButtons & XINPUT_GAMEPAD_X;
		stateStorage->button4 = state.Gamepad.wButtons & XINPUT_GAMEPAD_Y;
		stateStorage->specialButtonLeft = state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
		stateStorage->specialButtonRight = state.Gamepad.wButtons & XINPUT_GAMEPAD_START;
	} else {
		//TODO: Clear input?
	}
}

/*-----------------------------------------------------------------------------------------
                                File system query
-----------------------------------------------------------------------------------------*/

void PrintErrorMessage(DWORD ErrorCode)
{
    TCHAR *pMsgBuf = NULL;
    DWORD nMsgLen = FormatMessage(
    	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, 
        ErrorCode, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&pMsgBuf, 
        0, 
        NULL
    );
    if (!nMsgLen)
        return;

    printf( "%s\n", pMsgBuf );

    LocalFree(pMsgBuf);
}

#include <strsafe.h>
//NOTE: this allocates the file size + 1 byte, and sets that last byte to zero
static void* ReadWholeFile( char* filename, Stack* allocater, int* bytesRead ) {
	HANDLE fileHandle;
	//TODO: look into other options, such as: Overlapped, No_Buffering, and Random_Access
	fileHandle = CreateFile( 
		filename, 
		GENERIC_READ, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		0, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		0 
	);
	if( fileHandle == INVALID_HANDLE_VALUE ) {
		PrintErrorMessage( GetLastError() );
		return NULL;
	}

	//Determine amount of data
	LARGE_INTEGER fileSize;
	GetFileSizeEx( fileHandle, &fileSize );
	if( fileSize.QuadPart == 0 ) {
		return NULL;
	}
	//Unsupported, file is too big
	assert( fileSize.QuadPart <= 0xFFFFFFFF );

	//Reserve Space
	void* data = 0;
	data = StackAllocAligned( allocater, fileSize.QuadPart + 1 );
	assert( data != 0 );
	memset( data, 0, fileSize.QuadPart + 1 );

	//Read data
	DWORD dataRead = 0;
	//TODO: adjust for overlapped or async stuff?
	BOOL readSuccess = ReadFile( fileHandle, data, fileSize.QuadPart, &dataRead, 0 );
	if( !readSuccess || dataRead != fileSize.QuadPart ) {
		FreeFromStack( allocater, data );
		return NULL;
	}

	if( bytesRead != NULL ) {
		*bytesRead = dataRead;
	}

	//Close
	BOOL closeReturnValue = CloseHandle( fileHandle );
	assert( closeReturnValue );

	return data;
}

static void WriteFile( char* fileName, void* data, int dataToWrite ) {
	HANDLE fileHandle;
	//TODO: look into other options, such as: Overlapped, No_Bufffering, and Random_Access
	fileHandle = CreateFile( 
		fileName, 
		GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		0, 
		CREATE_NEW, 
		FILE_ATTRIBUTE_NORMAL, 
		0 
	);
	if( fileHandle == INVALID_HANDLE_VALUE ) {
		fileHandle = CreateFile(
			fileName, 
			GENERIC_WRITE, 
			FILE_SHARE_WRITE,
			0,
			TRUNCATE_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			0
		);

		if( fileHandle == INVALID_HANDLE_VALUE ) {
			PrintErrorMessage( GetLastError() );
			printf("Did not write to file %s\n", fileName );
			return;
		}
	}

	DWORD bytesWritten = 0;
	DWORD writeSuccess = WriteFile( 
		fileHandle, 
		data, 
		dataToWrite, 
		&bytesWritten, 
		NULL //NOT NULL if i ever do overlapped stuff according to msdn
	);

	if( writeSuccess != 1 ) {
		PrintErrorMessage( GetLastError() );
		printf("Did not write to file %s\n", fileName );
		return;
	}

	//Close
	BOOL closeReturnValue = CloseHandle( fileHandle );
	assert( closeReturnValue );
}

//TODO: support Unicode, currently this breaks if used w/ UNICODE
//Searches for file with most recent writeTime whose name contains "search" value
//return 0 and Writes fileName to bestResult if a file was found
//returns -1 if no file found & leave bestResult alone
int GetMostRecentMatchingFile( char* search, char* bestResult ) {
	char appendedSearchFile [128] = { };
	int searchLen = strlen( search );
	memcpy( &appendedSearchFile[0], search, searchLen );
	appendedSearchFile[ searchLen ] = '*';

	char bestFile [128] = { };
	FILETIME latestTime = { };
	WIN32_FIND_DATA fileData = { };
	HANDLE searchHandle = FindFirstFile( appendedSearchFile, &fileData );

	if( searchHandle == INVALID_HANDLE_VALUE ) 
		return -1;
	else {
		latestTime = fileData.ftLastWriteTime;
		#ifdef UNICODE
		    assert(false);
		#else
		    int fileNameLen = strlen( (char*)fileData.cFileName );
		    memcpy( bestFile, fileData.cFileName, fileNameLen );
		#endif
	}

	FindNextFile( searchHandle, &fileData );
	while( FindNextFile( searchHandle, &fileData ) ) {
		FILETIME time = fileData.ftLastWriteTime;
		if( CompareFileTime( &latestTime, &time ) == -1 ) {
			latestTime = fileData.ftLastWriteTime;
		    #ifdef UNICODE
			    assert(false);
		    #else
			    int fileNameLen = strlen( (char*)fileData.cFileName );
			    memset( bestFile, 0, 128 );
			    memcpy( bestFile, fileData.cFileName, fileNameLen );
		    #endif
		}
	}

	int bestFileNameLen = strlen( bestFile );
	memcpy( (void*)bestResult, bestFile, bestFileNameLen );
	return 0;
}

int TrackFileUpdates( char* filePath ) {
	WIN32_FIND_DATA fileFindData = { };
	HANDLE handle = FindFirstFile( filePath, &fileFindData );

	if( handle != INVALID_HANDLE_VALUE ) {
		int filePathLen = strlen( filePath );
		FILETIME fileTime = fileFindData.ftLastWriteTime;
		char* recordedNameStart = fileTracking.writeHead;

		int newTrackIndex = fileTracking.numTrackedFiles;
		strcpy( recordedNameStart, filePath );
		fileTracking.fileNames[ newTrackIndex ] = recordedNameStart;
		fileTracking.fileTimes[ newTrackIndex ] = fileTime;
		fileTracking.writeHead += filePathLen + 1;
		++fileTracking.numTrackedFiles;

		FindClose( handle );

		return newTrackIndex;
	} else {
		return -1;
	}
}

bool DidFileUpdate( int trackingIndex ) {
	WIN32_FIND_DATA findFileData = { };
	char* fileName = fileTracking.fileNames[ trackingIndex ];
	HANDLE h = FindFirstFile( fileName, &findFileData );

	if( h != INVALID_HANDLE_VALUE ) {
		FILETIME currentFileTime = findFileData.ftLastWriteTime;
		int compareResult = CompareFileTime( 
			&currentFileTime, 
			&fileTracking.fileTimes[ trackingIndex ] 
		);
		FindClose( h );

		bool result = compareResult == 1;
		if( result ) {
			fileTracking.fileTimes[ trackingIndex ] = currentFileTime;
		}

		return result;
	} else {
		return false;
	}
}

#define GAME_CODE_FILEPATH "App.dll"
#define RUNNING_GAME_CODE_FILEPATH "App_Running.dll" 
void LoadGameCode( App* gameapp ) {
	if( gameapp->gameCodeHandle != NULL ) {
		FreeLibrary( gameapp->gameCodeHandle );
		gameapp->GameInit = NULL;
		gameapp->UpdateAndRender = NULL;
		gameapp->MixSound = NULL;
	}

	CopyFile( GAME_CODE_FILEPATH, RUNNING_GAME_CODE_FILEPATH, false );

	HMODULE dllHandle = LoadLibrary( RUNNING_GAME_CODE_FILEPATH );
	gameapp->gameCodeHandle = dllHandle;
	gameapp->GameInit = (gameInit*)GetProcAddress( dllHandle, "GameInit" );
	gameapp->UpdateAndRender = (updateAndRender*)GetProcAddress( dllHandle, "UpdateAndRender" );
	gameapp->MixSound = (mixSound*)GetProcAddress( dllHandle, "MixSound" );
}

static WorkerThreadData threadData;

static void InitWin32WorkerThread( Stack* systemMem ) {
	void (*workerThread)(void*) = &WorkThreadMain;

	maxJobs = 8;
	workQueue = (Job*)StackAllocAligned( systemMem, sizeof( Job ) * maxJobs );
	threadData.jobs = workQueue;
	threadData.maxJobs = maxJobs;

	DWORD threadId = 0;
	HANDLE threadHandle = CreateThread(
		NULL, 
		0, 
		(LPTHREAD_START_ROUTINE)workerThread, 
		&threadData, 
		0, 
		&threadId
	);
	
	if( threadHandle != NULL ) {
		printf( "Created Worker Thread.\n" );
	}
}

/*-----------------------------------------------------------------------------------------
	                            Sound Related Stuff
-----------------------------------------------------------------------------------------*/

#include <mmsystem.h>
#include <dsound.h>
#include <comdef.h>

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name( LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter )
typedef DIRECT_SOUND_CREATE( direct_sound_create );

struct Win32Sound {
	SoundDriver driver;

	int32 writeBufferSize;
	int32 safetySampleBytes;
	int32 expectedBytesPerFrame;
	uint8 bytesPerSample;
	LPDIRECTSOUNDBUFFER writeBuffer;

	uint64 runningSampleIndex;
	DWORD bytesToWrite, byteToLock;
};

static LoadedSound LoadWaveFile( char* filePath, Stack* allocater ) {
    #define RIFF_CODE( a, b, c, d ) ( ( (uint32)(a) << 0 ) | ( (uint32)(b) << 8 ) | ( (uint32)(c) << 16 ) | ( (uint32)(d) << 24 ) )
	enum {
		WAVE_ChunkID_fmt = RIFF_CODE( 'f', 'm', 't', ' ' ),
		WAVE_ChunkID_data = RIFF_CODE( 'd', 'a', 't', 'a' ),
		WAVE_ChunkID_RIFF = RIFF_CODE( 'R', 'I', 'F', 'F' ),
		WAVE_ChunkID_WAVE = RIFF_CODE( 'W', 'A', 'V', 'E' )
	};

	#pragma pack( push, 1 )
	struct WaveHeader{
		uint32 RIFFID;
		uint32 size;
		uint32 WAVEID;
	};

	struct WaveChunk {
		uint32 ID;
		uint32 size;
	};

	struct Wave_fmt {
		uint16 wFormatTag;
		uint16 nChannels;
		uint32 nSamplesPerSec;
		uint32 nAvgBytesPerSec;
		uint16 nBlockAlign;
		uint16 wBitsPerSample;
		uint16 cbSize;
		uint16 wValidBitsPerSample;
		uint32 dwChannelMask;
		uint8 SubFormat [8];
	};
    #pragma pack( pop )

	LoadedSound result = { };

	void* fileData = ReadWholeFile( filePath, allocater, NULL );

	if( fileData != NULL ) {
		struct RiffIterator {
			uint8* currentByte;
			uint8* stop;
		};

		auto ParseChunkAt = []( WaveHeader* header, void* stop ) -> RiffIterator {
			return { (uint8*)header, (uint8*)stop };
		};
		auto IsValid = []( RiffIterator iter ) -> bool  {
			return iter.currentByte < iter.stop;
		};
		auto NextChunk = []( RiffIterator iter ) -> RiffIterator {
			WaveChunk* chunk = (WaveChunk*)iter.currentByte;
			//This is for alignment: ( ptr + ( targetalignment - 1 ) & ~( targetalignment - 1)) aligns ptr with the correct bit padding
			uint32 size = ( chunk->size + 1 ) & ~1;
			iter.currentByte += sizeof( WaveChunk ) + size;
			return iter;
		};
		auto GetChunkData = []( RiffIterator iter ) -> void* {
			void* result = ( iter.currentByte  + sizeof( WaveChunk ) );
			return result;
		};
		auto GetType = []( RiffIterator iter ) -> uint32 {
			WaveChunk* chunk = (WaveChunk*)iter.currentByte;
			uint32 result = chunk->ID;
			return result;
		};
		auto GetChunkSize = []( RiffIterator iter) -> uint32 {
			WaveChunk* chunk = (WaveChunk*)iter.currentByte;
			uint32 result = chunk->size;
			return result;
		};

		WaveHeader* header = (WaveHeader*)fileData;
		assert( header->RIFFID == WAVE_ChunkID_RIFF );
		assert( header->WAVEID == WAVE_ChunkID_WAVE );

		uint32 channelCount = 0;
		uint32 sampleDataSize = 0;
		void* sampleData = 0;
		for( RiffIterator iter = ParseChunkAt( header + 1, (uint8*)( header + 1 ) + header->size - 4 ); 
			IsValid( iter ); iter = NextChunk( iter ) ) {
			switch( GetType( iter ) ) {
				case WAVE_ChunkID_fmt: {
					Wave_fmt* fmt = (Wave_fmt*)GetChunkData( iter );
					assert( fmt->wFormatTag == 1 ); //NOTE: only supporting PCM
					assert( fmt->nSamplesPerSec == 48000 );
					assert( fmt->wBitsPerSample == 16 );
					channelCount = fmt->nChannels;
				}break;
				case WAVE_ChunkID_data: {
					sampleData = GetChunkData( iter );
					sampleDataSize = GetChunkSize( iter );
				}break;
			}
		}

		assert( sampleData != 0 );

		result.sampleCount = sampleDataSize / ( channelCount * sizeof( uint16 ) );
		result.channelCount = channelCount;

		if( channelCount == 1 ) {
			result.samples[0] = (int16*)sampleData;
			result.samples[1] = 0;
		} else if( channelCount == 2 ) {

		} else {
			assert(false);
		}
	}
	
	return result;
}

static Win32Sound Win32InitSound( HWND hwnd, int targetGameHZ, Stack* systemStorage ) {
	const int32 SamplesPerSecond = 48000;
	const int32 BufferSize = SamplesPerSecond * sizeof( int16 ) * 2;
	HMODULE DirectSoundDLL = LoadLibraryA( "dsound.dll" );

	Win32Sound win32Sound = { };

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

			if( SUCCEEDED( directSound->SetCooperativeLevel( hwnd, DSSCL_PRIORITY ) ) ) {

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
			HRESULT result = directSound->CreateSoundBuffer( 
				&bufferDescription, 
				&win32Sound.writeBuffer, 
				0 
			);
			if( !SUCCEEDED( result ) ) {
				printf("Couldn't secure a writable buffer\n");
			}

			win32Sound.byteToLock = 0;
			win32Sound.bytesToWrite = 0;
			win32Sound.writeBufferSize = BufferSize;
			win32Sound.bytesPerSample = sizeof( int16 );
 			win32Sound.safetySampleBytes = ( ( SamplesPerSecond / targetGameHZ ) / 2 ) * win32Sound.bytesPerSample;
 			win32Sound.expectedBytesPerFrame = ( 48000 * win32Sound.bytesPerSample * 2 ) / targetGameHZ;
			win32Sound.runningSampleIndex = 0;

			{
				SoundDriver* driver = &win32Sound.driver;
				driver->srb.samplesPerSecond = SamplesPerSecond;
				driver->srb.samplesToWrite = BufferSize / sizeof( int16 );
				driver->srb.samples = (int16*)StackAllocAligned(
					systemStorage,
					BufferSize 
				);

				size_t playingInfoBufferSize = sizeof( QueuedSound ) * MAXSOUNDSATONCE;
				driver->activeSoundList = (QueuedSound*)StackAllocAligned(
					systemStorage,
					playingInfoBufferSize 
				);
				for( int i = 0; i < MAXSOUNDSATONCE; ++i ) {
					driver->activeSoundList[ i ].playing = false;
				}
			}

			HRESULT playResult = win32Sound.writeBuffer->Play( 0, 0, DSBPLAY_LOOPING );
			if( !SUCCEEDED( playResult ) ) {
				printf("failed to play\n");
			}

		} else {
			printf("couldn't create direct sound object\n");
		}
	} else {
		printf("couldn't create direct sound object\n");
	}

	win32Sound.driver.LoadWaveFile = &LoadWaveFile;

	return win32Sound;
}

static void PushAudioToSoundCard( App* gameapp, Win32Sound* win32Sound ) {
	memset( 
		win32Sound->driver.srb.samples,
		0, 
		win32Sound->writeBufferSize
	);

	ImGui::Begin( "Audio State" );

	//Setup info needed for writing (where to, how much, etc.)
	DWORD playCursorPosition, writeCursorPosition;
	if( SUCCEEDED( win32Sound->writeBuffer->GetCurrentPosition( &playCursorPosition, &writeCursorPosition) ) ) {
		static bool firstTime = true;
		if( firstTime ) {
			win32Sound->runningSampleIndex = writeCursorPosition / win32Sound->bytesPerSample;
			firstTime = false;
		}

	    //Pick up where we left off
		win32Sound->byteToLock = ( win32Sound->runningSampleIndex * win32Sound->bytesPerSample * 2 ) % win32Sound->writeBufferSize;

	    //Calculate how much to write
		DWORD ExpectedFrameBoundaryByte = playCursorPosition + win32Sound->expectedBytesPerFrame;

		//DSound can be latent sometimes when reporting the current writeCursor, so this is 
		//the farthest ahead that the cursor could possibly be
		DWORD safeWriteCursor = writeCursorPosition;
		if( safeWriteCursor < playCursorPosition ) {
			safeWriteCursor += win32Sound->writeBufferSize;
		}
		safeWriteCursor += win32Sound->safetySampleBytes;

		bool AudioCardIsLowLatency = safeWriteCursor < ExpectedFrameBoundaryByte;

		//Determine up to which byte we should write
		DWORD targetCursor = 0;
		if( AudioCardIsLowLatency ) {
			targetCursor = ( ExpectedFrameBoundaryByte + win32Sound->expectedBytesPerFrame );
		} else {
			targetCursor = ( writeCursorPosition + win32Sound->expectedBytesPerFrame + win32Sound->safetySampleBytes );
		}
		targetCursor = targetCursor % win32Sound->writeBufferSize;

		//Wrap up on math, how many bytes do we actually write
		if( win32Sound->byteToLock >= targetCursor ) {
			win32Sound->bytesToWrite = win32Sound->writeBufferSize - win32Sound->byteToLock;
			win32Sound->bytesToWrite += targetCursor;
		} else {
			win32Sound->bytesToWrite = targetCursor - win32Sound->byteToLock;
		}

		if( win32Sound->bytesToWrite == 0 ) {
			printf( "BTW is zero, BTL:%lu, TC:%lu, PC:%lu, WC:%lu \n", win32Sound->byteToLock, targetCursor, playCursorPosition, writeCursorPosition );
			assert(false);
		}

	    //Save number of samples that can be written to platform independent struct
		win32Sound->driver.srb.samplesToWrite = win32Sound->bytesToWrite / win32Sound->bytesPerSample;
	} else {
		printf("couldn't get cursor\n");
		return;
	}

	//Mix together currently playing sounds
	gameapp->MixSound( &win32Sound->driver );

	//Push mixed sounds to the actual card
	VOID* region0;
	DWORD region0Size;
	VOID* region1;
	DWORD region1Size;

	HRESULT result = win32Sound->writeBuffer->Lock( 
		win32Sound->byteToLock, win32Sound->bytesToWrite,
		&region0, &region0Size,
		&region1, &region1Size,
		0 );
	if( !SUCCEEDED( result ) ) {
		_com_error error( result );
		LPCTSTR errMessage = error.ErrorMessage();
		//printf( "Couldn't Lock Sound Buffer\n" );
		printf( "Failed To Lock Buffer %s \n", errMessage );
		//printf( "BTL:%lu BTW:%lu r0:%lu r0s:%lu r1:%lu r1s:%lu\n", win32Sound->byteToLock, 
		//	win32Sound->bytesToWrite, region0, region0Size, region1, region1Size );
		//printf( "SoundRenderBuffer Info: Samples--%d\n", win32Sound->srb.samplesToWrite );
		return;
	}

	int16* sampleSrc = win32Sound->driver.srb.samples;
	int16* sampleDest = (int16*)region0;
	DWORD region0SampleCount = region0Size / ( win32Sound->bytesPerSample * 2 );
	for( DWORD sampleIndex = 0; sampleIndex < region0SampleCount; ++sampleIndex ) {
		*sampleDest++ = *sampleSrc++;
		*sampleDest++ = *sampleSrc++;
		++win32Sound->runningSampleIndex;
	}
	sampleDest = (int16*)region1;
	DWORD region1SampleCount = region1Size / ( win32Sound->bytesPerSample * 2 );
	for( DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex ) {
		*sampleDest++ = *sampleSrc++;
		*sampleDest++ = *sampleSrc++;
		++win32Sound->runningSampleIndex;
	}

	//memset( win32Sound->srb.samples, 0, sizeof( int16 ) * 2 * win32Sound->srb.samplesToWrite );
	//win32Sound->srb.samplesToWrite = 0;

	result = win32Sound->writeBuffer->Unlock( region0, region0Size, region1, region1Size );
	if( !SUCCEEDED( result ) ) {
		printf("Couldn't Unlock\n");
	}

	ImGui::End();
}

/*----------------------------------------------------------------------------------------
                       Renderer.h function prototype implementations
-----------------------------------------------------------------------------------------*/

#include "GLRenderer.h"

GLRenderDriver Win32InitGLRenderer( 
	HWND hwnd,
	System* system,
	GLRendererGlobalState* state
) {
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,
	        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		    PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
		    32,                       //Colordepth of the framebuffer.
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //Don't care....
		    32,                       //Number of bits for the depthbuffer
	        0,                        //Number of bits for the stencilbuffer
		    0,                        //Number of Aux buffers in the framebuffer.
		    0, 0, 0, 0, 0             //Don't care...
	};

	deviceContext = GetDC( hwnd );

	int letWindowsChooseThisPixelFormat;
	letWindowsChooseThisPixelFormat = ChoosePixelFormat( deviceContext, &pfd ); 
	SetPixelFormat( deviceContext, letWindowsChooseThisPixelFormat, &pfd );

	//TODO: print & log the pixel format the user got

	openglRenderContext = wglCreateContext( deviceContext );
	if( wglMakeCurrent ( deviceContext, openglRenderContext ) == false ) {
		printf( "Couldn't make GL context current.\n" );
		return { };
	}

	#define GLE( ret, name, ... ) \
	    gl##name = (name##proc*)wglGetProcAddress( "gl" #name );

	GL_FUNCS
	#undef GLE

	return InitGLRenderer(
		state,
		system->windowWidth, 
		system->windowHeight
	);
}

/*----------------------------------------------------------------------------------------
                                 Win32 App required stuff
------------------------------------------------------------------------------------------*/

static void EndProgram( HWND hwnd ) {
	appIsRunning = false;
	wglMakeCurrent( deviceContext, NULL );
	wglDeleteContext( openglRenderContext );
	ReleaseDC( hwnd, deviceContext );

	if( gameapp.gameCodeHandle != NULL ) {
		FreeLibrary( gameapp.gameCodeHandle );
	}

	DeleteFile( RUNNING_GAME_CODE_FILEPATH );
}

static LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch (uMsg) {
		case WM_CLOSE: {
			EndProgram( hwnd );
			DestroyWindow( hwnd );
			break;
		}

		case WM_DESTROY: {
			EndProgram( hwnd );
			PostQuitMessage( 0 );
			break;
		}

		//...I forget why this is here, windows opengl magic I think...
		case WM_ERASEBKGND: {
			return 666;
		}

		case WM_KEYDOWN: {
			char keyCharValue = 0;
			if( wParam >= 0x30 && wParam <= 0x39 ) {
				keyCharValue = ( wParam - 0x30 ) + '0';
			} else if( wParam >= 0x41 && wParam <= 0x5A ) {
				bool isShiftHeld = ( GetAsyncKeyState( VK_SHIFT ) & ( 1 << 16 ) );
				keyCharValue = ( wParam - 0x41 ) + 'a';
				keyCharValue += ( ( (int)isShiftHeld ) * ( 'A' - 'a' ) );
			} else if ( wParam == VK_SPACE ) {
				keyCharValue = ' ';
			} else if ( wParam == VK_OEM_2 ) { //TODO: this is U.S. only, make more general
				keyCharValue = '/';
			} else if ( wParam == VK_OEM_PERIOD ) {
				keyCharValue = '.';
			} else if ( wParam == VK_OEM_MINUS ) {
				keyCharValue = '-';
			}

			keypressHistory[ keypressHistoryIndex++ ] = keyCharValue;

			break;
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

static int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
	AllocConsole(); //create a console
	freopen( "conin$","r",stdin );
	freopen( "conout$","w",stdout );
	freopen( "conout$","w",stderr );
	printf( "Program Started, console initialized\n" );

	const char WindowName[] = "Wet Clay";

	int64 mSecsPerFrame = 16; //60FPS
	System system = { };
	system.windowHeight = 680;
	system.windowWidth = 1080;
	system.ReadWholeFile = &ReadWholeFile;
	system.GetMostRecentMatchingFile = &GetMostRecentMatchingFile;
	system.TrackFileUpdates = &TrackFileUpdates;
	system.DidFileUpdate = &DidFileUpdate;
	system.WriteFile = &WriteFile;
	system.HasFocus = &HasFocus;

	//Center position of window
	uint16 fullScreenWidth = GetSystemMetrics( SM_CXSCREEN );
	uint16 fullScreenHeight = GetSystemMetrics( SM_CYSCREEN );
	uint16 windowPosX = ( fullScreenWidth / 2 ) - (system.windowWidth / 2 );
	uint16 windowPosY = ( fullScreenHeight / 2 ) - (system.windowHeight / 2 );

	WNDCLASSEX wc = { };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WindowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if( !RegisterClassEx( &wc ) ) {
		printf( "Failed to register class\n" );
		return -1;
	}

	EnableCrashingOnCrashes();
 
	// at this point WM_CREATE message is sent/received
	// the WM_CREATE branch inside WinProc function will execute here
	HWND hwnd = CreateWindowEx(
		0, 
		WindowName, 
		"Wet Clay App", 
		WS_BORDER, 
		windowPosX, 
		windowPosY, 
		system.windowWidth, 
		system.windowHeight,
		NULL, 
		NULL, 
		hInstance, 
		NULL
	);

	RECT windowRect = { };
	GetClientRect( hwnd, &windowRect );

	SetWindowLong( hwnd, GWL_STYLE, 0 );
	ShowWindow ( hwnd, SW_SHOWNORMAL );
	UpdateWindow( hwnd );

	LARGE_INTEGER timerResolution;
	BOOL canSupportHiResTimer = QueryPerformanceFrequency( &timerResolution );
	assert( canSupportHiResTimer );

	size_t systemsMemorySize = MEGABYTES( 8 );
	Stack gameSlab;
	gameSlab.size = SIZEOF_GLOBAL_HEAP + systemsMemorySize;
	gameSlab.start = VirtualAlloc( 
		NULL, 
		gameSlab.size, 
		MEM_COMMIT | MEM_RESERVE, 
		PAGE_EXECUTE_READWRITE 
	);
	gameSlab.current = gameSlab.start;
	assert( gameSlab.start != NULL );

	Stack systemsMemory = AllocateNewStackFromStack( &gameSlab, systemsMemorySize );

	InitWin32WorkerThread( &systemsMemory );

	GLRendererGlobalState glRendererStorage = { };
	globalGLRendererStorageRef = &glRendererStorage;
	GLRenderDriver glDriver = Win32InitGLRenderer( hwnd, &system, globalGLRendererStorageRef );
	Win32Sound win32Sound = Win32InitSound( hwnd, 60, &systemsMemory );

	void* imguistate = InitImGui_LimeStone(	&systemsMemory,	(RenderDriver*)&glDriver, system.windowWidth, system.windowHeight );

	printf( "Remaining System Memory: %Id\n", SPACE_IN_STACK( (&gameSlab) ) );

	LoadGameCode( &gameapp );
	assert( gameapp.GameInit != NULL );

	fileTracking = { };
	fileTracking.writeHead = &fileTracking.stringBuffer[0];

	int dllTrackingIndex = TrackFileUpdates( GAME_CODE_FILEPATH );
	assert( dllTrackingIndex != -1 );

	void* gameMemoryPtr = gameapp.GameInit( 
		&gameSlab, 
		(RenderDriver*)&glDriver,
		&win32Sound.driver,
		&system
	);

	appIsRunning = true;

	InputState inputSnapshot1 = { };
	InputState inputSnapshot2 = { };
	inputSnapshot1.prevState = &inputSnapshot2;
	inputSnapshot2.prevState = &inputSnapshot1;
	InputState* currentSnapshotStorage = &inputSnapshot1;

	MSG Msg;
	while( appIsRunning ) {
		if( DidFileUpdate( dllTrackingIndex ) ) {
			LoadGameCode( &gameapp );
		}

		static LARGE_INTEGER startTime;
		LARGE_INTEGER elapsedTime;
		LARGE_INTEGER lastTime = startTime;
		QueryPerformanceCounter( &startTime );

		elapsedTime.QuadPart = startTime.QuadPart - lastTime.QuadPart;
		elapsedTime.QuadPart *= 1000;
		elapsedTime.QuadPart /= timerResolution.QuadPart;

		//GAME LOOP
		if(	gameapp.UpdateAndRender != NULL && gameapp.MixSound != NULL ) {

			currentSnapshotStorage = currentSnapshotStorage->prevState;
		    QueryInput( 
		    	system.windowWidth, 
		    	system.windowHeight,
		    	windowPosX,
		    	windowPosY,
		    	currentSnapshotStorage
		    );
		    keypressHistoryIndex = 0;

		    UpdateImgui( currentSnapshotStorage, imguistate, system.windowWidth, system.windowHeight );

			appIsRunning = gameapp.UpdateAndRender( 
				gameMemoryPtr, 
				(float)elapsedTime.QuadPart, 
				currentSnapshotStorage,
				&win32Sound.driver,
				(RenderDriver*)&glDriver,
				&system,
				imguistate
			);

		    PushAudioToSoundCard( &gameapp, &win32Sound );

		    ImGui::Render();

			BOOL swapBufferSuccess = SwapBuffers( deviceContext );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		} else {
			printf( "Game code was not loaded...\n" );
		}

		//Windows Message pump is here, so if there is a system level Close or
		//Destory command, the UpdateAndRender method does not reset the appIsRunning
		//flag to true
		while( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) ) {
			TranslateMessage( &Msg );
			DispatchMessage( &Msg );
		}

		//End loop timer query
		LARGE_INTEGER endTime, computeTime;
		{
			QueryPerformanceCounter( &endTime );
			computeTime.QuadPart = endTime.QuadPart - startTime.QuadPart;
			computeTime.QuadPart *= 1000;
			computeTime.QuadPart /= timerResolution.QuadPart;
		}

		if( computeTime.QuadPart <= mSecsPerFrame ) {
			Sleep(mSecsPerFrame - computeTime.QuadPart );
		} else {
			printf("Didn't sleep, compute was %ld\n", computeTime.QuadPart );
		}
	};

	FreeConsole();

	return Msg.wParam;
}