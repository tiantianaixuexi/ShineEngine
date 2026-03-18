#include "imgui/imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui/imgui_stdlib.h"

#include "string/shine_string.h"

namespace
{
    struct InputTextCallback_UserData
    {
        shine::SString*          str;
        ImGuiInputTextCallback   chainCallback;
        void*                    chainCallbackUserData;
    };

    static int InputTextCallback(ImGuiInputTextCallbackData* data)
    {
        auto* userData = static_cast<InputTextCallback_UserData*>(data->UserData);
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            IM_ASSERT(data->Buf == userData->str->data());
            userData->str->resize(static_cast<shine::SString::size_type>(data->BufTextLen));
            data->Buf = userData->str->data();
        }
        else if (userData->chainCallback)
        {
            data->UserData = userData->chainCallbackUserData;
            return userData->chainCallback(data);
        }
        return 0;
    }

    static bool InputTextImpl(const char* label, shine::SString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData)
    {
        IM_ASSERT(str != nullptr);
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData callbackUserData{};
        callbackUserData.str = str;
        callbackUserData.chainCallback = callback;
        callbackUserData.chainCallbackUserData = userData;
        return ImGui::InputText(label, str->data(), str->capacity() + 1, flags, InputTextCallback, &callbackUserData);
    }

    static bool InputTextMultilineImpl(const char* label, shine::SString* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData)
    {
        IM_ASSERT(str != nullptr);
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData callbackUserData{};
        callbackUserData.str = str;
        callbackUserData.chainCallback = callback;
        callbackUserData.chainCallbackUserData = userData;
        return ImGui::InputTextMultiline(label, str->data(), str->capacity() + 1, size, flags, InputTextCallback, &callbackUserData);
    }

    static bool InputTextWithHintImpl(const char* label, const char* hint, shine::SString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData)
    {
        IM_ASSERT(str != nullptr);
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData callbackUserData{};
        callbackUserData.str = str;
        callbackUserData.chainCallback = callback;
        callbackUserData.chainCallbackUserData = userData;
        return ImGui::InputTextWithHint(label, hint, str->data(), str->capacity() + 1, flags, InputTextCallback, &callbackUserData);
    }
}

bool ImGui::InputText(const char* label, shine::SString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData)
{
    return InputTextImpl(label, str, flags, callback, userData);
}

bool ImGui::InputTextMultiline(const char* label, shine::SString* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData)
{
    return InputTextMultilineImpl(label, str, size, flags, callback, userData);
}

bool ImGui::InputTextWithHint(const char* label, const char* hint, shine::SString* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* userData)
{
    return InputTextWithHintImpl(label, hint, str, flags, callback, userData);
}

#endif // #ifndef IMGUI_DISABLE
