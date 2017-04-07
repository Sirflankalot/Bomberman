#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

out vec2 vTexCoord;

uniform vec2 origin;
uniform vec2 size;

void main () {
	vec2 scaled = ((position + 1.0) / 2.0) * size;
	vec2 moved = (scaled + origin - 0.5) * 2;

	gl_Position = vec4(moved, 0.0, 1.0);
	vTexCoord = vec2(texcoord.x, 1 - texcoord.y);
}
