#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec2 vertexUV;

out vec4 ThruColor;
out vec2 ThruUV;
uniform mat4 MVP;

void main(){
    ThruUV = vertexUV;
    ThruColor = vertexColor / 256.0;
    gl_Position = MVP * vec4(vertexPosition,1.0);
}
