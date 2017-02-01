#version 330 core

uniform vec3 input_color;
uniform vec3 light_pos;
out vec4 color;

/* varying vec3 surfaceNormal;
varying vec3 v; */

void main()
{
	vec3 lightColor = vec3(1.0f);
	float ambientStrength = 0.25;	
	vec3 ambient = ambientStrength * lightColor;
	vec3 result = ambient * input_color;

	/* vec3 lightNorm = normalize(light_pos - v);	
	vec3 diffuseLight = vec4(0.0f, 0.0f, 0.0f, 1.0f) * max(dot(surfaceNormal, lightNorm), 0.0);	 */
	//color = vec4(diffuseLight, 1.0);
	
	color = vec4(result, 1.0f);
}