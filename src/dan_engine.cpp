#include "file_utilities.cpp"
#include "debug_output.cpp"
#include "memory_arena.cpp"
#include "tokenizer.cpp"
#include "hash.cpp"
#include "directory_helper.cpp"
#include "json_data_file.cpp"
#include "opengl.cpp"
#include "asset_loading.cpp"
#include "render_commands.cpp"
#include "input.cpp"
#include "rectangle_packing.cpp"
#include "collision.cpp"
#include "sprites.cpp"
#include "fonts.cpp"
#include "sound.cpp"
#include "uid_map.cpp"
#include "game_sound.cpp"
#include "game_math.cpp"
#include "message_queue.cpp"

void ApplicationInit(const char *windowName, v2i gameBufferSize, int windowToBufferRatio)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_DisplayMode currentDisplayMode;
	SDL_GetCurrentDisplayMode(0, &currentDisplayMode);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	App.baseFolder = GetExecutableDir();
	App.currentTime = 0;

	App.windowToBufferRatio = windowToBufferRatio;
	App.gameWindowSize = gameBufferSize;
	App.windowSize = {gameBufferSize.x * windowToBufferRatio, gameBufferSize.y * windowToBufferRatio};

	App.window = SDL_CreateWindow(windowName,
							  SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED,
							  App.windowSize.x,
							  App.windowSize.y,
							  SDL_WINDOW_OPENGL | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

	App.glContext = SDL_GL_CreateContext(App.window);
	SDL_GL_SetSwapInterval(1);
	SDL_ShowWindow(App.window);

	App.retinaRatio = 2;

	InitArena(&Arena, GIGABYTES(1));
	App.running = true;
}

void RendererInit()
{
	glGetIntegerv(GL_VIEWPORT, Rend.windowViewport);

#ifdef _WIN32
	// Initialize glew
	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit()) {
		// GLEW failed!
		exit(1);
	}
#endif

	glGenBuffers(ArrayCount(Rend.drawVBOs), Rend.drawVBOs); 
	for (int i = 0; i < ArrayCount(Rend.drawVBOs); i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, Rend.drawVBOs[i]);
		glBufferData(GL_ARRAY_BUFFER, KILOBYTES(500), NULL, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &Rend.baseVAO);
	glBindVertexArray(Rend.baseVAO);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glDisable(GL_CULL_FACE);

	Rend.vertexData = PUSH_ARRAY(&Arena, uint8_t, MEGABYTES(32));
}

void SetupGameData()
{
	InitHash(&Shaders, 16);
	InitHash(&Textures, 16);
	InitHash(&Sprites, 64);
	InitHash(&TileSprites, 64);
	InitHash(&BasicAnimations, 64);
	InitHash(&ColliderDescriptions, 64);
	InitHash(&FrameBuffers, 16);
	InitHash(&Fonts, 16);
	InitHash(&Sounds, 64);
	InitHash(&ComponentData, 64);
	InitHash(&PrefabData, 64);
	InitHash(&Levels, 64);
	InitUIDMap(&QueuedSounds, 100, true, sizeof(queued_sound));

	ShaderFolder = LoadDirectory(App.baseFolder, "shaders");
	ReadInShadersFromDirectory(&ShaderFolder);
	WatchDirectory(&ShaderFolder, ShaderDirCallback);

	ColliderDescriptionsFolder = LoadDirectory(App.baseFolder, "collider_descriptions");
	ReadInJsonDataFromDirectory(&ColliderDescriptionsFolder, &ColliderDescriptions);

	TextureFolder = LoadDirectory(App.baseFolder, "textures");
	ReadInTexturesFromDirectory(&TextureFolder);
	WatchDirectory(&TextureFolder, TextureDirCallback);

	SoundFolder = LoadDirectory(App.baseFolder, "sounds");
	ReadInSoundsFromDirectory(&SoundFolder);
	WatchDirectory(&SoundFolder, SoundDirCallback);

	FontFolder = LoadDirectory(App.baseFolder, "fonts");
	ReadInFontsFromDirectory(&FontFolder);

	ComponentFolder = LoadDirectory(App.baseFolder, "components");
	ReadInJsonDataFromDirectory(&ComponentFolder, &ComponentData);

	BasicAnimationFolder = LoadDirectory(App.baseFolder, "basic_animations");
	ReadInJsonDataFromDirectory(&BasicAnimationFolder, &BasicAnimations);

	LevelFolder = LoadDirectory(App.baseFolder, "levels");
	ReadInJsonDataFromDirectory(&LevelFolder, &Levels);

	PrefabFolder = LoadDirectory(App.baseFolder, "prefabs");
	ReadInJsonDataFromDirectory(&PrefabFolder, &PrefabData);
}

