#pragma once

#include "objparser.hpp"
#include <GL/glew.h>
#include <vector>

namespace gamegrid {
	enum class StateType { empty, block, powerup_trap, powerup_ammo, powerup_bomb, trap };

	struct State {
		StateType type;
		int data = 0;
	};

	struct GameGrid {
		std::vector<State> state;
		std::size_t width;
		std::size_t height;
	};

	extern GameGrid gamegrid;

	extern ObjFile spikeycube;
	extern ObjFile bomb;
	extern ObjFile bullet;

	void initialize(std::size_t width, std::size_t height);
	void render(GLuint world_matrix_uniform);
}