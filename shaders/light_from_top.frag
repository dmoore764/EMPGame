#version 330 core

uniform sampler2D Texture;

in vec2 ThruUV;
in vec4 ThruColor;
out vec4 color;

void main() {
	color = ThruColor * texture(Texture, ThruUV);
    if (color.a < 0.05)
    {
		discard;
    }
}
