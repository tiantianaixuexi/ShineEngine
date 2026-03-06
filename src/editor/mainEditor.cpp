#include "mainEditor.h"

#include "editor/main_editor/WindowRegistry.h"

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

	bool MainEditor::ToggleAssetBrowserImpl()
	{
        if (!windowRegistry_)
        {
            return false;
        }
		return windowRegistry_->ToggleAssetBrowser();
	}

    bool MainEditor::TogglePropertyInspectorImpl()
    {
        if(!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->TogglePropertyInspector();
    }    

    bool MainEditor::ToggleMemoryProfilerImpl()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->ToggleMemoryProfiler();
    }

	bool MainEditor::ToggleEngineSettingsImpl()
	{
        if (!windowRegistry_)
        {
            return false;
        }
		return windowRegistry_->ToggleEngineSettings();
	}

    bool MainEditor::ToggleLogImpl()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->ToggleLog();
    }

    bool MainEditor::ToggleSceneHierarchyImpl()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->ToggleSceneHierarchy();
    }

    bool MainEditor::TogglePlacementPaletteImpl()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->TogglePlacementPalette();
    }

    bool MainEditor::ToggleDebugTextureImpl()
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->ToggleDebugTexture();
    }

    bool MainEditor::IsMemoryProfilerOpenImpl() const
    {
        if (!windowRegistry_)
        {
            return false;
        }
        return windowRegistry_->IsMemoryProfilerOpen();
    }
} // namespace shine::editor::main_editor
