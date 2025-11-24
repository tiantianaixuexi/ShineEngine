#pragma once


#include "shine_define.h"

namespace shine::widget::widgetState{

    enum class ButtonState : u8{
        Normal,
        Hovered,
        Pressed,
        Disabled
    };


    enum class ButtonEvent: u8{
        Pressed,
        Released,
        Hovered,
        Unhovered,
        Clicked
    };



}