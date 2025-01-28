#version 410

out vec4 FragColor;

uniform sampler2D albedo;
uniform sampler2D zatoon;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

uniform vec3 _EyePos;

vec3 _LightDirection = vec3(0.0, -1.0, 0.0);

vec3 toon_lighting(vec3 normal, vec3 light_dir) {
	float diff = (dot(normal, light_dir) + 1.0) * 0.5;
	float step = texture(zatoon, vec2(diff)).r;
	vec3 light_color = mix(shadow, highlight, step);
	return light_color * step;
}

void main() {
	vec3 normal = normalize(vs_normal);
	vec3 light_color = toon_lighting(normal, _LightDirection);
	vec3 object_color = texture(albedo, vs_texcoord).rgb;
	FragColor = vec4(object_color * light_color, 1.0);
}
