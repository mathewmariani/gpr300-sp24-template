#version 410

struct Material{
	float Ka;
	float Kd;
	float Ks;
	float Shininess;
};

out vec4 FragColor;

in Surface {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
} fs_in;

uniform sampler2D texture0; 
uniform Material material;
uniform vec3 _EyePos;
uniform vec3 _LightDirection = vec3(0.0, -1.0, 0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3, 0.4, 0.46);

void main() {
	vec3 normal = normalize(fs_in.normal);
	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal, toLight), 0.0);

	vec3 toEye = normalize(_EyePos - fs_in.position);
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal, h), 0.0), material.Shininess);

	vec3 lightColor = (material.Kd * diffuseFactor + material.Ks * specularFactor) * _LightColor;
	lightColor += _AmbientColor * material.Ka;

	vec3 objectColor = texture(texture0, fs_in.texcoord).rgb;
	FragColor = vec4(objectColor * lightColor,1.0);
}
