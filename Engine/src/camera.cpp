#include "Camera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace SE {
	namespace {
		constexpr float PITCH_LIMIT = 89.0f;
		constexpr float ROTATION_SMOOTHING = 0.1f; // Apply rotation directly
		constexpr float MIN_DELTA = 0.0001f; // Minimum delta to consider for rotation
		constexpr float MAX_DELTA = 0.5f; // Maximum rotation per frame in radians (~28 degrees)

		float clampFov(float fov) {
			return std::clamp(fov, 1.0f, 179.0f);
		}

		float clampAspectRatio(float ratio) {
			return std::max(ratio, 0.1f);
		}

		// Helper function to sanitize rotation input
		void sanitizeRotationDelta(float& deltaYaw, float& deltaPitch) {
			// Optionally remove clamping
			deltaYaw = std::clamp(deltaYaw, -MAX_DELTA, MAX_DELTA);
			deltaPitch = std::clamp(deltaPitch, -MAX_DELTA, MAX_DELTA);

			// Filter out very small movements that could add noise
			if (std::abs(deltaYaw) < MIN_DELTA) deltaYaw = 0.0f;
			if (std::abs(deltaPitch) < MIN_DELTA) deltaPitch = 0.0f;
		}

		// Helper function to ensure quaternion stability
		glm::quat stabilizeQuaternion(const glm::quat& q) {
			glm::quat normalized = glm::normalize(q);

			// Ensure w component is positive to prevent sign flips
			if (normalized.w < 0.0f) {
				normalized = -normalized;
			}

			return normalized;
		}
	}

	Camera::Camera(const Editor::ViewportState* state, SDL_Window* window, const glm::vec3& position,
		const glm::vec3& target, float fov, float aspectRatio)
		: m_Position(position)
		, m_Fov(clampFov(fov))
		, m_AspectRatio(clampAspectRatio(aspectRatio))
		, m_ViewportState(state)
		, m_Window(window)
	{
		// Calculate initial yaw & pitch from target
		glm::vec3 direction = glm::normalize(target - position);
		m_Pitch = glm::degrees(asin(glm::clamp(direction.y, -1.0f, 1.0f)));
		m_Yaw = glm::degrees(atan2(direction.z, direction.x));

		// Initialize orientation quaternion
		m_Orientation = stabilizeQuaternion(
			glm::quat(glm::vec3(glm::radians(-m_Pitch), glm::radians(-m_Yaw), 0.0f))
		);

		updateVectors();
		updateProjectionMatrix();
	}

	void Camera::updateVectors() {
		// Use stabilized quaternion to update vectors
		m_Forward = glm::normalize(m_Orientation * glm::vec3(0.0f, 0.0f, -1.0f));
		m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
		m_Up = glm::normalize(glm::cross(m_Right, m_Forward));

		updateViewMatrix();
	}

	void Camera::updateViewMatrix() {
		// Create view matrix from stabilized orientation and position
		glm::mat4 rotationMatrix = glm::mat4_cast(glm::inverse(m_Orientation));
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -m_Position);
		m_View = rotationMatrix * translationMatrix;

		// Update view-projection matrix
		m_ViewProjection = m_Projection * m_View;
	}

	void Camera::updateProjectionMatrix() {
		m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
		m_Projection[1][1] *= -1; // Flip Y for Vulkan coordinate system
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
			if (state[SDL_SCANCODE_LSHIFT]) velocity *= 2.0f; // Sprint multiplier
			movement = glm::normalize(movement) * velocity;
			m_Position += movement;
			updateViewMatrix();
		}
	}

	bool Camera::isMouseInViewport(int mouseX, int mouseY) {
		if (!m_ViewportState) return true;

		int viewportX = static_cast<int>(m_ViewportState->position.x);
		int viewportY = static_cast<int>(m_ViewportState->position.y);
		int viewportWidth = static_cast<int>(m_ViewportState->viewportSize.x);
		int viewportHeight = static_cast<int>(m_ViewportState->viewportSize.y);

		return (mouseX >= viewportX && mouseX <= viewportX + viewportWidth &&
			mouseY >= viewportY && mouseY <= viewportY + viewportHeight);
	}

	void Camera::handleEvent(const SDL_Event& event, float deltaTime) {
		switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_RIGHT &&
				(!m_ViewportState || m_ViewportState->isHovered)) {
				m_IsRotating = true;
				m_LastMouseX = event.button.x;
				m_LastMouseY = event.button.y;
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			if (event.button.button == SDL_BUTTON_RIGHT && m_IsRotating) {
				m_IsRotating = false;
				if (m_ViewportState && m_Window) {
					SDL_WarpMouseInWindow(m_Window,
						m_ViewportState->viewportCenter.x,
						m_ViewportState->viewportCenter.y);
				}
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
			break;

		case SDL_MOUSEMOTION:
			if (m_IsRotating) {
				int mouseX, mouseY;
				SDL_GetMouseState(&mouseX, &mouseY);

				if (!isMouseInViewport(mouseX, mouseY)) {
					if (m_ViewportState && m_Window) {
						SDL_WarpMouseInWindow(m_Window,
							m_ViewportState->viewportCenter.x,
							m_ViewportState->viewportCenter.y);
					}
					return;
				}

				float deltaYaw = -event.motion.xrel * m_MouseSensitivity * deltaTime;
				float deltaPitch = -event.motion.yrel * m_MouseSensitivity * deltaTime;
				deltaYaw = glm::radians(deltaYaw);
				deltaPitch = glm::radians(deltaPitch);

				sanitizeRotationDelta(deltaYaw, deltaPitch);

				// Apply rotation with smoothing
				applyRotation(deltaYaw, deltaPitch);
			}
			break;
		}
	}

	void Camera::applyRotation(float deltaYaw, float deltaPitch) {
		// Apply rotations
		const float maxRotationSpeed = glm::radians(45.0f); // Max 45 degrees per frame
		deltaYaw = glm::clamp(deltaYaw, -maxRotationSpeed, maxRotationSpeed);
		deltaPitch = glm::clamp(deltaPitch, -maxRotationSpeed, maxRotationSpeed);

		// Apply rotations
		glm::quat pitchQuat = glm::angleAxis(deltaPitch, m_Right);
		glm::quat yawQuat = glm::angleAxis(deltaYaw, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::quat newOrientation = yawQuat * pitchQuat * m_Orientation;

		// Stabilize quaternion
		newOrientation = stabilizeQuaternion(newOrientation);

		// Update forward vector to calculate pitch angle
		glm::vec3 newForward = glm::normalize(newOrientation * glm::vec3(0.0f, 0.0f, -1.0f));
		float newPitch = glm::degrees(glm::asin(glm::clamp(newForward.y, -1.0f, 1.0f)));

		// Clamp pitch angle
		if (newPitch > PITCH_LIMIT) {
			deltaPitch = glm::radians(PITCH_LIMIT - m_Pitch);
			pitchQuat = glm::angleAxis(deltaPitch, m_Right);
			newOrientation = yawQuat * pitchQuat * m_Orientation;
		}
		else if (newPitch < -PITCH_LIMIT) {
			deltaPitch = glm::radians(-PITCH_LIMIT - m_Pitch);
			pitchQuat = glm::angleAxis(deltaPitch, m_Right);
			newOrientation = yawQuat * pitchQuat * m_Orientation;
		}

		// Stabilize quaternion
		m_Orientation = stabilizeQuaternion(newOrientation);

		// Update pitch and yaw
		m_Pitch += glm::degrees(deltaPitch);
		m_Yaw += glm::degrees(deltaYaw);

		// Update camera vectors
		updateVectors();
	}

	void Camera::rotateCamera(float rotX, float rotY) {
		sanitizeRotationDelta(rotX, rotY);
		applyRotation(rotX, rotY);
	}

	void Camera::updateAspectRatio(glm::vec2 viewportSize) {
		if (viewportSize.y > 0.0f) {
			m_AspectRatio = clampAspectRatio(viewportSize.x / viewportSize.y);
			updateProjectionMatrix();
		}
	}

	void Camera::setPosition(const glm::vec3& position) {
		m_Position = position;
		updateViewMatrix();
	}

	void Camera::setTarget(const glm::vec3& target) {
		glm::vec3 direction = glm::normalize(target - m_Position);

		// Calculate pitch and yaw from direction, with proper clamping
		m_Pitch = glm::degrees(std::asin(glm::clamp(direction.y, -1.0f, 1.0f)));
		m_Yaw = glm::degrees(std::atan2(direction.z, direction.x));

		// Create and stabilize the orientation quaternion
		m_Orientation = stabilizeQuaternion(
			glm::quat(glm::vec3(glm::radians(-m_Pitch), glm::radians(-m_Yaw), 0.0f))
		);

		updateVectors();
	}

	void Camera::setFov(float fov) {
		m_Fov = clampFov(fov);
		updateProjectionMatrix();
	}
} // namespace SE