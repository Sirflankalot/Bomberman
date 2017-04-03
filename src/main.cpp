#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "camera.hpp"
#include "fps_meter.hpp"
#include "objparser.hpp"
#include "light.hpp"
#include "sdlmanager.hpp"
#include "shader.hpp"

#ifdef _WIN32
#define APIENTRY __stdcall
#else
#define APIENTRY
#endif

struct RenderInfo {
	GLuint gBuffer;
	GLuint gPosition, gNormal, gAlbedoSpec, gDepth;
	GLuint lBuffer;
	GLuint lColor, lDepth;
	GLuint ssaoBuffer, ssaoBlurBuffer;
	GLuint ssaoNoiseTexture;
	GLuint ssaoColor, ssaoDepth, ssaoBlurColor;
};

void APIENTRY openglCallbackFunction(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*,
                                     const void*);
void RenderFullscreenQuad();
void PrepareBuffers(size_t x, size_t y, RenderInfo& data);
void DeleteBuffers(RenderInfo& data);
glm::mat4 Resize(SDL_Manager& sdlm, RenderInfo& data);

template <class T1, class T2, class T3>
auto lerp(T1 a, T2 b, T3 f) {
	return a + f * (b - a);
}

int main(int argc, char** argv) {
	(void) argc;
	(void) argv;

	//////////////////////////
	// Parse an object file //
	//////////////////////////

	auto file = parse_obj_file("objects/teapot.obj");
	auto worldfile = parse_obj_file("objects/grid.obj");

	///////////////
	// SDL Setup //
	///////////////

	SDL_Manager sdlm;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

#ifndef NDEBUG
	if (glDebugMessageCallback) {
		std::cout << "Register OpenGL debug callback " << '\n';
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openglCallbackFunction, nullptr);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
	}
	else {
		std::cout << "glDebugMessageCallback not available" << '\n';
	}
