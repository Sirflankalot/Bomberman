#include "gamegrid.hpp"
#include "render.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tuple>

gamegrid::GameGrid gamegrid::gamegrid;

ObjFile gamegrid::spikeycube;
ObjFile gamegrid::bomb;
ObjFile gamegrid::bullet;

GLuint spikeycube_vao, spikeycube_vbo;
GLuint bomb_vao, bomb_vbo;
GLuint bullet_vao, bullet_vbo;

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
}

void gamegrid::render(GLuint world_matrix_uniform) {
	for (std::size_t i = 0; i < gamegrid.state.size(); ++i) {
		if (gamegrid.state[i].type == StateType::empty) {
			continue;
		}

		std::size_t x = i % gamegrid.width;
		std::size_t y = i / gamegrid.width;

		glm::mat4 world =
		    glm::translate(glm::mat4{}, glm::vec3(x - (gamegrid.width - 1.0f) / 2.0f, 0.5,
		                                          y - (gamegrid.height - 1.0f) / 2.0f));

		switch (gamegrid.state[i].type) {
			case StateType::empty:
				break;
			case StateType::block:
				break;
			case StateType::powerup_trap:
				break;
			case StateType::powerup_ammo:
				render::render_object(bullet_vao, bullet_vbo, bullet.objects[0].vertices.size(),
				                      world_matrix_uniform, world);
				break;
			case StateType::powerup_bomb:
				render::render_object(bomb_vao, bomb_vbo, bomb.objects[0].vertices.size(),
				                      world_matrix_uniform, world);
				break;
			case StateType::trap:
				render::render_object(spikeycube_vao, spikeycube_vbo,
				                      spikeycube.objects[0].vertices.size(), world_matrix_uniform,
				                      world);
				break;
		}
	}
}
