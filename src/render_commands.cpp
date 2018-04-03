float GetWidthOfText(const char *line, ...)
{
	float result = 0;

	if (Rend.currentFont == NULL || Rend.currentFontSize == NULL)
		return result;

	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, line);
	vsprintf(message, line, args);
	va_end(args);

	int chars_to_draw = utf8len(message);
	void *str_iter = message;
	for (int i = 0; i < chars_to_draw; i++)
	{
		int c;
		str_iter = utf8codepoint(str_iter, &c);
		font_character *fc = (font_character *)GetFromHash(&Rend.currentFontSize->chars, c);
		if (fc == NULL)
			fc = AddCharacterToFontSize(Rend.currentFontSize, c, false);

		int cNext;
		utf8codepoint(str_iter, &cNext);
		result += GetAdvanceForCharacter(Rend.currentFont, Rend.currentFontSize, fc, c, cNext);
	}
	return result;
}

float GetWidthOfTextGui(const char *line, ...)
{
	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, line);
	vsprintf(message, line, args);
	va_end(args);

	float result = GetWidthOfText(message);
	result *= 100.0f / Rend.gameViewport[2];
	return result;
}

glm::mat2 GetBoundingBoxOfText(float x, float y, const char *line, ...)
{
	glm::mat2 result = glm::mat2(0.0);
	float min_x = 10000000, min_y = 10000000, max_x = -10000000, max_y = -10000000;

	if (Rend.recentlySetTexture == NULL || Rend.currentFont == NULL || Rend.currentFontSize == NULL)
		return result;

	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, line);
	vsprintf(message, line, args);
	va_end(args);

	float cursor = x;

	int chars_to_draw = utf8len(message);
	void *str_iter = message;
	for (int i = 0; i < chars_to_draw; i++)
	{
		int c;
		str_iter = utf8codepoint(str_iter, &c);
		font_character *fc = (font_character *)GetFromHash(&Rend.currentFontSize->chars, c);
		if (fc == NULL)
			fc = AddCharacterToFontSize(Rend.currentFontSize, c, false);

		float px_l = floorf(cursor + fc->x0);
		float px_r = floorf(cursor + fc->x1);
		float py_t = floorf(y - fc->y0);
		float py_b = floorf(y - fc->y1);

		min_x = Min(min_x, Min(px_l, px_r));
		min_y = Min(min_y, Min(py_t, py_b));
		max_x = Max(max_x, Max(px_l, px_r));
		max_y = Max(max_y, Max(py_t, py_b));

		int cNext;
		utf8codepoint(str_iter, &cNext);
		cursor += GetAdvanceForCharacter(Rend.currentFont, Rend.currentFontSize, fc, c, cNext);
	}
	result[0].x = min_x;
	result[0].y = min_y;
	result[1].x = max_x;
	result[1].y = max_y;
	return result;
}

void GetSizeOfFrameBuffer(framebuffer *fb, int *width, int *height)
{
	if (fb)
	{
		*width = fb->tex.width;
		*height = fb->tex.height;
	}
	else
	{
		//back buffer size
		int vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		*width = vp[2];
		*height = vp[3];
	}
}

void SetView(float x, float y, float zoom)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_SET_VIEW;
	cmd->x = x;
	cmd->y = y;
	cmd->rotation = 0;
	cmd->zoom = zoom;
}

void SetViewFromMatrix(glm::mat4 *matrix)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_SET_VIEW_FROM_MATRIX;
	cmd->matrix = matrix;
}


void SetShader(const char *shader_name)
{
	shader *s = (shader *)GetFromHash(&Shaders, shader_name);
	if (s)
	{
		render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
		cmd->type = RC_SET_SHADER;
		cmd->shdr = s;
		Rend.recentlySetShader = s;
	}
	else
	{
		ERR("Attempted to set shader '%s' - shader not found", shader_name);
	}
}

void SetTexture(texture *t, int slot)
{
	//TODO: check if opengl texture is valid
	Rend.recentlySetTexture = t;
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_SET_TEXTURE;
	cmd->tex = t;
	cmd->tex_slot = 0;
}

void SetTexture(const char *texture_name, int slot)
{
	texture *t = (texture *)GetFromHash(&Textures, texture_name);
	if (t)
	{
		Rend.recentlySetTexture = t;
		render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
		cmd->type = RC_SET_TEXTURE;
		cmd->tex = t;
		cmd->tex_slot = 0;
	}
	else
	{
		ERR("Attempted to set texture '%s' - texture not found", texture_name);
	}
}

void SetTextureFromFrameBuffer(const char *framebuffer_name)
{
	framebuffer *fb = (framebuffer *)GetFromHash(&FrameBuffers, framebuffer_name);
	if (fb)
	{
		render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
		cmd->type = RC_SET_TEXTURE;
		cmd->tex = &fb->tex;
		cmd->tex_slot = 0;
		Rend.recentlySetTexture = &fb->tex;
	}
	else
	{
		ERR("Attempted to set tex from framebuffer '%s' - framebuffer not found", framebuffer_name);
	}
}


void SetBlendMode(blend_mode mode)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_SET_BLEND_MODE;
	cmd->blend = mode;
}

void SetFontAsPercentageOfScreen(const char *font_name, float size)
{
	SetFont(font_name, size * Rend.gameViewport[3] * 0.01f);
}

void SetFont(const char *font_name, int pixel_size)
{
	Rend.currentFont = NULL;
	Rend.currentFontSize = NULL;
	font *fnt = (font *)GetFromHash(&Fonts, font_name);
	if (fnt)
	{
		Rend.currentFont = fnt;
		font_size *fs = (font_size *)GetFromHash(&fnt->sizes, pixel_size);
		if (fs)
			Rend.currentFontSize = fs;
		else
		{
			//Render out that font size
			RenderSize(fnt, pixel_size, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.,/;:'\"[]{}\\|01234567890!@#$%^&*()-=_+ <>?");
			font_size *fs = (font_size *)GetFromHash(&fnt->sizes, pixel_size);
			if (fs)
				Rend.currentFontSize = fs;
		}
	}
	else
	{
		ERR("Did not find font '%s'", font_name);
	}
}

void SetFontColor(uint32_t color)
{
	Rend.currentFontColor = color;
}

void SetTextAlignment(text_alignment align)
{
	Rend.currentTextAlignment = align;
}

void SetFrameBuffer(const char *framebuffer_name)
{
	framebuffer *fb = (framebuffer *)GetFromHash(&FrameBuffers, framebuffer_name);
	if (fb)
	{
		render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
		cmd->type = RC_SET_FRAMEBUFFER;
		cmd->fb = fb;
	}
	else
	{
		ERR("Attempted to set framebuffer '%s' - framebuffer not found", framebuffer_name);
	}
}

void ResetFrameBuffer()
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_RESET_FRAMEBUFFER;
}

