#pragma once

#include <cstddef>

#include "imgui/imgui.h"
#include "string/shine_string.h"

namespace shine::editor::util {

class ScopedImGuiID {
public:
    explicit ScopedImGuiID(int id) {
        ImGui::PushID(id);
    }

    explicit ScopedImGuiID(std::size_t id) {
        ImGui::PushID(static_cast<int>(id));
    }

    explicit ScopedImGuiID(const void* id) {
        ImGui::PushID(id);
    }

    explicit ScopedImGuiID(const char* id) {
        ImGui::PushID(id);
    }

    explicit ScopedImGuiID(const shine::SString& id) {
        ImGui::PushID(id.c_str());
    }

    ~ScopedImGuiID() {
        ImGui::PopID();
    }

    ScopedImGuiID(const ScopedImGuiID&) = delete;
    ScopedImGuiID& operator=(const ScopedImGuiID&) = delete;
};

} // namespace shine::editor::util