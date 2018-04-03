enum sound_channel
{
	CHANNEL_MAIN,
	CHANNEL_EFFECTS,
	CHANNEL_MUSIC,
};

global_variable const char *SoundChannelNames[] = {
	"CHANNEL_MAIN",
	"CHANNEL_EFFECTS",
	"CHANNEL_MUSIC",
};

struct sound_file
{
	char name[128];
	bool loaded;
	SoundRef sample;
	char filename[KILOBYTES(1)];
	bool is_music;
	bool loops;
	float default_volume;
	sound_channel default_channel;
	uint32_t last_time_played;
};

enum sound_state
{
	SND_NO_SOURCE,

	SND_PLAYING,
	SND_STOPPED,
	SND_PAUSED,
};

enum fade_complete_action
{
	FADE_COMPLETE_RELEASE_SOURCE,
	FADE_COMPLETE_STOP,
	FADE_COMPLETE_PAUSE,
	FADE_COMPLETE_KEEP_PLAYING,
};

#define SOUND_CALLBACK(Name) void Name(sound_file *sf, void *data)
typedef SOUND_CALLBACK(sound_callback);

struct queued_sound
{
	sound_state state;
	SourceRef source;
	sound_file *parent;
	float volume;
	sound_channel current_channel;

	bool is_fading;
	struct
	{
		fade_complete_action action;
		float total_time;
		float time_remaining;
		float from_volume;
		float to_volume;
		sound_callback *on_complete;
		void *on_complete_data;
	} fade;

	sound_callback *on_complete;
	void *on_complete_data;
};

queued_sound *GetNextAvailableSound();

uint32_t PlaySound(const char *sound_name, 
				   queued_sound **handle = NULL, 
				   float volume = -1.0f);

queued_sound *GetSoundByID(uint32_t uid);

/*
queued_sound *qs = FindPlayingSound("example_music_clip");
if (qs)
	SfxSourceSetPitch(qs->source, 1.5f);
*/
queued_sound *FindPlayingSound(const char *sound_name, 
							   uint32_t *uid = NULL);

bool IsValidSoundSource(queued_sound *sound);
void PauseSound(queued_sound *sound);

//This will release the queued sound (cannot be started again)
void StopSound(queued_sound *sound);

//Rewinds to the beginning, pauses it
void RewindSound(queued_sound *sound);

//Plays a paused sound
void PlaySound(queued_sound *sound);

void UnloadSoundResource(const char *sound_name);
void UnloadSoundResource(sound_file *sf);

void FadeToFrom(queued_sound *sound,
				float fade_time, 
				fade_complete_action action, 
				float from_volume,
				float to_volume, 
				sound_callback *on_complete = NULL,
				void *on_complete_data = NULL);

void UpdateGameSounds(float delta_t);
