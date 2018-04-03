#define SHADER_COMPILE_OUTPUT

void CreateTexture(texture *t, uint8_t *data, bool clamp, bool nearest)
{
	glGenTextures(1, &t->gl_id);
	glBindTexture(GL_TEXTURE_2D, t->gl_id);

	if (clamp)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (nearest)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void CompileProgram(shader *s, const char *shader_name, const char *vert_src, const char *frag_src)
{
	if (!vert_src)
	{
		ERR("Vertex source not loaded for %s", shader_name);
		return;
	}
	if (!frag_src)
	{
		ERR("Fragment source not loaded for %s", shader_name);
		return;
	}

    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    
    GLint Result = GL_FALSE;
    int InfoLogLength;
    
    // Compile Fragment Shader
    glShaderSource(VertexShaderID, 1, &vert_src, NULL);
    glCompileShader(VertexShaderID);
    
#ifdef SHADER_COMPILE_OUTPUT
    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
		DBG("Vertex shader problem for: %s", shader_name);
		char *message = (char *)malloc(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, message);
		DBG("%s", message);
		free (message);
    }
#endif
    
    
    // Compile Fragment Shader
    glShaderSource(FragmentShaderID, 1, &frag_src, NULL);
    glCompileShader(FragmentShaderID);
    
#ifdef SHADER_COMPILE_OUTPUT
    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
		DBG("Fragment shader problem for: %s", shader_name);
		char *message = (char *)malloc(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, message);
		DBG("%s", message);
		free(message);
    }
#endif
    
    
    // Link the program
    s->gl_id = glCreateProgram();
    glAttachShader(s->gl_id, VertexShaderID);
    glAttachShader(s->gl_id, FragmentShaderID);
    glLinkProgram(s->gl_id);
    
#ifdef SHADER_COMPILE_OUTPUT
    // Check the program
    glGetProgramiv(s->gl_id, GL_LINK_STATUS, &Result);
    glGetProgramiv(s->gl_id, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
		DBG("Problem linking program: %s", shader_name);
		char *message = (char *)malloc(InfoLogLength + 1);
        glGetProgramInfoLog(s->gl_id, InfoLogLength, NULL, message);
		DBG("%s", message);
		free(message);
    }
#endif
    
    glDetachShader(s->gl_id, VertexShaderID);
    glDetachShader(s->gl_id, FragmentShaderID);
    
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
}

void CreateFrameBuffer(const char *name, int width, int height, bool depth_and_stencil, bool clamp, bool nearest)
{
	framebuffer *fb = (framebuffer *)GetFromHash(&FrameBuffers, name);
	if (fb)
	{
		ERR("Framebuffer '%s' already exists", name);
		return;
	}

	fb = PUSH_ITEM(&Arena, framebuffer);
	AddToHash(&FrameBuffers, fb, name);
	strcpy(fb->name, name);
	fb->tex.width = width;
	fb->tex.height = height;

	glGenFramebuffers(1, &fb->gl_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fb->gl_id);
    
	CreateTexture(&fb->tex, NULL, clamp, nearest);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->tex.gl_id, 0);
    
    if (depth_and_stencil)
    {
        glGenRenderbuffers(1, &fb->depth_and_stencil_gl_id);
        glBindRenderbuffer(GL_RENDERBUFFER, fb->depth_and_stencil_gl_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->depth_and_stencil_gl_id);
    }
    
    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Status != GL_FRAMEBUFFER_COMPLETE)
    {
		ERR("Frame Buffer Creation Failed.");
    }
    else
    {
		DBG("Frame Buffer Generated!");
    }
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CreateShadowBuffer(const char *name, int size)
{
	framebuffer *fb = (framebuffer *)GetFromHash(&FrameBuffers, name);
	if (fb)
	{
		ERR("Framebuffer '%s' already exists", name);
		return;
	}

	fb = PUSH_ITEM(&Arena, framebuffer);
	AddToHash(&FrameBuffers, fb, name);
	strcpy(fb->name, name);
	fb->tex.width = size;
	fb->tex.height =size;

	glGenFramebuffers(1, &fb->gl_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fb->gl_id);

	glGenTextures(1, &fb->tex.gl_id);
	glBindTexture(GL_TEXTURE_2D, fb->tex.gl_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->tex.gl_id, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void GetFrameBufferWindowProjection(char *name, glm::mat4 *Projection)
{
	framebuffer *fb = (framebuffer *)GetFromHash(&FrameBuffers, name);
	if (!fb)
	{
		*Projection = glm::mat4(1.0f);
		return;
	}

	*Projection = glm::ortho(0.0f, (float)fb->tex.width, 0.0f, (float)fb->tex.height, 150000.0f, -150000.0f);
}


texture LoadAndCreateTexture(char *name, char *imgFile)
{
	texture result;
	strcpy(result.name, name);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &result.gl_id);
	glBindTexture(GL_TEXTURE_2D, result.gl_id);
	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	//load the texture files
	image_file img = LoadImageFile(imgFile);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.w, img.h, 0, img.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, img.image_data);

	FreeImageFile(&img);

	return result;
}


