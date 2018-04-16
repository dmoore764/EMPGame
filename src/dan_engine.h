#include "useful_defines.h"

//Get the location of the executable file
#include "string.h"
#include "errno.h"

#ifdef __APPLE__
	// POSIX
	#include "unistd.h"	
	#include "libproc.h"
	#include "dirent.h"

	//Posix thread stuff
	#include "pthread.h"

	//mac thing to get the api for reading the mac directory
#define Component AppleComponent
	#include <CoreServices/CoreServices.h>
#undef Component

	//mac specific style of including OpenGL
	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>
#else
	// Need to use glew to include OpenGL on Windows since OpenGL > v1.1 is not
	// officially supported.  Glew automatically loads the extensions that
	// expose new GL 4.6 functionality and defines functions to allow us to use
	// it as if GL 4.6 were natively supported.
	#include <GL/glew.h>

	// Windows OS header
	#define WIN32_LEAN_AND_MEAN
	#include "Windows.h"
#endif

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "memory.h"
#include "utf8/utf8.h"
#include "string_utilities.h"

#include "assert.h"

#include "SDL.h"
#ifdef _WIN32
	#include "SDL_syswm.h"
#endif

#define GENERIC_CALLBACK(Name) void Name(void *data)
typedef GENERIC_CALLBACK(generic_callback);

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "game_math.h"
#include "file_utilities.h"
#include "debug_output.h"
#include "memory_arena.h"
#include "tokenizer.h"
#include "hash.h"
#include "directory_helper.h"
#include "json_data_file.h"
#include "opengl.h"
#include "asset_loading.h"
#include "rectangle_packing.h"
#include "collision.h"
#include "sprites.h"
#include "fonts.h"
#include "render_commands.h"
#include "input.h"
#include "sound.h"
#include "uid_map.h"
#include "game_sound.h"
#include "message_queue.h"

#include "file_record.h"


struct application
{
	char *baseFolder;
	v2i windowSize;
	v2i gameWindowSize;
	uint32_t currentTime;
	SDL_Window *window;
	SDL_GLContext glContext;

	int retinaRatio;
	int windowToBufferRatio;

	bool running;

	dds_message_queue msgQueue;
};

struct renderer
{
	uint32_t drawVBOs[64];
	int currentVBO;

	uint32_t baseVAO;

	uint8_t *vertexData;
	void *pointInStream;
	void *renderedToPoint;

	int windowViewport[4];
	int gameViewport[4];

	//these store information about what has happened as render commands have
	//been issued
	uint32_t currentFontColor;
	font *currentFont;
	font_size *currentFontSize;
	texture *recentlySetTexture;
	shader *recentlySetShader;
	text_alignment currentTextAlignment;

	//the following store the state of the rendering (changes will cause the 
	//render commands to flush)
	glm::mat4 MVP;
	shader *currentShader;
	framebuffer *currentFramebuffer;
	blend_mode currentBlendMode;
	int textureSlots[4];
	size_t currentVertexDataSize;
	int currentVertexCount;

	render_command renderCommands[10000];
	int currentCommand;
};

//There is a difference between the actual keyboard or gamepad key that
//is pressed and the game key it represents.  E.g. w -> up, s -> down,
//and it's useful to be able to remap those.  KeyPresses, KeyDown, KeyReleases
//are all game keys, and KeyboardKeys represents the current state of the actual
//keyboard.

//TODO: Implement a "GamepadKeys" equivalent to KeyboardKeys
global_variable bool			KeyPresses[KEY_COUNT];
global_variable bool			KeyDown[KEY_COUNT];
global_variable bool			KeyReleases[KEY_COUNT];

global_variable bool			KeyboardPresses[KB_COUNT];
global_variable bool			KeyboardDown[KB_COUNT];
global_variable bool			KeyboardReleases[KB_COUNT];

global_variable bool			CtrlHeld;
global_variable bool			ShiftHeld;
global_variable bool			AltHeld;

global_variable bool			PreviousKeyboardDown[KB_COUNT];

global_variable keyboard_keys	SDLKeyToKBEnum[300];
global_variable keyboard_keys	KeyMapping[KEY_COUNT];

//In the past, I've found it useful to have mouse coordinates for converted to the 
//game based on both the game MVP matrix and the gui MVP matrix.  I don't know if
//this is really the best way.

global_variable vec2			MousePos;
global_variable vec2			PreviousMousePos;
global_variable vec2			GuiMousePos;
global_variable vec2			PreviousGuiMousePos;

global_variable bool			MousePresses[3];
global_variable bool			MouseDown[3];
global_variable bool			MouseReleases[3];

global_variable bool			PreviousMouseDown[3];

//our game's orthographic projection matrix
//these get mouse coordinates automatically calculated
global_variable glm::mat4		GameVP;
global_variable glm::mat4		GameGuiVP;
global_variable glm::mat4		GameWindowProj;

global_variable glm::mat4		GuiVP;

global_variable bool			SendingGameUpdateEvents;


global_variable generic_hash	Shaders;
global_variable generic_hash	Textures;
global_variable sprite_pack		SpritePack;		//stores info about all the atlases
global_variable generic_hash	Sprites;		//essentially indicates a portion of a texture
global_variable generic_hash	TileSprites;
global_variable generic_hash	ColliderDescriptions;
global_variable generic_hash	BasicAnimations;
global_variable generic_hash	FrameBuffers;
global_variable generic_hash	Fonts;
global_variable generic_hash	Sounds;
global_variable memory_arena	Arena;
global_variable uid_map			QueuedSounds;

global_variable generic_hash	ComponentData;	//stores the json component information
global_variable generic_hash	PrefabData;		//stores the json prefab information
global_variable generic_hash	Levels;


global_variable bool			RecompileShaders;
global_variable bool			ReloadSounds;
global_variable char			SoundFileToReload[KILOBYTES(1)];
global_variable bool			ReloadTextures;
global_variable char			TextureFileToReload[KILOBYTES(1)];


/*glm::vec2 CoordConversion(glm::vec2 guiCoords, glm::mat4 *startMVP, glm::mat4 *endMVP)
{
	glm::vec4 start = glm::vec4(guiCoords.x, guiCoords.y, 0.0f, 1.0f);
	glm::vec4 end = glm::inverse(*endMVP) * (*startMVP * start);
	return vec2(end.x, end.y);
}*/

global_variable dir_folder ShaderFolder;
global_variable dir_folder ColliderDescriptionsFolder;
global_variable dir_folder TextureFolder;
global_variable dir_folder SoundFolder;
global_variable dir_folder FontFolder;
global_variable dir_folder ComponentFolder;
global_variable dir_folder BasicAnimationFolder;
global_variable dir_folder LevelFolder;
global_variable dir_folder PrefabFolder;

void ApplicationInit(const char *windowName, v2i gameBufferSize, int windowToBufferRatio);
void RendererInit();
void SetupGameData();
void SetupGameInput();
bool ProcessEvents();
void ShowFrame();
void ReloadDynamicData();
void ApplicationShutdown();
