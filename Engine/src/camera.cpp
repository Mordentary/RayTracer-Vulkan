#include "Camera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm\gtx\quaternion.hpp>

namespace Engine {

	namespace {
		constexpr float PITCH_LIMIT = 89.0f;

		float clampFov(float fov) {
			return std::clamp(fov, 1.0f, 179.0f);
		}

		float clampAspectRatio(float ratio) {
			return std::max(ratio, 0.1f);
		}
	} // anonymous namespace

	Camera::Camera(const glm::vec3& position, const glm::vec3& target, float fov, float aspectRatio)
		: m_Position(position)
		, m_Fov(clampFov(fov))
		, m_AspectRatio(clampAspectRatio(aspectRatio))
	{
		// Calculate initial yaw & pitch from target
		glm::vec3 direction = glm::normalize(target - position);
		m_Pitch = glm::degrees(asin(direction.y));
		m_Yaw = glm::degrees(atan2(direction.z, direction.x));

		// Initialize orientation quaternion
		m_Orientation = glm::quat(glm::vec3(glm::radians(-m_Pitch), glm::radians(-m_Yaw), 0.0f));

		updateVectors();
		updateProjectionMatrix();
	}

	void Camera::updateVectors() {
		// Rotate the default direction vectors by the orientation quaternion
		m_Forward = glm::normalize(m_Orientation * glm::vec3(0.0f, 0.0f, -1.0f));
		m_Right = glm::normalize(m_Orientation * glm::vec3(1.0f, 0.0f, 0.0f));
		m_Up = glm::normalize(glm::cross(m_Right, m_Forward));

		updateViewMatrix();
	}

	void Camera::updateViewMatrix() {
		// Compute the inverse of the camera's world transform
		glm::mat4 rotationMatrix = glm::mat4_cast(glm::inverse(m_Orientation));
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -m_Position);
		m_View = rotationMatrix * translationMatrix;

	}

	void Camera::updateProjectionMatrix()
	{

		m_Projection = glm::perspective(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);

		m_Projection[1][1] *= -1;
		m_ViewProjection = m_Projection * m_View;
	}

	void Camera::processKeyboard(const Uint8* state, float deltaTime) {
		if (!state) return;
		glm::vec3 movement(0.0f);
		float velocity = m_MovementSpeed * deltaTime;

		// Accumulate movement in camera space
		if (state[SDL_SCANCODE_W]) movement += m_Forward;
		if (state[SDL_SCANCODE_S]) movement -= m_Forward;
		if (state[SDL_SCANCODE_A]) movement -= m_Right;
		if (state[SDL_SCANCODE_D]) movement += m_Right;
		if (state[SDL_SCANCODE_SPACE]) movement += glm::vec3(0.0f, 1.0f, 0.0f);
		if (state[SDL_SCANCODE_LCTRL]) movement -= glm::vec3(0.0f, 1.0f, 0.0f);

		if (glm::length2(movement) > 0.0f) {

			if (state[SDL_SCANCODE_LSHIFT]) velocity += 1.f;
			movement = glm::normalize(movement) * velocity;
			m_Position += movement;
			updateViewMatrix();
		}
	}

	void Camera::handleEvent(const SDL_Event& event) {
		switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_RIGHT) {
				m_IsRotating = true;
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			if (event.button.button == SDL_BUTTON_RIGHT) {
				m_IsRotating = false;
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
			break;

		case SDL_MOUSEMOTION:
			if (m_IsRotating) {
				float deltaYaw = -event.motion.xrel * m_MouseSensitivity;
				float deltaPitch = -event.motion.yrel * m_MouseSensitivity;

				// Convert to radians
				deltaYaw = glm::radians(deltaYaw);
				deltaPitch = glm::radians(deltaPitch);

				// Create quaternions for pitch and yaw rotations
				glm::quat yawQuat = glm::angleAxis(deltaYaw, glm::vec3(0.0f, 1.0f, 0.0f));
				glm::quat pitchQuat = glm::angleAxis(deltaPitch, m_Right);

				// Combine rotations
				glm::quat newOrientation = glm::normalize(pitchQuat * yawQuat * m_Orientation);

				// Calculate new forward vector
				glm::vec3 newForward = glm::normalize(newOrientation * glm::vec3(0.0f, 0.0f, -1.0f));
				float pitchAngle = glm::degrees(asin(newForward.y));

				// Clamp pitch to prevent flipping
				if (pitchAngle > PITCH_LIMIT || pitchAngle < -PITCH_LIMIT) {
					// Do not apply pitch rotation beyond limits
					newOrientation = glm::normalize(yawQuat * m_Orientation);
				}

				m_Orientation = newOrientation;

				updateVectors();
			}
			break;
		}
	}

	void Camera::updateAspectRatio(float width, float height) {
		if (height > 0.0f) {
			m_AspectRatio = clampAspectRatio(width / height);
			updateProjectionMatrix();
		}
	}

	void Camera::setPosition(const glm::vec3& position) {
		m_Position = position;
		updateViewMatrix();
	}

	void Camera::setTarget(const glm::vec3& target) {
		glm::vec3 direction = glm::normalize(target - m_Position);
		m_Pitch = glm::degrees(asin(direction.y));
		m_Yaw = glm::degrees(atan2(direction.z, direction.x));

		// Update orientation quaternion
		m_Orientation = glm::quat(glm::vec3(glm::radians(-m_Pitch), glm::radians(-m_Yaw), 0.0f));

		updateVectors();
	}

	void Camera::setFov(float fov) {
		m_Fov = clampFov(fov);
		updateProjectionMatrix();
	}

} // namespace Engine
