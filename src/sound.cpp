#include <EASTL/list.h>
#include <EASTL/fixed_vector.h>

using namespace eastl;

// Use a namespace to hide interals

namespace _Sfx {

	// Globals
	ALCdevice * m_device;
	ALCcontext * m_context;
	class SourceManager;
	SourceManager* m_sourceManager;

	// Internal functions
	inline void _SfxSourceInvalidate(SourceRef source) {
		source->alSource = 0;
	}

	inline void _SfxSetSoundSource(SourceRef source, ALuint alSource) {
		source->alSource = alSource;
	}

#define ASSERT_AL_NO_ERROR  \
	switch(alGetError()) { \
		case AL_INVALID_NAME: ERR("OpenAL Invalid Name"); assert(false); break;\
		case AL_INVALID_ENUM: ERR("OpenAL Invalid Enum"); assert(false); break; \
		case AL_INVALID_VALUE: ERR("OpenAL Invalid Value"); assert(false); break; \
		case AL_INVALID_OPERATION: ERR("OpenAL Invalid Operation"); assert(false); break; \
		case AL_OUT_OF_MEMORY: ERR("OpenAL Out of Memory"); assert(false); break; \
	}

#ifdef __APPLE__
#undef ASSERT_AL_NO_ERROR
#define ASSERT_AL_NO_ERROR
#endif


	void InitSoundALSource(SoundRef sound, ALuint alSource) {

		if (sound->isStream) {
			//Store the AL source
			sound->alSource = alSource;

			//Que the two buffers onto the source
			alSourceQueueBuffers(alSource, BUFFERS_PER_STREAM, sound->alBuffers);
			ASSERT_AL_NO_ERROR
		} else {
			//Set Looping attribute of source
			alSourcei(alSource, AL_LOOPING, (sound->looping ? 1 : 0));
			ASSERT_AL_NO_ERROR

				//Queue the buffer onto the source
				alSourceQueueBuffers(alSource, 1, &sound->alBuffer);
			ASSERT_AL_NO_ERROR
		}
	}

	void InitVorbisFile(
		const char* fileName,
		ALsizei& frequency,
		ALenum& format,
		long long& bytes_to_load,
		OggVorbis_File& ov_file
	) {
		//Clear the memory of the vorbis file
		memset(&ov_file, 0, sizeof(OggVorbis_File));

		int ov_open_res = ov_fopen(fileName, &ov_file);
		assert(ov_open_res == 0);

		long long pcm_samples = ov_pcm_total(&ov_file, -1);

		// Get info from the file
		vorbis_info * file_info = ov_info(&ov_file, -1);

		//Detect the data format (16 bit mono or stereo)
		if (file_info->channels == 1) {
			format = AL_FORMAT_MONO16;
		} else if (file_info->channels == 2) {
			format = AL_FORMAT_STEREO16;
			//Stereo data should have an even number of samples
			if (pcm_samples % 2 != 0) {
				pcm_samples -= 1;
			}
		} else {
			//More than 2 channels are not supported
			assert(0);
		}

		//Always use 16-bit format
		bytes_to_load = pcm_samples * 2;

		frequency = file_info->rate;
	}