void SetViewPort(float x, float y, float w, float h)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_SET_VIEWPORT;
	cmd->viewport_x = x;
	cmd->viewport_y = y;
	cmd->viewport_w = w;
	cmd->viewport_h = h;
}

void ResetViewPort()
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_RESET_VIEWPORT;
}

void EnableScissor(float xl, float xr, float yb, float yt)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ENABLE_SCISSOR;
	cmd->scissorXL = xl;
	cmd->scissorXR = xr;
	cmd->scissorYB = yb;
	cmd->scissorYT = yt;
}

void DisableScissor()
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_DISABLE_SCISSOR;
}

void EnableDepthTest()
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ENABLE_DEPTH_TEST;
}

void DisableDepthTest()
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_DISABLE_DEPTH_TEST;
}




void PushPosition(float x, float y, float z)
{
	ADD_FLOAT_TO_STREAM(Rend.pointInStream, x);
	MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
	ADD_FLOAT_TO_STREAM(Rend.pointInStream, y);
	MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
	ADD_FLOAT_TO_STREAM(Rend.pointInStream, z);
	MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
}

void PushColor(uint32_t c)
{
	ADD_UINT32_TO_STREAM(Rend.pointInStream, c);
	MOVE_BUFFER_ONE_UINT32(Rend.pointInStream);
}

void PushUV(float u, float v)
{
	ADD_FLOAT_TO_STREAM(Rend.pointInStream, u);
	MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
	ADD_FLOAT_TO_STREAM(Rend.pointInStream, v);
	MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
}

void PushVertex(float *x, float *y, uint32_t *color, float *u, float *v)
{
	//Always has position attribute
	PushPosition(*x, *y, 0);

	if (Rend.recentlySetShader->uses_color)
	{
		if (color)
			PushColor(*color);
		else
			MOVE_BUFFER_ONE_UINT32(Rend.pointInStream);
	}

	if (Rend.recentlySetShader->uses_uv)
	{
		if (u && v)
			PushUV(*u, *v);
		else
		{
			MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
			MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
		}
	}
}


void PushVertex3D(float *x, float *y, float *z, uint32_t *color, float *u, float *v)
{
	//Always has position attribute
	PushPosition(*x, *y, *z);

	if (Rend.recentlySetShader->uses_color)
	{
		if (color)
			PushColor(*color);
		else
			MOVE_BUFFER_ONE_UINT32(Rend.pointInStream);
	}

	if (Rend.recentlySetShader->uses_uv)
	{
		if (u && v)
			PushUV(*u, *v);
		else
		{
			MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
			MOVE_BUFFER_ONE_FLOAT(Rend.pointInStream);
		}
	}
}



void DrawQuad(uint32_t color, v2 p0, v2 p1, v2 p2, v2 p3)
{
	assert(Rend.recentlySetShader);
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;

	PushVertex(&p0.x, &p0.y, &color, NULL, NULL);
	PushVertex(&p1.x, &p1.y, &color, NULL, NULL);
	PushVertex(&p3.x, &p3.y, &color, NULL, NULL);
	PushVertex(&p3.x, &p3.y, &color, NULL, NULL);
	PushVertex(&p1.x, &p1.y, &color, NULL, NULL);
	PushVertex(&p2.x, &p2.y, &color, NULL, NULL);
}

