#pragma once

#include <memory>

namespace shine
{
    class EngineContext;
}

namespace shine::editor::main_editor
{
    class MainEditor;
}

namespace shine::editor::main_editor
{
    class EditorCompositionRoot
    {
    public:
        static void RegisterEditorSystems(EngineContext& context);
        static std::unique_ptr<MainEditor> BuildMainEditor(EngineContext& context);
    };
}
