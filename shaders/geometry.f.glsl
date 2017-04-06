#version 330 core

uniform sampler2D tex;

in vec2 vTexCoords;
in vec3 vNormal;
in vec3 vFragPos;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

void main() {
	// Position vector	
	gPosition = vFragPos;
	// Normal vector
	gNormal = vNormal;
	// Diffuse color
	gAlbedoSpec.rgb = texture(tex, vec2(vTexCoords.x, 1 - vTexCoords.y)).rgb;
	// Specular
	gAlbedoSpec.a = 1.0;
}
