#include <GL/glew.h>

#include "camera.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

void Camera::move(const glm::vec3& offset, float speed_mult) {
	position += offset.x * (right * (speed * speed_mult));
	position += offset.y * (up * (speed * speed_mult));
	position += offset.z * (front * (speed * speed_mult));

	recalculate_view_matrix();
}

void Camera::rotate(const float pitch_offset, const float yaw_offset, float speed_mult) {
	set_rotation(pitch + (pitch_offset * speed_mult), yaw + (yaw_offset * speed_mult));
}

void Camera::set_location(const glm::vec3& loc) {
	position = loc;

	recalculate_view_matrix();
}

void Camera::set_rotation(const float new_pitch, const float new_yaw) {
	pitch = std::max(std::min(new_pitch, 89.9f), -89.9f);
	yaw = new_yaw;
	// std::cerr << "Pitch: " << pitch << " Yaw: " << yaw << "\n";

	front.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	front.y = -sin(glm::radians(pitch));
	front.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	front = glm::normalize(front);
	// std::cerr << front.x << ", " << front.y << ", " << front.z << '\n';
	right = glm::normalize(glm::cross(front, up));

	recalculate_view_matrix();
}

void Camera::use(GLuint uniform) {
	glUniformMatrix4fv(uniform, 1, GL_FALSE, glm::value_ptr(view));
}

void Camera::recalculate_view_matrix() {
	view = glm::lookAt(position, position + front, up);
}

const glm::vec3& Camera::get_location() {
	return position;
}
const glm::vec3& Camera::get_front_vec() {
	return front;
}
const glm::vec3& Camera::get_up_vec() {
	return up;
}
const glm::vec3& Camera::get_right_vec() {
	return right;
}
const glm::mat4& Camera::get_matrix() {
	return view;
}