#endif

	///////////////////////////////////
	// Setup Random Number Generator //
	///////////////////////////////////

	std::mt19937 prng(std::random_device{}());

	/////////////////
	// Shader Prep //
	/////////////////

	Shader_Program geometrypass;
	geometrypass.add("shaders/geometry.v.glsl", Shader::VERTEX);
	geometrypass.add("shaders/geometry.f.glsl", Shader::FRAGMENT);
	geometrypass.compile();
	geometrypass.link();
	geometrypass.use();

	auto uGeoWorld = geometrypass.getUniform("world", Shader::MANDITORY);
	auto uGeoView = geometrypass.getUniform("view", Shader::MANDITORY);
	auto uGeoProjection = geometrypass.getUniform("projection", Shader::MANDITORY);

	auto world_world =
	    glm::scale(glm::translate(glm::mat4(), glm::vec3(0, 5, 0)), glm::vec3(10, 10, 10));
	auto monkey_world = glm::translate(glm::mat4(), glm::vec3(0, 0, 0));
	auto projection = glm::perspective(glm::radians(60.0f), sdlm.size.ratio, 0.5f, 1000.0f);

	Shader_Program lightingpass;
	lightingpass.add("shaders/lighting.v.glsl", Shader::VERTEX);
	lightingpass.add("shaders/lighting.f.glsl", Shader::FRAGMENT);
	lightingpass.compile();
	lightingpass.link();

	auto uLightViewPos = lightingpass.getUniform("viewPos");

	// Set gBuffer textures
	lightingpass.use();
	glUniform1i(lightingpass.getUniform("gPosition"), 0);
	glUniform1i(lightingpass.getUniform("gNormal"), 1);
	glUniform1i(lightingpass.getUniform("gAlbedoSpec"), 2);
	glUniform1i(lightingpass.getUniform("ssaoInput"), 5);

	Shader_Program lightbound;
	lightbound.add("shaders/lighteffect.v.glsl", Shader::VERTEX);
	lightbound.add("shaders/lighteffect.f.glsl", Shader::FRAGMENT);
	lightbound.compile();
	lightbound.link();

	auto uLightBoundWorld = lightbound.getUniform("world", Shader::MANDITORY);
	auto uLightBoundView = lightbound.getUniform("view", Shader::MANDITORY);
	auto uLightBoundPerspective = lightbound.getUniform("perspective", Shader::MANDITORY);

	auto uLightBoundViewPos = lightbound.getUniform("viewPos");
	auto uLightBoundResolution = lightbound.getUniform("resolution");

	auto uLightBoundLightPosition = lightbound.getUniform("lightposition");
	auto uLightBoundLightColor = lightbound.getUniform("lightcolor");

	auto uLightBoundRadius = lightbound.getUniform("radius");

	lightbound.use();
	glUniform1i(lightbound.getUniform("gPosition"), 0);
	glUniform1i(lightbound.getUniform("gNormal"), 1);
	glUniform1i(lightbound.getUniform("gAlbedoSpec"), 2);

	Shader_Program ssaoPass1;
	ssaoPass1.add("shaders/lighting.v.glsl", Shader::VERTEX);
	ssaoPass1.add("shaders/ssao-pass1.f.glsl", Shader::FRAGMENT);
	ssaoPass1.compile();
	ssaoPass1.link();
	ssaoPass1.use();

	glUniform1i(ssaoPass1.getUniform("gPositionDepth"), 0);
	glUniform1i(ssaoPass1.getUniform("gNormal"), 1);
	glUniform1i(ssaoPass1.getUniform("gDepth"), 6);
	glUniform1i(ssaoPass1.getUniform("texNoise"), 3);

	auto uSSAOPass1Samples = ssaoPass1.getUniform("samples", Shader::MANDITORY);
	auto uSSAOPass1Projection = ssaoPass1.getUniform("projection", Shader::MANDITORY);

	Shader_Program ssaoPass2;
	ssaoPass2.add("shaders/lighting.v.glsl", Shader::VERTEX);
	ssaoPass2.add("shaders/ssao-pass2.f.glsl", Shader::FRAGMENT);
	ssaoPass2.compile();
	ssaoPass2.link();
	ssaoPass2.use();

	glUniform1i(ssaoPass2.getUniform("ssaoInput", Shader::MANDITORY), 4);

	Shader_Program hdr_pass;

	hdr_pass.add("shaders/lighting.v.glsl", Shader::VERTEX);
	hdr_pass.add("shaders/hdr-pass.f.glsl", Shader::FRAGMENT);
	hdr_pass.compile();
	hdr_pass.link();
	hdr_pass.use();

	glUniform1i(hdr_pass.getUniform("inval", Shader::MANDITORY), 0);

	auto uHDRExposure = hdr_pass.getUniform("exposure");

	///////////////////////
	// Vertex Array Prep //
	///////////////////////

	GLuint Monkey_VAO, Monkey_VBO;
	glGenVertexArrays(1, &Monkey_VAO);
	glBindVertexArray(Monkey_VAO);

	glGenBuffers(1, &Monkey_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, Monkey_VBO);
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

	// World
	GLuint World_VAO, World_VBO;
	glGenVertexArrays(1, &World_VAO);
	glBindVertexArray(World_VAO);

	glGenBuffers(1, &World_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, World_VBO);
	glBufferData(GL_ARRAY_BUFFER, worldfile.objects[0].vertices.size() * sizeof(Vertex),
	             worldfile.objects[0].vertices.data(), GL_STATIC_DRAW);

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

	/////////////////////
	// Prepare gBuffer //
	/////////////////////

	RenderInfo reninfo;
	PrepareBuffers(WINDOW_WIDTH, WINDOW_HEIGHT, reninfo);

	//////////////////////
	// SSAO Sample Prep //
	//////////////////////

	std::vector<glm::vec3> ssaoKernel;
	ssaoKernel.reserve(64);
	std::vector<glm::vec3> ssaoNoise;
	ssaoNoise.reserve(16);
	{
		std::uniform_real_distribution<float> unitFloats(0.0, 1.0);
		std::uniform_real_distribution<float> negFloats(-1.0, 1.0);
		for (std::size_t i = 0; i < 64; ++i) {
			glm::vec3 sample(negFloats(prng), negFloats(prng), unitFloats(prng));
			sample = glm::normalize(sample);
			sample *= unitFloats(prng);
			float scale = static_cast<float>(i) / 64.0;

			scale = lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			ssaoKernel.push_back(sample);
		}

		for (std::size_t i = 0; i < 16; ++i) {
			ssaoNoise.emplace_back(negFloats(prng), negFloats(prng), 0.0f);
		}
	}

	glGenTextures(1, &reninfo.ssaoNoiseTexture);
	glBindTexture(GL_TEXTURE_2D, reninfo.ssaoNoiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	ssaoPass1.use();
	glUniform3fv(uSSAOPass1Samples, 64, glm::value_ptr(ssaoKernel[0]));

	///////////////
	// Game Loop //
	///////////////

	bool forward = false;
	bool SSAO = true;
	bool dynamic_lighting = true;
	bool loop = true;
	bool fullscreen = false, gotmouse = true;
	std::unordered_map<SDL_Keycode, bool> keys;
	float exposure = 1.0;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	float mouseX = 0, mouseY = 0;
	float mouseLX = 0, mouseLY = 0;
	float mouseDX = 0, mouseDY = 0;
	(void) mouseLX;
	(void) mouseLY;

	FPS_Meter fps(true, 2);
	Camera cam(glm::vec3(0, 10, 25));
	cam.set_rotation(30, 0);

	///////////////
	// Game Loop //
	///////////////

	while (loop) {
		int mousePixelX, mousePixelY;
		SDL_GetRelativeMouseState(&mousePixelX, &mousePixelY);

		mouseDX = (mousePixelX / (float) sdlm.size.height);
		mouseDY = (mousePixelY / (float) sdlm.size.height);

		mouseX = mouseX + mouseDX;
		mouseY = mouseY + mouseDY;

		mouseLX = std::exchange(mouseX, mouseX + mouseDX);
		mouseLY = std::exchange(mouseX, mouseY + mouseDY);

		if (mouseDX || mouseDY) {
			cam.rotate(mouseDY, mouseDX, 50);
		}

		fps.frame(0);

		const float cameraSpeed = 5.0f * fps.get_delta_time();

		// Event Handling
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					loop = false;
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						projection = Resize(sdlm, reninfo);
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							loop = false;
							break;
						case SDLK_F10:
							if (fullscreen) {
								SDL_SetWindowFullscreen(sdlm.mainWindow, 0);
								fullscreen = false;
							}
							else {
								SDL_SetWindowFullscreen(sdlm.mainWindow,
								                        SDL_WINDOW_FULLSCREEN_DESKTOP);
								fullscreen = true;
							}
							break;
						case SDLK_0:
							//remove_light(lightcount);
							break;
						case SDLK_RIGHTBRACKET:
							//create_light(10);
							break;
						case SDLK_LEFTBRACKET:
							//remove_light(10);
							break;
						case SDLK_EQUALS:
							//create_light(1);
							break;
						case SDLK_MINUS:
							//remove_light(1);
							break;
						case SDLK_LALT:
						case SDLK_RALT:
							if (gotmouse) {
								SDL_SetRelativeMouseMode(SDL_FALSE);
							}
							else {
								SDL_SetRelativeMouseMode(SDL_TRUE);
							}
							gotmouse = !gotmouse;
							break;
						case SDLK_m:
							if (forward) {
								std::cerr << "Enabling deferred rendering.\n";
								forward = false;
							}
							else {
								std::cerr << "Enabling forward rendering.\n";
								forward = true;
							}
							break;
						case SDLK_n:
							if (SSAO) {
								std::cerr << "Disabiling SSAO\n";
								SSAO = false;
							}
							else {
								std::cerr << "Enabling SSAO\n";
								SSAO = true;
							}
							break;
						case SDLK_b:
							if (dynamic_lighting) {
								std::cerr << "Disabiling dynamic lighting\n";
								dynamic_lighting = false;
							}
							else {
								std::cerr << "Enabling dynamic lighting\n";
								dynamic_lighting = true;
							}
							break;
						default:
							break;
					}
					keys[event.key.keysym.sym] = true;
					break;
				case SDL_KEYUP:
					keys[event.key.keysym.sym] = false;
					break;
			}
		}

		if (keys[SDLK_w]) {
			cam.move(glm::vec3(0, 0, cameraSpeed));
		}
		if (keys[SDLK_s]) {
			cam.move(glm::vec3(0, 0, -cameraSpeed));
		}
		if (keys[SDLK_a]) {
			cam.move(glm::vec3(-cameraSpeed, 0, 0));
		}
		if (keys[SDLK_d]) {
			cam.move(glm::vec3(cameraSpeed, 0, 0));
		}
		if (keys[SDLK_LSHIFT]) {
			cam.move(glm::vec3(0, cameraSpeed, 0));
		}
		if (keys[SDLK_LCTRL]) {
			cam.move(glm::vec3(0, -cameraSpeed, 0));
		}		

		///////////////////
		// Geometry Pass //
		///////////////////

		// Use geometry pass shaders
		geometrypass.use();

		// Update matrix uniforms
		glUniformMatrix4fv(uGeoWorld, 1, GL_FALSE, glm::value_ptr(monkey_world));
		glUniformMatrix4fv(uGeoView, 1, GL_FALSE, glm::value_ptr(cam.get_matrix()));
		glUniformMatrix4fv(uGeoProjection, 1, GL_FALSE, glm::value_ptr(projection));

		// Bind gBuffer in order to write to it
		glBindFramebuffer(GL_FRAMEBUFFER, reninfo.gBuffer);

		// Clear the gBuffer
		glClearColor(0, 0, 0, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use normal depth function
		glDepthFunc(GL_LESS);

		// Bind monkey vertex data
		glBindVertexArray(Monkey_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, Monkey_VBO);

		// Draw elements on the gBuffer
		glDrawArrays(GL_TRIANGLES, 0, file.objects[0].vertices.size());

		// Bind world vertex data
		glBindVertexArray(World_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, World_VBO);

		glUniformMatrix4fv(uGeoWorld, 1, GL_FALSE, glm::value_ptr(world_world));

		glDrawArrays(GL_TRIANGLES, 0, worldfile.objects[0].vertices.size());

		// Unbind arrays
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Unbind framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		////////////////
		// Depth Blit //
		////////////////

		// Blit depth pass to light buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, reninfo.gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reninfo.lBuffer);

		glBlitFramebuffer(0, 0, sdlm.size.width, sdlm.size.height, 0, 0, sdlm.size.width,
			                sdlm.size.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		if (SSAO) {
			// Blit depth pass to ssao buffer
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reninfo.ssaoBuffer);

			glBlitFramebuffer(0, 0, sdlm.size.width, sdlm.size.height, 0, 0, sdlm.size.width,
				                sdlm.size.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, reninfo.lBuffer);

		// Bind the buffers
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, reninfo.gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, reninfo.gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, reninfo.gAlbedoSpec);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, reninfo.ssaoNoiseTexture);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, reninfo.ssaoColor);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, reninfo.ssaoBlurColor);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, reninfo.gDepth);

		///////////////
		// SSAO Pass //
		///////////////

		if (SSAO) {
			glBindFramebuffer(GL_FRAMEBUFFER, reninfo.ssaoBuffer);

			glClearColor(1.0, 1.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);

			ssaoPass1.use();

			glDepthFunc(GL_GREATER);
			glDepthMask(GL_FALSE);

			glUniformMatrix4fv(uSSAOPass1Projection, 1, GL_FALSE, glm::value_ptr(projection));

			RenderFullscreenQuad();

			ssaoPass2.use();

			glBindFramebuffer(GL_FRAMEBUFFER, reninfo.ssaoBlurBuffer);
			glClear(GL_COLOR_BUFFER_BIT);

			RenderFullscreenQuad();
		}
		else {
			glBindFramebuffer(GL_FRAMEBUFFER, reninfo.ssaoBlurBuffer);

			glClearColor(1.0, 1.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, reninfo.lBuffer);

		///////////////////
		// Lighting Pass //
		///////////////////

		// Clear color
		glClearColor(0.118, 0.428, 0.860, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		lightingpass.use();

		// Fire the fragment shader if there is an object in front of the square
		// The square is drawn at the very back
		glDepthFunc(GL_GREATER);
		glDepthMask(GL_FALSE);

		// Upload current view position
		glUniform3fv(uLightViewPos, 1, glm::value_ptr(cam.get_location()));

		// Render a quad
		RenderFullscreenQuad();

		glDepthMask(GL_TRUE);

		//////////////////////////////////
		// Calculate Per Light Lighting //
		//////////////////////////////////

		if (dynamic_lighting) {
			lightbound.use();

			glBindVertexArray(lights::Light_VAO);

			glUniformMatrix4fv(uLightBoundPerspective, 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(uLightBoundView, 1, GL_FALSE, glm::value_ptr(cam.get_matrix()));
			glUniform3fv(uLightBoundViewPos, 1, glm::value_ptr(cam.get_location()));
			glUniform2f(uLightBoundResolution, sdlm.size.width, sdlm.size.height);

			glDepthFunc(GL_LESS);
			glDepthMask(GL_FALSE);

			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			glEnable(GL_STENCIL_TEST);

			glDisableVertexAttribArray(0);
			glEnableVertexAttribArray(7);

			for (size_t i = 0; i < lights::lightcount; ++i) {
				glClear(GL_STENCIL_BUFFER_BIT);

				glUniformMatrix4fv(uLightBoundWorld, 1, GL_FALSE,
					                glm::value_ptr(lights::lighteffectworldmatrix[i]));
				glUniform3fv(uLightBoundLightColor, 1, glm::value_ptr(lights::lightcolor[i]));
				glUniform3fv(uLightBoundLightPosition, 1, glm::value_ptr(lights::lightdata[i].position));
				glUniform1f(uLightBoundRadius, lights::lightdata[i].size);

				// Front (near) faces only
				// Colour write is disabled
				// Z-write is disabled
				// Z function is 'Less/Equal'
				// Z-Fail writes non-zero value to Stencil buffer (for example,
				// 'Increment-Saturate')
				// Stencil test result does not modify Stencil buffer

				glCullFace(GL_BACK);
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glDepthMask(GL_FALSE);
				glDepthFunc(GL_LEQUAL);
				glStencilMask(GL_TRUE);
				glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
				glStencilFunc(GL_ALWAYS, 0, 0xFF);

				glDrawArrays(GL_TRIANGLES, 0, lights::circlefile.objects[0].vertices.size());

				// Back (far) faces only
				// Colour write enabled
				// Z-write is disabled
				// Z function is 'Greater/Equal'
				// Stencil function is 'Equal' (Stencil ref = zero)
				// Always clears Stencil to zero

				glCullFace(GL_FRONT);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				// Z-write already disabled
				glDepthFunc(GL_GEQUAL);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				glStencilFunc(GL_EQUAL, 0, 0x00);

				glDrawArrays(GL_TRIANGLES, 0, lights::circlefile.objects[0].vertices.size());
			}

			glDisableVertexAttribArray(7);
			glEnableVertexAttribArray(0);

			glCullFace(GL_BACK);
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LEQUAL);
			glStencilFunc(GL_ALWAYS, 0, 0xFF);
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_BLEND);
		}

		// Blit depth pass to current depth
		glBindFramebuffer(GL_READ_FRAMEBUFFER, reninfo.gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, reninfo.lBuffer);

		glBlitFramebuffer(0, 0, sdlm.size.width, sdlm.size.height, 0, 0, sdlm.size.width,
			                sdlm.size.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, reninfo.lBuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		////////////////////////////
		// HDR/Gamma Post Process //
		////////////////////////////

		// Average color
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, reninfo.lColor);
		glGenerateMipmap(GL_TEXTURE_2D);

		size_t mipmap_levels =
		    1 + std::floor(std::log2(std::max(sdlm.size.width, sdlm.size.height)));
		glm::vec3 avg;
		glGetTexImage(GL_TEXTURE_2D, mipmap_levels - 1, GL_RGB, GL_FLOAT, glm::value_ptr(avg));

		// Change exposure
		float luminosity = 0.21 * avg.r + 0.71 * avg.g + 0.07 * avg.b;
		float newexposure = 1.0 / (luminosity + (1.0 - 0.4));
		float diff = newexposure - exposure;
		if (diff < 0) {
			exposure += (diff * fps.get_delta_time()) / 0.5;
		}
		else {
			exposure += std::min<float>(diff, 0.2 * fps.get_delta_time());
		}

#ifdef DLDEBUG
// std::cerr << luminosity << " - " << (1.0 / exposure) - (1.0 - 0.3) << '\n';
#endif

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		hdr_pass.use();

		glUniform1f(uHDRExposure, exposure);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);

		RenderFullscreenQuad();

		glEnable(GL_DEPTH_TEST);

		// Swap buffers
		SDL_GL_SwapWindow(sdlm.mainWindow);
	}

	return 0;
}

