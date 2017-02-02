#version 330 core
layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec3 a_Normal;

//uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;
uniform mat4 mvp;	// model-view-projection matrix
uniform mat4 mv;	// model-view matrix

//attribute vec4 a_Position;
//attribute vec3 a_Normal;
//attribute vec4 a_Color;


varying vec3 v_Position;
//varying vec3 v_Color;
varying vec3 v_Normal;

void main()
{	
	v_Position = vec3(mv * a_Position); // transform vertex into eye space
	//v_Color = a_Color; // pass through the color
	v_Normal = vec3(mv * vec4(a_Normal, 0.0)); // transform normal's orientation into eye space
	
	gl_Position = mvp * a_Position;
	//gl_Position = projection * view * model * vec4(position, 1.0);	
}