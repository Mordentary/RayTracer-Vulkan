#pragma once

#include <functional>
#include <vector>
#include <algorithm>

namespace SE
{
	template<typename... Args>
	class Signal {
	public:
		using SlotType = std::function<void(Args...)>;

		void connect(const SlotType& slot) {
			m_Slots.push_back(slot);
		}

		void disconnect(const SlotType& slot) {
			m_Slots.erase(std::remove(m_Slots.begin(), m_Slots.end(), slot), m_Slots.end());
		}

		void emit(Args... args) {
			for (auto& slot : m_Slots) {
				slot(args...);
			}
		}

	private:
		std::vector<SlotType> m_Slots;
	};
}