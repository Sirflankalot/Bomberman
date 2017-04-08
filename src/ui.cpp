#include "ui.hpp"
#include "image.hpp"
#include "player.hpp"
#include "render.hpp"
#include "shader.hpp"
#include <memory>

static GLuint ammo_tex_0, ammo_tex_1, ammo_tex_2, ammo_tex_3, ammo_tex_4;
static GLuint bomb_tex;
static GLuint reset_tex;

static std::unique_ptr<Shader_Program> image_prog;

static GLuint choose_tex(std::size_t ammo_count) {
	switch (ammo_count) {
		case 0:
			return ammo_tex_0;
		case 1:
			return ammo_tex_1;
		case 2:
			return ammo_tex_2;
		case 3:
			return ammo_tex_3;
		case 4:
		default:
			return ammo_tex_4;
	}
}

void ui::initialize() {
	image::image ammo_image_0 = image::create_ogl_image("textures/ammo0.png");
	image::image ammo_image_1 = image::create_ogl_image("textures/ammo1.png");
	image::image ammo_image_2 = image::create_ogl_image("textures/ammo2.png");
	image::image ammo_image_3 = image::create_ogl_image("textures/ammo3.png");
	image::image ammo_image_4 = image::create_ogl_image("textures/ammo4.png");
	image::image bomb_image = image::create_ogl_image("textures/bomb_icon.png");
	image::image reset_image = image::create_ogl_image("textures/resettext.png");

	ammo_tex_0 = render::upload_texture(ammo_image_0, false);
	ammo_tex_1 = render::upload_texture(ammo_image_1, false);
	ammo_tex_2 = render::upload_texture(ammo_image_2, false);
	ammo_tex_3 = render::upload_texture(ammo_image_3, false);
	ammo_tex_4 = render::upload_texture(ammo_image_4, false);
	bomb_tex = render::upload_texture(bomb_image, false);
	reset_tex = render::upload_texture(reset_image, false);

	image_prog = std::make_unique<Shader_Program>();
	image_prog->add("shaders/image.v.glsl", Shader::VERTEX);
	image_prog->add("shaders/image.f.glsl", Shader::FRAGMENT);
	image_prog->compile();
	image_prog->link();
	image_prog->use();

	glUniform1i(image_prog->getUniform("textTexture"), 0);
}

void ui::render(std::size_t screen_width, std::size_t screen_height) {
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

	image_prog->use();

	glUniform2f(image_prog->getUniform("size"), 40.0f / float(screen_width),
	            100.0f / float(screen_height));

	if (players::player_list[0].active) {
		glUniform2f(image_prog->getUniform("origin"), 10.0f / float(screen_width),
		            (float(screen_height) - 110.0f) / float(screen_height));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, choose_tex(players::player_list[0].ammo_count));

		render::render_fullscreen_quad();
	}

	if (players::player_list[1].active) {
		glUniform2f(image_prog->getUniform("origin"),
		            (float(screen_width) - 50.f) / float(screen_width),
		            (float(screen_height) - 110.0f) / float(screen_height));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, choose_tex(players::player_list[1].ammo_count));

		render::render_fullscreen_quad();
	}

	if (players::player_list[2].active) {
		glUniform2f(image_prog->getUniform("origin"),
		            (float(screen_width) - 50.f) / float(screen_width), 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, choose_tex(players::player_list[2].ammo_count));

		render::render_fullscreen_quad();
	}

	if (players::player_list[3].active) {
		glUniform2f(image_prog->getUniform("origin"), 10.0f, 10.0f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, choose_tex(players::player_list[3].ammo_count));

		render::render_fullscreen_quad();
	}

	glUniform2f(image_prog->getUniform("size"), 100.0f / float(screen_width),
	            102.0f / float(screen_height));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bomb_tex);

	if (players::player_list[0].active &&
	    players::player_list[0].power == players::player_info::powerup::bomb) {
		glUniform2f(image_prog->getUniform("origin"), 60.0f / float(screen_width),
		            (float(screen_height) - 110.0f) / float(screen_height));

		render::render_fullscreen_quad();
	}

	if (players::player_list[1].active &&
	    players::player_list[1].power == players::player_info::powerup::bomb) {
		glUniform2f(image_prog->getUniform("origin"),
		            (float(screen_width) - 160.f) / float(screen_width),
		            (float(screen_height) - 110.0f) / float(screen_height));

		render::render_fullscreen_quad();
	}

	if (players::player_list[2].active &&
	    players::player_list[2].power == players::player_info::powerup::bomb) {
		glUniform2f(image_prog->getUniform("origin"),
		            (float(screen_width) - 160.f) / float(screen_width), 0);

		render::render_fullscreen_quad();
	}

	if (players::player_list[3].active &&
	    players::player_list[3].power == players::player_info::powerup::bomb) {
		glUniform2f(image_prog->getUniform("origin"), 60.0f, 10.0f);

		render::render_fullscreen_quad();
	}

	glUniform2f(image_prog->getUniform("size"), 547.0f / float(screen_width),
	            43.0f / float(screen_height));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reset_tex);

	glUniform2f(image_prog->getUniform("origin"),
	            ((float(screen_width) / 2.0f) - 273.5f) / float(screen_width),
	            (float(screen_height) - 43.0f - 20.0f) / float(screen_height));

	render::render_fullscreen_quad();

	glDisable(GL_BLEND);
}