#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec2 vertexUV;
layout(location = 4) in vec3 jointWeights;
layout(location = 5) in ivec3 jointIndices;

out vec4 ThruColor;
out vec2 ThruUV;

const int MAX_JOINTS = 50;
const int MAX_WEIGHTS = 3;

uniform mat4 MVP;
uniform mat4 ModelMat;
uniform vec3 LightPos;
uniform mat4 JointTransforms[MAX_JOINTS];

void main() {
	vec3 color = vertexColor.rgb / 255.0;
	
	vec4 totalLocalPos = vec4(0.0);
	vec4 totalNormal = vec4(0.0);
	
	for (int i = 0; i < MAX_WEIGHTS; i++)
	{
		mat4 jointTransform = JointTransforms[jointIndices[i]];
		vec4 posePosition = jointTransform * vec4(vertexPosition, 1.0);
		totalLocalPos += posePosition * jointWeights[i];

		vec4 worldNormal = jointTransform * vec4(vertexNormal, 0.0);
		totalNormal += worldNormal * jointWeights[i];
	}


	//light pointing down
	float lightTx = -dot(LightPos, totalNormal.xyz);

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
	gl_Position = MVP * ModelMat * totalLocalPos;
}
