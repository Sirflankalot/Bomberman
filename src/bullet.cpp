#include "bullet.hpp"
#include "gamegrid.hpp"
#include "image.hpp"
#include "objparser.hpp"
#include "player.hpp"
#include "render.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

std::vector<bullet::bullet_data> bullet::bullets;

static std::size_t bullet_vertex_count;

static GLuint bullet_vao, bullet_vbo;
static GLuint bullet_tex;

void bullet::initialize() {
	image::image img;
	img.width = 1;
	img.height = 1;
	img.data.push_back(image::pixel{0, 255, 0, 255});
	bullet_tex = render::upload_texture(img);
	ObjFile bullet_model = parse_obj_file("objects/laser.obj");
	bullet_vertex_count = bullet_model.objects[0].vertices.size();
	std::tie(bullet_vao, bullet_vbo) = render::upload_model(bullet_model);
}

void bullet::add_bullet(float pos_x, float pos_y, float vel_x, float vel_y, float lifespan) {
	bullet_data bd;
	bd.loc_x = pos_x;
	bd.loc_y = pos_y;
	bd.vel_x = vel_x;
	bd.vel_y = vel_y;
	bd.lifespan = lifespan;

	constexpr float offset = 1.01f;
	if (std::abs(vel_x) > std::abs(vel_y)) {
		if (vel_x > 0) {
			bd.loc_x += offset;
			bd.dir = bullet_data::direction::right;
		}
		else {
			bd.loc_x -= offset;
			bd.dir = bullet_data::direction::left;
		}
	}
	else {
		if (vel_y > 0) {
			bd.loc_y += offset;
			bd.dir = bullet_data::direction::down;
		}
		else {
			bd.loc_y -= offset;
			bd.dir = bullet_data::direction::up;
		}
	}

	bullets.push_back(bd);
}

void bullet::update_bullets(float time_elapsed) {
	std::vector<std::size_t> removals;

	for (std::size_t i = 0; i < bullets.size(); ++i) {
		auto&& bd = bullets[i];
		bd.loc_x += bd.vel_x * time_elapsed;
		bd.loc_y += bd.vel_y * time_elapsed;
		bd.lifespan -= time_elapsed;

		if (bd.lifespan < 0) {
			removals.push_back(i);
			continue;
		}

		int32_t block_x = static_cast<int32_t>(std::round(bd.loc_x));
		int32_t block_y = static_cast<int32_t>(std::round(bd.loc_y));

		if (0 <= block_x && block_x < static_cast<int64_t>(gamegrid::gamegrid.width) && //
		    0 <= block_y && block_y < static_cast<int64_t>(gamegrid::gamegrid.height)) {
			auto&& block = gamegrid::gamegrid.state[block_y * gamegrid::gamegrid.width + block_x];

			// Check for collisions
			switch (block.type) {
				case gamegrid::StateType::trap:
					removals.push_back(i);
					break;
				default:
					break;
			}

			// Check for player contact
			auto found_itr = std::find_if(
			    players::player_list.begin(), players::player_list.end(), [&](auto pi) {
				    if (!pi.active) {
					    return false;
				    }
				    auto grid_loc = glm::mix(glm::vec2(pi.last_x, pi.last_y),
				                             glm::vec2(pi.loc_x, pi.loc_y), pi.factor);
				    return std::abs(grid_loc.x - static_cast<float>(block_x)) < 0.7 &&
				           std::abs(grid_loc.y - static_cast<float>(block_y)) < 0.7;
				});

			bool found = found_itr != players::player_list.end();

			if (found) {
				removals.push_back(i);
				players::respawn(found_itr - players::player_list.begin());
				continue;
			}
		}
	}

	std::size_t i = 0;
	bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
	                             [&](bullet_data) {
		                             return std::find(removals.begin(), removals.end(), i++) !=
		                                    removals.end();
		                         }),
	              bullets.end());
}

void bullet::render(GLuint world_matrix_uniform) {
	for (std::size_t i = 0; i < bullets.size(); ++i) {
		auto&& bullet = bullets[i];
		glm::vec2 grid_location = glm::vec2(bullet.loc_x, bullet.loc_y);

		glm::vec2 real_location =
		    grid_location -
		    (glm::vec2{gamegrid::gamegrid.width, gamegrid::gamegrid.height} - 1.0f) / 2.0f;
		auto translate =
		    glm::translate(glm::mat4{}, glm::vec3(real_location.x, 0.5, real_location.y));
		auto rot = glm::rotate(translate, 1.570796327f * static_cast<uint8_t>(bullet.dir),
		                       glm::vec3(0, 1, 0));
		render::render_object(bullet_vao, bullet_vbo, bullet_vertex_count, bullet_tex,
		                      world_matrix_uniform, rot);
	}
}
