#pragma once

#include<string>
#include <functional>


#include "widget/widgetState.h"

namespace shine::widget::button
{

	// 按钮
	class shineButton
	{
	public:
		explicit shineButton(std::string _name) :buttonName(_name) {
			buttonState = widgetState::ButtonState::Normal;
		}

		shineButton(shineButton&& other) noexcept = default;
		shineButton& operator=(shineButton&& other) noexcept = default;
		shineButton(const shineButton& other) = delete;
		shineButton& operator=(const shineButton& other) = delete;

		~shineButton()  = default;



		void Init();
		void Render();

		[[nodiscard]] constexpr std::string_view getLabel() const noexcept {
            return buttonName;
        }

		[[nodiscard]] constexpr widgetState::ButtonState getState() const noexcept {
            return buttonState;
        }

        [[nodiscard]] constexpr bool isClicked() const noexcept {
            return buttonState == widgetState::ButtonState::Pressed;
        }

        [[nodiscard]] constexpr bool isHovered() const noexcept {
            return buttonState == widgetState::ButtonState::Hovered;
        }

		void SetOnPressed(std::move_only_function<void()> func) noexcept {
			onPressed = std::move(func);
		}

		void SetOnReleased(std::move_only_function<void()> func) noexcept {
			onReleased = std::move(func);
		}

		void SetOnHovered(std::move_only_function<void()> func) noexcept {
			onHovered = std::move(func);
		}

		void SetOnUnHovered(std::move_only_function<void()> func) noexcept {
			onUnHovered = std::move(func);
		}


	private:
		std::string buttonName;
		widgetState::ButtonState buttonState;

		std::move_only_function<void()> onPressed;
		std::move_only_function<void()> onReleased;
		std::move_only_function<void()> onHovered;
		std::move_only_function<void()> onUnHovered;
	};
}

