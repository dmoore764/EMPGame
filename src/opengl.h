struct vertex_attribute
{
	size_t single_unit_size;
	int number_of_units;
    GLenum type;
};

struct shader
{
	char name[64];
	bool uses_color;
	bool uses_uv;
	int number_of_tex_units;

	int number_of_attributes;
	vertex_attribute attributes[10];

	size_t vertex_stride;
	GLuint gl_id;
};

struct texture
{
	char name[64];
	GLuint gl_id;
	int width;
	int height;
};

struct framebuffer
{
	char name[64];
	GLuint gl_id;
	GLuint depth_and_stencil_gl_id;
	bool has_depth_and_stencil;
	texture tex;
};

void CreateTexture(texture *t, uint8_t *data, bool clamp, bool nearest);
void CompileProgram(shader *s, const char *shader_name, const char *vert_src, const char *frag_src);
void CreateFrameBuffer(const char *name, int width, int height, bool depth_and_stencil, bool clamp, bool nearest);
void CreateShadowBuffer(const char *name, int size);
void GetFrameBufferWindowProjection(char *name, glm::mat4 *Projection);
texture CreateCubeMap(char *name, char *imagePosX, char *imageNegX, char *imagePosY, char *imageNegY, char *imagePosZ, char *imageNegZ);