void DrawQuad4Colors(uint32_t color0, uint32_t color1, uint32_t color2, uint32_t color3, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
{
	assert(Rend.recentlySetShader);
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;

	PushVertex(&p0.x, &p0.y, &color0, NULL, NULL);
	PushVertex(&p1.x, &p1.y, &color1, NULL, NULL);
	PushVertex(&p3.x, &p3.y, &color3, NULL, NULL);
	PushVertex(&p3.x, &p3.y, &color3, NULL, NULL);
	PushVertex(&p1.x, &p1.y, &color1, NULL, NULL);
	PushVertex(&p2.x, &p2.y, &color2, NULL, NULL);
}

void DrawCircle(uint32_t color, v2 p, float radius, int segments)
{
	assert(Rend.recentlySetShader);

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = segments * 3 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = segments * 3;

	glm::vec3 outer_p = {0, -radius, 1};
	glm::mat3 identity = glm::mat3(1.0f);
	glm::mat3 transform = glm::rotate(identity, (360.0f / segments) * DEGREES_TO_RADIANS);
	glm::vec2 glmP = {p.x, p.y};

	for (int i = 0; i < segments; i++)
	{
		PushVertex(&p.x, &p.y, &color, NULL, NULL);
		glm::vec2 p1 = outer_p + glm::vec3(glmP, 1);
		PushVertex(&p1.x, &p1.y, &color, NULL, NULL);
		outer_p = transform*outer_p;
		p1 = outer_p + glm::vec3(glmP, 1);
		PushVertex(&p1.x, &p1.y, &color, NULL, NULL);
	}
}

void DrawLine(uint32_t color0, uint32_t color1, v2 p0, v2 p1, float width, float proportion_on_left)
{
	assert(Rend.recentlySetShader);

	v2 length = p1 - p0;
	v2 perp = {length.y, -length.x};
	perp = width * Normalize(perp);
	v2 pc0 = {p0.x - proportion_on_left * perp.x, p0.y - proportion_on_left * perp.y};
	v2 pc1 = pc0 + perp;
	v2 pc2 = pc1 + length;
	v2 pc3 = pc2 - perp;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;

	PushVertex(&pc0.x, &pc0.y, &color0, NULL, NULL);
	PushVertex(&pc1.x, &pc1.y, &color0, NULL, NULL);
	PushVertex(&pc3.x, &pc3.y, &color1, NULL, NULL);
	PushVertex(&pc3.x, &pc3.y, &color1, NULL, NULL);
	PushVertex(&pc1.x, &pc1.y, &color0, NULL, NULL);
	PushVertex(&pc2.x, &pc2.y, &color1, NULL, NULL);
}

void DrawAsymmetricWidthLine(uint32_t color0, uint32_t color1, glm::vec2 p0, glm::vec2 p1, float width0, float width1, float proportion_on_left)
{
	assert(Rend.recentlySetShader);

	glm::vec2 length = p1 - p0;
	glm::vec2 perp = {length.y, -length.x};
	perp = glm::normalize(perp);
	glm::vec2 perp0 = width0 * perp;
	glm::vec2 perp1 = width1 * perp;
	glm::vec2 pc0 = {p0.x - proportion_on_left * perp0.x, p0.y - proportion_on_left * perp0.y};
	glm::vec2 pc1 = pc0 + perp0;
	glm::vec2 pc3 = {p1.x - proportion_on_left * perp1.x, p1.y - proportion_on_left * perp1.y};
	glm::vec2 pc2 = pc3 + perp1;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;

	PushVertex(&pc0.x, &pc0.y, &color0, NULL, NULL);
	PushVertex(&pc1.x, &pc1.y, &color0, NULL, NULL);
	PushVertex(&pc3.x, &pc3.y, &color1, NULL, NULL);
	PushVertex(&pc3.x, &pc3.y, &color1, NULL, NULL);
	PushVertex(&pc1.x, &pc1.y, &color0, NULL, NULL);
	PushVertex(&pc2.x, &pc2.y, &color1, NULL, NULL);
}

void DrawCollisionShape(uint32_t color, CollisionObject *s, float width)
{
	switch (s->type)
	{
		case CollisionObject::POINT:
		{
			DrawCircle(color, {s->point.x, s->point.y}, width, 6);
		} break;

		case CollisionObject::CIRCLE:
		{
			collision_circle *c = &s->circle;
			glm::vec3 outer_p = glm::vec3(0, -c->radius, 1);
			glm::mat3 rotate = glm::rotate(glm::mat3(1.0f), (360 / 30.0f) * DEGREES_TO_RADIANS);
			for (int i = 0; i < 30; i++)
			{
				v2 outer_pv2 = {outer_p.x, outer_p.y};
				v2 center = {c->center.x, c->center.y};
				v2 p0 = outer_pv2 + center;
				outer_p = rotate*outer_p;
				outer_pv2 = {outer_p.x, outer_p.y};
				v2 p1 = outer_pv2 + center;

				DrawLine(color, color, p0, p1, width);
			}

		} break;

		case CollisionObject::POLYGON:
		{
			collision_polygon *p = &s->poly;
			for (int i = 0; i < p->numVerts; i++)
			{
				v2 p0 = {p->vertices[i].x, p->vertices[i].y};
				glm::vec2 p1glm = p->vertices[(i+1)%p->numVerts];
				v2 p1 = {p1glm.x, p1glm.y};
				DrawLine(color, color, p0, p1, width);
			}

		} break;

		case CollisionObject::AABB:
		{
			DrawLine(color, color, {s->aabb.min.x, s->aabb.min.y}, {s->aabb.max.x, s->aabb.min.y}, width);
			DrawLine(color, color, {s->aabb.max.x, s->aabb.min.y}, {s->aabb.max.x, s->aabb.max.y}, width);
			DrawLine(color, color, {s->aabb.max.x, s->aabb.max.y}, {s->aabb.min.x, s->aabb.max.y}, width);
			DrawLine(color, color, {s->aabb.min.x, s->aabb.max.y}, {s->aabb.min.x, s->aabb.min.y}, width);
		} break;
	}
}


void Immediate3DLine(uint32_t color, v3 p1, v3 p2, float thickness)
{
	assert(Rend.recentlySetShader);

	v3 dir = p2 - p1;
	v3 perp = Cross(dir, {0,0,1});
	if (Inner(perp,perp) == 0) // if we accidently chose a parallel line to cross
		perp = Cross(dir, {0,1,0});

	perp = (thickness * 0.5f) * Normalize(perp);
	v3 up = Cross(perp, dir);
	up = (thickness * 0.5f) * Normalize(up);

	v3 corner1 = p1 + perp + up;
	v3 corner2 = p1 - perp + up;
	v3 corner3 = p1 - perp - up;
	v3 corner4 = p1 + perp - up;

	v3 corner5 = p2 + perp + up;
	v3 corner6 = p2 - perp + up;
	v3 corner7 = p2 - perp - up;
	v3 corner8 = p2 + perp - up;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->num_vertices = 24;
	cmd->vertex_data_size = cmd->num_vertices * Rend.recentlySetShader->vertex_stride;

	//Top
	PushVertex3D(&corner1.x, &corner1.y, &corner1.z, &color, NULL, NULL);
	PushVertex3D(&corner5.x, &corner5.y, &corner5.z, &color, NULL, NULL);
	PushVertex3D(&corner6.x, &corner6.y, &corner6.z, &color, NULL, NULL);

	PushVertex3D(&corner1.x, &corner1.y, &corner1.z, &color, NULL, NULL);
	PushVertex3D(&corner6.x, &corner6.y, &corner6.z, &color, NULL, NULL);
	PushVertex3D(&corner2.x, &corner2.y, &corner2.z, &color, NULL, NULL);

	//Left
	PushVertex3D(&corner4.x, &corner4.y, &corner4.z, &color, NULL, NULL);
	PushVertex3D(&corner8.x, &corner8.y, &corner8.z, &color, NULL, NULL);
	PushVertex3D(&corner5.x, &corner5.y, &corner5.z, &color, NULL, NULL);

	PushVertex3D(&corner4.x, &corner4.y, &corner4.z, &color, NULL, NULL);
	PushVertex3D(&corner5.x, &corner5.y, &corner5.z, &color, NULL, NULL);
	PushVertex3D(&corner1.x, &corner1.y, &corner1.z, &color, NULL, NULL);

	//Bottom
	PushVertex3D(&corner3.x, &corner3.y, &corner3.z, &color, NULL, NULL);
	PushVertex3D(&corner7.x, &corner7.y, &corner7.z, &color, NULL, NULL);
	PushVertex3D(&corner8.x, &corner8.y, &corner8.z, &color, NULL, NULL);

	PushVertex3D(&corner3.x, &corner3.y, &corner3.z, &color, NULL, NULL);
	PushVertex3D(&corner8.x, &corner8.y, &corner8.z, &color, NULL, NULL);
	PushVertex3D(&corner4.x, &corner4.y, &corner4.z, &color, NULL, NULL);

	//Bottom
	PushVertex3D(&corner2.x, &corner2.y, &corner2.z, &color, NULL, NULL);
	PushVertex3D(&corner6.x, &corner6.y, &corner6.z, &color, NULL, NULL);
	PushVertex3D(&corner7.x, &corner7.y, &corner7.z, &color, NULL, NULL);

	PushVertex3D(&corner2.x, &corner2.y, &corner2.z, &color, NULL, NULL);
	PushVertex3D(&corner7.x, &corner7.y, &corner7.z, &color, NULL, NULL);
	PushVertex3D(&corner3.x, &corner3.y, &corner3.z, &color, NULL, NULL);
}

//assume clockwise winding
void Immediate3DQuad(uint32_t color, v3 p1, v3 p2, v3 p3, v3 p4)
{
	assert(Rend.recentlySetShader);

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->num_vertices = 6;
	cmd->vertex_data_size = cmd->num_vertices * Rend.recentlySetShader->vertex_stride;

	PushVertex3D(&p1.x, &p1.y, &p1.z, &color, NULL, NULL);
	PushVertex3D(&p4.x, &p4.y, &p4.z, &color, NULL, NULL);
	PushVertex3D(&p3.x, &p3.y, &p3.z, &color, NULL, NULL);

	PushVertex3D(&p1.x, &p1.y, &p1.z, &color, NULL, NULL);
	PushVertex3D(&p3.x, &p3.y, &p3.z, &color, NULL, NULL);
	PushVertex3D(&p2.x, &p2.y, &p2.z, &color, NULL, NULL);
}

void ImmediateTextured3DQuad(uint32_t color, game_sprite *s, v3 p1, v3 p2, v3 p3, v3 p4)
{
	assert(Rend.recentlySetShader);
	if (s)
	{
		sprite_atlas *atlas = &SpritePack.atlas[s->atlas_index];
		SetTexture(atlas->tex);
	}

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->num_vertices = 6;
	cmd->vertex_data_size = cmd->num_vertices * Rend.recentlySetShader->vertex_stride;

	float u0 = 0, u1 = 1, v0 = 1, v1 = 0;
	if (s)
	{
		u0 = s->u0;
		u1 = s->u1;
		v0 = s->v0;
		v1 = s->v1;

		PushVertex3D(&p4.x, &p4.y, &p4.z, &color, &u0, &v1);
		PushVertex3D(&p3.x, &p3.y, &p3.z, &color, &u0, &v0);
		PushVertex3D(&p1.x, &p1.y, &p1.z, &color, &u1, &v1);

		PushVertex3D(&p1.x, &p1.y, &p1.z, &color, &u1, &v1);
		PushVertex3D(&p3.x, &p3.y, &p3.z, &color, &u0, &v0);
		PushVertex3D(&p2.x, &p2.y, &p2.z, &color, &u1, &v0);
	}
	else
	{
		PushVertex3D(&p4.x, &p4.y, &p4.z, &color, &u0, &v0);
		PushVertex3D(&p3.x, &p3.y, &p3.z, &color, &u1, &v0);
		PushVertex3D(&p1.x, &p1.y, &p1.z, &color, &u0, &v1);

		PushVertex3D(&p1.x, &p1.y, &p1.z, &color, &u0, &v1);
		PushVertex3D(&p3.x, &p3.y, &p3.z, &color, &u1, &v0);
		PushVertex3D(&p2.x, &p2.y, &p2.z, &color, &u1, &v1);
	}
}


void DrawTexture(uint32_t color, float x, float y, float x_scale, float y_scale)
{
	assert(Rend.recentlySetShader);
	if (!Rend.recentlySetTexture)
		return;

	float half_w = Rend.recentlySetTexture->width * 0.5f * x_scale;
	float half_h = Rend.recentlySetTexture->height * 0.5f * y_scale;

	//TL
	glm::vec2 p0, p1, p2, p3;
	p0.x = -half_w + x;
	p0.y = -half_h + y;

	//TR
	p1.x = half_w + x;
	p1.y = -half_h + y;

	//BR
	p2.x = half_w + x;
	p2.y = half_h + y;

	//BL
	p3.x = -half_w + x;
	p3.y = half_h + y;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;
	float u0 = 0, u1 = 1, v0 = 0, v1 = 1;

	PushVertex(&p0.x, &p0.y, &color, &u0, &v0);
	PushVertex(&p1.x, &p1.y, &color, &u1, &v0);
	PushVertex(&p3.x, &p3.y, &color, &u0, &v1);
	PushVertex(&p3.x, &p3.y, &color, &u0, &v1);
	PushVertex(&p1.x, &p1.y, &color, &u1, &v0);
	PushVertex(&p2.x, &p2.y, &color, &u1, &v1);
}

inline void DrawTile(int tileX, int tileY, float u0, float u1, float v0, float v1, glm::ivec2 offset)
{
	float x0 = (tileX << 12) - offset.x;
	float x1 = ((tileX+1) << 12) - offset.x;
	float y0 = (tileY << 12) - offset.y;
	float y1 = ((tileY+1) << 12) - offset.y;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;
	uint32_t color = COL_WHITE;

	PushVertex(&x0, &y0, &color, &u0, &v0);
	PushVertex(&x1, &y0, &color, &u1, &v0);
	PushVertex(&x0, &y1, &color, &u0, &v1);
	PushVertex(&x0, &y1, &color, &u0, &v1);
	PushVertex(&x1, &y0, &color, &u1, &v0);
	PushVertex(&x1, &y1, &color, &u1, &v1);
}

inline void DrawTileAtLocation(ivec2 location, float u0, float u1, float v0, float v1, glm::ivec2 offset)
{
	float x0 = (location.x) - offset.x;
	float x1 = (location.x + (1<<12)) - offset.x;
	float y0 = (location.y) - offset.y;
	float y1 = (location.y + (1<<12)) - offset.y;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;
	uint32_t color = COL_WHITE;

	PushVertex(&x0, &y0, &color, &u0, &v0);
	PushVertex(&x1, &y0, &color, &u1, &v0);
	PushVertex(&x0, &y1, &color, &u0, &v1);
	PushVertex(&x0, &y1, &color, &u0, &v1);
	PushVertex(&x1, &y0, &color, &u1, &v0);
	PushVertex(&x1, &y1, &color, &u1, &v1);
}

void DrawFacingSprite3D(game_sprite *s, uint32_t color, v3 pos, v2 scale, v3 camPos)
{
	assert(Rend.recentlySetShader);
	sprite_atlas *atlas = &SpritePack.atlas[s->atlas_index];
	SetTexture(atlas->tex);

	//TL
	v2 p0 = {-s->origin.x * scale.x, (s->tex_rect->h - s->origin.y) * scale.y};

	//TR
	v2 p1 = {(s->tex_rect->w - s->origin.x) * scale.x, p0.y};

	//BR
	v2 p2 = {p1.x, -s->origin.y * scale.y};

	//BL
	v2 p3 = {p0.x, p2.y};

	v3 upVec = {0,0,1};
	v3 camToPos = pos - camPos;
	v3 camRight = {camToPos.y,-camToPos.x,0};
	camRight = Normalize(camRight);

	v3 p0_3d = p0.x * camRight + p0.y * upVec + pos;
	v3 p1_3d = p1.x * camRight + p1.y * upVec + pos;
	v3 p2_3d = p2.x * camRight + p2.y * upVec + pos;
	v3 p3_3d = p3.x * camRight + p3.y * upVec + pos;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;
	float u0 = s->u0, u1 = s->u1, v0 = s->v0, v1 = s->v1;

	PushVertex3D(&p0_3d.x, &p0_3d.y, &p0_3d.z, &color, &u0, &v0);
	PushVertex3D(&p2_3d.x, &p2_3d.y, &p2_3d.z, &color, &u1, &v1);
	PushVertex3D(&p1_3d.x, &p1_3d.y, &p1_3d.z, &color, &u1, &v0);
	PushVertex3D(&p0_3d.x, &p0_3d.y, &p0_3d.z, &color, &u0, &v0);
	PushVertex3D(&p3_3d.x, &p3_3d.y, &p3_3d.z, &color, &u0, &v1);
	PushVertex3D(&p2_3d.x, &p2_3d.y, &p2_3d.z, &color, &u1, &v1);
}

void DrawBillboardSprite3D(game_sprite *s, uint32_t color, v3 pos, v2 scale, v3 camUp, v3 camRight)
{
	assert(Rend.recentlySetShader);
	sprite_atlas *atlas = &SpritePack.atlas[s->atlas_index];
	SetTexture(atlas->tex);

	//TL
	v2 p0 = {-s->origin.x * scale.x, (s->tex_rect->h - s->origin.y) * scale.y};

	//TR
	v2 p1 = {(s->tex_rect->w - s->origin.x) * scale.x, p0.y};

	//BR
	v2 p2 = {p1.x, -s->origin.y * scale.y};

	//BL
	v2 p3 = {p0.x, p2.y};

	v3 p0_3d = p0.x * camRight + p0.y * camUp + pos;
	v3 p1_3d = p1.x * camRight + p1.y * camUp + pos;
	v3 p2_3d = p2.x * camRight + p2.y * camUp + pos;
	v3 p3_3d = p3.x * camRight + p3.y * camUp + pos;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;
	float u0 = s->u0, u1 = s->u1, v0 = s->v0, v1 = s->v1;

	PushVertex3D(&p0_3d.x, &p0_3d.y, &p0_3d.z, &color, &u0, &v0);
	PushVertex3D(&p2_3d.x, &p2_3d.y, &p2_3d.z, &color, &u1, &v1);
	PushVertex3D(&p1_3d.x, &p1_3d.y, &p1_3d.z, &color, &u1, &v0);
	PushVertex3D(&p0_3d.x, &p0_3d.y, &p0_3d.z, &color, &u0, &v0);
	PushVertex3D(&p3_3d.x, &p3_3d.y, &p3_3d.z, &color, &u0, &v1);
	PushVertex3D(&p2_3d.x, &p2_3d.y, &p2_3d.z, &color, &u1, &v1);
}

void DrawSpriteSimple(game_sprite *s, uint32_t color, v2 pos, v2 scale)
{
	assert(Rend.recentlySetShader);
	//TL
	v2 p0 = {-s->origin.x * scale.x + pos.x, (s->tex_rect->h - s->origin.y) * scale.y + pos.y};

	//TR
	v2 p1 = {(s->tex_rect->w - s->origin.x) * scale.x + pos.x, p0.y};

	//BR
	v2 p2 = {p1.x, -s->origin.y * scale.y + pos.y};

	//BL
	v2 p3 = {p0.x, p2.y};

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;
	float u0 = s->u0, u1 = s->u1, v0 = s->v0, v1 = s->v1;

	PushVertex(&p0.x, &p0.y, &color, &u0, &v0);
	PushVertex(&p3.x, &p3.y, &color, &u0, &v1);
	PushVertex(&p1.x, &p1.y, &color, &u1, &v0);
	PushVertex(&p3.x, &p3.y, &color, &u0, &v1);
	PushVertex(&p2.x, &p2.y, &color, &u1, &v1);
	PushVertex(&p1.x, &p1.y, &color, &u1, &v0);
}

void DrawSprite(game_sprite *s, uint32_t color, v2 pos, float rotation, v2 scale)
{
	assert(Rend.recentlySetShader);
	sprite_atlas *atlas = &SpritePack.atlas[s->atlas_index];
	SetTexture(atlas->tex);
	rotation = DEGREES_TO_RADIANS * rotation;

	//TL
	glm::vec3 p0 = {-s->origin.x * scale.x, (s->tex_rect->h - s->origin.y) * scale.y, 1};

	//TR
	glm::vec3 p1 = {(s->tex_rect->w - s->origin.x) * scale.x, p0.y, 1};

	//BR
	glm::vec3 p2 = {p1.x, -s->origin.y * scale.y, 1};

	//BL
	glm::vec3 p3 = {p0.x, p2.y, 1};

	glm::mat3 identity = glm::mat3(1.0f);
	glm::vec2 glmPos = {pos.x, pos.y};
	glm::mat3 translate = glm::translate(identity, glmPos);
	glm::mat3 final_transform = glm::rotate(translate, rotation);

	p0 = final_transform*p0;
	p1 = final_transform*p1;
	p2 = final_transform*p2;
	p3 = final_transform*p3;

	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_ADD_VERTICES;
	cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
	cmd->num_vertices = 6;
	float u0 = s->u0, u1 = s->u1, v0 = s->v0, v1 = s->v1;

	PushVertex(&p0.x, &p0.y, &color, &u0, &v0);
	PushVertex(&p1.x, &p1.y, &color, &u1, &v0);
	PushVertex(&p3.x, &p3.y, &color, &u0, &v1);
	PushVertex(&p3.x, &p3.y, &color, &u0, &v1);
	PushVertex(&p1.x, &p1.y, &color, &u1, &v0);
	PushVertex(&p2.x, &p2.y, &color, &u1, &v1);
}

void DrawSprite(const char *sprite_name, uint32_t color, v2 pos, float rotation, v2 scale)
{
	game_sprite *s = (game_sprite *)GetFromHash(&Sprites, sprite_name);
	if (!s)
	{
		ERR("Couldn't find sprite with name '%s'", sprite_name);
		return;
	}
	DrawSprite(s, color, pos, rotation, scale);
}

namespace _Rendering
{

void DrawTextConvertedSize(float x, float y, float w_conversion, float h_conversion, char *message)
{
	assert(Rend.recentlySetShader);
	if (Rend.currentFont == NULL || Rend.currentFontSize == NULL)
		return;

	font_size *fs = Rend.currentFontSize;
	font *f = Rend.currentFont;
	
	switch (Rend.currentTextAlignment)
	{
		case ALIGN_LEFT: {} break;
		case ALIGN_RIGHT:
		{
			float line_width = GetWidthOfText(message);
			x -= line_width * w_conversion;
		} break;
		case ALIGN_CENTER:
		{
			float line_width = GetWidthOfText(message);
			x -= (line_width * 0.5f) * w_conversion;
		} break;
	}

	int chars_to_draw = utf8len(message);
	for (int atlas_index = 0; atlas_index < fs->current_atlas + 1; atlas_index++)
	{
		float cursor = x;

		texture *t = fs->tex[atlas_index];
		Rend.recentlySetTexture = t;
		render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
		cmd->type = RC_SET_TEXTURE;
		cmd->tex = t;
		cmd->tex_slot = 0;

		void *str_iter = message;

		for (int i = 0; i < chars_to_draw; i++)
		{

			int c;
			str_iter = utf8codepoint(str_iter, &c);
			font_character *fc = (font_character *)GetFromHash(&Rend.currentFontSize->chars, c);
			if (fc == NULL)
				fc = AddCharacterToFontSize(fs, c, false);

			if (fc->atlas_index == atlas_index)
			{
				render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
				cmd->type = RC_ADD_VERTICES;
				cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
				cmd->num_vertices = 6;

				float px_l = (cursor + (float)fc->x0 * w_conversion);
				float px_r = (cursor + (float)fc->x1 * w_conversion);
				float py_t = (y - (float)fc->y0 * h_conversion);
				float py_b = (y - (float)fc->y1 * h_conversion);
				float u0 = fc->u0, u1 = fc->u1, v0 = fc->v0, v1 = fc->v1;
				uint32_t color = Rend.currentFontColor;

				PushVertex(&px_l, &py_t, &color, &u0, &v0);
				PushVertex(&px_r, &py_t, &color, &u1, &v0);
				PushVertex(&px_l, &py_b, &color, &u0, &v1);
				PushVertex(&px_l, &py_b, &color, &u0, &v1);
				PushVertex(&px_r, &py_t, &color, &u1, &v0);
				PushVertex(&px_r, &py_b, &color, &u1, &v1);
			}

			int cNext;
			utf8codepoint(str_iter, &cNext);
			cursor += (GetAdvanceForCharacter(Rend.currentFont, Rend.currentFontSize, fc, c, cNext) * w_conversion);
		}
	}

	for (int i = 0; i < ArrayCount(fs->atlas); i++)
	{
		if (fs->atlas_tex_updated[i])
		{
			glBindTexture(GL_TEXTURE_2D, fs->tex[i]->gl_id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fs->tex[i]->width, fs->tex[i]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fs->rgba_tex_data[i]);
			fs->atlas_tex_updated[i] = false;
		}
	}
}


void DrawTextWrappedConvertedSize(float x, float y, float max_x, float line_height, float w_conversion, float h_conversion, char *message)
{
	if (Rend.recentlySetTexture == NULL || Rend.currentFont == NULL || Rend.currentFontSize == NULL)
		return;

	float char_dimensions[KILOBYTES(2)];
	int num_potential_line_break_positions = 0;
	void *potential_line_break_positions[KILOBYTES(2)];
	float test_cursor_x;

	int chars_to_draw = utf8len(message);
	void *str_iter = message;

	//Get the horizontal dimensions for each character, and also find indexes of potential line breaks
	for (int i = 0; i < chars_to_draw; i++)
	{
		int c;
		void *prev_str_iter = str_iter;
		str_iter = utf8codepoint(str_iter, &c);
		//if we find white space...
		if (c == ' ' || c == '\t')
		{
			//go to the next non-white space character
			while (i < chars_to_draw && (c == ' ' || c == '\t'))
			{
				int cNext;
				utf8codepoint(str_iter, &cNext);
				char_dimensions[i] = (GetAdvanceForCharacter(Rend.currentFont, Rend.currentFontSize, (font_character *)GetFromHash(&Rend.currentFontSize->chars, c), c, cNext) * w_conversion);
				i++;
				prev_str_iter = str_iter;
				str_iter = utf8codepoint(str_iter, &c);
			}
			//if there was one, then that will be the potential start of a new line
			if (i < chars_to_draw)
			{
				potential_line_break_positions[num_potential_line_break_positions++] = prev_str_iter;
			}
		}
		else if (c == '\n')
		{
			potential_line_break_positions[num_potential_line_break_positions++] = prev_str_iter;
		}

		int cNext;
		utf8codepoint(str_iter, &cNext);
		char_dimensions[i] = (GetAdvanceForCharacter(Rend.currentFont, Rend.currentFontSize, (font_character *)GetFromHash(&Rend.currentFontSize->chars, c), c, cNext) * w_conversion);
	}

	//Get number of characters to draw on this line:
	// - keep adding characters until we are past the cutoff width
	// - back up until we are at the previous line break pos
	// - draw a line of text
	//
	// - unless there isn't a previous, then keep going until we get to the next one
	// - then draw a line of text
	//
	// - at the end, draw a line of text

	void *beginning_pos_in_string = message;
	void *pos_in_string = message;
	float cursor = x;
	void *end_of_string = (uint8_t*)message + strlen(message);
	int pos_in_string_index = 0;
	while (pos_in_string < end_of_string)
	{
		cursor += char_dimensions[pos_in_string_index++];
		int c;
		pos_in_string = utf8codepoint(pos_in_string, &c);
		utf8codepoint(pos_in_string, &c);
		if (cursor > max_x || c == '\n')
		{
			//find the line break just previous to our pos_in_string
			void *line_break_pos;
			switch (Rend.currentFont->language)
			{
				case font::ENGLISH:
				{
					line_break_pos = end_of_string;
					for (int i = 1; i < num_potential_line_break_positions; i++)
					{
						if (potential_line_break_positions[i] > pos_in_string && //line break immediately after our position, then use line break before
								potential_line_break_positions[i-1] > beginning_pos_in_string) //unless that is where we started
						{
							line_break_pos = potential_line_break_positions[(i - 1)];
							break;
						}
					}
				} break;

				case font::JAPANESE:
				{
					line_break_pos = pos_in_string;	
					//go back one character
					line_break_pos = (uint8_t *)line_break_pos - 1;
					while (utf8valid(line_break_pos) != 0) //0 means valid???
					{
						line_break_pos = (uint8_t *)line_break_pos - 1;
					}

					//Don't back up a character if we only had one character or it was a linebreak
					if (line_break_pos <= beginning_pos_in_string || c == '\n')
						line_break_pos = pos_in_string;
				} break;
			}

			//check if there is one last linebreak
			if (line_break_pos == end_of_string && potential_line_break_positions[num_potential_line_break_positions - 1] > beginning_pos_in_string)
			{
				line_break_pos = potential_line_break_positions[num_potential_line_break_positions - 1];
			}

			if (line_break_pos > pos_in_string)
			{
				//go to the next line break (or until the end)
				while (pos_in_string < line_break_pos)
				{
					pos_in_string = utf8codepoint(pos_in_string, &c);
					cursor += char_dimensions[pos_in_string_index++];
				}
			}
			else
			{
				//rewind to that position
				while (pos_in_string > line_break_pos)
				{
					pos_in_string = (uint8_t *)pos_in_string - 1;
					while (utf8valid(pos_in_string) != 0) //0 means valid???
					{
						pos_in_string = (uint8_t *)pos_in_string - 1;
					}

					cursor -= char_dimensions[pos_in_string_index--];
				}
			}
			assert (pos_in_string > beginning_pos_in_string);

			//Draw a line of text, move down to the next line
			char line[KILOBYTES(1)];
			sprintf(line, "%.*s", (uint8_t *)pos_in_string - (uint8_t *)beginning_pos_in_string, (char *)beginning_pos_in_string);
			_Rendering::DrawTextConvertedSize(x, y, w_conversion, h_conversion, line);
			y -= line_height;
			cursor = x;
			beginning_pos_in_string = pos_in_string;
		}
	}

	//Draw the last (or possibly only) line of text
	if ((uint8_t *)pos_in_string - (uint8_t *)beginning_pos_in_string > 0)
	{
		char line[KILOBYTES(1)];
		sprintf(line, "%.*s", (uint8_t *)pos_in_string - (uint8_t *)beginning_pos_in_string, (char *)beginning_pos_in_string);
		_Rendering::DrawTextConvertedSize(x, y, w_conversion, h_conversion, line);
	}
}


}

void DrawText(float x, float y, const char *line, ...)
{
	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, line);
	vsprintf(message, line, args);
	va_end(args);
	_Rendering::DrawTextConvertedSize(x, y, 1, 1, message);
}

