#version 330 core

precision mediump float;

uniform vec3 light_pos;
uniform vec3 input_color;

varying vec3 v_Position;
//varying vec4 v_Color;
varying vec3 v_Normal;

out vec4 frag_color;


void main()
{
	float distance = length(light_pos - v_Position);
	vec3 lightVec = normalize(light_pos - v_Position);
	float diffuse = max(dot(v_Normal, lightVec), 0.25);
	float intensity = 75.0f;
	diffuse = diffuse * (intensity / (1.0 + (3.0 * distance * distance)));	
	
	frag_color = vec4((input_color * diffuse), 1.0f);

	/* float ambientStrength = 0.75f;
	vec3 ambient = ambientStrength * input_color;
	vec3 result = ambient * input_color;		
	frag_color = vec4(result, 1.0f); */
	
}