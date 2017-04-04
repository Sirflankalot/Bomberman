#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "render.hpp"

std::tuple<GLuint, GLuint> render::upload_model(const ObjFile& file) {
	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, file.objects[0].vertices.size() * sizeof(Vertex),
	             file.objects[0].vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
	                      (GLvoid*) (0 * sizeof(GLfloat))); // Position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
	                      (GLvoid*) (3 * sizeof(GLfloat))); // Texcoords
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
	                      (GLvoid*) (5 * sizeof(GLfloat))); // Normals

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	return std::make_tuple(VAO, VBO);
}

void render::render_object(GLuint VAO, GLuint VBO, std::size_t vertices,
                           GLuint world_matrix_uniform, glm::mat4 world_matrix) {
	// Bind world vertex data
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glUniformMatrix4fv(world_matrix_uniform, 1, GL_FALSE, glm::value_ptr(world_matrix));

	glDrawArrays(GL_TRIANGLES, 0, vertices);
}
