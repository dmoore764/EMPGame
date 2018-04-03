#version 330 core

uniform sampler2D Texture;

out vec4 color;

in vec4 ThruColor;
in vec2 ThruUV;

void main(){
    color = texture(Texture, ThruUV) * ThruColor;
	
    if (color.a < 0.05)
    {
		discard;
    }
}