	// Returns true if the read looped
	bool DecodeOggData(ALuint buffer, OggVorbis_File *vf, ALenum format, ALsizei frequency, int size, bool* file_done, bool loop) {

		// Read in PCM data from Ogg file

		//Temp buffer to hold the PCM data
		char * data = new char[size];
		memset(data, 0, size);

		if (!size) {
			//No data in Ogg file
			assert(0);
		}

		// read in the data
		int current_section;
		int eof = 0;
		int bytes_left = size;
		char * write_pos = data;

		//Initialize file_done to false
		if (file_done) {
			*file_done = false;
		}

		bool looped = false;

		// Read data into temp buffer
		while (!eof && bytes_left > 0) {
			long ret = ov_read(vf, write_pos, bytes_left, 0, 2, 1, &current_section);
			if (ret == 0) {
				// EOF
				if (loop == true) {
					looped = true;
					ov_time_seek(vf, 0.0);
				} else {
					if (bytes_left > 0) {
						memset(write_pos, 0, bytes_left);
					}
					if (file_done) {
						*file_done = true;
					}

					eof = 1;
				}
			} else if (ret == OV_EBADLINK) {
				// error in the stream.  Not a problem, just reporting it in case we (the app) cares.  In this case, we don't.
				assert(0);
			} else if (ret == OV_HOLE) {
				// error in the stream.  Not a problem, just reporting it in case we (the app) cares.  In this case, we don't.
				assert(0);
			} else if (ret == OV_EINVAL) {
				// Invalid argument passed to ov_read()
				assert(0);
			} else if (ret < 0) {
				// error in the stream.  Not a problem, just reporting it in case we (the app) cares.  In this case, we don't.
				assert(0);
			} else {
				// we don't bother dealing with sample rate changes, etc, but you'll have to
				// Move the pointer forward (and the remaining size backward) by the amount of bytes that were translated
				write_pos += ret;
				bytes_left -= ret;

				if (bytes_left <= 2) {
					eof = 1;
				}
			}
		}

		//Copy decompressed sound data to AL buffer
		alBufferData(buffer, format, data, size, frequency);
		ALenum temp = alGetError();
		ASSERT_AL_NO_ERROR

		delete[] data;

		return looped;
	}

	inline bool StreamingComplete(SoundRef sound) {
		return (sound->eofWrites > 2);
	}

	void StreamUpdate(SoundRef sound, SourceRef source) {

		//Make sure we're loaded
		assert(sound->alBuffers[0] != 0);

		//Check whether the queue needs to be updated
		int processed;
		alGetSourcei(sound->alSource, AL_BUFFERS_PROCESSED, &processed);
		ASSERT_AL_NO_ERROR;

		while (processed--)
		{
			ALuint buffer;

			alSourceUnqueueBuffers(sound->alSource, 1, &buffer);
			ASSERT_AL_NO_ERROR;

			//Load OGG data into the unqued buffer
			bool eof = false;

			bool looped = DecodeOggData(
				buffer,
				&sound->ovFile,
				sound->format,
				sound->frequency,
				STREAM_BUFFER_SIZE,
				&eof,
				sound->looping
			);

			//If this was a full zero write, increase the conter
			if (eof == true) {
				sound->eofWrites++;
			}

			// If sound is finished
			if (StreamingComplete(sound)) {
				// Stop the AL source
				alSourceStop(sound->alSource);
				ASSERT_AL_NO_ERROR;
			} else {
				//Sound isn't finished
				alSourceQueueBuffers(sound->alSource, 1, &buffer);
				ASSERT_AL_NO_ERROR;
			}
		}

		//Make sure the sound is still playing.  It may have stopped if we didn't
		//update fast enough.
		ALint state;
		alGetSourcei(sound->alSource, AL_SOURCE_STATE, &state);
		ASSERT_AL_NO_ERROR;

		//These next lines make it so that you can't pause or stop streams
		/*if (state != AL_PLAYING) {
			alSourcePlay(sound->alSource);
			ASSERT_AL_NO_ERROR;
		}*/
	}

