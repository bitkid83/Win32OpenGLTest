#version 450

precision mediump float;

uniform vec3 input_color;
uniform vec3 light_pos;
smooth in vec3 v_Position;
smooth in vec3 v_Normal;
smooth in vec3 v_LightPos;

out vec4 frag_color;


void main()
{		
	float distance = length(v_LightPos - v_Position);
	vec3 lightVec = normalize(v_LightPos - v_Position);
	float diffuse = max(dot(v_Normal, lightVec), 0.1);
	float intensity = 50.0f;
	diffuse = diffuse * (intensity / (1.0 + (2.0 * distance * distance)));
	
	frag_color = vec4((input_color * diffuse), 1.0f);

	/* float ambientStrength = 0.75f;
	vec3 ambient = ambientStrength * input_color;
	vec3 result = ambient * input_color;		
	frag_color = vec4(result, 1.0f); */
}