#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>

#include "string/shine_string.h"
#include "imgui/imgui.h"
#include "render/backend/render_backend.h"

namespace fs = std::filesystem;

namespace shine::launcher
{
    struct ProjectInfo
    {
        SString name;
        SString path;
        SString category{"Games"}; // Games, Templates, Samples, etc.
        SString engineVersion{"1.0.0"};
        SString description;
        SString thumbnail; // path to thumbnail image
        time_t lastModified = 0;
    };

    struct ProjectTemplate
    {
        SString name;
        SString description;
        SString category;
        SString templatePath; // path to template directory
        SString thumbnail;
    };

    class LauncherGUI
    {
    public:
        LauncherGUI();
        ~LauncherGUI();

        void Init(shine::render::backend::IRenderBackend* render);
        void Render();
        void Shutdown();

        // Project management
        void LoadRecentProjects();
        void SaveRecentProjects();
        void AddRecentProject(const ProjectInfo& project);
        void RemoveRecentProject(const SString& projectPath);
        void DeleteProjectFiles(const SString& projectPath);

        // Configuration management
        void LoadSettings();
        void SaveSettings();

        // Template management
        void LoadProjectTemplates();

        // Actions
        void LaunchProject(const ProjectInfo& project);
        void CreateNewProject(const SString& name, const SString& path, const ProjectTemplate& template_);

        // Project browsing
        void ScanForProjects(const SString& directory);
        void BrowseForProject();

        // Helper methods
        void CopyTemplateFiles(const fs::path& templatePath, const fs::path& projectPath);
        void CreateProjectConfig(const fs::path& projectPath, const SString& name, const ProjectTemplate& template_);
        void CreateBasicSourceFiles(const fs::path& projectPath, const SString& name);

    private:
        void RenderMainWindow();
        void RenderRecentProjectsTab();
        void RenderBrowseTab();
        void RenderLibraryTab();
        void RenderNewProjectDialog();
        void RenderDeleteConfirmDialog();

        // Library tab sub-components
        void RenderEngineVersionsTab();
        void RenderPluginsTab();
        void RenderContentTab();
        void RenderLibrarySettingsTab();

        void RenderProjectCard(const ProjectInfo& project, float cardWidth);
        void RenderTemplateCard(const ProjectTemplate& template_, float cardWidth);

        // UI state
        bool showNewProjectDialog = false;
        bool showDeleteConfirm = false;
        SString projectToDeletePath;
        SString newProjectName;
        SString newProjectPath;
        int selectedTemplateIndex = 0;

        // Data
        std::vector<ProjectInfo> recentProjects;
        std::vector<ProjectTemplate> projectTemplates;
        int currentTab = 0; // 0: Recent, 1: Browse, 2: Library

        // Settings
        SString engineRootPath;
        SString projectsRootPath;

        // Configuration
        struct LauncherSettings {
            bool showWelcomeDialog = true;
            SString defaultProjectPath;
            int maxRecentProjects = 10;
            bool autoLaunchLastProject = false;
        } settings;

        // Error handling
        struct ErrorInfo {
            SString message;
            SString details;
            time_t timestamp = 0;
        };
        
        std::vector<ErrorInfo> errorLog;
        bool showErrorDialog = false;
        ErrorInfo currentError;

        // Error handling
        void ReportError(const SString& message, const SString& details = "");
        void ClearErrorLog();
        void ShowErrorDialog(const ErrorInfo& error);
        void RenderErrorDialog();

        // Installation
        bool IsEngineInstalled();
        bool InstallEngine(const SString& installPath);
        void RenderInstallWizard();
        void RenderInstallLocationPage();
        void RenderInstallProgressPage();
        void RenderInstallCompletePage();
        void RenderEngineSelectPage();
        void SaveEngineSettings(const fs::path& configPath);

        // Installation state
        enum class InstallState {
            NotInstalled,
            SelectingLocation,
            Installing,
            Installed,
            Error
        };

        InstallState installState = InstallState::NotInstalled;
        float installProgress = 0.0f;
        SString installStatus;
        SString installPath;
        bool installSuccess = false;
        SString installError;

        // Engine versions
        struct EngineVersion {
            SString version;
            SString path;
            bool isInstalled = false;
            bool isLatest = false;
        };
        std::vector<EngineVersion> engineVersions;
        int selectedEngineVersion = 0;

        // Rendering
        shine::render::backend::IRenderBackend* renderBackend = nullptr;
        ImFont* titleFont = nullptr;
        ImFont* normalFont = nullptr;
    };

    // Global launcher instance
    extern std::unique_ptr<LauncherGUI> g_Launcher;
}
