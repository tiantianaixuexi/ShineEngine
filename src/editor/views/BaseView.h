#pragma once

#include <string>

#include "util/function/EventHandle.h"

namespace shine::editor::views {
class BaseView {

protected:
    BaseView() {

    };

public:
    BaseView(const BaseView &)            = delete;
    BaseView &operator=(const BaseView &) = delete;
    BaseView(BaseView &&)                 = delete;
    BaseView &operator=(BaseView &&)      = delete;
    virtual ~BaseView()                   = default;

public:
    util::EventHandle<bool> OnOpenChange;

public:
    bool               IsOpen() const { return isOpen; }
    const std::string &GetName() const { return name; }

    void SetName(const std::string &viewName) noexcept { name = viewName; }

    void SetOpen(bool open) {

        if (open != isOpen) {

            if (isFirstOpen && open) {
                FirstOpen();
                isFirstOpen = false;
            }

            isOpen = open;
        }
    }

    bool SetShow() noexcept {
        if (isOpen) {
            isOpen = false;
        } else {
            if (isFirstOpen) {
                FirstOpen();
                isFirstOpen = false;
            }
            isOpen = true;
        }
        return isOpen;
    }

    void CheckIsOpenChange() {
        if (isOpen != isLastOpen) {
            OnOpenChange.emit(isOpen);
            isLastOpen = isOpen;
        }
    }

    void RenderBase() {
        if (!isOpen)
            return;
        onRender();
    }

    virtual void FirstOpen() {}
    virtual void onInit()     = 0;
    virtual void onRender()   = 0;
    virtual void onShutDown() = 0;

protected:
    std::string name        = "";
    bool        isOpen      = false;
    bool        isLastOpen  = false;
    bool        isFirstOpen = true; // 用于首次打开时执行一次性初始化
};

} // namespace shine::editor::views