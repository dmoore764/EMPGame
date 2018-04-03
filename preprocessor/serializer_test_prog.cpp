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

	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>

#else
	#include <GL/glew.h>
	// Windows OS header
	#include "Windows.h"
#endif

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../src/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../src/stb_truetype.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "memory.h"

#include "assert.h"

#define KILOBYTES(Number) (1024L*Number)
#define MEGABYTES(Number) (1024L*KILOBYTES(Number))
#define GIGABYTES(Number) (1024L*MEGABYTES(Number))

//static has 3 different meanings
#define global_variable static
#define internal_function static
#define local_persistent static


#include "../src/file_utilities.h"
#include "../src/debug_output.h"
#include "../src/memory_arena.h"
#include "../src/tokenizer.h"
#include "../src/hash.h"
#include "../src/directory_helper.h"
#include "../src/opengl.h"
#include "../src/asset_loading.h"
#include "../src/rectangle_packing.h"
#include "../src/fonts.h"
#include "../src/render_commands.h"

global_variable memory_arena Arena;

#include "../src/file_utilities.cpp"
#include "../src/debug_output.cpp"
#include "../src/memory_arena.cpp"
#include "../src/tokenizer.cpp"
#include "../src/hash.cpp"
#include "../src/directory_helper.cpp"

using namespace glm;
#include "auto_component_code.cpp"

int main (int argcount, char **args)
{
	InitArena(&Arena, GIGABYTES(1));

	Component c;
	c.type = COMP_SPRITE;
	c.sprite.textureName = (char *)"EnemyShip";
	c.sprite.color = 0xffffffff;
	c.sprite.offset = {10, 20};
	c.sprite.boundingRadius = 8;
	c.sprite.blendMode = BLEND_MODE_ADD;

	SerializeComponent(&c, "serialized_component.json");

	return 0;
}