void APIENTRY openglCallbackFunction(GLenum source, GLenum type, GLuint id, GLenum severity,
                                     GLsizei length, const GLchar* message, const void* userParam) {

	static std::size_t error_num = 0;

	(void) source;
	(void) length;
	(void) userParam;

	std::cerr << '\n'
	          << error_num << " > ---------------------opengl-callback-start------------" << '\n';
	std::cerr << "message: " << message << '\n';
	std::cerr << "type: ";
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			std::cerr << "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			std::cerr << "DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			std::cerr << "UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			std::cerr << "PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			std::cerr << "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER:
			std::cerr << "OTHER";
			break;
	}
	std::cerr << '\n';

	std::cerr << "id: " << id << '\n';
	std::cerr << "severity: ";
	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:
			std::cerr << "LOW";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			std::cerr << "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_HIGH:
			std::cerr << "HIGH";
			break;
	}
	std::cerr << '\n';
	std::cerr << "---------------------opengl-callback-end--------------" << '\n';

	++error_num;
}

// RenderQuad() Renders a 1x1 quad in NDC, best used for framebuffer color
// targets
// and post-processing effects.
GLuint quadVAO = 0;
GLuint quadVBO;
void RenderFullscreenQuad() {
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*) 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		                      (GLvoid*) (3 * sizeof(GLfloat)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void Check_RenderBuffer() {
	auto fberr = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fberr != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer not complete!\n";
		switch (fberr) {
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				std::cerr << "Framebuffer incomplete attachment\n";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				std::cerr << "Framebuffer incomplete dimentions\n";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				std::cerr << "Framebuffer missing attachment\n";
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				std::cerr << "Framebuffer unsupported\n";
				break;
		}
		throw std::runtime_error("Framebuffer incomplete");
	}
}

