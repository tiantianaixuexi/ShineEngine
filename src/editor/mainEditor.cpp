#include "mainEditor.h"

#include "editor/main_editor/window_registry.h"

namespace shine::editor::main_editor {

	REGISTER_LOG_GROUP_END(EditorLog)

	MainEditor::MainEditor(shine::EngineContext& context) : m_Context(context)
	{
		ADD_LOG_CATEGORY(EditorLog, "init")
		ADD_LOG_CATEGORY_WITH_CONSOLE(EditorLog, "Rendering", true)
		ADD_LOG_CATEGORY_WITH_CONSOLE(EditorLog, "Input", true)
		ADD_LOG_CATEGORY_WITH_CONSOLE(EditorLog, "Assets", true)
		ADD_LOG_CATEGORY_WITH_CONSOLE(EditorLog, "Memory", false)

		SHINE_LOG_INFO(EditorLog, "init", "MainEditor constructor called");
	}

	MainEditor::~MainEditor() = default;

	void MainEditor::Init() {
		SHINE_LOG_INFO(EditorLog, "init", "[MainEditor] Init Start");
        windowRegistry_ = std::make_unique<WindowRegistry>();
        windowRegistry_->Init(this);

		SHINE_LOG_INFO(EditorLog, "init", "[MainEditor] Init Finish");
	}

	void MainEditor::Render() {
        if (windowRegistry_)
        {
            windowRegistry_->Render(mainDocker);
        }
	}

	bool MainEditor::ToggleAssetBrowser()
	{
        if (!windowRegistry_)
        {
            return false;
        }
		return windowRegistry_->ToggleAssetBrowser();
	}

    bool MainEditor::ToggleMemoryProfiler()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->ToggleMemoryProfiler();
    }

	bool MainEditor::ToggleEngineSettings()
	{
        if (!windowRegistry_)
        {
            return false;
        }
		return windowRegistry_->ToggleEngineSettings();
	}

    bool MainEditor::ToggleLog()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->ToggleLog();
    }

    bool MainEditor::ToggleSceneHierarchy()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->ToggleSceneHierarchy();
    }

    bool MainEditor::TogglePlacementPalette()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->TogglePlacementPalette();
    }

    bool MainEditor::IsMemoryProfilerOpen() const
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->IsMemoryProfilerOpen();
    }
} // namespace shine::editor::main_editor
