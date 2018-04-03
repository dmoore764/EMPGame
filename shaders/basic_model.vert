#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec2 vertexUV;

out vec4 ThruColor;
out vec2 ThruUV;

uniform mat4 MVP;
uniform vec3 LightPos;

void main() {
	vec3 color = vertexColor.rgb / 255.0;

	//light pointing down
	float lightTx = -dot(LightPos, vertexNormal);

	vec3 lightContrib;
	if (lightTx < 0)
	{
		lightContrib = vec3(0.0,0.0,0.0);
	}
	else
	{
		lightContrib = lightTx * color;
	}

	vec3 ambient = 0.1 * color + vec3(0.2,0.2,0.2);

	ThruColor = vec4(lightContrib + ambient, vertexColor.a / 255.0);
	ThruUV = vertexUV;
	gl_Position = MVP * vec4(vertexPosition, 1.0);
}
