#include "controller.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <iostream>
#include <iterator>

control::controller_manager control::manager;

void control::initialize() {
	return;
	std::size_t joysticks = SDL_NumJoysticks();
	manager.joy_count = 0;
	for (std::size_t i = 0; i < joysticks; ++i) {
		if (SDL_IsGameController(i)) {
			SDL_GameController* gpad = SDL_GameControllerOpen(i);
			SDL_Joystick* joy = SDL_GameControllerGetJoystick(gpad);
			int instance_id = SDL_JoystickInstanceID(joy);

			if (manager.joy_count < 4) {
				auto&& player = manager.players[manager.joy_count];
				player.id = gpad;
				player.joyid = joy;
				player.instance_id = instance_id;
			}
			manager.joy_count++;
		};
	}
}

void control::add_controller(const SDL_ControllerDeviceEvent& event) {
	SDL_GameController* gpad = SDL_GameControllerOpen(event.which);
	SDL_Joystick* joy = SDL_GameControllerGetJoystick(gpad);
	int instance_id = SDL_JoystickInstanceID(joy);

	// Check if any controller is inactive
	auto found_itr = std::find_if(std::begin(manager.players), std::end(manager.players),
	                              [&](auto a) { return a.id == nullptr; });
	bool found = found_itr != std::end(manager.players);

	if (found) {
		auto&& player = *found_itr;
		player.orig_id = event.which;
		player.id = gpad;
		player.joyid = joy;
		player.instance_id = instance_id;
	}
	else {
		SDL_GameControllerClose(gpad);
	}
}

void control::remove_controller(const SDL_ControllerDeviceEvent& event) {
	// Find controller
	auto found_itr = std::find_if(std::begin(manager.players), std::end(manager.players),
	                              [&](auto a) { return a.orig_id == event.which; });

	bool found = found_itr != std::end(manager.players);

	if (found) {
		SDL_GameControllerClose(found_itr->id);
		*found_itr = controller{};
	}
}

void control::controller_axis_movement(const SDL_ControllerAxisEvent& event) {
	// find controller
	auto found_itr = std::find_if(std::begin(manager.players), std::end(manager.players),
	                              [&](auto a) { return a.orig_id == event.which; });

	switch (event.axis) {
		case SDL_CONTROLLER_AXIS_LEFTX:
			found_itr->left_x = event.value;
			break;
		case SDL_CONTROLLER_AXIS_LEFTY:
			found_itr->left_y = event.value;
			break;
		case SDL_CONTROLLER_AXIS_RIGHTX:
			found_itr->right_x = event.value;
			break;
		case SDL_CONTROLLER_AXIS_RIGHTY:
			found_itr->right_y = event.value;
			break;
		case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
			found_itr->trigger_left = event.value;
			break;
		case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
			found_itr->trigger_right = event.value;
			break;
	}

	// std::cerr << "LX: " << found_itr->left_x << ' '          //
	//          << "LY: " << found_itr->left_y << ' '          //
	//          << "RX: " << found_itr->right_x << ' '         //
	//          << "RY: " << found_itr->right_y << ' '         //
	//          << "TL: " << found_itr->trigger_left << ' '    //
	//          << "TR: " << found_itr->trigger_right << '\n'; //
}

void control::controller_button_press(const SDL_ControllerButtonEvent& event) {
	// find controller
	auto found_itr = std::find_if(std::begin(manager.players), std::end(manager.players),
	                              [&](auto a) { return a.orig_id == event.which; });

	bool down = event.state == SDL_PRESSED;

	switch (event.button) {
		case SDL_CONTROLLER_BUTTON_A:
			found_itr->keys[0] = down;
			break;
		case SDL_CONTROLLER_BUTTON_B:
			found_itr->keys[1] = down;
			break;
		case SDL_CONTROLLER_BUTTON_X:
			found_itr->keys[2] = down;
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			found_itr->keys[3] = down;
			break;
		case SDL_CONTROLLER_BUTTON_BACK:
			found_itr->keys[4] = down;
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			found_itr->keys[5] = down;
			break;
		case SDL_CONTROLLER_BUTTON_START:
			found_itr->keys[6] = down;
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSTICK:
			found_itr->keys[7] = down;
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
			found_itr->keys[8] = down;
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			found_itr->keys[9] = down;
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			found_itr->keys[10] = down;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			found_itr->keys[11] = down;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			found_itr->keys[12] = down;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			found_itr->keys[13] = down;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			found_itr->keys[14] = down;
			break;
	}

	// std::cerr << "KeyReport:\n";
	// std::size_t i = 0;
	// for (bool k : found_itr->keys) {
	// 	std::cerr << i++ << ": " << k << '\n';
	// }
}

