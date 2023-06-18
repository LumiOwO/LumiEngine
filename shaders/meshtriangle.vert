#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

//push constants block
layout( push_constant ) uniform constants_Classname
{
	vec4 push_color;
	mat4 mvp;
};

void main()
{
	gl_Position = mvp * vec4(vPosition, 1.0);
	outColor = vColor.xyz;
}