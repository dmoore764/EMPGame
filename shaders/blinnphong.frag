#version 330 core

uniform sampler2D Texture;
uniform vec3 ViewPos;

in vec2 ThruUV;
in vec3 WorldPos;
in vec3 WorldNormal;

out vec4 color;

const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightPower = 1.0;
const vec3 ambientColor = vec3(0.1, 0.2, 0.1);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;
const float screenGamma = 2.2;
const float specularStrength = 1.0;

void main()
{
	vec3 diffuseColor = texture(Texture, ThruUV).rgb;

	vec3 normal = normalize(WorldNormal);
	vec3 lightDir = normalize(vec3(0.0, -1.0, -2.0));
	float lambertian = max(dot(lightDir, normal), 0.0);
	float specular = 0.0;

	if (lambertian > 0.0)
	{
		vec3 viewDir = normalize(ViewPos - WorldPos);
		vec3 reflectDir = reflect(-lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);
		specular = specularStrength * spec;
	}

	vec3 colorLinear = diffuseColor * ambientColor + diffuseColor * lambertian * lightColor * lightPower + specColor * specular * lightColor * lightPower;

	vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0/screenGamma));

	color = vec4(colorGammaCorrected, 1.0);

	//dot(normal, normalize(WorldPos - ViewPos));
	//vec3 viewDir = normalize(WorldPos - ViewPos);
	//vec3 halfDir = normalize(lightDir + viewDir);
	//color = max(dot(halfDir, normal), 0.0) * vec4(1.0, 1.0, 1.0, 1.0);
}
