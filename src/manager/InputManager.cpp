#include "InputManager.h"

#include <windowsx.h>


namespace shine::input_manager
{

#ifdef _WIN32
    static uint32_t GetModifierFlags()
    {
        uint32_t flags = 0;
        if ((GetKeyState(VK_SHIFT) & 0x8000) != 0)  { flags |= static_cast<uint32_t>(InputModifier::Shift); }
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0){ flags |= static_cast<uint32_t>(InputModifier::Ctrl); }
        if ((GetKeyState(VK_MENU) & 0x8000) != 0)   { flags |= static_cast<uint32_t>(InputModifier::Alt); }
        if ((GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0)
        {
            flags |= static_cast<uint32_t>(InputModifier::Super);
        }
        return flags;
    }
#endif

    template <typename T>
    static bool RemoveById(std::vector<T>& records, uint64_t id)
    {
        //auto iter = records.begin();

        //for (iter; iter!=records.end();++iter)
        //{
        //    if (iter.id == id)
        //    {
        //        records.erase(iter);
        //        return true;
        //    }
        //}

        return false;

    }

    BindingHandle InputManager::BindVirtualKey(int virtualKey, InputEventType type, InputCallback callback)
    {
        if (!callback) return {};
        const uint64_t id = nextId++;
        CallbackRecord rec{ id, std::move(callback) };
        if (type == InputEventType::KeyDown)
        {
            keyDownCallbacks[virtualKey].push_back(rec);
        }
        else
        {
            keyUpCallbacks[virtualKey].push_back(rec);
        }
        registrations.emplace(id, Registration{ Registration::Category::KeyVK, type, virtualKey });
        return BindingHandle{ id };
    }

    BindingHandle InputManager::BindAnyKey(InputEventType type, InputCallback callback)
    {
        if (!callback) return {};
        const uint64_t id = nextId++;
        CallbackRecord rec{ id, std::move(callback) };
        if (type == InputEventType::KeyDown)
            anyKeyDownCallbacks.push_back(rec);
        else
            anyKeyUpCallbacks.push_back(rec);

        registrations.emplace(id, Registration{ Registration::Category::AnyKey, type, -1 });
        return BindingHandle{ id };
    }

    BindingHandle InputManager::BindMouseButton(int button, InputEventType type, InputCallback callback)
    {
        if (!callback) return {};
        const uint64_t id = nextId++;
        CallbackRecord rec{ id, std::move(callback) };
        if (type == InputEventType::MouseDown)
            mouseDownCallbacks[button].push_back(rec);
        else
            mouseUpCallbacks[button].push_back(rec);

        registrations.emplace(id, Registration{ Registration::Category::MouseButton, type, button });
        return BindingHandle{ id };
    }

    BindingHandle InputManager::BindMouseMove(InputCallback callback)
    {
        if (!callback) return {};
        const uint64_t id = nextId++;
        mouseMoveCallbacks.push_back(CallbackRecord{ id, std::move(callback) });
        registrations.emplace(id, Registration{ Registration::Category::MouseMove, InputEventType::MouseMove, -1 });
        return BindingHandle{ id };
    }

    BindingHandle InputManager::BindMouseWheel(InputCallback callback)
    {
        if (!callback) return {};
        const uint64_t id = nextId++;
        mouseWheelCallbacks.push_back(CallbackRecord{ id, std::move(callback) });
        registrations.emplace(id, Registration{ Registration::Category::MouseWheel, InputEventType::MouseWheel, -1 });
        return BindingHandle{ id };
    }

