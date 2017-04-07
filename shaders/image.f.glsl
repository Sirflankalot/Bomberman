#version 330 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D textTexture;

void main() {
	vec4 mix_color = texture(textTexture, vTexCoord).rgba;

	FragColor = mix_color;
}
