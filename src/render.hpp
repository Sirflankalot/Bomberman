#pragma once

#include "objparser.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <tuple>

namespace render {
	std::tuple<GLuint, GLuint> upload_model(const ObjFile& file);
	void render_object(GLuint VAO, GLuint VBO, std::size_t vertices, GLuint world_matrix_uniform,
	                   glm::mat4 world_matrix = glm::mat4{});
}
