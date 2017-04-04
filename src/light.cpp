#include "light.hpp"
#include "objparser.hpp"

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace lights {
	std::size_t lightptrcount = 0;
	std::unordered_map<std::size_t, std::size_t> translation;
	std::size_t lightcount = 0;
	std::vector<LightData> lightdata;
	std::vector<glm::mat4> lighteffectworldmatrix;
	std::vector<glm::vec3> lightcolor;

	GLuint Light_VAO, Light_VBO;
	GLuint LightCircle_VBO;
	GLuint LightTransform_VBO;
	GLuint LightColor_VBO;
	GLuint LightPosition_VBO;

	std::size_t add(glm::vec3 color, glm::vec3 position) {
		constexpr float constant = 1.0;
		constexpr float linear = 0.7;
		constexpr float quadratic = 1.8;
		float lightMax = std::max(std::max(color.r, color.g), color.b);
		float size = (-linear + std::sqrt(linear * linear -
		                                  4 * quadratic * (constant - (256.0 / 5.0) * lightMax))) /
		             (2 * quadratic);

		size_t lightptr = lightptrcount++;
		lightdata.emplace_back(LightData{position, lightptrcount, size});
		lightcolor.emplace_back(color);
		lighteffectworldmatrix.resize(lighteffectworldmatrix.size() + 1);

		lightcount++;
		return lightptr;
	}

	// TODO: Reimpliment removal
	void remove(std::size_t num) {
		num = translation[num];
#ifndef NDEBUG
		if (num >= lightcount) {
			throw std::invalid_argument("removing light above light count");
		}
#endif
		for (std::size_t i = num + 1; i < lightcount; ++i) {
			translation[lightdata[num].backptr] -= 1;
		}
		lightdata.erase(lightdata.begin() + num);
		lighteffectworldmatrix.erase(lighteffectworldmatrix.begin() + num);
		lightcolor.erase(lightcolor.begin() + num);

		lightcount--;
	}

	ObjFile circlefile = parse_obj_file("objects/circle.obj");

	void initialize() {
		glGenVertexArrays(1, &Light_VAO);
		glBindVertexArray(Light_VAO);

		glGenBuffers(1, &LightTransform_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, LightTransform_VBO);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
		                      (void*) (0 * sizeof(GLfloat)));
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
		                      (void*) (4 * sizeof(GLfloat)));
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
		                      (void*) (8 * sizeof(GLfloat)));
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
		                      (void*) (12 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glEnableVertexAttribArray(5);
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);

		glGenBuffers(1, &LightColor_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, LightColor_VBO);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), NULL);
		glEnableVertexAttribArray(1);
		glVertexAttribDivisor(1, 1);

		glGenBuffers(1, &LightPosition_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, LightPosition_VBO);
		glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(LightData), NULL);
		glEnableVertexAttribArray(6);
		glVertexAttribDivisor(6, 1);

		glGenBuffers(1, &LightCircle_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, LightCircle_VBO);
		glBufferData(GL_ARRAY_BUFFER, circlefile.objects[0].vertices.size() * sizeof(Vertex),
		             circlefile.objects[0].vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);

		glBindVertexArray(0);
	}

	void updatetransforms() {
		// Update Light Transforms
		for (size_t i = 0; i < lightcount; ++i) {
			auto&& lp = lightdata[i];

			glm::mat4 scale = glm::scale(glm::mat4{}, glm::vec3{lp.size});
			glm::mat4 translate = glm::translate(scale, lp.position);

			lighteffectworldmatrix[i] = translate;
		}

		glBindVertexArray(Light_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, LightTransform_VBO);
		glBufferData(GL_ARRAY_BUFFER, lightcount * sizeof(glm::mat4), lighteffectworldmatrix.data(),
		             GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, LightPosition_VBO);
		glBufferData(GL_ARRAY_BUFFER, lightcount * sizeof(LightData), lightdata.data(),
		             GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, LightColor_VBO);
		glBufferData(GL_ARRAY_BUFFER, lightcount * sizeof(glm::vec3), lightcolor.data(),
		             GL_STREAM_DRAW);
	}
}