    bool InputManager::Unbind(BindingHandle handle)
    {
        if (!handle) return false;
        const auto it = registrations.find(handle.id);
        if (it == registrations.end()) return false;

        const Registration reg = it->second;
        registrations.erase(it);

        switch (reg.category)
        {
        case Registration::Category::KeyVK:
        {
            auto& mapRef = (reg.type == InputEventType::KeyDown) ? keyDownCallbacks : keyUpCallbacks;
            auto mapIt = mapRef.find(reg.keyOrButton);
            if (mapIt != mapRef.end())
            {
                return RemoveById(mapIt->second, handle.id);
            }
            return false;
        }
        case Registration::Category::AnyKey:
            if (reg.type == InputEventType::KeyDown) return RemoveById(anyKeyDownCallbacks, handle.id);
            return RemoveById(anyKeyUpCallbacks, handle.id);
        case Registration::Category::MouseButton:
        {
            auto& mapRef = (reg.type == InputEventType::MouseDown) ? mouseDownCallbacks : mouseUpCallbacks;
            auto mapIt = mapRef.find(reg.keyOrButton);
            if (mapIt != mapRef.end()) return RemoveById(mapIt->second, handle.id);
            return false;
        }
        case Registration::Category::MouseMove:
            return RemoveById(mouseMoveCallbacks, handle.id);
        case Registration::Category::MouseWheel:
            return RemoveById(mouseWheelCallbacks, handle.id);
        }
        return false;
    }

#ifdef _WIN32
    bool InputManager::processWin32Message(unsigned int msg, unsigned __int64 wParam, __int64 lParam)
    {
        switch (msg)
        {
        // 閿洏
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (wParam < 256)
            {
                const int vk = static_cast<int>(wParam);
                keyStates[vk] = true;

                InputEvent evt{};
                evt.type = InputEventType::KeyDown;
                evt.virtualKey = vk;
                evt.modifiers = GetModifierFlags();

                // 瑙﹀彂 AnyKeyDown
                auto any = anyKeyDownCallbacks; // 闃叉鍥炶皟淇敼瀵艰嚧杩唬澶辨晥
                for (const auto& r : any) r.callback(evt);

                // 瑙﹀彂鍏蜂綋閿?
                if (auto it = keyDownCallbacks.find(vk); it != keyDownCallbacks.end())
                {
                    auto cbs = it->second;
                    for (const auto& r : cbs) r.callback(evt);
                }
            }
            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            if (wParam < 256)
            {
                const int vk = static_cast<int>(wParam);
                keyStates[vk] = false;

                InputEvent evt{};
                evt.type = InputEventType::KeyUp;
                evt.virtualKey = vk;
                evt.modifiers = GetModifierFlags();

                auto any = anyKeyUpCallbacks;
                for (const auto& r : any) r.callback(evt);

                if (auto it = keyUpCallbacks.find(vk); it != keyUpCallbacks.end())
                {
                    auto cbs = it->second;
                    for (const auto& r : cbs) r.callback(evt);
                }
            }
            break;
        }

        // 榧犳爣鎸変笅
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            int button = 0;
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
            else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
            else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
            else if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            MouseDown[button] = true;

            InputEvent evt{};
            evt.type = InputEventType::MouseDown;
            evt.mouseButton = button;
            evt.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
            evt.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
            evt.modifiers = GetModifierFlags();

            if (auto it = mouseDownCallbacks.find(button); it != mouseDownCallbacks.end())
            {
                auto cbs = it->second;
                for (const auto& r : cbs) r.callback(evt);
            }
            break;
        }

        // 榧犳爣鎶捣
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            int button = 0;
            if (msg == WM_LBUTTONUP) { button = 0; }
            else if (msg == WM_RBUTTONUP) { button = 1; }
            else if (msg == WM_MBUTTONUP) { button = 2; }
            else if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            MouseDown[button] = false;

            InputEvent evt{};
            evt.type = InputEventType::MouseUp;
            evt.mouseButton = button;
            evt.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
            evt.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
            evt.modifiers = GetModifierFlags();

