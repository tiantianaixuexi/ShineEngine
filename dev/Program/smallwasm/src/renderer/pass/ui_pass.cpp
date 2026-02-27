#include "ui_pass.h"

#include "../../ui/ui_manager.h"

namespace shine { namespace renderer {

class UiPass final : public BasePass {
public:
    void run(DemoGame& /*game*/, const RenderPass& /*pass*/, float /*t*/, int ctx) override {
        shine::ui::UIManager::instance().onRender(ctx);
    }
};

BasePass* GetUiPass() {
    static UiPass s_ui;
    return &s_ui;
}

} } // namespace shine::renderer
