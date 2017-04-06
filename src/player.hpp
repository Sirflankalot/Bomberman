#pragma once

#include <GL/glew.h>

#include "controller.hpp"
#include <array>
#include <cstddef>

namespace players {
	struct player_info {
		enum class direction : uint8_t { up = 0, left = 1, down = 2, right = 3 };
		std::size_t loc_x, loc_y;
		std::size_t last_x, last_y;
		float factor = 1.0;
		direction dir;
		bool animated = false;
		bool active = false;
		enum class powerup : uint8_t { none = 0, trap, bomb };
		powerup power = powerup::none;
		uint8_t ammo_count = 1;
	};

	extern std::array<player_info, 4> player_list;

	void initialize();
	void update_players(const control::movement_report_type&, float time_elapsed);
	void render(GLuint world_matrix_uniform);
	void respawn(std::size_t player_index);
}