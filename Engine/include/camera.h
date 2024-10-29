#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL_events.h>

namespace SE {
	class Camera {
	public:
		explicit Camera(
			const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
			const glm::vec3& target = glm::vec3(0.0f, 0.0f, -1.0f),
			float fov = 70.0f,
			float aspectRatio = 16.0f / 9.0f
		);

		// Core functionality
		[[nodiscard]] const glm::mat4& getViewMatrix() const { return m_View; }
		[[nodiscard]] const glm::mat4& getProjectionMatrix() const { return m_Projection; }
		[[nodiscard]] const glm::mat4& getViewProjectionMatrix() const { return m_ViewProjection; }

		// Input handling
		void processKeyboard(const Uint8* state, float deltaTime);
		void handleEvent(const SDL_Event& event);

		// Window management
		void updateAspectRatio(float width, float height);

		// Getters
		[[nodiscard]] const glm::vec3& getPosition() const { return m_Position; }
		[[nodiscard]] float getYaw() const { return m_Yaw; }
		[[nodiscard]] float getPitch() const { return m_Pitch; }
		[[nodiscard]] float getFov() const { return m_Fov; }
		[[nodiscard]] float getAspectRatio() const { return m_AspectRatio; }

		// Setters
		void setPosition(const glm::vec3& position);
		void setTarget(const glm::vec3& target);
		void setFov(float fov);
		void setMovementSpeed(float speed) { m_MovementSpeed = std::max(speed, 0.0f); }
		void setMouseSensitivity(float sensitivity) { m_MouseSensitivity = std::max(sensitivity, 0.0f); }

	private:
		void updateVectors();
		void updateViewMatrix();
		void updateProjectionMatrix();
		glm::quat m_Orientation;
		// Core state
		glm::vec3 m_Position;
		float m_Yaw{ -90.0f };    // Rotation around Y-axis in degrees
		float m_Pitch{ 0.0f };    // Rotation around X-axis in degrees

		// Camera basis vectors - cached after rotation updates
		glm::vec3 m_Forward{ 0.0f, 0.0f, -1.0f };
		glm::vec3 m_Right{ 1.0f, 0.0f, 0.0f };
		glm::vec3 m_Up{ 0.0f, 1.0f, 0.0f };

		// Camera parameters
		float m_Fov;
		float m_AspectRatio;
		float m_NearPlane{ 1000.f };
		float m_FarPlane{ 0.1f };
		float m_MovementSpeed{ 5.0f };
		float m_MouseSensitivity{ 0.1f };

		glm::vec2 m_LastMousePos;

		// Input state
		bool m_IsRotating{ false };

		// Cached matrices
		glm::mat4 m_View{ 1.0f };
		glm::mat4 m_Projection{ 1.0f };
		glm::mat4 m_ViewProjection{ 1.0f };
	};
} // namespace SE
