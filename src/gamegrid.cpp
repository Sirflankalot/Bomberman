#include "gamegrid.hpp"
#include "controller.hpp"
#include "image.hpp"
#include "player.hpp"
#include "render.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <tuple>

gamegrid::GameGrid gamegrid::gamegrid;

ObjFile gamegrid::spikeycube;
ObjFile gamegrid::bomb;
ObjFile gamegrid::bullet;

static GLuint spikeycube_vao, spikeycube_vbo;
static GLuint bomb_vao, bomb_vbo;
static GLuint bullet_vao, bullet_vbo;

static GLuint spikeycube_tex;
static GLuint bomb_tex;
static GLuint bullet_tex;

void gamegrid::initialize(std::size_t width, std::size_t height) {
	gamegrid.width = width;
	gamegrid.height = height;
	gamegrid.state.reserve(width * height);
	gamegrid.state.resize(width * height, State{StateType::empty});

	spikeycube = parse_obj_file("objects/spikeycube.obj");
	bomb = parse_obj_file("objects/bomb.obj");
	bullet = parse_obj_file("objects/bullet.obj");

	std::tie(spikeycube_vao, spikeycube_vbo) = render::upload_model(spikeycube);
	std::tie(bomb_vao, bomb_vbo) = render::upload_model(bomb);
	std::tie(bullet_vao, bullet_vbo) = render::upload_model(bullet);

	auto bullet_raw = image::create_ogl_image("textures/bullet.png");
	bullet_tex = render::upload_texture(bullet_raw);

	auto bomb_raw = image::create_ogl_image("textures/bomb.png");
	bomb_tex = render::upload_texture(bomb_raw);

	auto spikes_raw = image::create_ogl_image("textures/spikes.png");
	spikeycube_tex = render::upload_texture(spikes_raw);

	regenerate();
}

void gamegrid::read_controls(const typename control::movement_report_type& rt) {
	if (std::any_of(rt.begin(), rt.end(),
	                [](auto controller) { return controller.keys[6] && controller.active; })) {
		regenerate();
		players::respawn(0);
		players::respawn(1);
		players::respawn(2);
		players::respawn(3);
	}
}

void gamegrid::regenerate() {
	static std::mt19937 prng{std::random_device{}()};

	std::uniform_int_distribution<int> gg_uid(0, 3);
	for (std::size_t x = 1; x < gamegrid::gamegrid.width - 1; ++x) {
		for (std::size_t y = 1; y < gamegrid::gamegrid.height - 1; ++y) {
			gamegrid::gamegrid.state[y * gamegrid::gamegrid.width + x].type =
			    static_cast<gamegrid::StateType>(gg_uid(prng));
		}
	}
}

void gamegrid::render(GLuint world_matrix_uniform) {
	for (std::size_t i = 0; i < gamegrid.state.size(); ++i) {
		if (gamegrid.state[i].type == StateType::empty) {
			continue;
		}

		auto x = float(i % gamegrid.width);
		auto y = float(i / gamegrid.width);

		glm::mat4 world =
		    glm::translate(glm::mat4{}, glm::vec3(x - (float(gamegrid.width) - 1.0f) / 2.0f, 0.5,
		                                          y - (float(gamegrid.height) - 1.0f) / 2.0f));

		switch (gamegrid.state[i].type) {
			case StateType::empty:
				break;
			case StateType::powerup_ammo:
				render::render_object(bullet_vao, bullet_vbo, bullet.objects[0].vertices.size(),
				                      bullet_tex, world_matrix_uniform, world);
				break;
			case StateType::powerup_bomb:
				render::render_object(bomb_vao, bomb_vbo, bomb.objects[0].vertices.size(), bomb_tex,
				                      world_matrix_uniform, world);
				break;
			case StateType::trap:
				render::render_object(spikeycube_vao, spikeycube_vbo,
				                      spikeycube.objects[0].vertices.size(), spikeycube_tex,
				                      world_matrix_uniform, world);
				break;
			default:
				break;
		}
	}
}
