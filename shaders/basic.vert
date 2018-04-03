#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;

out vec4 ThruColor;

uniform mat4 MVP;

void main(){
	ThruColor = vertexColor / 256.0;
    gl_Position = MVP * vec4(vertexPosition,1.0);
}

