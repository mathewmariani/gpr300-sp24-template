#version 410

out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0;
uniform float strength;

const float offset = 1.0 / 300.0;

const vec2 offsets[9] = vec2[](
	vec2(-offset, offset), // top-left
	vec2(0.0, offset), // top-center
	vec2(offset, offset), // top-right

	vec2(-offset, 0.0), // middle-left
	vec2(0.0, 0.0), // middle-center
	vec2(offset, 0.0), // middle-right

	vec2(-offset, -offset), // bottom-left
	vec2(0.0, -offset), // bottom-center
	vec2(offset, -offset) // bottom-right
);

const float kernel[9] = float[](
	1.0, 2.0, 1.0,
	2.0, 4.0, 2.0,
	1.0, 2.0, 1.0
);

void main() {
	vec3 avg = vec3(0.0);
	for (int i = 0; i < 9; i++)
	{
		vec3 local = texture(texture0, vs_texcoord + offsets[i]).rgb;
		avg += local * (kernel[i] / strength);
	}
	FragColor = vec4(avg.rgb, 1.0);
}
