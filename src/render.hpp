#pragma once

#include "image.hpp"
#include "objparser.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <tuple>

namespace render {
	std::tuple<GLuint, GLuint> upload_model(const ObjFile& file);
	GLuint upload_texture(const image::image& img);
	void render_object(GLuint VAO, GLuint VBO, std::size_t vertices, GLuint tex_id,
	                   GLuint world_matrix_uniform, glm::mat4 world_matrix = glm::mat4{});
}
