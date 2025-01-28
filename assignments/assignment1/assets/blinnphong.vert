#version 410

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

uniform mat4 model;
uniform mat4 view_proj;

out Surface {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
} vs_out;

void main(){
	vs_out.position = vec3(model * vec4(in_position, 1.0));
	vs_out.normal = transpose(inverse(mat3(model))) * in_normal;
	vs_out.texcoord = in_texcoord;

	gl_Position = view_proj * model * vec4(in_position, 1.0);
}
