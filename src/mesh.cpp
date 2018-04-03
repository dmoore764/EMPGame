struct basic_mesh
{
	int vaoHandle;
	int bufferHandle;
	int numVerts;
};

struct basic_mesh_vertex_format
{
	v3 position;
	v3 normal;
	v2 uv;
	uint32_t color;
};

basic_mesh BasicMeshGetFromColladaByName(collada_file *file, const char *name, game_sprite *spr = NULL)
{
	basic_mesh result;
	collada_node *instance = ColladaGetSceneObjectByName(&file->visualScenes[0], name);
	assert(instance);
	assert(instance->contentType == COLLADA_INSTANCE_GEOMETRY);

	collada_geometry *geometry = NULL;
	for (int i = 0; i < file->numGeometries; i++)
	{
		if (strcmp(file->geometries[i].id, instance->instanceGeometry.meshURL) == 0)
		{
			geometry = &file->geometries[i];
			break;
		}
	}
	assert(geometry);

	basic_mesh_vertex_format *verts = PUSH_ARRAY(&Arena, basic_mesh_vertex_format, geometry->numPolys * 3);

	int currentVert = 0;
	for (int i = 0; i < geometry->numPolys; i++)
	{
		for (int v = 0; v < 3; v++)
		{
			basic_mesh_vertex_format *cur = &verts[currentVert++];
			cur->color = MAKE_COLOR(255, 255, 255, 255);
			for (int s = 0; s < geometry->numSemantics; s++)
			{
				if (geometry->semantics[s].semantic[0] == 'V')
				{
					int index = geometry->polys[i].vert[v].position_index;
					collada_source_array *source = &geometry->sources[geometry->semantics[s].source_index];
					cur->position.x = source->values[index*source->stride + 0];
					cur->position.y = source->values[index*source->stride + 1];
					cur->position.z = source->values[index*source->stride + 2];
				}
				else if (geometry->semantics[s].semantic[0] == 'N') //Normal
				{
					int index = geometry->polys[i].vert[v].normal_index;
					collada_source_array *source = &geometry->sources[geometry->semantics[s].source_index];
					cur->normal.x = source->values[index*source->stride + 0];
					cur->normal.y = source->values[index*source->stride + 1];
					cur->normal.z = source->values[index*source->stride + 2];
				}
				else if (geometry->semantics[s].semantic[0] == 'T') //Texcoord
				{
					if (geometry->semantics[s].set == 0)
					{
						int index = geometry->polys[i].vert[v].map_index[0];
						collada_source_array *source = &geometry->sources[geometry->semantics[s].source_index];
						cur->uv.x = source->values[index*source->stride + 0];
						cur->uv.y = 1 - source->values[index*source->stride + 1];

						if (spr)
						{
							float newUVx = (spr->u1 - spr->u0)*cur->uv.x + spr->u0;
							float newUVy = (spr->v1 - spr->v0)*cur->uv.y + spr->v0;
							cur->uv.x = newUVx;
							cur->uv.y = newUVy;
						}
					}
				}
			}
		}
	}

	GLint currentVao;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);

	uint32_t vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(basic_mesh_vertex_format)*currentVert, verts, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(basic_mesh_vertex_format), (void *)(offsetof(basic_mesh_vertex_format, position)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(basic_mesh_vertex_format), (void *)(offsetof(basic_mesh_vertex_format, normal)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(basic_mesh_vertex_format), (void *)(offsetof(basic_mesh_vertex_format, color)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(basic_mesh_vertex_format), (void *)(offsetof(basic_mesh_vertex_format, uv)));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(currentVao);

	result.vaoHandle = vao;
	result.numVerts = currentVert;
	result.bufferHandle = vbo;

	return result;
}
