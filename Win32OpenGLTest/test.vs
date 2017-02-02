#version 450

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 mvp;	// model-view-projection matrix
uniform mat4 mv;	// model-view matrix
uniform vec3 light_pos;


smooth out vec3 v_Position;
smooth out vec3 v_Normal;
smooth out vec3 v_LightPos;

void main()
{		
	v_LightPos = vec3(view * vec4(light_pos, 1.0));
	v_Position = vec3(mv * a_Position); // transform vertex into eye space	
	//v_Normal = vec3(mv * vec4(a_Normal, 0.0)); // transform normal's orientation into eye space	
	v_Normal = vec3(inverse(transpose(mv)) * vec4(a_Normal, 0.0));

	gl_Position = mvp * a_Position;
	
	//gl_Position = projection * view * model * vec4(position, 1.0);	
}