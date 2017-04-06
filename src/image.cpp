#include "image.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <png.h>
#include <stdexcept>

static void read_from_iostream(png_structp png_ptr, png_bytep outBytes,
                               png_size_t byteCountToRead) {
	void* io_ptr = png_get_io_ptr(png_ptr);

	std::istream& stream = *reinterpret_cast<std::istream*>(io_ptr);
	stream.read((char*) outBytes, byteCountToRead);
}

template <bool has_alpha>
static void parse_png(image::image& out_image, png_struct* const png_ptr,
                      png_info* const info_ptr) {
	const auto width = out_image.width;
	const auto height = out_image.height;

	const png_uint_32 bytes_per_row = png_get_rowbytes(png_ptr, info_ptr);
	std::unique_ptr<uint8_t[]> row_data(new uint8_t[bytes_per_row]);

	// read rows
	for (std::size_t row = 0; row < height; ++row) {
		png_read_row(png_ptr, row_data.get(), nullptr);

		const std::size_t row_offset = row * width;

		std::size_t byte_index = 0;
		for (std::size_t col = 0; col < width; ++col) {
			const uint8_t red = row_data[byte_index + 0];
			const uint8_t green = row_data[byte_index + 1];
			const uint8_t blue = row_data[byte_index + 2];
			uint8_t alpha;
			if (has_alpha) {
				alpha = row_data[byte_index + 3];
				byte_index += 4;
			}
			else {
				alpha = 255;
				byte_index += 3;
			}

			const std::size_t index = row_offset + col;
			out_image.data[index] = image::pixel{red, green, blue, alpha};
		}
	}
}

image::image image::read_image(std::istream& stream) {
	// Take 8 off the top to check if it's a png
	std::array<char, 8> header;
	stream.read(&header[0], 8);

	bool is_png = !png_sig_cmp((png_bytep) &header[0], 0, 8);
	if (!is_png) {
		throw std::invalid_argument("Stream given isn't png");
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr) {
		throw std::logic_error("PNG struct cannot be initialized");
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		throw std::logic_error("PNG info struct cannot be initialized");
	}

	png_infop end_info_ptr = png_create_info_struct(png_ptr);
	if (!end_info_ptr) {
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		throw std::logic_error("PNG end info struct cannot be initialized");
	}

	// set custom read func
	png_set_read_fn(png_ptr, &stream, read_from_iostream);

	// we've already read the signature
	png_set_sig_bytes(png_ptr, 8);

	// read png info
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width = 0, height = 0;
	int bit_depth = 0, color_type = -1;
	uint32_t retval = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	                               nullptr, nullptr, nullptr);

	if (retval != 1) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info_ptr);
		throw std::logic_error("PNG info cannot be read");
	}

	// parse the file
	image img;
	img.width = width;
	img.height = height;
	img.data.resize(width * height);

	switch (color_type) {
		case PNG_COLOR_TYPE_RGB:
			parse_png<false>(img, png_ptr, info_ptr);
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			parse_png<true>(img, png_ptr, info_ptr);
			break;
		default:
			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info_ptr);
			throw std::logic_error("PNG has unsupported image format");
			break;
	}

	return img;
}

void image::ogl_flip_image(image& image) {
	std::unique_ptr<pixel[]> row_cache(new pixel[image.width]);
	for (std::size_t row = 0; row < image.height / 2; ++row) {
		auto offset1_start = image.data.begin() + (row * image.width);
		auto offset1_end = image.data.begin() + ((row + 1) * image.width);
		auto offset2_start = image.data.begin() + ((image.height - row - 1) * image.width);
		auto offset2_end = image.data.end();

		std::copy(offset1_start, offset1_end, row_cache.get());
		std::copy(offset2_start, offset2_end, offset1_start);
		std::copy(row_cache.get(), row_cache.get() + image.width, offset2_start);
	}
}

image::image image::create_ogl_image(const char* name) {
	std::ifstream file(name, std::ios::binary);
	auto img = read_image(file);
	//ogl_flip_image(img);
	return img;
}
