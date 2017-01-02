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
enum SoundType {
	Chirp = 0,
	LoadedStatic = 1
};

struct Envelope {
	float attackTime;
	float decayTime;
	float sustainLevel;
	float releaseTime;
};

static inline float GetStateOfEnvelope( Envelope* envelope, float t ) {
	const float decayEnd = envelope->decayTime + envelope->attackTime;

	if( t >= 0.0f ) {
		float attackValue = envelope->attackTime > 0.0f && t <= envelope->attackTime && t? 
		    t / envelope->attackTime : 
		    0.0f;
		float decayValue = envelope->decayTime > 0.0f && t > envelope->attackTime && t <= decayEnd ?
		    LERP( 1.0f, envelope->sustainLevel, ( t - envelope->attackTime ) / envelope->decayTime ) :
		    0.0f;
		float sustainValue = envelope->sustainLevel * (float)( t > decayEnd );

		return attackValue + decayValue + sustainValue;
	} else {
		t *= -1.0f;
		float releaseValue = envelope->releaseTime > 0.0f && t < envelope->releaseTime ?
	    ( 1.0f - ( t / envelope->releaseTime ) ) * envelope->sustainLevel:
	    0.0f;

	    return releaseValue;
	}
}

struct QueuedSound {
	float volume;
	uint32 lastPlayLocation;
	bool playing;
	bool looping;

	Envelope ampEnvelope;

	char soundType;
	union {
		LoadedSound* baseSound;
		int futurewaveTableIndexOrOtherData;
	};
};

struct SoundDriver {
	LoadedSound (*LoadWaveFile) (char*, Stack*);
	SoundRenderBuffer srb;
	QueuedSound* activeSoundList;
};

#define MIX_SOUND(name) void name(SoundDriver* driver)
typedef MIX_SOUND( mixSound );

#ifdef DLL_ONLY
QueuedSound* QueueLoadedSound( LoadedSound* sound, QueuedSound* activeSoundList ) {
	for( uint8 soundIndex = 0; soundIndex < MAXSOUNDSATONCE; ++soundIndex ) {
		if( activeSoundList[ soundIndex ].playing == false ) {
			activeSoundList[ soundIndex ].playing = true;
			activeSoundList[ soundIndex ].volume = 1.0f;
			activeSoundList[ soundIndex ].lastPlayLocation = 0;
			activeSoundList[ soundIndex ].soundType = SoundType::LoadedStatic;
			activeSoundList[ soundIndex ].baseSound = sound;
			return &activeSoundList[ soundIndex ];
		}
	}

	return NULL;
}

QueuedSound* QueueChirp( SoundDriver* soundCard ) {
	for( uint8 soundIndex = 0; soundIndex < MAXSOUNDSATONCE; ++soundIndex ) {
		if( soundCard->activeSoundList[ soundIndex ].playing == false ) {
			QueuedSound* newSound = &soundCard->activeSoundList[ soundIndex ];

			newSound->playing = true;
			newSound->volume = 1.0f;
			newSound->lastPlayLocation = 0;
			newSound->soundType = SoundType::Chirp;

			newSound->ampEnvelope = {
				0.1f, 0.05f, 0.75f, 0.2f
			};

			return newSound;
		}
	}

	return NULL;
}