	void SoundLoad(Sound* sound) {
		if (sound->isStream) {
			//Make sure we're not already loaded
			assert(sound->alBuffers[0] == 0);

			//Initialize the vorbis file
			long long bytes_to_load;
			InitVorbisFile(
				sound->fileName,
				sound->frequency,
				sound->format,
				bytes_to_load,
				sound->ovFile
			);

			//OpenAL buffers cannot be larger than INT_MAX in bytes, so this sound is
			//too large to be loaded with a single buffer and should be streamed.
			assert(bytes_to_load <= INT_MAX);

			// Generate the buffers
			alGenBuffers(BUFFERS_PER_STREAM, sound->alBuffers);
			ASSERT_AL_NO_ERROR;

			//Reset the eof write counter
			sound->eofWrites = 0;

			//Read the first two chunks of the file into the two buffers
			bool eof = false;
			for (size_t i = 0; i < BUFFERS_PER_STREAM; ++i) {
				DecodeOggData(
					sound->alBuffers[i],
					&sound->ovFile,
					sound->format,
					sound->frequency,
					STREAM_BUFFER_SIZE,
					&eof,
					sound->looping
				);

				if (eof) {
					++sound->eofWrites;
				}
			}
		} else {
			//Make sure we're not already loaded
			assert(sound->alBuffer == 0);

			//Initialize the vorbis file
			OggVorbis_File ov_file;
			ALsizei frequency;
			ALenum format;
			long long bytes_to_load;
			InitVorbisFile(sound->fileName, frequency, format, bytes_to_load, ov_file);

			//OpenAL buffers cannot be larger than INT_MAX in bytes, so this sound is
			//too large to be loaded with a single buffer and should be streamed.
			assert(bytes_to_load <= INT_MAX);

			// Generate the buffer
			alGenBuffers(1, &sound->alBuffer);
			ASSERT_AL_NO_ERROR;

			//Read the whole file into the buffer
			DecodeOggData(
				sound->alBuffer,
				&ov_file,
				format,
				frequency,
				(int)bytes_to_load,
				nullptr,
				false
			);

			//Clear out the vorbis file
			ov_clear(&ov_file);
		}
	}

	// Internal class
	class SourceManager {
	public:
		SourceManager();
		~SourceManager();

		struct PlayingSource {
			ALuint source;
			SourceRef handle;
			SoundRef sound;
		};

		typedef fixed_vector<PlayingSource, MAX_PLAYING_SOUNDS, false>::iterator PlayingSourcesIterator;

		void StopSource(PlayingSourcesIterator& it);
		void StopAllSourcesPlayingSound(SoundRef sound);
		ALuint GetNextAvailiableSource(SourceRef handle, SoundRef sound);
		void ReclaimStoppedSources();
	protected:
		fixed_vector<ALuint, MAX_PLAYING_SOUNDS * 4, false> m_unusedSources;
		fixed_vector<PlayingSource, MAX_PLAYING_SOUNDS, false> m_playingSources;
	};

	void SoundUnload(SoundRef sound) {
		//All sources playing the sound must be stopped
		assert(m_sourceManager);
		m_sourceManager->StopAllSourcesPlayingSound(sound);

		if (sound->isStream) {
			//Free OpenAL buffers
			if (sound->alBuffers[0]) {
				alDeleteBuffers(BUFFERS_PER_STREAM, sound->alBuffers);
				ASSERT_AL_NO_ERROR;

				sound->alBuffers[0] = 0;
				sound->alBuffers[1] = 0;
			}

			//Clear out the vorbis file
			ov_clear(&sound->ovFile);
		} else {
			//Free OpenAL buffer
			if (sound->alBuffer) {
				alDeleteBuffers(1, &sound->alBuffer);
				assert(alGetError() == AL_NO_ERROR);
				sound->alBuffer = 0;
			}
		}
	}


	SourceManager::SourceManager() {
		//Create OpenAL sound sources
		m_unusedSources.resize(MAX_PLAYING_SOUNDS);
		alGenSources(MAX_PLAYING_SOUNDS, &(m_unusedSources[0]));
		ASSERT_AL_NO_ERROR;
	}

	SourceManager::~SourceManager() {

		//Delete both playing and unused OpenAL sources
		ALsizei count = (ALsizei)m_unusedSources.size();
		if (count > 0) {
			alDeleteSources(count, &(m_unusedSources[0]));
			ASSERT_AL_NO_ERROR;
		}
		m_unusedSources.clear();

		for (auto it : m_playingSources) {
			_SfxSourceInvalidate(it.handle);
			alDeleteSources(1, &it.source);
			ASSERT_AL_NO_ERROR;
		}
		m_playingSources.clear();
	}

