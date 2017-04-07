#pragma once

#include <SDL2/SDL.h>
#include <array>
#include <cinttypes>
#include <map>

namespace control {
	struct controller {
		int32_t orig_id;
		SDL_GameController* id = nullptr;
		SDL_Joystick* joyid = nullptr;
		int instance_id = 0;

		int16_t left_x = 0;
		int16_t left_y = 0;
		int16_t right_x = 0;
		int16_t right_y = 0;
		int16_t trigger_left = 0;
		int16_t trigger_right = 0;

		std::array<bool, 15> keys = {false};
	};

	struct controller_report {
		enum class direction { none, up, down, left, right };
		direction left_stick_dir;
		direction right_stick_dir;
		std::array<bool, 15> keys;
		bool active;
		bool ltrigger;
		bool rtrigger;
	};

	extern struct controller_manager {
		std::size_t joy_count = 0;
		controller players[4];
	} manager;

	using movement_report_type = std::array<controller_report, 4>;

	void initialize();
	void add_controller(const SDL_ControllerDeviceEvent& event);
	void remove_controller(const SDL_ControllerDeviceEvent& event);
	void controller_axis_movement(const SDL_ControllerAxisEvent& event);
	void controller_button_press(const SDL_ControllerButtonEvent& event);
	movement_report_type movement_report();
}