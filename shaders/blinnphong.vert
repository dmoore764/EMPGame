#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 3) in vec2 vertexUV;

out vec2 ThruUV;
out vec3 WorldPos;
out vec3 WorldNormal;

uniform mat4 MVP;
uniform mat4 ModelMat;

void main() 
{
	WorldNormal = (ModelMat * vec4(vertexNormal, 0.0)).xyz;
	WorldPos = (ModelMat * vec4(vertexPosition, 1.0)).xyz;
	ThruUV = vertexUV;

	gl_Position = MVP * ModelMat * vec4(vertexPosition, 1.0);
}
