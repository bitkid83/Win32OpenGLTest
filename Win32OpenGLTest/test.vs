#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


/* varying vec3 surfaceNormal;
varying vec3 v; */

void main()
{
	//gl_Position = projection * view * model * vec4(position, 1.0);
	//gl_Position = projection * view * model * gl_Vertex;
	
	/* v = vec3(projection * view * model * vec4(position, 1.0));
	surfaceNormal = vec3(0.0f, 0.0f, 0.5f);
	//surfaceNormal = normalize(vec3(gl_NormalMatrix * gl_Normal)); */

	gl_Position = projection * view * model * vec4(position, 1.0);
}