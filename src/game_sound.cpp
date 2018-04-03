queued_sound *GetNextAvailableSound(uint32_t *uid)
{
	assert(UIDMapIsValid(&QueuedSounds));
	uid_entry *entry = UIDAddEntry(&QueuedSounds);
	queued_sound *result = (queued_sound *)entry->data;
	if (uid)
	   *uid = entry->id;
	return result;
}

uint32_t PlaySound(const char *sound_name, queued_sound **handle, float volume)
{
	uint32_t result = 0;
	queued_sound *sound = NULL;
	sound_file *sf = (sound_file *)GetFromHash(&Sounds, sound_name);

	if (sf == NULL)
	{
		ERR("Could not find sound file named '%s'", sound_name);
		return 0;
	}

	if (sf->loaded == false)
	{
		if (sf->is_music)
			sf->sample = SfxCreateStream(sf->filename, sf->loops);
		else
			sf->sample = SfxCreateSample(sf->filename, sf->loops);
		sf->loaded = true;
	}

	sf->last_time_played = App.currentTime;
	sound = GetNextAvailableSound(&result);

	if (handle)
		*handle = sound;

	if (sound == NULL)
		return 0;

	sound->state = SND_PLAYING;
	sound->parent = sf;
	sound->volume = volume == -1 ? sf->default_volume : volume;
	sound->current_channel = sf->default_channel;
	sound->is_fading = false;
	sound->source = SfxSoundPlay(sf->sample);
	SfxSourceSetVolume(sound->source, sound->volume);
	return result;
}

queued_sound *GetSoundByID(uint32_t uid)
{
	queued_sound *result;
	result = (queued_sound *)UIDMapGet(&QueuedSounds, uid);
	return result;
}

queued_sound *FindPlayingSound(const char *sound_name, uint32_t *uid)
{
	queued_sound *result = NULL;
	if (uid)
		*uid = 0;

	sound_file *sf = (sound_file *)GetFromHash(&Sounds, sound_name);
	if (sf)
	{
		for (int i = 0; i < QueuedSounds.numEntries; i++)
		{
			queued_sound *qs = (queued_sound *)QueuedSounds.table[i].data;
			if (qs->state != SND_NO_SOURCE && qs->parent == sf)
			{
				result = qs;
				if (uid)
					*uid = QueuedSounds.table[i].id;
				return result;
			}
		}
	}

	return result;
}

bool IsValidSoundSource(queued_sound *sound)
{
	bool result = false;
	if (SfxSourceIsValid(sound->source))
		result = true;
	else
		sound->state = SND_NO_SOURCE;

	return result;
}

void PauseSound(queued_sound *sound)
{
	if (!IsValidSoundSource(sound))
		return;
	sound->state = SND_PAUSED;
	SfxSourcePause(sound->source);
}

void StopSound(queued_sound *sound)
{
	if (!IsValidSoundSource(sound))
		return;
	sound->state = SND_STOPPED;
	SfxSourceStop(sound->source);
}

void RewindSound(queued_sound *sound)
{
	if (!IsValidSoundSource(sound))
		return;

	switch (sound->state)
	{
		case SND_PLAYING:
		{
			SfxSourceStop(sound->source);
			SfxSourcePlay(sound->source);
		} break;

		case SND_PAUSED:
		{
			SfxSourceStop(sound->source);
			SfxSourcePlay(sound->source);
			SfxSourcePause(sound->source);
		} break;

		default:
		{ } break;
	}
}

void PlaySound(queued_sound *sound)
{
	if (!IsValidSoundSource(sound))
		return;
	sound->state = SND_PLAYING;
	SfxSourcePlay(sound->source);
}

void UnloadSoundResource(const char *sound_name)
{
	sound_file *sf = (sound_file *)GetFromHash(&Sounds, sound_name);
	UnloadSoundResource(sf);
}

void UnloadSoundResource(sound_file *sf)
{
	if (sf == NULL)
	{
		ERR("No sound file specified");
		return;
	}
	else if (!sf->loaded)
	{
		return;
	}
	sf->loaded = false;
	sf->sample = NULL;
}

void FadeToFrom(queued_sound *sound, float fade_time, fade_complete_action action, float from_volume, float to_volume, sound_callback *on_complete, void *on_complete_data)
{
	if (sound->is_fading)
	{
		DBG("Setting a sound to fading that is already fading");
	}
	if (!IsValidSoundSource(sound))
		return;
	sound->is_fading = true;
	sound->fade.action = action;
	sound->fade.to_volume = to_volume;
	sound->fade.from_volume = from_volume == -1 ? sound->volume : from_volume;
	sound->fade.total_time = fade_time;
	sound->fade.time_remaining = fade_time;
	sound->fade.on_complete = on_complete;
	sound->fade.on_complete_data = on_complete_data;
}

void _UnqueueSoundClip(uid_entry *entry)
{
	queued_sound *sound = (queued_sound *)entry->data;
	sound->state = SND_NO_SOURCE;
	sound->source = NULL;
	UIDMapRemove(&QueuedSounds, entry->id);
}

void UpdateGameSounds(float delta_t)
{
	//iterate backwards in case we have to delete sounds
	for (int i = QueuedSounds.numEntries - 1; i >= 0; i--)
	{
		uid_entry *entry = &QueuedSounds.table[i];
		queued_sound *sound = (queued_sound *)entry->data;

		//if the sound was manually stopped or it came to a stop after finishing
		if ((sound->state == SND_PLAYING || sound->state == SND_STOPPED) && !SfxSourceIsPlaying(sound->source) && !SfxSourceIsPaused(sound->source))
		{
			//source is stopped.  Let's delete it.
			//run callback first
			if (sound->on_complete)
				sound->on_complete(sound->parent, sound->on_complete_data);

			_UnqueueSoundClip(entry);
		}

		if (sound->state == SND_PLAYING && sound->is_fading)
		{
			sound->fade.time_remaining -= delta_t;

			if (sound->fade.time_remaining <= 0)
			{
				sound->volume = sound->fade.to_volume;
				sound->is_fading = false;
			}
			else
			{
				//calculate volume after delta_t
				float time_fraction = (sound->fade.total_time - sound->fade.time_remaining) / sound->fade.total_time;
				sound->volume = sound->fade.from_volume + (sound->fade.to_volume - sound->fade.from_volume)*time_fraction;
			}
			SfxSourceSetVolume(sound->source, sound->volume);

			if (!sound->is_fading)
			{
				//run callback
				if (sound->fade.on_complete)
					sound->fade.on_complete(sound->parent, sound->fade.on_complete_data);

				switch (sound->fade.action)
				{
					case FADE_COMPLETE_RELEASE_SOURCE:
					{
						_UnqueueSoundClip(entry);
					} break;

					//Actually just resets to the beginning of the clip and pauses it
					case FADE_COMPLETE_STOP:
					{
						StopSound(sound);
						PlaySound(sound);
						PauseSound(sound);
					} break;

					case FADE_COMPLETE_PAUSE:
					{
						PauseSound(sound);
					} break;
					
					case FADE_COMPLETE_KEEP_PLAYING:
					{
						//do nothing
					} break;
				}
			}
		}
	}
}
