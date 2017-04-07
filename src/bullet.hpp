#pragma once

#include <GL/glew.h>
#include <cinttypes>
#include <vector>

namespace bullet {
	struct bullet_data {
		enum class direction : uint8_t { up = 0, left = 1, down = 2, right = 3 };
		float loc_x, loc_y;
		float vel_x, vel_y;
		direction dir;
		float lifespan;
	};

	extern std::vector<bullet_data> bullets;

	void initialize();
	void add_bullet(float pos_x, float pos_y, float vel_x, float vel_y, float lifespan);
	void update_bullets(float time_elapsed);
	void render(GLuint world_matrix_uniform);
}