	ALuint SourceManager::GetNextAvailiableSource(SourceRef handle, SoundRef sound) {
		//Check if there are any sources in the unused source list
		if (m_unusedSources.size() == 0) {
			//There are currently no unused sources.  Try to reclaim sources that
			//have been stopped
			ReclaimStoppedSources();

			//If there are no unused sources at this point, then
			//MAX_PLAYING_SOUNDS should be increased or fewer sounds should be
			//played.
			assert(m_unusedSources.size() > 0);
		}

		//Get the next unused source and then pop it off the stack
		ALuint alSource = m_unusedSources.back();
		m_unusedSources.pop_back();

		//Create a new source handle and add the AL source to the playing stack
		PlayingSource ps;
		ps.source = alSource;
		ps.handle = handle;
		ps.sound = sound;

		m_playingSources.push_back(ps);

		//Set the source id in the handle
		_SfxSetSoundSource(handle, alSource);

		// Set default values on source
		alSourcef(alSource, AL_PITCH, 1.0f);
		ASSERT_AL_NO_ERROR;

		float values[3] = { 0.0f, 0.0f, 0.0f };
		alSourcefv(alSource, AL_POSITION, values);
		ASSERT_AL_NO_ERROR;

		alSourcef(alSource, AL_GAIN, 1.0f);
		ASSERT_AL_NO_ERROR;

		return alSource;
	}

	void SourceManager::ReclaimStoppedSources() {
		//Loop through all of the currently playing sources and check to see
		//if they have stopped

		PlayingSourcesIterator it = m_playingSources.begin();
		while (it != m_playingSources.end()) {
			ALuint alSource = it->source;

			ALint state;
			alGetSourcei(alSource, AL_SOURCE_STATE, &state);
			ASSERT_AL_NO_ERROR;

			if (!it->sound->isStream) {
				if (state == AL_STOPPED) {
					//This source has stopped, so reclaim it
					StopSource(it);
				} else {
					//Push forward the iterator since we didn't erase an element
					++it;
				}
			} else {
				SoundRef stream = it->sound;
				StreamUpdate(stream, it->handle);
				if (state == AL_STOPPED && StreamingComplete(stream)) {
					//This source has stopped, so reclaim it
					StopSource(it);

					//Since it's a stream, we need to release and reload it for
					//the next time it's used.
					SfxSoundReload(stream);
				} else {
					//Push forward the iterator since we didn't erase an element
					++it;
				}
			}
		}
	}

	void SourceManager::StopSource(PlayingSourcesIterator& it) {
		//First, get its data and remove it from the playing stack
		PlayingSource ps = *it;
		it = m_playingSources.erase(it);

		//Invalidate its handle and add it to the unused stack
		_SfxSourceInvalidate(ps.handle);
		m_unusedSources.push_back(ps.source);

		//Stop the source and unqueue its buffers
		alSourceStop(ps.source);
		ASSERT_AL_NO_ERROR;

		ALint num_buffers;
		alGetSourcei(ps.source, AL_BUFFERS_PROCESSED, &num_buffers);
		ASSERT_AL_NO_ERROR;

		ALuint removed_buffers[MAX_PLAYING_SOUNDS * 2];
		assert(num_buffers < MAX_PLAYING_SOUNDS * 2);
		alSourceUnqueueBuffers(ps.source, num_buffers, removed_buffers);
		ASSERT_AL_NO_ERROR;
	}

	void SourceManager::StopAllSourcesPlayingSound(SoundRef sound) {
		//Loop through all the playing sources, and stop any that match the
		//passed in sound address

		PlayingSourcesIterator it = m_playingSources.begin();
		while (it != m_playingSources.end()) {
			if (it->sound == sound) {
				//This sound matches, so stop it
				StopSource(it);
			} else {
				//Push forward the iterator since we didn't erase an element
				++it;
			}
		}
	}
}

