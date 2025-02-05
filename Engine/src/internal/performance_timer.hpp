#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <deque>
#include <stack>
#include <numeric>
#include <imgui.h>

namespace SE {
	class Timer {
	public:
		struct TimerMeasurement {
			std::string name;
			double startTime;
			double duration;
			ImVec4 color;
			std::vector<TimerMeasurement> children;
			TimerMeasurement* parent;

			TimerMeasurement() : startTime(0), duration(0), parent(nullptr) {}
		};

		struct FrameData {
			TimerMeasurement root;
			double totalFrameTime;
		};

		static Timer& getInstance() {
			static Timer instance;
			return instance;
		}

		void beginFrame() {
			// Calculate delta time
			auto currentTime = std::chrono::high_resolution_clock::now();
			m_DeltaTime = std::chrono::duration<double>(currentTime - m_LastFrameTime).count();
			m_LastFrameTime = currentTime;

			// Existing frame initialization code...
			m_CurrentFrame.root = TimerMeasurement();
			m_CurrentFrame.root.name = "Frame";
			m_CurrentFrame.root.startTime = getCurrentTime();
			m_CurrentFrame.root.color = ImVec4(1, 1, 1, 1);
			m_ActiveStack.push(&m_CurrentFrame.root);
		}

		void endFrame() {
			if (!m_ActiveStack.empty()) {
				m_ActiveStack.top()->duration = getCurrentTime() - m_ActiveStack.top()->startTime;
				m_ActiveStack.pop();
			}

			m_CurrentFrame.totalFrameTime = m_CurrentFrame.root.duration;
			m_FrameHistory.push_back(m_CurrentFrame);
			if (m_FrameHistory.size() > m_MaxFrameHistory) {
				m_FrameHistory.pop_front();
			}

			// Update statistics
			if (m_CurrentFrame.totalFrameTime < m_MinFrameTime) m_MinFrameTime = m_CurrentFrame.totalFrameTime;
			if (m_CurrentFrame.totalFrameTime > m_MaxFrameTime) m_MaxFrameTime = m_CurrentFrame.totalFrameTime;

			m_FrameTimeSum += m_CurrentFrame.totalFrameTime;
			m_FrameTimeHistory.push_back(m_CurrentFrame.totalFrameTime);
			if (m_FrameTimeHistory.size() > m_AverageWindow) {
				m_FrameTimeSum -= m_FrameTimeHistory.front();
				m_FrameTimeHistory.pop_front();
			}
		}

		void beginSection(const std::string& name, const ImVec4& color = ImVec4(1, 1, 1, 1)) {
			if (m_ActiveStack.empty()) return;

			TimerMeasurement measurement;
			measurement.name = name;
			measurement.startTime = getCurrentTime();
			measurement.color = color;
			measurement.parent = m_ActiveStack.top();

			m_ActiveStack.top()->children.push_back(measurement);
			m_ActiveStack.push(&m_ActiveStack.top()->children.back());
		}

		void endSection(const std::string& name) {
			if (!m_ActiveStack.empty()) {
				m_ActiveStack.top()->duration = getCurrentTime() - m_ActiveStack.top()->startTime;
				m_ActiveStack.pop();
			}
		}

		void drawImGuiWindow(bool* p_open = nullptr) {
			if (ImGui::Begin("Performance Timer", p_open)) {
				// Frame time graph
				if (!m_FrameHistory.empty()) {
					std::vector<float> frameTimes;
					frameTimes.reserve(m_FrameHistory.size());
					for (const auto& frame : m_FrameHistory) {
						frameTimes.push_back(static_cast<float>(frame.totalFrameTime));
					}

					ImGui::PlotLines("Frame Times", frameTimes.data(), static_cast<int>(frameTimes.size()),
						0, "ms", m_MinFrameTime, m_MaxFrameTime, ImVec2(0, 80));
				}

				// Statistics
				ImGui::Text("Frame Time: %.2f ms", getLastFrameTime());
				ImGui::Text("Average (%.1f frames): %.2f ms", static_cast<float>(m_AverageWindow), getAverageFrameTime());
				ImGui::Text("Min: %.2f ms", m_MinFrameTime);
				ImGui::Text("Max: %.2f ms", m_MaxFrameTime);

				ImGui::Separator();

				// Hierarchical view of current frame
				if (!m_FrameHistory.empty()) {
					const auto& lastFrame = m_FrameHistory.back();
					drawHierarchy(lastFrame.root);
				}
			}
			ImGui::End();
		}

