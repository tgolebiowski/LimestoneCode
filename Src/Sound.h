struct LoadedSound {
	int32 sampleCount;
	int32 channelCount;
	int16* samples [2];
};

struct SoundRenderBuffer {
	int32 samplesPerSecond;
	int32 samplesToWrite;
	int16* samples;
};

#define MAXSOUNDSATONCE 16
struct PlayingSound {
	LoadedSound* baseSound;
	float volume;
	bool playing;
	uint32 lastPlayLocation;
};

struct SoundDriver {
	LoadedSound (*LoadWaveFile) (char*, Stack*);
	SoundRenderBuffer srb;
	PlayingSound* activeSoundList;
};

#define MIX_SOUND(name) void name(SoundDriver* driver)
typedef MIX_SOUND( mixSound );

#ifdef APP_H
PlayingSound* QueueLoadedSound( LoadedSound* sound, PlayingSound* activeSoundList ) {
	for( uint8 soundIndex = 0; soundIndex < MAXSOUNDSATONCE; ++soundIndex ) {
		if( activeSoundList[ soundIndex ].baseSound == NULL ) {
			activeSoundList[ soundIndex ].baseSound = sound;
			activeSoundList[ soundIndex ].volume = 1.0f;
			activeSoundList[ soundIndex ].lastPlayLocation = 0;
			activeSoundList[ soundIndex ].playing = false;
			return &activeSoundList[ soundIndex ];
		}
	}

	return NULL;
}

void OutputTestTone( SoundRenderBuffer* srb, int hz = 440, int volume = 3000 ) {
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
		srb->samples[ i ] += sampleValue; //Left Channel
		srb->samples[ i + 1 ] += sampleValue; //Right Channel
	}
}

extern "C" MIX_SOUND( MixSound ) {
	SoundRenderBuffer* srb = &driver->srb;
	PlayingSound* activeSoundList = driver->activeSoundList;

	for( uint8 soundIndex = 0; soundIndex < MAXSOUNDSATONCE; ++soundIndex ) {
		if( activeSoundList[ soundIndex ].baseSound != NULL ) {
			if( !activeSoundList[ soundIndex ].playing ) continue;
			PlayingSound* activeSound = &activeSoundList[ soundIndex ];

			uint32 samplesToWrite = srb->samplesToWrite / 2;
			uint32 samplesLeftInSound = activeSound->baseSound->sampleCount - activeSound->lastPlayLocation;
			if( samplesLeftInSound < samplesToWrite ) {
				samplesToWrite = samplesLeftInSound;
			}
			for( int32 sampleIndex = 0; sampleIndex < samplesToWrite; ++sampleIndex ) {
				int16 value = (int16)( activeSound->volume * (float)activeSound->baseSound->samples[0][ ( sampleIndex + activeSound->lastPlayLocation ) ] );

				int32 i = sampleIndex * 2;

		        srb->samples[ i ] += value;     //Left Channel
		        srb->samples[ i + 1 ] += value; //Right Channel
		    }
		    activeSound->lastPlayLocation += samplesToWrite;

			if( activeSound->lastPlayLocation >= activeSound->baseSound->sampleCount ) {
				activeSound->baseSound = NULL;
				activeSound->lastPlayLocation = 0;
			}
		}
	}
}
#endif