#include "shineButton.h"


#include <fmt/format.h>
#include <imgui/imgui.h>

namespace shine::widget::button {

	using namespace shine::widget::widgetState;

	void shineButton::Init() {

	}

	void shineButton::Render() {

		bool isClick = ImGui::Button(buttonName.c_str());

		if (isClick && onPressed) {

			buttonState = ButtonState::Pressed;
			onPressed();
			return;
		}

		if (buttonState == ButtonState::Pressed) {

			buttonState = ButtonState::Normal;
			if (onReleased) {
				onReleased();
			}

			return;
		}

		if (buttonState != ButtonState::Pressed) {

			if (ImGui::IsItemHovered()) {

				if (buttonState != ButtonState::Hovered) {

					buttonState = ButtonState::Hovered;
					if (onHovered) {
						onHovered();
					}
				}
			}
			else {
				if (buttonState == ButtonState::Hovered) {
					buttonState = ButtonState::Normal;
					if (onUnHovered) {
						onUnHovered();
					}
				}
			}
		}
	}

} // namespace shine::widget::button