void PrepareBuffers(size_t x, size_t y, RenderInfo& data) {
	glGenFramebuffers(1, &data.gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, data.gBuffer);

	// - Position color buffer
	glGenTextures(1, &data.gPosition);
	glBindTexture(GL_TEXTURE_2D, data.gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, x, y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data.gPosition, 0);

	// - Normal color buffer
	glGenTextures(1, &data.gNormal);
	glBindTexture(GL_TEXTURE_2D, data.gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, x, y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, data.gNormal, 0);

	// - Color + Specular color buffer
	glGenTextures(1, &data.gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, data.gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, data.gAlbedoSpec,
	                       0);

	// - Depth buffer
	glGenTextures(1, &data.gDepth);
	glBindTexture(GL_TEXTURE_2D, data.gDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, x, y, 0, GL_DEPTH_STENCIL,
	             GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, data.gDepth,
	                       0);

	// - Tell OpenGL which color attachments we'll use (of this framebuffer) for
	// rendering
	constexpr GLuint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
	                                   GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, attachments);

	Check_RenderBuffer();

	///////////////////
	// Light Buffers //
	///////////////////

	glGenFramebuffers(1, &data.lBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, data.lBuffer);

	// Light buffer
	glGenTextures(1, &data.lColor);
	glBindTexture(GL_TEXTURE_2D, data.lColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, x, y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data.lColor, 0);

	// Depth Buffer
	glGenTextures(1, &data.lDepth);
	glBindTexture(GL_TEXTURE_2D, data.lDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, x, y, 0, GL_DEPTH_STENCIL,
	             GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, data.lDepth, 0);

	Check_RenderBuffer();

	//////////////////
	// SSAO Buffers //
	//////////////////

	// First pass
	glGenFramebuffers(1, &data.ssaoBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, data.ssaoBuffer);

	glGenTextures(1, &data.ssaoColor);
	glBindTexture(GL_TEXTURE_2D, data.ssaoColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, x, y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data.ssaoColor, 0);

	glGenTextures(1, &data.ssaoDepth);
	glBindTexture(GL_TEXTURE_2D, data.ssaoDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, x, y, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
	             NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, data.ssaoDepth, 0);

	Check_RenderBuffer();

	// Second pass
	glGenFramebuffers(1, &data.ssaoBlurBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, data.ssaoBlurBuffer);

	glGenTextures(1, &data.ssaoBlurColor);
	glBindTexture(GL_TEXTURE_2D, data.ssaoBlurColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, x, y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data.ssaoBlurColor,
	                       0);

	Check_RenderBuffer();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeleteBuffers(RenderInfo& data) {
	glDeleteTextures(1, &data.gPosition);
	glDeleteTextures(1, &data.gNormal);
	glDeleteTextures(1, &data.gAlbedoSpec);
	glDeleteTextures(1, &data.gDepth);
	glDeleteTextures(1, &data.lColor);
	glDeleteTextures(1, &data.lDepth);
	glDeleteTextures(1, &data.ssaoColor);
	glDeleteTextures(1, &data.ssaoBlurColor);
	glDeleteFramebuffers(1, &data.gBuffer);
	glDeleteFramebuffers(1, &data.lBuffer);
	glDeleteFramebuffers(1, &data.ssaoBuffer);
	glDeleteFramebuffers(1, &data.ssaoBlurBuffer);
}

glm::mat4 Resize(SDL_Manager& sdlm, RenderInfo& data) {
	sdlm.refresh_size();
	DeleteBuffers(data);
	PrepareBuffers(sdlm.size.width, sdlm.size.height, data);
	return glm::perspective(glm::radians(60.0f), sdlm.size.ratio, 0.5f, 1000.0f);
}
