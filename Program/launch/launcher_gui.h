#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>

#include "imgui.h"
#include "render/core/render_backend.h"

namespace fs = std::filesystem;

namespace shine::launcher
{
    struct ProjectInfo
    {
        std::string name;
        std::string path;
        std::string category = "Games"; // Games, Templates, Samples, etc.
        std::string engineVersion = "1.0.0";
        std::string description;
        std::string thumbnail; // path to thumbnail image
        time_t lastModified = 0;
    };

    struct ProjectTemplate
    {
        std::string name;
        std::string description;
        std::string category;
        std::string templatePath; // path to template directory
        std::string thumbnail;
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

        // Configuration management
        void LoadSettings();
        void SaveSettings();

        // Template management
        void LoadProjectTemplates();

        // Actions
        void LaunchProject(const ProjectInfo& project);
        void CreateNewProject(const std::string& name, const std::string& path, const ProjectTemplate& template_);

        // Project browsing
        void ScanForProjects(const std::string& directory);
        void BrowseForProject();

        // Helper methods
        void CopyTemplateFiles(const fs::path& templatePath, const fs::path& projectPath);
        void CreateProjectConfig(const fs::path& projectPath, const std::string& name, const ProjectTemplate& template_);
        void CreateBasicSourceFiles(const fs::path& projectPath, const std::string& name);

    private:
        void RenderMainWindow();
        void RenderRecentProjectsTab();
        void RenderBrowseTab();
        void RenderLibraryTab();
        void RenderNewProjectDialog();

        // Library tab sub-components
        void RenderEngineVersionsTab();
        void RenderPluginsTab();
        void RenderContentTab();
        void RenderLibrarySettingsTab();

        void RenderProjectCard(const ProjectInfo& project, float cardWidth);
        void RenderTemplateCard(const ProjectTemplate& template_, float cardWidth);

        // UI state
        bool showNewProjectDialog = false;
        std::string newProjectName;
        std::string newProjectPath;
        int selectedTemplateIndex = 0;

        // Data
        std::vector<ProjectInfo> recentProjects;
        std::vector<ProjectTemplate> projectTemplates;
        int currentTab = 0; // 0: Recent, 1: Browse, 2: Library

        // Settings
        std::string engineRootPath;
        std::string projectsRootPath;

        // Configuration
        struct LauncherSettings {
            bool showWelcomeDialog = true;
            std::string defaultProjectPath;
            int maxRecentProjects = 10;
            bool autoLaunchLastProject = false;
        } settings;

        // Error handling
        struct ErrorInfo {
            std::string message;
            std::string details;
            time_t timestamp = 0;
        };
        
        std::vector<ErrorInfo> errorLog;
        bool showErrorDialog = false;
        ErrorInfo currentError;

        // Error handling
        void ReportError(const std::string& message, const std::string& details = "");
        void ClearErrorLog();
        void ShowErrorDialog(const ErrorInfo& error);
        void RenderErrorDialog();

        // Rendering
        shine::render::backend::IRenderBackend* renderBackend = nullptr;
        ImFont* titleFont = nullptr;
        ImFont* normalFont = nullptr;
    };

    // Global launcher instance
    extern std::unique_ptr<LauncherGUI> g_Launcher;
}
