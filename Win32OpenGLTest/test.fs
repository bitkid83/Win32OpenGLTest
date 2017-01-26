#version 330 core

uniform vec3 input_color;
out vec4 color;

void main()
{
	vec3 lightColor = vec3(1.0f);
	float ambientStrength = 0.25;	
	vec3 ambient = ambientStrength * lightColor;
	vec3 result = ambient * input_color;
	color = vec4(result, 1.0f);
}