static inline void ProcessStaticQueuedSound( 
	QueuedSound* activeSound, 
	SoundRenderBuffer* srb 
) {
	uint32 samplesToWrite = srb->samplesToWrite / 2;
	uint32 samplesLeftInSound = activeSound->baseSound->sampleCount - activeSound->lastPlayLocation;
	if( samplesLeftInSound < samplesToWrite && !activeSound->looping ) {
		samplesToWrite = samplesLeftInSound - 1;
	}

	for( int32 sampleIndex = 0; sampleIndex < samplesToWrite; ++sampleIndex ) {
		int32 readIndex = ( sampleIndex + activeSound->lastPlayLocation ) % activeSound->baseSound->sampleCount;
		float baseSampleValue = (float)activeSound->baseSound->samples[0][ readIndex ];
		int16 value = (int16)( activeSound->volume * baseSampleValue );

		int32 writeIndex = sampleIndex * 2;

		srb->samples[ writeIndex ] += value;     //Left Channel
		srb->samples[ writeIndex + 1 ] += value; //Right Channel
	}
	activeSound->lastPlayLocation += samplesToWrite;

	if( activeSound->lastPlayLocation >= activeSound->baseSound->sampleCount ) {
		if( !activeSound->looping ) {
			activeSound->baseSound = NULL;
			activeSound->lastPlayLocation = 0;
			activeSound->playing = false;
		} else {
			activeSound->lastPlayLocation = 
			activeSound->lastPlayLocation % activeSound->baseSound->sampleCount;
		}
	}
}

static inline void ProcessQueuedChirp(
	QueuedSound* activeSound,
	SoundRenderBuffer* srb
) {
	const int hz = 440;
	const int volume = 3000;
	const int WavePeriodInSamples = srb->samplesPerSecond / hz;
	static float tSine = 0.0f;
	const float tSineStep = 2.0f * PI / (float)WavePeriodInSamples;

	int32 samplesToWrite = srb->samplesToWrite / 2;

	const float chirpLengthInSec = 1.0f;
	const int chirpLengthInSamples = srb->samplesPerSecond * chirpLengthInSec;

	float lastEnvelopeOut = 0.0f;
	for( int32 sampleIndex = 0; sampleIndex < samplesToWrite; ++sampleIndex ) {
		//Calculate base amplitude of chirp
		tSine += tSineStep;
		if( tSine > ( 2.0f * PI ) ) {
			tSine -= ( 2.0f * PI );
		}
		float sampleValue = volume * sinf( tSine );

		//Apply Modulators
		float tSec = (float)( sampleIndex + activeSound->lastPlayLocation ) / (float)srb->samplesPerSecond;
		if( tSec > chirpLengthInSec ) {
			tSec = chirpLengthInSec - tSec;
		}
		float modValue = GetStateOfEnvelope( &activeSound->ampEnvelope, tSec );
		sampleValue *= modValue;

		//write
		int16 finalSampleValue = (int16)sampleValue;
		int32 i = sampleIndex * 2;
		srb->samples[ i ] += sampleValue; //Left Channel
		srb->samples[ i + 1 ] += sampleValue; //Right Channel

		const int discontinuityCutoff = 200;
		if( sampleIndex > 0 ) {
			int16 prevSample = srb->samples[ i - 2 ];
			int16 diff = prevSample - srb->samples[ i ];

			if( ABSf( diff ) > discontinuityCutoff ) {
				printf( "Discontinuity Detected, sample is %d, prev was %d\n", 
					srb->samples[ i ], 
					prevSample
				);
	        }
		}


		lastEnvelopeOut = modValue;
	}

	activeSound->lastPlayLocation += samplesToWrite;

	if( activeSound->lastPlayLocation > chirpLengthInSamples && 
		lastEnvelopeOut <= 0.0f
	) {
		activeSound->playing = false;
	}
}

extern "C" MIX_SOUND( MixSound ) {
	SoundRenderBuffer* srb = &driver->srb;
	QueuedSound* activeSoundList = driver->activeSoundList;

	for( uint8 soundIndex = 0; soundIndex < MAXSOUNDSATONCE; ++soundIndex ) {

		if( !activeSoundList[ soundIndex ].playing ) continue;

		QueuedSound* activeSound = &activeSoundList[ soundIndex ];

		if( activeSound->soundType == SoundType::LoadedStatic ) {
			ProcessStaticQueuedSound( activeSound, srb );
		} else {
			ProcessQueuedChirp( activeSound, srb );
		}
	}
}
#endif