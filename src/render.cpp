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
	                      reinterpret_cast<GLvoid*>(0 * sizeof(GLfloat))); // Position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
	                      reinterpret_cast<GLvoid*>(3 * sizeof(GLfloat))); // Texcoords
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
	                      reinterpret_cast<GLvoid*>(5 * sizeof(GLfloat))); // Normals

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	return std::make_tuple(VAO, VBO);
}

GLuint render::upload_texture(const image::image& img, bool srgb) {
	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA, img.width, img.height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, img.data.data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

void render::render_object(GLuint VAO, GLuint VBO, std::size_t vertices, GLuint tex,
                           GLuint world_matrix_uniform, glm::mat4 world_matrix) {
	// Bind world vertex data
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	glUniformMatrix4fv(world_matrix_uniform, 1, GL_FALSE, glm::value_ptr(world_matrix));

	glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(vertices));
}

// RenderQuad() Renders a 1x1 quad in NDC, best used for framebuffer color
// targets
// and post-processing effects.
GLuint quadVAO = 0;
GLuint quadVBO;
void render::render_fullscreen_quad() {
	if (quadVAO == 0) {
		constexpr GLfloat quadVertices[] = {
		    // Positions            // Texture Coords
		    -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
		    1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		                      reinterpret_cast<GLvoid*>(0));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		                      reinterpret_cast<GLvoid*>(3 * sizeof(GLfloat)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
