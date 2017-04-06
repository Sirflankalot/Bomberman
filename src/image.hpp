#pragma once

#include <cstdint>
#include <ios>
#include <vector>

namespace image {
	struct pixel {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};

	struct image {
		std::vector<pixel> data;
		std::size_t width;
		std::size_t height;
	};

	image read_image(std::istream&);
	void ogl_flip_image(image&);
	image create_ogl_image(const char*);
}