            if (auto it = mouseUpCallbacks.find(button); it != mouseUpCallbacks.end())
            {
                auto cbs = it->second;
                for (const auto& r : cbs) r.callback(evt);
            }
            break;
        }

        // 婊氳疆锛堝瀭鐩达級
        case WM_MOUSEWHEEL:
        {
            InputEvent evt{};
            evt.type = InputEventType::MouseWheel;
            evt.wheelY = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            evt.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
            evt.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
            evt.modifiers = GetModifierFlags();
            auto cbs = mouseWheelCallbacks;
            for (const auto& r : cbs) r.callback(evt);
            break;
        }
        // 婊氳疆锛堟按骞筹級
        case WM_MOUSEHWHEEL:
        {
            InputEvent evt{};
            evt.type = InputEventType::MouseWheel;
            evt.wheelX = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            evt.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
            evt.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
            evt.modifiers = GetModifierFlags();
            auto cbs = mouseWheelCallbacks;
            for (const auto& r : cbs) r.callback(evt);
            break;
        }

        // 榧犳爣绉诲姩
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
        {
            InputEvent evt{};
            evt.type = InputEventType::MouseMove;
            evt.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
            evt.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
            evt.modifiers = GetModifierFlags();
            auto cbs = mouseMoveCallbacks;
            for (const auto& r : cbs) r.callback(evt);
            break;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        {
            isForce = (msg == WM_SETFOCUS);

            InputEvent evt{};
            evt.type = (msg == WM_SETFOCUS) ? InputEventType::FocusGained : InputEventType::FocusLost;
            evt.hasFocus = isForce;
            // 涓嶅箍鎾壒瀹氬洖璋冿紝鐣欎綔鎵╁睍
            break;
        }

        case WM_INPUTLANGCHANGE:
            return true;

        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE:
        case WM_DPICHANGED:
        case WM_SETCURSOR:
        case WM_DEVICECHANGE:
            break;
        }

        return true; // 娑堟伅宸插鐞?
    }
#else
    bool InputManager::processWin32Message(unsigned int, unsigned __int64, __int64)
    {
        return false;
    }
#endif
} // namespace shine::input_manager

// Define all the static const key objects
const shine::input_manager::FKey shine::input_manager::EKeys::MouseX("MouseX");
const shine::input_manager::FKey shine::input_manager::EKeys::MouseY("MouseY");
const shine::input_manager::FKey shine::input_manager::EKeys::Mouse2D("Mouse2D");
const shine::input_manager::FKey shine::input_manager::EKeys::MouseScrollUp("MouseScrollUp");
const shine::input_manager::FKey shine::input_manager::EKeys::MouseScrollDown("MouseScrollDown");
const shine::input_manager::FKey shine::input_manager::EKeys::MouseWheelAxis("MouseWheelAxis");

const shine::input_manager::FKey shine::input_manager::EKeys::LeftMouseButton("LeftMouseButton");
const shine::input_manager::FKey shine::input_manager::EKeys::RightMouseButton("RightMouseButton");

const shine::input_manager::FKey shine::input_manager::EKeys::BackSpace("BackSpace");
const shine::input_manager::FKey shine::input_manager::EKeys::Tab("Tab");
const shine::input_manager::FKey shine::input_manager::EKeys::Enter("Enter");
const shine::input_manager::FKey shine::input_manager::EKeys::Pause("Pause");

const shine::input_manager::FKey shine::input_manager::EKeys::CapsLock("CapsLock");
const shine::input_manager::FKey shine::input_manager::EKeys::Escape("Escape");
const shine::input_manager::FKey shine::input_manager::EKeys::SpaceBar("SpaceBar");
const shine::input_manager::FKey shine::input_manager::EKeys::PageUp("PageUp");
const shine::input_manager::FKey shine::input_manager::EKeys::PageDown("PageDown");
const shine::input_manager::FKey shine::input_manager::EKeys::End("End");
const shine::input_manager::FKey shine::input_manager::EKeys::Home("Home");

const shine::input_manager::FKey shine::input_manager::EKeys::Left("Left");
const shine::input_manager::FKey shine::input_manager::EKeys::Up("Up");
const shine::input_manager::FKey shine::input_manager::EKeys::Right("Right");
const shine::input_manager::FKey shine::input_manager::EKeys::Down("Down");

const shine::input_manager::FKey shine::input_manager::EKeys::Insert("Insert");
const shine::input_manager::FKey shine::input_manager::EKeys::Delete("Delete");

