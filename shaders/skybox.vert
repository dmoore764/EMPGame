#version 330 core
out vec3 dir;
uniform mat4 invPV;
void main()
{
	vec2 pos  = vec2( (gl_VertexID & 2)>>1, 1 - (gl_VertexID & 1)) * 2.0 - 1.0;
	vec4 front= invPV * vec4(pos, -1.0, 1.0);
	vec4 back = invPV * vec4(pos,  1.0, 1.0);

	dir=back.xzy / back.w - front.xzy / front.w;
	gl_Position = vec4(pos,0.999,1.0);
}