void DrawTextGui(float x, float y, const char *line, ...)
{
	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, line);
	vsprintf(message, line, args);
	va_end(args);
	_Rendering::DrawTextConvertedSize(x, y, 100.0f / Rend.gameViewport[2], 100.0f / Rend.gameViewport[3], message);
}

void DrawTextWrapped(float x, float y, float max_x, float line_height, const char *line, ...)
{
	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, line);
	vsprintf(message, line, args);
	va_end(args);
	_Rendering::DrawTextWrappedConvertedSize(x, y, max_x, line_height, 1, 1, message);
}

void DrawTextWrappedGui(float x, float y, float max_x, float line_height, const char *line, ...)
{
	char message[KILOBYTES(1)];
	va_list args;
	va_start(args, line);
	vsprintf(message, line, args);
	va_end(args);
	_Rendering::DrawTextWrappedConvertedSize(x, y, max_x, line_height, 100.0f / Rend.gameViewport[2], 100.0f / Rend.gameViewport[3], message);
}


void DrawMesh(int vao, int numVerts)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_DRAW_MESH;
	cmd->vao = vao;
	cmd->numVerts = numVerts;
}


void ClearScreen(float r, float g, float b, float a, bool depth_clear)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_CLEAR_SCREEN;
	cmd->red = r;
	cmd->green = g;
	cmd->blue = b;
	cmd->alpha = a;
	cmd->depth_clear = depth_clear;
}


