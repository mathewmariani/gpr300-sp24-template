#version 410

out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0; 

void main() {
	vec3 albedo = texture(texture0, vs_texcoord).rgb;
	// naive
	// float average = (albedo.r + albedo.g + albedo.b) / 3.0;

	// realistic
	// learnopengl.com
	float average = (0.2126 * albedo.r) + (0.7152 * albedo.g) + (0.0722 * albedo.b);
	FragColor = vec4(average, average, average, 1.0);
}