control::movement_report_type control::movement_report() {
	constexpr auto joystick_deadzone = 3000;

	std::array<control::controller_report, 4> report;

	for (std::size_t i = 0; i < 4; ++i) {
		auto&& player = manager.players[i];
		auto&& report_section = report[i];

		if (std::abs(player.left_x) < joystick_deadzone &&
		    std::abs(player.left_y) < joystick_deadzone) {
			report_section.left_stick_dir = controller_report::direction::none;
		}
		else {
			if (std::abs(player.left_x) > std::abs(player.left_y)) {
				if (player.left_x > 0) {
					report_section.left_stick_dir = controller_report::direction::right;
				}
				else {
					report_section.left_stick_dir = controller_report::direction::left;
				}
			}
			else {
				if (player.left_y > 0) {
					report_section.left_stick_dir = controller_report::direction::down;
				}
				else {
					report_section.left_stick_dir = controller_report::direction::up;
				}
			}
		}

		if (std::abs(player.right_x) < joystick_deadzone &&
		    std::abs(player.right_y) < joystick_deadzone) {
			report_section.right_stick_dir = controller_report::direction::none;
		}
		else {
			if (std::abs(player.right_x) > std::abs(player.right_y)) {
				if (player.right_x > 0) {
					report_section.right_stick_dir = controller_report::direction::right;
				}
				else {
					report_section.right_stick_dir = controller_report::direction::left;
				}
			}
			else {
				if (player.right_y > 0) {
					report_section.right_stick_dir = controller_report::direction::down;
				}
				else {
					report_section.right_stick_dir = controller_report::direction::up;
				}
			}
		}

		report_section.ltrigger = std::abs(player.trigger_left) > joystick_deadzone;
		report_section.rtrigger = std::abs(player.trigger_right) > joystick_deadzone;

		report_section.active = player.id != nullptr;

		report_section.keys = player.keys;

		// if (i != 0) {
		// 	continue;
		// }

		// std::cerr << "LDir: ";
		// switch (report_section.left_stick_dir) {
		// 	case controller_report::direction::none:
		// 		std::cerr << "none";
		// 		break;
		// 	case controller_report::direction::right:
		// 		std::cerr << "right";
		// 		break;
		// 	case controller_report::direction::left:
		// 		std::cerr << "left";
		// 		break;
		// 	case controller_report::direction::up:
		// 		std::cerr << "up";
		// 		break;
		// 	case controller_report::direction::down:
		// 		std::cerr << "down";
		// 		break;
		// }

		// std::cerr << " RDir: ";
		// switch (report_section.right_stick_dir) {
		// 	case controller_report::direction::none:
		// 		std::cerr << "none";
		// 		break;
		// 	case controller_report::direction::right:
		// 		std::cerr << "right";
		// 		break;
		// 	case controller_report::direction::left:
		// 		std::cerr << "left";
		// 		break;
		// 	case controller_report::direction::up:
		// 		std::cerr << "up";
		// 		break;
		// 	case controller_report::direction::down:
		// 		std::cerr << "down";
		// 		break;
		// }

		// std::cerr << " LT: " << report_section.ltrigger;
		// std::cerr << " RT: " << report_section.rtrigger;
		// std::cerr << '\n';
	}

	return report;
}