const shine::input_manager::FKey shine::input_manager::EKeys::Zero("Zero");
const shine::input_manager::FKey shine::input_manager::EKeys::One("One");
const shine::input_manager::FKey shine::input_manager::EKeys::Two("Two");
const shine::input_manager::FKey shine::input_manager::EKeys::Three("Three");
const shine::input_manager::FKey shine::input_manager::EKeys::Four("Four");
const shine::input_manager::FKey shine::input_manager::EKeys::Five("Five");
const shine::input_manager::FKey shine::input_manager::EKeys::Six("Six");
const shine::input_manager::FKey shine::input_manager::EKeys::Seven("Seven");
const shine::input_manager::FKey shine::input_manager::EKeys::Eight("Eight");
const shine::input_manager::FKey shine::input_manager::EKeys::Nine("Nine");

const shine::input_manager::FKey shine::input_manager::EKeys::A("A");
const shine::input_manager::FKey shine::input_manager::EKeys::B("B");
const shine::input_manager::FKey shine::input_manager::EKeys::C("C");
const shine::input_manager::FKey shine::input_manager::EKeys::D("D");
const shine::input_manager::FKey shine::input_manager::EKeys::E("E");
const shine::input_manager::FKey shine::input_manager::EKeys::F("F");
const shine::input_manager::FKey shine::input_manager::EKeys::G("G");
const shine::input_manager::FKey shine::input_manager::EKeys::H("H");
const shine::input_manager::FKey shine::input_manager::EKeys::I("I");
const shine::input_manager::FKey shine::input_manager::EKeys::J("J");
const shine::input_manager::FKey shine::input_manager::EKeys::K("K");
const shine::input_manager::FKey shine::input_manager::EKeys::L("L");
const shine::input_manager::FKey shine::input_manager::EKeys::M("M");
const shine::input_manager::FKey shine::input_manager::EKeys::N("N");
const shine::input_manager::FKey shine::input_manager::EKeys::O("O");
const shine::input_manager::FKey shine::input_manager::EKeys::P("P");
const shine::input_manager::FKey shine::input_manager::EKeys::Q("Q");
const shine::input_manager::FKey shine::input_manager::EKeys::R("R");
const shine::input_manager::FKey shine::input_manager::EKeys::S("S");
const shine::input_manager::FKey shine::input_manager::EKeys::T("T");
const shine::input_manager::FKey shine::input_manager::EKeys::U("U");
const shine::input_manager::FKey shine::input_manager::EKeys::V("V");
const shine::input_manager::FKey shine::input_manager::EKeys::W("W");
const shine::input_manager::FKey shine::input_manager::EKeys::X("X");
const shine::input_manager::FKey shine::input_manager::EKeys::Y("Y");
const shine::input_manager::FKey shine::input_manager::EKeys::Z("Z");

const shine::input_manager::FKey shine::input_manager::EKeys::NumPadZero("NumPadZero");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadOne("NumPadOne");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadTwo("NumPadTwo");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadThree("NumPadThree");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadFour("NumPadFour");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadFive("NumPadFive");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadSix("NumPadSix");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadSeven("NumPadSeven");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadEight("NumPadEight");
const shine::input_manager::FKey shine::input_manager::EKeys::NumPadNine("NumPadNine");

const shine::input_manager::FKey shine::input_manager::EKeys::Multiply("Multiply");
const shine::input_manager::FKey shine::input_manager::EKeys::Add("Add");
const shine::input_manager::FKey shine::input_manager::EKeys::Subtract("Subtract");
const shine::input_manager::FKey shine::input_manager::EKeys::Decimal("Decimal");
const shine::input_manager::FKey shine::input_manager::EKeys::Divide("Divide");

