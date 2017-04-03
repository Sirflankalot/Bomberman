#pragma once

#include "objparser.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace lights {
	struct LightData {
		glm::vec3 position;
		std::size_t backptr;
		float size;
	};

	std::size_t add(glm::vec3 color, glm::vec3 position);
	void initialize();
	void updatetransforms();
	void remove(std::size_t num);

	extern std::vector<LightData> lightdata;
	extern std::vector<glm::mat4> lighteffectworldmatrix;
	extern std::vector<glm::vec3> lightcolor;

	extern GLuint Light_VAO, Light_VBO;
	extern GLuint LightCircle_VBO;
	extern GLuint LightTransform_VBO;
	extern GLuint LightColor_VBO;
	extern GLuint LightPosition_VBO;

	extern ObjFile circlefile;
	extern std::size_t lightcount;
}