void SetupGameInput()
{
	SetupSDLKeyToKeyboardEnumMapping();
	ParseKeymapConfig(App.baseFolder, "KeyMapping.txt");
}

bool ProcessEvents()
{
	bool result = true;

	//collect events
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{

			switch (e.type)
			{
				case SDL_QUIT:
				{
					result = false;
				} break;

				case SDL_KEYDOWN:
				{
					if (!e.key.repeat)
					{
						keyboard_keys kbk = SDLKeyToKBEnum[e.key.keysym.scancode];
						KeyboardDown[kbk] = true;
					}
				} break;

				case SDL_KEYUP:
				{
					keyboard_keys kbk = SDLKeyToKBEnum[e.key.keysym.scancode];
					KeyboardDown[kbk] = false;
				} break;

				case SDL_WINDOWEVENT:
				{
					switch (e.window.event) {
						case SDL_WINDOWEVENT_SIZE_CHANGED:
						case SDL_WINDOWEVENT_RESIZED:
						{
							SDL_GetWindowSize(App.window, &App.windowSize.x, &App.windowSize.y);
#ifdef _WIN32
							glViewport(0, 0, e.window.data1, e.window.data2);
							Rend.windowViewport[2] = e.window.data1;
							Rend.windowViewport[3] = e.window.data2;
#else
							glGetIntegerv(GL_VIEWPORT, Rend.windowViewport);
#endif
						} break;

						case SDL_WINDOWEVENT_CLOSE:
						{
							result = false;
						} break;
					}
				} break;
			}
		}
	}


	//Update mouse
	{
		uint32_t mouseState = SDL_GetMouseState(NULL, NULL);
		MouseDown[0] = mouseState & SDL_BUTTON(SDL_BUTTON_LEFT);
		MouseDown[1] = mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT);
		MouseDown[2] = mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE);

		for (int i = 0; i < 3; i++)
		{
			MousePresses[i] = MouseDown[i] && !PreviousMouseDown[i];
			MouseReleases[i] = !MouseDown[i] && PreviousMouseDown[i];
		}

		memcpy(PreviousMouseDown, MouseDown, sizeof(MouseDown));
	}


	//Calculate game mouse
	{

		/* Determine how large to draw the framebuffer so that it fills the window as much as possible */
		//framebuffer *game = (framebuffer *)GetFromHash(&FrameBuffers, "game_screen");
		float aspect_ratio = (float)App.gameWindowSize.x / (float)App.gameWindowSize.y;
		float bb_aspect_ratio = Rend.windowViewport[2] / (float)Rend.windowViewport[3];

		//instead of this, make the screen a set multiple of the buffer size, either 4 or 8?
		float fbScale = App.windowToBufferRatio * App.retinaRatio;
		/*
		   if (bb_aspect_ratio > aspect_ratio) //height limited
		   fbScale = Rend.windowViewport[3] / App.gameWindowSize.y;
		   else //width limited
		   fbScale = Rend.windowViewport[2] / App.gameWindowSize.x;
		   */

		//Get mouse coordinates, convert them to game coordinates
		int win_mouse_x, win_mouse_y;
		SDL_GetMouseState(&win_mouse_x, &win_mouse_y);

		//map to the location on screen the framebuffer is
		float win_to_viewport_scale = Rend.windowViewport[2] / (float) App.windowSize.x;
		float win_mouse_game_x_f = win_mouse_x*win_to_viewport_scale - (Rend.windowViewport[2] - fbScale*App.gameWindowSize.x) * 0.5f;
		float win_mouse_game_y_f = (App.windowSize.y - win_mouse_y)*win_to_viewport_scale - (Rend.windowViewport[3] - fbScale*App.gameWindowSize.y) * 0.5f;

		//Depending on whether letter boxes are on the sides or top, one of these should be zero.
		//The other should be the size of the letterbox
		float gui_tl_x = (Rend.windowViewport[2] - (fbScale*(float)App.gameWindowSize.x)) * 0.5f;
		float gui_tl_y = (Rend.windowViewport[3] - (fbScale*(float)App.gameWindowSize.y)) * 0.5f;

		//Points on the screen where the game window's framebuffer is drawn
		float gui_left = gui_tl_x;
		float gui_right = gui_tl_x + (fbScale*(float)App.gameWindowSize.x);
		float gui_bottom = gui_tl_y + (fbScale*(float)App.gameWindowSize.y);
		float gui_top = gui_tl_y;
		Rend.gameViewport[0] = gui_left;
		Rend.gameViewport[1] = Rend.windowViewport[3] - gui_bottom;
		Rend.gameViewport[2] = gui_right - gui_left;
		Rend.gameViewport[3] = gui_bottom - gui_top;

		GuiVP = glm::ortho(0.0f, 100.0f, 0.0f, 100.0f, 150000.0f, -150000.0f);

		//convert to opengl screen space (-1 to 1)
		win_mouse_game_x_f /= (fbScale*(float)App.gameWindowSize.x);
		win_mouse_game_y_f /= (fbScale*(float)App.gameWindowSize.y);

		win_mouse_game_x_f = win_mouse_game_x_f * 2 - 1;
		win_mouse_game_y_f = win_mouse_game_y_f * 2 - 1;

		//Use glm matrix math to go from clip space (am I saying that right?) to game space
		glm::vec4 win_mouse_gl = glm::vec4(win_mouse_game_x_f, win_mouse_game_y_f, 0.0f, 1.0f);
		glm::vec4 mouse_world = glm::inverse(GameVP) * win_mouse_gl;
		PreviousMousePos = MousePos;
		MousePos = {mouse_world.x, mouse_world.y};


		//gui space
		glm::mat4 gui_mvp_inverse = glm::inverse(GuiVP);
		glm::vec4 mouse_gui = gui_mvp_inverse * win_mouse_gl;
		PreviousGuiMousePos = GuiMousePos;
		GuiMousePos = {mouse_gui.x, mouse_gui.y};

	}


	//Set the key state
	{
		for (int i = 0; i < KB_COUNT; i++)
			KeyboardPresses[i] = (KeyboardDown[i] && !PreviousKeyboardDown[i]);

		for (int i = 0; i < KB_COUNT; i++)
			KeyboardReleases[i] = (!KeyboardDown[i] && PreviousKeyboardDown[i]);


		AltHeld = (KeyboardDown[KB_L_ALT] || KeyboardDown[KB_R_ALT]);
		CtrlHeld = (KeyboardDown[KB_L_CTRL] || KeyboardDown[KB_R_CTRL]);
		ShiftHeld = (KeyboardDown[KB_L_SHIFT] || KeyboardDown[KB_R_SHIFT]);

		for (int i = 0; i < KEY_COUNT; i++)
		{
			keyboard_keys kbk = KeyMapping[i];
			if (KeyboardPresses[kbk])
			{
				KeyPresses[i] = true;
				KeyDown[i] = true;
				KeyReleases[i] = false;
			}
			else if (KeyboardReleases[kbk])
			{
				KeyPresses[i] = false;
				KeyDown[i] = false;
				KeyReleases[i] = true;
			}
			else
			{
				KeyPresses[i] = false;
				KeyReleases[i] = false;
			}
		}

		memcpy(PreviousKeyboardDown, KeyboardDown, sizeof(KeyboardDown));
	}

	return result;
}



void ShowFrame()
{
	SDL_GL_SwapWindow(App.window);
}

void ReloadDynamicData()
{
	if (RecompileShaders)
	{
		ReloadDirectory(&ShaderFolder);
		ReadInShadersFromDirectory(&ShaderFolder);
		RecompileShaders = false;
	}

	if (ReloadSounds)
	{
		ReloadDirectory(&SoundFolder);

		dir_file *file = GetFileInDirectory(&SoundFolder, true, SoundFileToReload);
		if (file)
			file->modified = true;

		ReadInSoundsFromDirectory(&SoundFolder);
		ReloadSounds = false;
	}

	if (ReloadTextures)
	{
		ReloadDirectory(&TextureFolder);

		dir_file *file = GetFileInDirectory(&TextureFolder, true, TextureFileToReload);
		if (file)
			file->modified = true;

		ReadInTexturesFromDirectory(&TextureFolder);
		ReloadTextures = false;
	}
}

void ApplicationShutdown()
{
	SfxShutdown();
	SDL_GL_DeleteContext(App.glContext);
	SDL_DestroyWindow(App.window);
	SDL_Quit();
}
