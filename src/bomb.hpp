#pragma once

#include <cstddef>
#include <vector>

namespace bomb {
	struct bomb_data {
		std::size_t x, y;
		float time;
		bool live = true;
		bool active = true;
	};

	extern std::vector<bomb_data> bombs;

	void initialize();
	void add_bomb(std::size_t x, std::size_t y, float time);
	void update_bombs(float time_elapsed);
	void render(GLuint world_matrix_uniform);
}
