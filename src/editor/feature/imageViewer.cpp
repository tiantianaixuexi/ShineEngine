#include "imageViewer.h"

#include <gl/glew.h>

#include "imgui.h"

namespace shine::editor::imageViewer
{

	constexpr  GLint swz_rgba[4] = { GL_RED, GL_GREEN,GL_BLUE , GL_ALPHA };
	constexpr  GLint swz_r[4] = { GL_RED, GL_ZERO, GL_ZERO, GL_ONE };
	constexpr  GLint swz_g[4] = { GL_ZERO, GL_GREEN, GL_ZERO, GL_ONE };
	constexpr  GLint swz_b[4] = { GL_ZERO, GL_ZERO, GL_BLUE, GL_ONE };
	constexpr  GLint swz_a[4] = { GL_ALPHA, GL_ALPHA, GL_ALPHA, GL_ZERO };

	void SImageViewer::Render()
	{
	
		auto updateTexture = [&](const GLint* swizzle) noexcept
		{
			glBindTexture(GL_TEXTURE_2D, texture->getHandle());
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz_a);
			glBindTexture(GL_TEXTURE_2D, 0);
		};

		// 鍒ゆ柇绾圭悊鍙ユ焺鏄惁鏈夋晥锛堥潪绌猴級
		if (texture->isValid()) {

			static int channel = 0;
			ImGui::Begin("OpenGL Texture Text", &open);
			ImGui::Text("size = %d x %d", texture->getWidth(), texture->getHeight());

			ImGui::PushID(0);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 1.f, 1.f, 1.0f));

			if (ImGui::Button("RGBA"))
			{
				if (channel != 0)
				{
					channel = 0;

					updateTexture(swz_rgba);
				}
			}

			ImGui::PopStyleColor(3);
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::PushID(1);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.f, 0.f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.f, 0.f, 1.0f));
			if (ImGui::Button("R"))
			{
				if (channel != 1)
				{
					channel = 1;

					updateTexture(swz_r);

				}
			}

			ImGui::PopStyleColor(3);
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::PushID(2);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 1.f, 0.f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.7f, 0.f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.8f, 0.f, 1.0f));
			if (ImGui::Button("G"))
			{
				if (channel != 2)
				{
					channel = 2;

					updateTexture(swz_g);
				}
			}
			ImGui::PopStyleColor(3);
			ImGui::PopID();

			ImGui::PushID(3);
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 1.f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.7f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.8f, 1.0f));
			if (ImGui::Button("B"))
			{
				if (channel != 3)
				{
					channel = 3;

					updateTexture(swz_b);

				}
			}
			ImGui::PopStyleColor(3);
			ImGui::PopID();

			ImGui::SameLine();
			ImGui::PushID(4);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.8f, 0.8f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.8f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 0.8f));
			if (ImGui::Button("A"))
			{
				if (channel != 4)
				{
					channel = 4;
					updateTexture(swz_a);
				}

			}
			ImGui::PopStyleColor(3);
			ImGui::PopID();

			ImGui::Image((ImTextureID)(std::intptr_t)texture->getHandle(),
				ImVec2(static_cast<float>(texture->getWidth()),
					static_cast<float>(texture->getHeight()))); ImGui::End();
		}

	}
}


