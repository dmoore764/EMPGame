#pragma once

#define MAX_PLAYING_SOUNDS 16
#define STREAM_BUFFER_SIZE 65536
#define BUFFERS_PER_STREAM 8

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

#include <memory>
using namespace std;

struct Sound;
struct Source;

typedef shared_ptr<Sound> SoundRef;
typedef weak_ptr<Sound> SoundWeakRef;

typedef shared_ptr<Source> SourceRef;
typedef weak_ptr<Source> SourceWeakRef;

struct Sound {
	char* fileName;
	bool looping;
	bool isStream;

	Sound(const char* fileName, bool isStream, bool looping);
	~Sound();

	union {
		struct { // Sound Samples
			ALuint alBuffer;
		};
		struct { // Sound Streams
			ALuint alBuffers[BUFFERS_PER_STREAM];
			OggVorbis_File ovFile;
			ALsizei frequency;
			ALenum format;
			size_t eofWrites;
			ALuint alSource;
		};
	};
};

struct Source {
	SoundRef sound;
	ALuint alSource;
};

// Start, update, Shutdown

// Initialize the sound system.  Must be called once before any other sound functions are called.
void SfxInitialize();

// Update the sound system.  Must be called periodically (not necessarily every frame) to keep streams playing and free source handles.
void SfxUpdate();

// Free resorces used by the sound system.  Will not free individual sound pointers.
void SfxShutdown();

// Get the current master volume.  This affects the volume of all playing sounds.  1.0 is 100%
float SfxGetMasterVolume();

// Sets the current master volume.  This affects the volume of all playing sounds.  1.0 is 100%
void SfxSetMasterVolume(float newVolume);

// Sound

// Loads a sound sample from the given Ogg file
// The entire file will be decoded into a sound buffer that can then be played over and over.
// If looping is true, the sound will loop over and over when played.
SoundRef SfxCreateSample(const char* fileName, bool looping = false);

// Creates a streaming sound.  This is used for large files, such as music or dialog, that would take up too much memory
// if they were loaded all at once.  Streams maintain multiple buffers and load the sound a little at a time as it's played.
// You can only play each stream object once, but you can create multiple streams that point to the same file.
SoundRef SfxCreateStream(const char* fileName, bool looping = false);

// Stops the sound and frees all the saved data buffers that it uses, but then reloads it again from disk.
// This might be useful if the file has changed.
void SfxSoundReload(SoundRef sound);

// Plays the given sound object and returns a source handle that can be used to manipulate its properties during playback.
// If startPaused is set, the sound won't play right away, but the source handle can be used to play it after the desired properties are set.
// The source handle refers to a specific copy of the sound being played, so when playing the same sample multipe times, you can still
// adjust the properties of each copy of the sound individually.
SourceRef SfxSoundPlay(SoundRef sound, bool startPaused = false);

// Source

// Plays a stopped or paused source
void SfxSourcePlay(SourceRef source);

// Pauses a source.  Calling SfxSourcePlay will cause it to pick up where it left off.
void SfxSourcePause(SourceRef source);

// Stops a source.  Calling SfxSourcePlay on a stream will cause it to start over from the begining.
void SfxSourceStop(SourceRef source);

// Checks whether a source handle is currently valid.  Handles become invalid after the instance of the sound they are related to stops playing.
bool SfxSourceIsValid(SourceRef source);

// Checks whether a particular source of sound is currently playing.
bool SfxSourceIsPlaying(SourceRef source);

// Checks whether a particular source of sound is currently paused.
bool SfxSourceIsPaused(SourceRef source);

// Sets the pitch for a sound source.  Higher pitches will make the sound shorter as well as higher, while lower ones do the reverse.
// 1.0 is the default pitch.
void SfxSourceSetPitch(SourceRef source, float pitch);

// Gets the pitch of a sound source.
float SfxSourceGetPitch(SourceRef source);

// Sets the volume of a particular sound source.  Default is 1.0 = 100%
void SfxSourceSetVolume(SourceRef source, float volume);

// Gets the volume of a particular sound source.
float SfxSourceGetVolume(SourceRef source);

// Sets the position in 3D space of a sound source.  The listener is fixed at 0,0,0
void SfxSourceSetPosition(SourceRef source, glm::vec3& position);
void SfxSourceSetPosition(SourceRef source, float x, float y, float z);

// Gets the position in 3D space of a sound source.
void SfxSourceGetPosition(SourceRef source, glm::vec3& out);
