void SetupSDLKeyToKeyboardEnumMapping()
{
    SDLKeyToKBEnum[SDL_SCANCODE_A]				= KB_A;
   	SDLKeyToKBEnum[SDL_SCANCODE_B]				= KB_B;
   	SDLKeyToKBEnum[SDL_SCANCODE_C]				= KB_C;
   	SDLKeyToKBEnum[SDL_SCANCODE_D]				= KB_D;
   	SDLKeyToKBEnum[SDL_SCANCODE_E]				= KB_E;
   	SDLKeyToKBEnum[SDL_SCANCODE_F]				= KB_F;
   	SDLKeyToKBEnum[SDL_SCANCODE_G]				= KB_G;
   	SDLKeyToKBEnum[SDL_SCANCODE_H]				= KB_H;
   	SDLKeyToKBEnum[SDL_SCANCODE_I]				= KB_I;
   	SDLKeyToKBEnum[SDL_SCANCODE_J]				= KB_J;
   	SDLKeyToKBEnum[SDL_SCANCODE_K]				= KB_K;
   	SDLKeyToKBEnum[SDL_SCANCODE_L]				= KB_L;
   	SDLKeyToKBEnum[SDL_SCANCODE_M]				= KB_M;
    SDLKeyToKBEnum[SDL_SCANCODE_N]				= KB_N;
   	SDLKeyToKBEnum[SDL_SCANCODE_O]				= KB_O;
   	SDLKeyToKBEnum[SDL_SCANCODE_P]				= KB_P;
   	SDLKeyToKBEnum[SDL_SCANCODE_Q]				= KB_Q;
   	SDLKeyToKBEnum[SDL_SCANCODE_R]				= KB_R;
   	SDLKeyToKBEnum[SDL_SCANCODE_S]				= KB_S;
   	SDLKeyToKBEnum[SDL_SCANCODE_T]				= KB_T;
   	SDLKeyToKBEnum[SDL_SCANCODE_U]				= KB_U;
   	SDLKeyToKBEnum[SDL_SCANCODE_V]				= KB_V;
   	SDLKeyToKBEnum[SDL_SCANCODE_W]				= KB_W;
   	SDLKeyToKBEnum[SDL_SCANCODE_X]				= KB_X;
   	SDLKeyToKBEnum[SDL_SCANCODE_Y]				= KB_Y;
   	SDLKeyToKBEnum[SDL_SCANCODE_Z]				= KB_Z;

    SDLKeyToKBEnum[SDL_SCANCODE_SPACE]			= KB_SPACE;
   	SDLKeyToKBEnum[SDL_SCANCODE_LSHIFT]			= KB_L_SHIFT;
   	SDLKeyToKBEnum[SDL_SCANCODE_RSHIFT]			= KB_R_SHIFT;
   	SDLKeyToKBEnum[SDL_SCANCODE_TAB]			= KB_TAB;
   	SDLKeyToKBEnum[SDL_SCANCODE_RETURN]			= KB_ENTER;
   	SDLKeyToKBEnum[SDL_SCANCODE_LEFT]			= KB_LEFT;
   	SDLKeyToKBEnum[SDL_SCANCODE_RIGHT]			= KB_RIGHT;
   	SDLKeyToKBEnum[SDL_SCANCODE_UP]				= KB_UP;
   	SDLKeyToKBEnum[SDL_SCANCODE_DOWN]			= KB_DOWN;

    SDLKeyToKBEnum[SDL_SCANCODE_LCTRL]			= KB_L_CTRL;
   	SDLKeyToKBEnum[SDL_SCANCODE_LALT]			= KB_L_ALT;
   	SDLKeyToKBEnum[SDL_SCANCODE_RCTRL]			= KB_R_CTRL;
   	SDLKeyToKBEnum[SDL_SCANCODE_RALT]			= KB_R_ALT;
   	SDLKeyToKBEnum[SDL_SCANCODE_BACKSPACE]		= KB_DELETE;

   	SDLKeyToKBEnum[SDL_SCANCODE_0]				= KB_0;
   	SDLKeyToKBEnum[SDL_SCANCODE_1]				= KB_1;
   	SDLKeyToKBEnum[SDL_SCANCODE_2]				= KB_2;
   	SDLKeyToKBEnum[SDL_SCANCODE_3]				= KB_3;
   	SDLKeyToKBEnum[SDL_SCANCODE_4]				= KB_4;
   	SDLKeyToKBEnum[SDL_SCANCODE_5]				= KB_5;
   	SDLKeyToKBEnum[SDL_SCANCODE_6]				= KB_6;
   	SDLKeyToKBEnum[SDL_SCANCODE_7]				= KB_7;
   	SDLKeyToKBEnum[SDL_SCANCODE_8]				= KB_8;
   	SDLKeyToKBEnum[SDL_SCANCODE_9]				= KB_9;

    SDLKeyToKBEnum[SDL_SCANCODE_ESCAPE]			= KB_ESCAPE;
   	SDLKeyToKBEnum[SDL_SCANCODE_MINUS]			= KB_MINUS;
   	SDLKeyToKBEnum[SDL_SCANCODE_EQUALS]			= KB_EQUALS;
   	SDLKeyToKBEnum[SDL_SCANCODE_APOSTROPHE]		= KB_QUOTE;
   	SDLKeyToKBEnum[SDL_SCANCODE_SEMICOLON]		= KB_SEMICOLON;
   	SDLKeyToKBEnum[SDL_SCANCODE_SLASH]			= KB_FORWARD_SLASH;
   	SDLKeyToKBEnum[SDL_SCANCODE_PERIOD]			= KB_PERIOD;
   	SDLKeyToKBEnum[SDL_SCANCODE_COMMA]			= KB_COMMA;
}

void ParseKeymapConfig(const char *folder, const char *filename)
{
	char src_file[1024];
	CombinePath(src_file, folder, filename);
	char *config_src = ReadFileIntoString(src_file);
	if (config_src)
	{
		transient_arena transient = GetTransientArena(&Arena);
		token *config = Tokenize(config_src, src_file, &Arena);

		while (config)
		{
			assert(config && config->type == TOKEN_TYPE_WORD);
			bool found_gamekey = false;
			for (int i = 0; i < ArrayCount(GameKeyStrings); i++)
			{
				if (TokenMatches(config, GameKeyStrings[i]))
				{
					found_gamekey = true;
					config = config->next;
					assert(config && config->type == TOKEN_TYPE_COLON);
					config = config->next;
					assert(config && config->type == TOKEN_TYPE_WORD);

					bool found_keyboardkey = false;
					for (int j = 0; j < ArrayCount(KeyboardKeyStrings); j++)
					{
						if (TokenMatches(config, KeyboardKeyStrings[j]))
						{
							found_keyboardkey = true;
							KeyMapping[i] = (keyboard_keys)j;
							break;
						}
					}
					if (!found_keyboardkey)
					{
						ERR("Could not assign value to '%s', because '%.*s' not found", GameKeyStrings[i], config->length, config->text);
					}
					config = config->next;
					break;
				}
			}
			if (!found_gamekey)
			{
				ERR("Did not find game key called '%.*s'", config->length, config->text);
				config = config->next;
				assert(config && config->type == TOKEN_TYPE_COLON);
				config = config->next;
				assert(config && config->type == TOKEN_TYPE_WORD);
				config = config->next;
			}
		}

		FreeTransientArena(transient);
		free(config_src);
	}
	else
	{
		ERR("Could not find the keymap file '%s'", src_file);
	}
}