texture CreateCubeMap(char *name, char *imagePosX, char *imageNegX, char *imagePosY, char *imageNegY, char *imagePosZ, char *imageNegZ)
{
	texture result;
	strcpy(result.name, name);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glGenTextures(1, &result.gl_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, result.gl_id);
	
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	//load the texture files
	image_file xPos = LoadImageFile(imagePosX);
	image_file xNeg = LoadImageFile(imageNegX);
	image_file yPos = LoadImageFile(imagePosY);
	image_file yNeg = LoadImageFile(imageNegY);
	image_file zPos = LoadImageFile(imagePosZ);
	image_file zNeg = LoadImageFile(imageNegZ);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, xPos.w, xPos.h, 0, xPos.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, xPos.image_data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, xNeg.w, xNeg.h, 0, xNeg.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, xNeg.image_data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, yPos.w, yPos.h, 0, yPos.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, yPos.image_data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, yNeg.w, yNeg.h, 0, yNeg.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, yNeg.image_data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, zPos.w, zPos.h, 0, zPos.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, zPos.image_data);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, zNeg.w, zNeg.h, 0, zNeg.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, zNeg.image_data);

	FreeImageFile(&xPos);
	FreeImageFile(&xNeg);
	FreeImageFile(&yPos);
	FreeImageFile(&yNeg);
	FreeImageFile(&zPos);
	FreeImageFile(&zNeg);

	return result;
}

struct _async_create_cube_map_data
{
	texture tex;
};

GENERIC_CALLBACK(AsyncFinishCubeMap)
{
	async_load_image_result *msgData = (async_load_image_result *)data;
	_async_create_cube_map_data *cubeData = (_async_create_cube_map_data *)msgData->callerData;

	texture tex = cubeData->tex;
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex.gl_id);
	image_file img = msgData->files[0];
	int sizeOfFaces = img.w / 4;

	int stride = img.w;
	int dataStride = stride*img.n;
	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, sizeOfFaces, sizeOfFaces, 0, img.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 
			img.image_data + (sizeOfFaces*img.n*2) + (sizeOfFaces*dataStride));
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, sizeOfFaces, sizeOfFaces, 0, img.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 
			img.image_data + (sizeOfFaces*dataStride));
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, sizeOfFaces, sizeOfFaces, 0, img.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 
			img.image_data + (sizeOfFaces*img.n));
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, sizeOfFaces, sizeOfFaces, 0, img.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 
			img.image_data + (sizeOfFaces*img.n) + (sizeOfFaces*2*dataStride));
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, sizeOfFaces, sizeOfFaces, 0, img.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 
			img.image_data + (sizeOfFaces*img.n) + (sizeOfFaces*dataStride));
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, sizeOfFaces, sizeOfFaces, 0, img.n == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 
			img.image_data + (sizeOfFaces*img.n*3) + (sizeOfFaces*dataStride));
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	FreeImageFile(&img);

	free(cubeData);
	free(msgData->files);
	free(data);
}

texture CreateCubeMap(char *name, char *imageAllInOne)
{
	texture result;
	strcpy(result.name, name);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glGenTextures(1, &result.gl_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, result.gl_id);
	
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	//load the texture files
	char *fileNames[1];
	fileNames[0] = imageAllInOne;
	_async_create_cube_map_data *data = (_async_create_cube_map_data *)malloc(sizeof(_async_create_cube_map_data));
	data->tex = result;
	AsyncLoadImageFiles(fileNames, 1, AsyncFinishCubeMap, data);

	return result;
}