// Public API

Sound::Sound(const char* fileName, bool isStream, bool looping) : isStream(isStream), looping(looping) {
	// Copy the string so pointer isn't reliant on external buffer
	this->fileName = new char[strlen(fileName) + 1];
	strcpy(this->fileName, fileName);

	// Sounds have different data depending on whether they are streams or not
	if (isStream) {
		alBuffers[0] = 0;
		alBuffers[1] = 0;
		eofWrites = 0;
	} else {
		alBuffer = 0;
	}

	// Go ahead and load the sound data into the buffer right away
	_Sfx::SoundLoad(this);
}

Sound::~Sound() {
	if (fileName != nullptr) {
		delete[] fileName;
		fileName = nullptr;
	}
}

void SfxInitialize() {
	using namespace _Sfx;

	//Initialize OpenAL
	m_device = alcOpenDevice(nullptr);
	ASSERT_AL_NO_ERROR;

	if (m_device) {
		m_context = alcCreateContext(m_device, nullptr);
		ASSERT_AL_NO_ERROR;

		alcMakeContextCurrent(m_context);
		ASSERT_AL_NO_ERROR;
	}

	//Create OpenAL source manager
	m_sourceManager = new SourceManager();
}

void SfxShutdown() {
	using namespace _Sfx;

	//Release OpenAL sound sources
	if (m_sourceManager) {
		delete m_sourceManager;
		m_sourceManager = 0;
	}

	//Clean up OpenAL
	alcMakeContextCurrent(nullptr);
	ASSERT_AL_NO_ERROR;

	alcDestroyContext(m_context);
	ASSERT_AL_NO_ERROR;

	alcCloseDevice(m_device);
	ASSERT_AL_NO_ERROR;
}

void SfxUpdate() {
	using namespace _Sfx;
	if (m_sourceManager) {
		m_sourceManager->ReclaimStoppedSources();
	}
}

float SfxGetMasterVolume() {
	float gain;

	alGetListenerf(AL_GAIN, &gain);
	ASSERT_AL_NO_ERROR;

	return gain;
}

void SfxSetMasterVolume(float newVolume) {
	alListenerf(AL_GAIN, newVolume);
	ASSERT_AL_NO_ERROR;
}

// Sound

SoundRef SfxCreateStream(const char* fileName, bool looping) {
	return SoundRef(new Sound(fileName, true, looping));
}

SoundRef SfxCreateSample(const char* fileName, bool looping) {
	return SoundRef(new Sound(fileName, false, looping));
}

SourceRef SfxSoundPlay(SoundRef sound, bool startPaused) {
	using namespace _Sfx;

	assert(m_sourceManager);

	// Get next availiable source
	SourceRef newSource(new Source);
	newSource->alSource = 0;
	newSource->sound = sound;

	ALuint alSource = m_sourceManager->GetNextAvailiableSource(newSource, sound);

	//Add the sound buffers to the AL source
	InitSoundALSource(sound, alSource);

	if (startPaused) {
		alSourcePause(alSource);
		ASSERT_AL_NO_ERROR;
	}
	else {
		alSourcePlay(alSource);
		ASSERT_AL_NO_ERROR;
	}
	
	return newSource;
}

void SfxSourcePlay(SourceRef source) {
	if (SfxSourceIsValid(source)) {
		alSourcePlay(source->alSource);
		ASSERT_AL_NO_ERROR;
	}	
}

void SfxSoundReload(SoundRef sound) {
	using namespace _Sfx;

	// Unload AL buffer and then reload from file.
	SoundUnload(sound);
	SoundLoad(sound.get());
}

// Source
bool SfxSourceIsValid(SourceRef source) {
	return (source && source->alSource != 0);
}