void Flush3D(int vao, int numVerts)
{
	GLint currentVao;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);
	glBindVertexArray(vao);

	for (int i = 0; i < Rend.currentShader->number_of_tex_units; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i); 
		glBindTexture(GL_TEXTURE_2D, Rend.textureSlots[i]);
	}
	glActiveTexture(GL_TEXTURE0);

	glUniformMatrix4fv(glGetUniformLocation(Rend.currentShader->gl_id, "MVP"), 1, false, glm::value_ptr(Rend.MVP));

    glDrawArrays(GL_TRIANGLES, 0, numVerts*3);
    
	glBindVertexArray(currentVao);
}


void FlushBuffer()
{
	glBindBuffer(GL_ARRAY_BUFFER, Rend.drawVBOs[Rend.currentVBO]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, Rend.currentVertexDataSize, Rend.renderedToPoint);

	for (int i = 0; i < Rend.currentShader->number_of_tex_units; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i); 
		glBindTexture(GL_TEXTURE_2D, Rend.textureSlots[i]);
	}
	glActiveTexture(GL_TEXTURE0);

	glUniformMatrix4fv(glGetUniformLocation(Rend.currentShader->gl_id, "MVP"), 1, false, glm::value_ptr(Rend.MVP));
	size_t current_offset = 0;
	for (int i = 0; i < Rend.currentShader->number_of_attributes; i++)
	{
		vertex_attribute *attr = &Rend.currentShader->attributes[i];
		glVertexAttribPointer(i, attr->number_of_units, attr->type, GL_FALSE, Rend.currentShader->vertex_stride, (void*)current_offset);
		glEnableVertexAttribArray(i);
		current_offset += (attr->number_of_units * attr->single_unit_size);
	}

    glDrawArrays(GL_TRIANGLES, 0, Rend.currentVertexCount);
    
    for (int i = 0; i < Rend.currentShader->number_of_attributes; i++)
        glDisableVertexAttribArray(i);
    
	Rend.currentVBO = (Rend.currentVBO + 1) % ArrayCount(Rend.drawVBOs);
	Rend.renderedToPoint = (void *)((uint8_t *)Rend.renderedToPoint + Rend.currentVertexDataSize);
	Rend.currentVertexDataSize = 0;
	Rend.currentVertexCount = 0;
}

