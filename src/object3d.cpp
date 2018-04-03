enum object3d_type
{
	OBJECT3D_BONE_MODEL,
	OBJECT3D_BASIC_GEOMETRY,
};

enum object3d_physics_behavior
{
	PHYS_NONE,
	PHYS_STATIC,		//non-moving
	PHYS_DYNAMIC,		//movement controlled by physics engine
	PHYS_KINETIC,		//movement controlled by game
};

struct object3d
{
	object3d_type type;
	v3 position;
	quaternion rotation;
	glm::mat4 modelMat;
	union
	{
		bone_model_instance boneModelInstance;
		basic_mesh geometry;
	};
	texture *tex0;
	game_sprite *sprite0;
};

void Object3DUpdate(object3d *obj, float deltaT)
{
	switch (obj->type)
	{
		case OBJECT3D_BONE_MODEL:
		{
			bone_model_instance *animated_model = &obj->boneModelInstance;
			animated_model->currentTime += deltaT;
			if (animated_model->currentTime > animated_model->model->animation.keyframes[animated_model->model->animation.numFrames-1].timeStamp)
				animated_model->currentTime = animated_model->model->animation.keyframes[0].timeStamp;
			BoneModelInstanceUpdateTransforms(animated_model);
		} break;

		case OBJECT3D_BASIC_GEOMETRY:
		{
		} break;
	}
}

void Object3DCalculateModelMat(object3d *obj)
{
	obj->modelMat = toMat4(ToGLMQuat(&obj->rotation));
	obj->modelMat[3][0] = obj->position.x;
	obj->modelMat[3][1] = obj->position.y;
	obj->modelMat[3][2] = obj->position.z;
}

void Object3DSetModelMatToBone(object3d *obj, object3d *target, const char *boneName)
{
	assert(target->type == OBJECT3D_BONE_MODEL);
	bone_model *model = target->boneModelInstance.model;
	int jointIndex = -1;
	for (int i = 0; i < model->numJoints; i++)
	{
		if (strcmp(model->joints[i].name, boneName) == 0)
		{
			jointIndex = i;
			break;
		}
	}
	obj->modelMat = target->modelMat * target->boneModelInstance.boneTransformsNoInverseBind[jointIndex];
}

void Object3DSetUniforms(object3d *obj)
{
	render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
	cmd->type = RC_SET_UNIFORM;
	sprintf(cmd->uniform_name, "ModelMat");
	cmd->count = 1;
	cmd->unitype = UNIFORM_MAT4;
	cmd->mat = &obj->modelMat;

	switch(obj->type)
	{
		case OBJECT3D_BASIC_GEOMETRY:
		{
		} break;

		case OBJECT3D_BONE_MODEL:
		{
			bone_model_instance *animated_model = &obj->boneModelInstance;
			render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
			cmd->type = RC_SET_UNIFORM;
			sprintf(cmd->uniform_name, "JointTransforms");
			cmd->count = animated_model->model->numJoints;
			cmd->unitype = UNIFORM_MAT4;
			cmd->mat = animated_model->boneTransforms;
		} break;
	}
	
	if (obj->tex0)
		SetTexture(obj->tex0, 0);
	else if (obj->sprite0)
	{
		sprite_atlas *atlas = &SpritePack.atlas[obj->sprite0->atlas_index];
		SetTexture(atlas->tex);
	}
}


void Object3DDrawMesh(object3d *obj)
{
	switch(obj->type)
	{
		case OBJECT3D_BASIC_GEOMETRY:
		{
			DrawMesh(obj->geometry.vaoHandle, obj->geometry.numVerts);
		} break;

		case OBJECT3D_BONE_MODEL:
		{
			DrawMesh(obj->boneModelInstance.model->mesh.vaoHandle, obj->boneModelInstance.model->mesh.numVerts);
		} break;
	}
}