void SfxSourceSetPitch(SourceRef source, float pitch) {
	if (!SfxSourceIsValid(source)) {
		// Do nothing since the source has already been invalidated
		return;
	}

	assert(pitch > 0.0f);

	alSourcef(source->alSource, AL_PITCH, pitch);
	ASSERT_AL_NO_ERROR;
}

float SfxSourceGetPitch(SourceRef source) {
	if (!SfxSourceIsValid(source)) {
		return 1.0f;
	}

	float pitch;
	alGetSourcef(source->alSource, AL_PITCH, &pitch);
	ASSERT_AL_NO_ERROR;
	return pitch;
}

void SfxSourceSetVolume(SourceRef source, float volume) {
	if (!SfxSourceIsValid(source)) {
		// Do nothing since the source has already been invalidated
		return;
	}

	if (volume < 0 || volume > 1)
	{
		ERR("Sound volume must be [0.0f, 1.0f]");
		ClampToRange(0.0f, &volume, 1.0f);
	}

	alSourcef(source->alSource, AL_GAIN, volume);
	ASSERT_AL_NO_ERROR;
}

float SfxSourceGetVolume(SourceRef source) {
	if (!SfxSourceIsValid(source)) {
		return 0.0f;
	}

	float volume;
	alGetSourcef(source->alSource, AL_GAIN, &volume);
	ASSERT_AL_NO_ERROR;
	return volume;
}

void SfxSourceSetPosition(SourceRef source, glm::vec3& position) {
	if (!SfxSourceIsValid(source)) {
		// Do nothing since the source has already been invalidated
		return;
	}

	float values[3] = { position.x, position.y, position.z };

	alSourcefv(source->alSource, AL_POSITION, values);
	ASSERT_AL_NO_ERROR;
}

void SfxSourceSetPosition(SourceRef source, float x, float y, float z) {
	if (!SfxSourceIsValid(source)) {
		// Do nothing since the source has already been invalidated
		return;
	}

	float values[3] = {x, y, z };

	alSourcefv(source->alSource, AL_POSITION, values);
	ASSERT_AL_NO_ERROR;
}

void SfxSourceGetPosition(SourceRef source, glm::vec3& out) {
	if (!SfxSourceIsValid(source)) {
		// Do nothing since the source has already been invalidated
		out.x = 0.0f;
		out.y = 0.0f;
		out.z = 0.0f;
		return;
	}

	float pos[3];
	alGetSourcefv(source->alSource, AL_POSITION, pos);
	ASSERT_AL_NO_ERROR;

	out.x = pos[0];
	out.y = pos[1];
	out.z = pos[2];
}

bool SfxSourceIsPlaying(SourceRef source) {
	if (!SfxSourceIsValid(source)) {
		return false;
	}

	//Check the state of the AL source
	ALint state;
	alGetSourcei(source->alSource, AL_SOURCE_STATE, &state);
	ASSERT_AL_NO_ERROR;

	return (state == AL_PLAYING);
}

void SfxSourcePause(SourceRef source) {
	if (!SfxSourceIsValid(source)) {
		// Do nothing since the source has already been invalidated
		return;
	}

	if (SfxSourceIsPaused(source) == false) {
		alSourcePause(source->alSource);
		ASSERT_AL_NO_ERROR;
	}
}

bool SfxSourceIsPaused(SourceRef source) {
	if (!SfxSourceIsValid(source)) {
		return false;
	}

	ALint state;
	alGetSourcei(source->alSource, AL_SOURCE_STATE, &state);
	ASSERT_AL_NO_ERROR;

	return (state == AL_PAUSED);
}

void SfxSourceStop(SourceRef source) {
	if (!SfxSourceIsValid(source)) {
		// Do nothing since the source has already been invalidated
		return;
	}

	//Stop the AL source
	alSourceStop(source->alSource);
	ASSERT_AL_NO_ERROR;
}
