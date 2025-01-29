#version 410

out vec4 FragColor;

in vec2 vs_texcoord;

uniform sampler2D texture0;
uniform vec3 offset;
uniform vec2 direction;

void main() {
	FragColor.r = texture(texture0, vs_texcoord + (direction * vec2(offset.r))).r;
	FragColor.g = texture(texture0, vs_texcoord + (direction * vec2(offset.g))).g;
	FragColor.b = texture(texture0, vs_texcoord + (direction * vec2(offset.b))).b;
	FragColor.a = texture(texture0, vs_texcoord).a;
}

