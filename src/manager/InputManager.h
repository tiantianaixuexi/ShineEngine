#pragma once


#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <unordered_map>



#include "util/singleton.h"
#include "shine_define.h"



namespace shine::input_manager
{


	struct FKey
    {


    public:

        FKey(const std::string& __str): KeyName(__str) {}
        FKey(const char* __str): KeyName(__str) {}

        std::string_view GetKeyName() const { return KeyName; }


        friend bool operator==(const FKey& KeyA, const FKey& KeyB){ return KeyA.KeyName == KeyB.KeyName;}
        friend bool operator!=(const FKey& KeyA, const FKey& KeyB){return  KeyA.KeyName != KeyB.KeyName;}

    private:

        std::string KeyName;
    };

    // 输入事件类型
	enum class InputEventType
    {
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove,
        MouseWheel,
        FocusGained,
        FocusLost
    };

    // 修饰键（鼠标滚轮）
	enum class InputModifier : std::uint32_t
    {
        None   = 0,
        Shift  = 1u << 0,
        Ctrl   = 1u << 1,
        Alt    = 1u << 2,
        Super  = 1u << 3
    };

    // 输入事件数据
	struct InputEvent
    {
        InputEventType type { InputEventType::KeyDown };
        // 键盘
        int virtualKey { -1 };      // Win32 VK_*
        std::string keyName;        // 可读的定义名
        // 鼠标
        int mouseButton { -1 };     // 0=L,1=R,2=M,3/4=extras
        float mouseX { 0.0f };
        float mouseY { 0.0f };
        float wheelX { 0.0f };
        float wheelY { 0.0f };
        // 修饰键位
        std::uint32_t modifiers { 0u };  // OR of InputModifier
        bool hasFocus { true };
    };

	using InputCallback = std::function<void(const InputEvent&)>;
	struct BindingHandle { u64 id {0}; explicit operator bool() const { return id != 0; } };




	struct EKeys
    {
        static  const FKey MouseX;
        static  const FKey MouseY;
        static  const FKey Mouse2D;
        static  const FKey MouseScrollUp;
        static  const FKey MouseScrollDown;
        static  const FKey MouseWheelAxis;
        static  const FKey LeftMouseButton;
        static  const FKey RightMouseButton;

        static  const FKey BackSpace;
        static  const FKey Tab;
        static  const FKey Enter;
        static  const FKey Pause;

        static  const FKey CapsLock;
        static  const FKey Escape;
        static  const FKey SpaceBar;
        static  const FKey PageUp;
        static  const FKey PageDown;
        static  const FKey End;
        static  const FKey Home;

        static  const FKey Left;
        static  const FKey Up;
        static  const FKey Right;
        static  const FKey Down;

        static  const FKey Insert;
        static  const FKey Delete;

        static  const FKey Zero;
        static  const FKey One;
        static  const FKey Two;
        static  const FKey Three;
        static  const FKey Four;
        static  const FKey Five;
        static  const FKey Six;
        static  const FKey Seven;
        static  const FKey Eight;
        static  const FKey Nine;

        static  const FKey A;
        static  const FKey B;
        static  const FKey C;
        static  const FKey D;
        static  const FKey E;
        static  const FKey F;
        static  const FKey G;
        static  const FKey H;
        static  const FKey I;
        static  const FKey J;
        static  const FKey K;
        static  const FKey L;
        static  const FKey M;
        static  const FKey N;
        static  const FKey O;
        static  const FKey P;
        static  const FKey Q;
        static  const FKey R;
        static  const FKey S;
        static  const FKey T;
        static  const FKey U;
        static  const FKey V;
        static  const FKey W;
        static  const FKey X;
        static  const FKey Y;
        static  const FKey Z;
        static  const FKey F1;
        static  const FKey F2;
        static  const FKey F3;
        static  const FKey F4;
        static  const FKey F5;
        static  const FKey F6;
        static  const FKey F7;
        static  const FKey F8;
        static  const FKey F9;
        static  const FKey F10;
        static  const FKey F11;
        static  const FKey F12;

        static  const FKey NumPadZero;
        static  const FKey NumPadOne;
        static  const FKey NumPadTwo;
        static  const FKey NumPadThree;
        static  const FKey NumPadFour;
        static  const FKey NumPadFive;
        static  const FKey NumPadSix;
        static  const FKey NumPadSeven;
        static  const FKey NumPadEight;
        static  const FKey NumPadNine;


