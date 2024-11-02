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
	}

	Camera::Camera(const Editor::ViewportState* state, SDL_Window* window, const glm::vec3& position,
		const glm::vec3& target, float fov, float aspectRatio)
		: m_Position(position)
		, m_Fov(clampFov(fov))
		, m_AspectRatio(clampAspectRatio(state->availableSpace.x / state->availableSpace.y))
		, m_ViewportState(state)
		, m_Window(window)
	{
		glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 direction = target - m_Position;

		float dot = glm::dot(forward, direction);

		if (glm::abs(dot + 1.0f) < 0.000001f) {
			// Vector points exactly opposite, rotate 180 degrees around up vector
			m_Orientation = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else {
			// Create quaternion from the two vectors
			glm::vec3 rotationAxis = glm::cross(forward, direction);
			if (glm::length2(rotationAxis) < 0.000001f) {
				// Vectors are parallel, no rotation needed
				m_Orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			}
			else {
				float rotationAngle = std::acos(dot);
				m_Orientation = glm::angleAxis(rotationAngle, glm::normalize(rotationAxis));
			}
		}

		m_Orientation = glm::normalize(m_Orientation);

		updateVectors();
		updateProjectionMatrix();
	}

	void Camera::updateVectors() {
		// Extract basis vectors directly from quaternion
		m_Forward = glm::normalize(m_Orientation * glm::vec3(0.0f, 0.0f, -1.0f));  // Forward is -Z in view space
		m_Right = glm::normalize(m_Orientation * glm::vec3(1.0f, 0.0f, 0.0f));     // Right is +X in view space
		m_Up = glm::normalize(m_Orientation * glm::vec3(0.0f, 1.0f, 0.0f));        // Up is +Y in view space

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
		constexpr float maxRotationSpeed = glm::radians(45.0f);
		deltaYaw = glm::clamp(deltaYaw, -maxRotationSpeed, maxRotationSpeed);
		deltaPitch = glm::clamp(deltaPitch, -maxRotationSpeed, maxRotationSpeed);

		glm::quat yawQuat = glm::angleAxis(deltaYaw, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::quat pitchQuat = glm::angleAxis(deltaPitch, m_Right);

		glm::quat newOrientation = yawQuat * pitchQuat * m_Orientation;

		glm::vec3 newUp = glm::normalize(newOrientation * glm::vec3(0.0f, 1.0f, 0.0f));
		float upDot = glm::dot(newUp, glm::vec3(0.0f, 1.0f, 0.0f));

		if (upDot < 0.0f) {
			m_Orientation = glm::normalize(yawQuat * m_Orientation);
		}
		else {
			glm::vec3 newForward = glm::normalize(newOrientation * glm::vec3(0.0f, 0.0f, -1.0f));
			float pitch = glm::degrees(glm::asin(glm::clamp(newForward.y, -1.0f, 1.0f)));

			// Apply rotation based on pitch limits
			if (std::abs(pitch) <= PITCH_LIMIT) {
				m_Orientation = glm::normalize(newOrientation);
			}
			else {
				// Only apply yaw if we would exceed pitch limits
				m_Orientation = glm::normalize(yawQuat * m_Orientation);
			}
		}

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

		// Create rotation matrix that aligns camera with target
		glm::mat4 rotationMatrix = glm::lookAt(
			glm::vec3(0.0f),  // From origin
			direction,        // To direction
			glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
		);

		// Convert rotation matrix to quaternion
		m_Orientation = glm::normalize(glm::quat_cast(rotationMatrix));

		updateVectors();
	}

	void Camera::setFov(float fov) {
		m_Fov = clampFov(fov);
		updateProjectionMatrix();
	}
} // namespace SE