#pragma once

#include "controller.hpp"
#include "objparser.hpp"
#include <GL/glew.h>
#include <vector>

namespace gamegrid {
	enum class StateType { empty = 0, powerup_ammo = 1, powerup_bomb = 2, trap = 3 };

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
	void regenerate();
	void read_controls(const control::movement_report_type& rt);
}