const shine::input_manager::FKey shine::input_manager::EKeys::F1("F1");
const shine::input_manager::FKey shine::input_manager::EKeys::F2("F2");
const shine::input_manager::FKey shine::input_manager::EKeys::F3("F3");
const shine::input_manager::FKey shine::input_manager::EKeys::F4("F4");
const shine::input_manager::FKey shine::input_manager::EKeys::F5("F5");
const shine::input_manager::FKey shine::input_manager::EKeys::F6("F6");
const shine::input_manager::FKey shine::input_manager::EKeys::F7("F7");
const shine::input_manager::FKey shine::input_manager::EKeys::F8("F8");
const shine::input_manager::FKey shine::input_manager::EKeys::F9("F9");
const shine::input_manager::FKey shine::input_manager::EKeys::F10("F10");
const shine::input_manager::FKey shine::input_manager::EKeys::F11("F11");
const shine::input_manager::FKey shine::input_manager::EKeys::F12("F12");

const shine::input_manager::FKey shine::input_manager::EKeys::NumLock("NumLock");

const shine::input_manager::FKey shine::input_manager::EKeys::ScrollLock("ScrollLock");

const shine::input_manager::FKey shine::input_manager::EKeys::LeftShift("LeftShift");
const shine::input_manager::FKey shine::input_manager::EKeys::RightShift("RightShift");
const shine::input_manager::FKey shine::input_manager::EKeys::LeftControl("LeftControl");
const shine::input_manager::FKey shine::input_manager::EKeys::RightControl("RightControl");
const shine::input_manager::FKey shine::input_manager::EKeys::LeftAlt("LeftAlt");
const shine::input_manager::FKey shine::input_manager::EKeys::RightAlt("RightAlt");
const shine::input_manager::FKey shine::input_manager::EKeys::LeftCommand("LeftCommand");
const shine::input_manager::FKey shine::input_manager::EKeys::RightCommand("RightCommand");

const shine::input_manager::FKey shine::input_manager::EKeys::Semicolon("Semicolon");
const shine::input_manager::FKey shine::input_manager::EKeys::Equals("Equals");
const shine::input_manager::FKey shine::input_manager::EKeys::Comma("Comma");
const shine::input_manager::FKey shine::input_manager::EKeys::Underscore("Underscore");
const shine::input_manager::FKey shine::input_manager::EKeys::Hyphen("Hyphen");
const shine::input_manager::FKey shine::input_manager::EKeys::Period("Period");
const shine::input_manager::FKey shine::input_manager::EKeys::Slash("Slash");
const shine::input_manager::FKey shine::input_manager::EKeys::Tilde("Tilde");
const shine::input_manager::FKey shine::input_manager::EKeys::LeftBracket("LeftBracket");
const shine::input_manager::FKey shine::input_manager::EKeys::LeftParantheses("LeftParantheses");
const shine::input_manager::FKey shine::input_manager::EKeys::Backslash("Backslash");
const shine::input_manager::FKey shine::input_manager::EKeys::RightBracket("RightBracket");
const shine::input_manager::FKey shine::input_manager::EKeys::RightParantheses("RightParantheses");
const shine::input_manager::FKey shine::input_manager::EKeys::Apostrophe("Apostrophe");
const shine::input_manager::FKey shine::input_manager::EKeys::Quote("Quote");

const shine::input_manager::FKey shine::input_manager::EKeys::Asterix("Asterix");
const shine::input_manager::FKey shine::input_manager::EKeys::Ampersand("Ampersand");
const shine::input_manager::FKey shine::input_manager::EKeys::Caret("Caret");
const shine::input_manager::FKey shine::input_manager::EKeys::Dollar("Dollar");
const shine::input_manager::FKey shine::input_manager::EKeys::Exclamation("Exclamation");
const shine::input_manager::FKey shine::input_manager::EKeys::Colon("Colon");

const shine::input_manager::FKey shine::input_manager::EKeys::A_AccentGrave("A_AccentGrave");
const shine::input_manager::FKey shine::input_manager::EKeys::E_AccentGrave("E_AccentGrave");
const shine::input_manager::FKey shine::input_manager::EKeys::E_AccentAigu("E_AccentAigu");
const shine::input_manager::FKey shine::input_manager::EKeys::C_Cedille("C_Cedille");
const shine::input_manager::FKey shine::input_manager::EKeys::Section("Section");