		void reset() {
			m_FrameHistory.clear();
			m_FrameTimeHistory.clear();
			m_FrameTimeSum = 0.0;
			m_MinFrameTime = std::numeric_limits<double>::max();
			m_MaxFrameTime = 0.0;
			while (!m_ActiveStack.empty()) m_ActiveStack.pop();
		}

		double getLastFrameTime() const {
			return m_FrameHistory.empty() ? 0.0 : m_FrameHistory.back().totalFrameTime;
		}

		double getDeltaTime() const { return m_DeltaTime; }

		double getAverageFrameTime() const {
			return m_FrameTimeHistory.empty() ? 0.0 : m_FrameTimeSum / m_FrameTimeHistory.size();
		}

	private:
		Timer() : m_MaxFrameHistory(300), m_AverageWindow(60),
			m_MinFrameTime(std::numeric_limits<double>::max()),
			m_MaxFrameTime(0.0), m_FrameTimeSum(0.0) {
		}

		Timer(const Timer&) = delete;
		Timer& operator=(const Timer&) = delete;

		void drawHierarchy(const TimerMeasurement& measurement, int depth = 0) {
			ImGui::PushStyleColor(ImGuiCol_Text, measurement.color);

			bool isOpen = false;
			if (measurement.children.empty()) {
				ImGui::Indent(depth * 20.0f);
				ImGui::Text("%s: %.3f ms", measurement.name.c_str(), measurement.duration);
				ImGui::Unindent(depth * 20.0f);
			}
			else {
				ImGui::Indent(depth * 20.0f);
				isOpen = ImGui::TreeNode(measurement.name.c_str(), "%s: %.3f ms",
					measurement.name.c_str(), measurement.duration);
				ImGui::Unindent(depth * 20.0f);
			}

			ImGui::PopStyleColor();

			if (isOpen) {
				for (const auto& child : measurement.children) {
					drawHierarchy(child, depth + 1);
				}
				ImGui::TreePop();
			}
		}

		double getCurrentTime() const {
			auto now = std::chrono::high_resolution_clock::now();
			return std::chrono::duration<double, std::milli>(now - m_StartTime).count();
		}

		std::chrono::high_resolution_clock::time_point m_LastFrameTime{ std::chrono::high_resolution_clock::now() };
		double m_DeltaTime = 0.0;
		std::chrono::high_resolution_clock::time_point m_StartTime{ std::chrono::high_resolution_clock::now() };
		FrameData m_CurrentFrame;
		std::stack<TimerMeasurement*> m_ActiveStack;
		std::deque<FrameData> m_FrameHistory;

		// Statistics
		std::deque<double> m_FrameTimeHistory;
		double m_FrameTimeSum;
		double m_MinFrameTime;
		double m_MaxFrameTime;

		const size_t m_MaxFrameHistory;
		const size_t m_AverageWindow;
	};

	// Keep the same RAII wrapper and macros
	class ScopedTimer {
	public:
		ScopedTimer(const std::string& name, const ImVec4& color = ImVec4(1, 1, 1, 1))
			: m_Name(name) {
			Timer::getInstance().beginSection(name, color);
		}

		~ScopedTimer() {
			Timer::getInstance().endSection(m_Name);
		}

	private:
		std::string m_Name;
	};

#define SCOPED_TIMER(name) ScopedTimer scopedTimer##__LINE__(name)
#define SCOPED_TIMER_COLORED(name, color) ScopedTimer scopedTimer##__LINE__(name, color)
} // namespace SE