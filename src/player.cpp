#include "player.hpp"
#include "bomb.hpp"
#include "bullet.hpp"
#include "gamegrid.hpp"
#include "image.hpp"
#include "objparser.hpp"
#include "render.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

std::array<players::player_info, 4> players::player_list;

static std::size_t player_vertex_count;

static GLuint player_vao, player_vbo;
static std::array<GLuint, 4> player_texture;

static std::array<float, 4> time_since_bullet{{0.0f, 0.0f, 0.0f, 0.0f}};
static std::array<float, 4> time_since_bomb{{0.0f, 0.0f, 0.0f, 0.0f}};

void players::initialize() {
	respawn(0);
	respawn(1);
	respawn(2);
	respawn(3);

	ObjFile model = parse_obj_file("objects/monster.obj");
	std::tie(player_vao, player_vbo) = render::upload_model(model);
	player_vertex_count = model.objects[0].vertices.size();

	std::array<image::image, 4> images;
	images[0] = image::create_ogl_image("textures/monster1.png");
	images[1] = image::create_ogl_image("textures/monster2.png");
	images[2] = image::create_ogl_image("textures/monster3.png");
	images[3] = image::create_ogl_image("textures/monster4.png");
	for (std::size_t i = 0; i < 4; ++i) {
		player_texture[i] = render::upload_texture(images[i]);
	}
}

void players::update_players(const control::movement_report_type& report, float time_elapsed) {
	for (std::size_t i = 0; i < 4; ++i) {
		auto&& controller = report[i];
		auto&& player = player_list[i];

		if (!controller.active) {
			player.active = false;
			respawn(i);
			continue;
		}
		else {
			player.active = true;
		}

		if (player.animated) {
			if (player.factor < 1.0f) {
				player.factor = std::min(1.0f, player.factor + (time_elapsed / 0.25f));
			}
			else {
				player.animated = false;
			}
		}
		else {
			auto&& current_block =
			    gamegrid::gamegrid.state[player.loc_y * gamegrid::gamegrid.width + player.loc_x];

			switch (current_block.type) {
				case gamegrid::StateType::powerup_ammo: {
					player.ammo_count = static_cast<uint8_t>(player.ammo_count + 2);
					current_block.type = gamegrid::StateType::empty;
					break;
				}
				case gamegrid::StateType::powerup_bomb:
					if (player.power != player_info::powerup::bomb) {
						player.power = player_info::powerup::bomb;
						current_block.type = gamegrid::StateType::empty;
					}
					break;
				case gamegrid::StateType::trap:
					respawn(i);
					break;
				default:
					break;
			}

			if (controller.left_stick_dir != control::controller_report::direction::none) {
				player.factor = 0.0f;
				player.animated = true;
				player.last_x = player.loc_x;
				player.last_y = player.loc_y;
			}
			switch (controller.left_stick_dir) {
				case control::controller_report::direction::left:
					if (player.loc_x > 0) {
						player.dir = player_info::direction::left;
						player.loc_x -= 1;
					}
					break;
				case control::controller_report::direction::right:
					if (player.loc_x < gamegrid::gamegrid.width - 1) {
						player.dir = player_info::direction::right;
						player.loc_x += 1;
					}
					break;
				case control::controller_report::direction::up:
					if (player.loc_y > 0) {
						player.dir = player_info::direction::up;
						player.loc_y -= 1;
					}
					break;
				case control::controller_report::direction::down:
					if (player.loc_y < gamegrid::gamegrid.height - 1) {
						player.dir = player_info::direction::down;
						player.loc_y += 1;
					}
					break;

				default:
					break;
			}
		}

		if (controller.rtrigger && time_since_bullet[i] >= 0.5f && player.ammo_count >= 1) {
			time_since_bullet[i] = 0;
			player.ammo_count = static_cast<uint8_t>(player.ammo_count - 1);
			auto grid_loc = glm::mix(glm::vec2(player.last_x, player.last_y),
			                         glm::vec2(player.loc_x, player.loc_y), player.factor);
			glm::vec2 velocity;

			constexpr float bullet_speed = 15.0f;
			switch (player.dir) {
				case player_info::direction::left:
					velocity = glm::vec2(-bullet_speed, 0.0f);
					break;
				case player_info::direction::right:
					velocity = glm::vec2(bullet_speed, 0.0f);
					break;
				case player_info::direction::up:
					velocity = glm::vec2(0.0f, -bullet_speed);
					break;
				case player_info::direction::down:
					velocity = glm::vec2(0.0f, bullet_speed);
					break;
				default:
					break;
			}

			bullet::add_bullet(grid_loc.x, grid_loc.y, velocity.x, velocity.y, 10.0f);
		}
		if (controller.ltrigger && time_since_bomb[i] >= 2.0f &&
		    player.power == player_info::powerup::bomb) {
			time_since_bomb[i] = 0;

			player.power = player_info::powerup::none;

			bomb::add_bomb(player.loc_x, player.loc_y, 1.5f);
		}

		time_since_bullet[i] += time_elapsed;
		time_since_bomb[i] += time_elapsed;
	}
}

void players::render(GLuint world_matrix_uniform) {
	for (std::size_t i = 0; i < 4; ++i) {
		auto&& player = player_list[i];
		if (!player.active) {
			continue;
		}
		glm::vec2 grid_location = glm::mix(glm::vec2(player.last_x, player.last_y),
		                                   glm::vec2(player.loc_x, player.loc_y), player.factor);

		glm::vec2 real_location =
		    grid_location -
		    (glm::vec2{gamegrid::gamegrid.width, gamegrid::gamegrid.height} - 1.0f) / 2.0f;
		auto translate =
		    glm::translate(glm::mat4{}, glm::vec3(real_location.x, 0.5, real_location.y));
		auto rot = glm::rotate(translate, 1.570796327f * static_cast<uint8_t>(player.dir),
		                       glm::vec3(0, 1, 0));
		render::render_object(player_vao, player_vbo, player_vertex_count, player_texture[i],
		                      world_matrix_uniform, rot);
	}
}

void players::respawn(std::size_t player_index) {
	auto&& player = player_list[player_index];

	switch (player_index) {
		case 0:
			player.loc_x = 0;
			player.loc_y = 0;
			player.dir = player_info::direction::right;
			break;
		case 1:
			player.loc_x = gamegrid::gamegrid.width - 1;
			player.loc_y = 0;
			player.dir = player_info::direction::down;
			break;
		case 2:
			player.loc_x = gamegrid::gamegrid.width - 1;
			player.loc_y = gamegrid::gamegrid.height - 1;
			player.dir = player_info::direction::left;
			break;
		case 3:
			player.loc_x = 0;
			player.loc_y = gamegrid::gamegrid.height - 1;
			player.dir = player_info::direction::up;
			break;
		default:
			break;
	}

	player.factor = 1.0f;
	player.animated = false;
	player.last_x = 0;
	player.last_y = 0;
	player.ammo_count = 1;
	player.power = player_info::powerup::none;
}