void Prepare2DRender()
{
	//Prepare to build render command list.
	//This must happen before any rendering for this frame, otherwise
	//stuff from the previous frame may affect this frame.
	Rend.currentCommand = 0;
	Rend.pointInStream = (void *)Rend.vertexData;
	Rend.currentFont = NULL;
	Rend.currentFontSize = NULL;
	Rend.currentFontColor = COL_WHITE;
	Rend.recentlySetTexture = NULL;
	Rend.recentlySetShader = NULL;
	Rend.currentTextAlignment = ALIGN_LEFT;
}

void Process2DRenderList()
{
	Rend.renderedToPoint = Rend.vertexData;
	Rend.currentShader = NULL;
	Rend.currentFramebuffer = NULL;
	Rend.currentVertexDataSize = 0;
	Rend.currentVertexCount = 0;
	Rend.textureSlots[0] = -1;
	Rend.textureSlots[1] = -1;
	Rend.textureSlots[2] = -1;
	Rend.textureSlots[3] = -1;

	for (int i = 0; i < Rend.currentCommand; i++)
	{
		render_command *cmd = &Rend.renderCommands[i];
		switch (cmd->type)
		{
			case RC_SET_SHADER:
			{
				if (Rend.currentShader != cmd->shdr)
				{
					if (Rend.currentVertexCount > 0)
						FlushBuffer();

					Rend.currentShader = cmd->shdr;
					glUseProgram(Rend.currentShader->gl_id);
				}
			} break;

			case RC_SET_TEXTURE:
			{
				assert(cmd->tex_slot >= 0 && cmd->tex_slot < ArrayCount(Rend.textureSlots));
				if (Rend.textureSlots[cmd->tex_slot] != cmd->tex->gl_id && Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				Rend.textureSlots[cmd->tex_slot] = cmd->tex->gl_id;
			} break;

			case RC_SET_FRAMEBUFFER:
			{
				if (Rend.currentFramebuffer != cmd->fb)
				{
					if (Rend.currentVertexCount > 0)
					{
						FlushBuffer();
					}
					Rend.currentFramebuffer = cmd->fb;
					glBindFramebuffer(GL_FRAMEBUFFER, Rend.currentFramebuffer->gl_id);
					glViewport(0,0,Rend.currentFramebuffer->tex.width, Rend.currentFramebuffer->tex.height);
				}
			} break;

			case RC_RESET_FRAMEBUFFER:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				Rend.currentFramebuffer = NULL;
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(Rend.gameViewport[0], Rend.gameViewport[1], Rend.gameViewport[2], Rend.gameViewport[3]);
			} break;

			case RC_SET_VIEWPORT:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				glViewport(cmd->viewport_x, cmd->viewport_y, cmd->viewport_w, cmd->viewport_h);
				glEnable(GL_SCISSOR_TEST);
				glScissor(cmd->viewport_x, cmd->viewport_y, cmd->viewport_w, cmd->viewport_h);
			} break;

			case RC_RESET_VIEWPORT:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				if (Rend.currentFramebuffer != NULL)
					glViewport(0,0,Rend.currentFramebuffer->tex.width, Rend.currentFramebuffer->tex.height);
				else
					glViewport(Rend.gameViewport[0], Rend.gameViewport[1], Rend.gameViewport[2], Rend.gameViewport[3]);
				glDisable(GL_SCISSOR_TEST);

			} break;

			case RC_ENABLE_SCISSOR:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				glEnable(GL_SCISSOR_TEST);
				glScissor(cmd->scissorXL, cmd->scissorYB, cmd->scissorXR - cmd->scissorXL, cmd->scissorYT - cmd->scissorYB);
			} break;

			case RC_DISABLE_SCISSOR:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				glDisable(GL_SCISSOR_TEST);
			} break;

			case RC_ENABLE_DEPTH_TEST:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				glEnable(GL_DEPTH_TEST);
			} break;

			case RC_DISABLE_DEPTH_TEST:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				glDisable(GL_DEPTH_TEST);
			} break;

			case RC_SET_BLEND_MODE:
			{
				if (cmd->blend != Rend.currentBlendMode)
				{
					FlushBuffer();
				}
				Rend.currentBlendMode = cmd->blend;
				//TODO: set the glBlendFunc and glBlendEquation
			} break;

			case RC_CLEAR_SCREEN:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				glClearColor(cmd->red, cmd->green, cmd->blue, cmd->alpha);
				GLbitfield bits_to_clear = GL_COLOR_BUFFER_BIT;
				if (cmd->depth_clear)
					bits_to_clear |= GL_DEPTH_BUFFER_BIT;
				glClear(bits_to_clear);
			} break;

			case RC_SET_VIEW:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				int current_buffer_width, current_buffer_height;
				GetSizeOfFrameBuffer(Rend.currentFramebuffer, &current_buffer_width, &current_buffer_height);
				float one_over_zoom = 1 / cmd->zoom;
				Rend.MVP = glm::ortho(-current_buffer_width * 0.5f * one_over_zoom + cmd->x, current_buffer_width * 0.5f * one_over_zoom + cmd->x, -current_buffer_height * 0.5f * one_over_zoom + cmd->y, current_buffer_height * 0.5f * one_over_zoom + cmd->y, 150000.0f, -150000.0f);
			} break;

			case RC_SET_VIEW_FROM_MATRIX:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				Rend.MVP = *cmd->matrix;
			} break;

			case RC_SET_UNIFORM:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				if (Rend.currentShader)
				{
					int uniform_loc = glGetUniformLocation(Rend.currentShader->gl_id, cmd->uniform_name);
					switch(cmd->unitype)
					{
						case UNIFORM_BOOL:
							glUniform1i(uniform_loc, (int)cmd->b);
							break;
						case UNIFORM_INT1:
							glUniform1iv(uniform_loc, 1, &cmd->i1);
							break;
						case UNIFORM_INT2:
							glUniform2iv(uniform_loc, cmd->count, cmd->i2);
							break;
						case UNIFORM_INT3:
							glUniform3iv(uniform_loc, cmd->count, cmd->i3);
							break;
						case UNIFORM_INT4:
							glUniform4iv(uniform_loc, cmd->count, cmd->i4);
							break;
						case UNIFORM_FLOAT1:
							glUniform1fv(uniform_loc, 1, &cmd->f1);
							break;
						case UNIFORM_FLOAT2:
							glUniform2fv(uniform_loc, cmd->count, cmd->f2);
							break;
						case UNIFORM_FLOAT3:
							glUniform3fv(uniform_loc, cmd->count, cmd->f3);
							break;
						case UNIFORM_FLOAT4:
							glUniform4fv(uniform_loc, cmd->count, cmd->f4);
							break;
						case UNIFORM_MAT4:
							glUniformMatrix4fv(uniform_loc, cmd->count, GL_FALSE, glm::value_ptr(cmd->mat[0]));
							break;
					}
				}
			} break;

			case RC_ADD_VERTICES:
			{
				if (Rend.currentVertexDataSize > KILOBYTES(490))
				{
					FlushBuffer();
				}
				Rend.currentVertexCount += cmd->num_vertices;
				Rend.currentVertexDataSize += cmd->vertex_data_size;
			} break;


			case RC_DRAW_MESH:
			{
				if (Rend.currentVertexCount > 0)
				{
					FlushBuffer();
				}
				Flush3D(cmd->vao, cmd->numVerts);

			} break;
		}
	}

	if (Rend.currentVertexCount > 0)
		FlushBuffer();
}
