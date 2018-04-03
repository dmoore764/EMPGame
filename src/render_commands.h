#pragma once

#include "sprites.h"
#include "collision.h"

enum render_command_type
{
	RC_SET_SHADER,
	RC_SET_TEXTURE,
	RC_SET_FRAMEBUFFER,
	RC_RESET_FRAMEBUFFER,
	RC_SET_VIEWPORT,
	RC_RESET_VIEWPORT,
	RC_ENABLE_SCISSOR,
	RC_DISABLE_SCISSOR,
	RC_ENABLE_DEPTH_TEST,
	RC_DISABLE_DEPTH_TEST,
	RC_SET_BLEND_MODE,
	RC_SET_UNIFORM,
	RC_CLEAR_SCREEN,
	RC_SET_VIEW,
	RC_SET_VIEW_FROM_MATRIX,
	RC_ADD_VERTICES,
	RC_DRAW_MESH,
};

enum uniform_type
{
    UNIFORM_BOOL,
    UNIFORM_FLOAT1,
    UNIFORM_INT1,
    UNIFORM_FLOAT2,
    UNIFORM_INT2,
    UNIFORM_FLOAT3,
    UNIFORM_INT3,
    UNIFORM_FLOAT4,
    UNIFORM_INT4,
	UNIFORM_MAT4,
};

enum blend_mode
{
	BLEND_MODE_ADD,
	BLEND_MODE_SUBTRACT,
	BLEND_MODE_MULTIPLY,
	BLEND_MODE_MIN,
	BLEND_MODE_MAX,
};

global_variable const char *BlendModeStrings[] = 
{
	"BLEND_MODE_ADD",
	"BLEND_MODE_SUBTRACT",
	"BLEND_MODE_MULTIPLY",
	"BLEND_MODE_MIN",
	"BLEND_MODE_MAX",
};

struct render_command
{
	render_command_type type;

	union
	{
		struct //set shader
		{
			shader *shdr;
		};

		struct //set texture
		{
			texture *tex;
			int tex_slot;
		};

		struct //set framebuffer
		{
			framebuffer *fb;
		};

		struct //set viewport
		{
			float viewport_x;
			float viewport_y;
			float viewport_w;
			float viewport_h;
		};

		struct //enable scissor
		{
			float scissorXL, scissorXR;
			float scissorYB, scissorYT;
		};

		struct //set blend mode
		{
			blend_mode blend;
		};

		struct //set uniform
		{
			char uniform_name[24];
			int count;
			uniform_type unitype;
			union
			{
				bool b;
				float f1;
				float f2[2];
				float f3[3];
				float f4[4];
				int i1;
				int i2[2];
				int i3[3];
				int i4[4];
				glm::mat4 *mat;
			};
		};

		struct //clear screen
		{
			float red;
			float green;
			float blue;
			float alpha;
			bool depth_clear;
		};

		struct //set view
		{
			float x;
			float y;
			float rotation;
			float zoom;
		};

		struct //set view from matrix
		{
			glm::mat4 *matrix;
		};

		struct //add vertices
		{
			size_t vertex_data_size;
			int num_vertices;
		};

		struct //draw mesh
		{
			int vao;
			int numVerts;
		};
	};
};

//Helpers
float GetWidthOfText(const char *line, ...);
float GetWidthOfTextGui(const char *line, ...);
glm::mat2 GetBoundingBoxOfText(float x, float y, const char *line, ...);
void GetSizeOfFrameBuffer(framebuffer *fb, int *width, int *height);

//Change state
void SetView(float x, float y, float zoom);
void SetViewFromMatrix(glm::mat4 *matrix);

void SetShader(const char *shader_name);
void SetTexture(texture *t, int slot = 0);
void SetTexture(const char *texture_name, int slot = 0);
void SetTextureFromFrameBuffer(const char *framebuffer_name);
void SetBlendMode(blend_mode mode);
void SetFont(const char *font_name, int pixel_size);
void SetFontAsPercentageOfScreen(const char *font_name, float size);
void SetFontColor(uint32_t color);
void SetTextAlignment(text_alignment align);
void SetFrameBuffer(const char *framebuffer_name);
void ResetFrameBuffer();
void SetViewPort(float x, float y, float w, float h);
void ResetViewPort();
void EnableScissor(float xl, float xr, float yb, float yt);
void DisableScissor();

void EnableDepthTest();
void DisableDepthTest();

//Adding to the VBO stream
void PushPosition(float x, float y, float z);
void PushColor(uint32_t c);
void PushUV(float u, float v);
void PushVertex(float *x, float *y, uint32_t *color, float *u, float *v);

//Drawing functions
void DrawQuad(uint32_t color, v2 p0, v2 p1, v2 p2, v2 p3);
void DrawQuad4Colors(uint32_t color0, uint32_t color1, uint32_t color2, uint32_t color3, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3);
void DrawCircle(uint32_t color, v2 p, float radius, int segments = 10);
void DrawLine(uint32_t color0, uint32_t color1, v2 p0, v2 p1, float width, float proportion_on_left = 0.5f);
void DrawAsymmetricWidthLine(uint32_t color0, uint32_t color1, glm::vec2 p0, glm::vec2 p1, float width0, float width1, float proportion_on_left = 0.5f);
void DrawCollisionShape(uint32_t color, CollisionObject *s, float width);

void Immediate3DLine(uint32_t color, v3 p1, v3 p2, float thickness);
void Immediate3DQuad(uint32_t color, v3 p1, v3 p2, v3 p3, v3 p4);
void ImmediateTextured3DQuad(uint32_t color, game_sprite *s, v3 p1, v3 p2, v3 p3, v3 p4);


//Textured things
void DrawTexture(uint32_t color, float x, float y, float x_scale, float y_scale);
inline void DrawTile(int tileX, int tileY, float u0, float u1, float v0, float v1, glm::ivec2 offset);
inline void DrawTileAtLocation(ivec2 location, float u0, float u1, float v0, float v1, glm::ivec2 offset);
void DrawFacingSprite3D(game_sprite *s, uint32_t color, v3 pos, v2 scale, v3 camPos);
void DrawBillboardSprite3D(game_sprite *s, uint32_t color, v3 pos, v2 scale, v3 camUp, v3 camRight);
void DrawSpriteSimple(game_sprite *s, uint32_t color, v2 pos, v2 scale);
void DrawSprite(game_sprite *s, uint32_t color, v2 pos, float rotation, v2 scale);
void DrawSprite(const char *sprite_name, uint32_t color, v2 pos, float rotation, v2 scale);

//Font drawing
void DrawText(float x, float y, const char *line, ...);
void DrawTextWrapped(float x, float y, float max_x, float line_height, const char *line, ...);

//x/y are a percentage of screen size (e.g. 50, 50 -> center of screen)
void DrawTextGui(float x, float y, const char *line, ...);
void DrawTextWrappedGui(float x, float y, float max_x, float line_height, const char *line, ...);


//3d drawing
void DrawMesh(int vao, int numVerts);


void ClearScreen(float r, float g, float b, float a, bool depth_clear);

//The two functions that actually perform opengl calls
void FlushBuffer();
void Process2DRenderList();

void Prepare2DRender();