        static  const FKey Multiply;
        static  const FKey Add;
        static  const FKey Subtract;
        static  const FKey Decimal;
        static  const FKey Divide;

        static  const FKey NumLock;

        static  const FKey ScrollLock;

        static  const FKey LeftShift;
        static  const FKey RightShift;
        static  const FKey LeftControl;
        static  const FKey RightControl;
        static  const FKey LeftAlt;
        static  const FKey RightAlt;
        static  const FKey LeftCommand;
        static  const FKey RightCommand;

        static  const FKey Semicolon;
        static  const FKey Equals;
        static  const FKey Comma;
        static  const FKey Underscore;
        static  const FKey Hyphen;
        static  const FKey Period;
        static  const FKey Slash;
        static  const FKey Tilde;
        static  const FKey LeftBracket;
        static  const FKey Backslash;
        static  const FKey RightBracket;
        static  const FKey Apostrophe;

        static  const FKey Ampersand;
        static  const FKey Asterix;
        static  const FKey Caret;
        static  const FKey Colon;
        static  const FKey Dollar;
        static  const FKey Exclamation;
        static  const FKey LeftParantheses;
        static  const FKey RightParantheses;
        static  const FKey Quote;

        static  const FKey A_AccentGrave;
        static  const FKey E_AccentGrave;
        static  const FKey E_AccentAigu;
        static  const FKey C_Cedille;
        static  const FKey Section;
    };



	class InputManager : public shine::util::Singleton<InputManager>
    {

    public:

        bool processKeyEvent()
        {
            //const int keyCode = static_cast<int>(wParam);
            return true;
        }

        bool processWin32Message(unsigned int msg, unsigned __int64 wParam, __int64 lParam);

        // 绑定/解绑 API
        // 键盘（虚拟键 VK_*）
        BindingHandle BindVirtualKey(int virtualKey, InputEventType type, InputCallback callback);
        // 任意键（所有KeyDown/KeyUp都会触发）
        BindingHandle BindAnyKey(InputEventType type, InputCallback callback);
        // 鼠标按键，Button 0..4
        BindingHandle BindMouseButton(int button, InputEventType type, InputCallback callback);
        // 鼠标移动 & 滚轮
        BindingHandle BindMouseMove(InputCallback callback);
        BindingHandle BindMouseWheel(InputCallback callback);
        // 解绑
        bool Unbind(BindingHandle handle);

        // 查询接口
        bool IsKeyDown(int virtualKey) const { return virtualKey >= 0 && virtualKey < 256 && keyStates[virtualKey]; }
        bool IsMouseDown(int button) const { return button >= 0 && button < 5 && MouseDown[button]; }


        bool MouseDown[5] { false, false, false, false, false }; //0 = left, 1 = right, 2 = middle 3,4= extras(鼠标侧键，前进后退)
        bool keyStates[256] = { false };


        bool isForce = true;

    private:
        struct CallbackRecord { uint64_t id; InputCallback callback; };

        // 按类型存储回调
        std::unordered_map<int, std::vector<CallbackRecord>> keyDownCallbacks;  // VK -> cbs
        std::unordered_map<int, std::vector<CallbackRecord>> keyUpCallbacks;    // VK -> cbs
        std::vector<CallbackRecord> anyKeyDownCallbacks;
        std::vector<CallbackRecord> anyKeyUpCallbacks;

        std::unordered_map<int, std::vector<CallbackRecord>> mouseDownCallbacks; // button -> cbs
        std::unordered_map<int, std::vector<CallbackRecord>> mouseUpCallbacks;   // button -> cbs
        std::vector<CallbackRecord> mouseMoveCallbacks;
        std::vector<CallbackRecord> mouseWheelCallbacks;

        struct Registration
        {
            enum class Category { KeyVK, AnyKey, MouseButton, MouseMove, MouseWheel };
            Category category { Category::KeyVK };
            InputEventType type { InputEventType::KeyDown };
            int keyOrButton { -1 };
        };

        std::unordered_map<uint64_t, Registration> registrations;
        u64 nextId { 1 };
    };







}

