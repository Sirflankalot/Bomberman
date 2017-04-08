#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "bomb.hpp"
#include "gamegrid.hpp"
#include "image.hpp"
#include "objparser.hpp"
#include "player.hpp"
#include "render.hpp"

#include <algorithm>

std::vector<bomb::bomb_data> bomb::bombs;

static std::size_t bomb_vertex_count;
static GLuint bomb_vao, bomb_vbo;
static GLuint bomb_tex;

static std::size_t explosion_vertex_count;
static GLuint explosion_vao, explosion_vbo;
static GLuint explosion_tex;

void bomb::initialize() {
	image::image img = image::create_ogl_image("textures/ticking_bomb.png");
	bomb_tex = render::upload_texture(img);
	ObjFile bomb_model = parse_obj_file("objects/ticking_bomb.obj");
	bomb_vertex_count = bomb_model.objects[0].vertices.size();
	std::tie(bomb_vao, bomb_vbo) = render::upload_model(bomb_model);

	image::image explode_image = image::create_ogl_image("textures/explosion.png");
	explosion_tex = render::upload_texture(explode_image);
	ObjFile explosion_model = parse_obj_file("objects/explosion.obj");
	explosion_vertex_count = explosion_model.objects[0].vertices.size();
	std::tie(explosion_vao, explosion_vbo) = render::upload_model(explosion_model);
}

void bomb::add_bomb(std::size_t x, std::size_t y, float time) {
	bomb_data bd;
	bd.x = x;
	bd.y = y;
	bd.time = time;

	bombs.push_back(bd);
}

void bomb::update_bombs(float time_elapsed) {
	for (std::size_t i = 0; i < bombs.size(); ++i) {
		auto&& bomb = bombs[i];

		bomb.time -= time_elapsed;

		if (bomb.time < 0.0f) {
			bomb.live = false;
			for (std::size_t bi = 0; bi < players::player_list.size(); ++bi) {
				auto&& p = players::player_list[bi];
				glm::vec2 player_location =
				    glm::mix(glm::vec2(p.last_x, p.last_y), glm::vec2(p.loc_x, p.loc_y), p.factor);
				glm::vec2 bomb_location(bomb.x, bomb.y);

				float dist = glm::distance(player_location, bomb_location);

				if (dist <= 3) {
					players::respawn(bi);
				}
			}
		}

		if (bomb.time < -0.15f) {
			bomb.active = false;
		}
	}

	bombs.erase(std::remove_if(bombs.begin(), bombs.end(), [](auto bd) { return !bd.active; }),
	            bombs.end());
}

void bomb::render(GLuint world_matrix_uniform) {
	for (std::size_t i = 0; i < bombs.size(); ++i) {
		auto&& bomb = bombs[i];
		glm::vec2 grid_location = glm::vec2(bomb.x, bomb.y);

		glm::vec2 real_location =
		    grid_location -
		    (glm::vec2{gamegrid::gamegrid.width, gamegrid::gamegrid.height} - 1.0f) / 2.0f;
		auto translate =
		    glm::translate(glm::mat4{}, glm::vec3(real_location.x, 0.5, real_location.y));
		if (bomb.live) {
			render::render_object(bomb_vao, bomb_vbo, bomb_vertex_count, bomb_tex,
			                      world_matrix_uniform, translate);
		}
		else {
			auto scale = glm::scale(translate, glm::vec3{4});
			render::render_object(explosion_vao, explosion_vbo, explosion_vertex_count,
			                      explosion_tex, world_matrix_uniform, scale);
		}
	}
}