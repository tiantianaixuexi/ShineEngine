#include "launcher_gui.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <sstream>

#include "imgui/imgui_internal.h"
#include "fmt/format.h"

#ifdef SHINE_PLATFORM_WIN64
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#endif

namespace shine::launcher
{
    std::unique_ptr<LauncherGUI> g_Launcher;

    LauncherGUI::LauncherGUI()
    {
        // Initialize paths
        engineRootPath = fs::current_path().string().c_str();

        LoadSettings();
        LoadRecentProjects();
        LoadProjectTemplates();
    }

    LauncherGUI::~LauncherGUI()
    {
        SaveRecentProjects();
    }

    void LauncherGUI::Init(shine::render::backend::IRenderBackend* render)
    {
        renderBackend = render;

        // Setup ImGui style for launcher - UE5 inspired theme
        ImGui::StyleColorsDark();
        auto& style = ImGui::GetStyle();

        // Launcher-specific styling - more polished
        style.WindowRounding = 12.0f;
        style.FrameRounding = 8.0f;
        style.GrabRounding = 8.0f;
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.TabRounding = 8.0f;
        style.ScrollbarRounding = 8.0f;
        style.ChildRounding = 8.0f;
        style.WindowPadding = ImVec2(16, 16);
        style.FramePadding = ImVec2(12, 8);
        style.ItemSpacing = ImVec2(8, 8);
        style.ItemInnerSpacing = ImVec2(8, 8);

        // UE5 inspired color palette - blues and grays
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);        // Dark blue-gray
        colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.15f, 0.19f, 1.00f);         // Slightly lighter
        colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);         // Popup background

        // Title bars
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.10f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.12f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.10f, 0.14f, 1.00f);

        // Buttons - UE5 style with blue accents
        colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.27f, 0.31f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.37f, 0.41f, 1.00f);

        // Primary action button (like Launch) - blue accent
        colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 0.89f, 1.00f);      // UE5 blue

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.56f, 0.89f, 1.00f);      // Active tab blue
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.15f, 0.19f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);

        // Borders and separators
        colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.26f, 0.50f);
        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.22f, 0.26f, 0.50f);

        // Text colors
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

        // Selection and highlighting
        colors[ImGuiCol_Header] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.27f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.56f, 0.89f, 1.00f);
    }

    void LauncherGUI::Render()
    {
        RenderMainWindow();
        RenderNewProjectDialog();
        RenderDeleteConfirmDialog();
        RenderErrorDialog();
    }

    void LauncherGUI::Shutdown()
    {
        SaveSettings();
        SaveRecentProjects();
    }

    void LauncherGUI::RenderMainWindow()
    {
        // Fullscreen window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("ShineEngine Launcher", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Header - UE5 style with logo and version info
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 1.00f));
        ImGui::BeginChild("Header", ImVec2(ImGui::GetContentRegionAvail().x, 90), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Logo and title area
        ImGui::SetCursorPos(ImVec2(24, 16));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f)); // UE5 blue
        ImGui::Text("SHINE");
        ImGui::PopStyleColor();
        ImGui::SetCursorPos(ImVec2(24, 40));
        ImGui::TextDisabled("引擎启动器 v1.0.0");

        // Right side buttons
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 420, 20));

        // Settings button
        if (ImGui::Button("设置", ImVec2(100, 36)))
        {
            currentTab = 2; // Switch to Library tab
        }
        ImGui::SameLine();

        // Library button
        if (ImGui::Button("库", ImVec2(100, 36)))
        {
            currentTab = 2; // Switch to Library tab
        }
        ImGui::SameLine();

        // Launch button - primary action, blue accent
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.45f, 0.71f, 1.00f));
        if (ImGui::Button("启动", ImVec2(180, 36)))
        {
            // Launch the engine directly
            LaunchProject(ProjectInfo{});
        }
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Search bar and filters
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.15f, 0.19f, 1.00f));
        ImGui::BeginChild("Toolbar", ImVec2(ImGui::GetContentRegionAvail().x, 60), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPos(ImVec2(24, 16));

        // Search input
        static char searchBuffer[256] = "";
        ImGui::PushItemWidth(300);
        ImGui::InputText("##搜索", searchBuffer, IM_ARRAYSIZE(searchBuffer));
        ImGui::PopItemWidth();

        ImGui::SameLine();

        // Filter dropdown
        const char* filterItems[] = { "所有项目", "游戏", "模板", "示例" };
        static int selectedFilter = 0;
        ImGui::PushItemWidth(150);
        ImGui::Combo("##过滤", &selectedFilter, filterItems, IM_ARRAYSIZE(filterItems));
        ImGui::PopItemWidth();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Main content area
        ImGui::BeginChild("Content", ImGui::GetContentRegionAvail(), false);

        // Tabs
        if (ImGui::BeginTabBar("LauncherTabs"))
        {
            if (ImGui::BeginTabItem("最近项目"))
            {
                currentTab = 0;
                RenderRecentProjectsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("浏览"))
            {
                currentTab = 1;
                RenderBrowseTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("库"))
            {
                currentTab = 2;
                RenderLibraryTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();

        ImGui::End();

        ImGui::PopStyleVar(3);
    }

    void LauncherGUI::RenderRecentProjectsTab()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(24, 24));

        // Action buttons - top row
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.45f, 0.71f, 1.00f));
        if (ImGui::Button("新建项目", ImVec2(160, 44)))
        {
            showNewProjectDialog = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        if (ImGui::Button("浏览", ImVec2(120, 44)))
        {
            BrowseForProject();
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.26f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.27f, 0.31f, 1.00f));
        if (ImGui::Button("示例", ImVec2(120, 44)))
        {
            // Switch to Browse tab with samples filter
            currentTab = 1;
        }
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Projects section header
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
        ImGui::Text("最近项目");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
        ImGui::Text("(%zu 个项目)", recentProjects.size());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Recent projects grid
        if (recentProjects.empty())
        {
            // Empty state
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 center = ImVec2(avail.x * 0.5f, avail.y * 0.5f - 100);

            ImGui::SetCursorPos(center);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
            ImGui::Text("没有最近项目");
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(ImVec2(center.x, center.y + 40));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
            ImGui::TextWrapped("创建一个新项目或浏览现有项目来开始使用。");
            ImGui::PopStyleColor();
        }
        else
        {
            // Calculate grid layout
            float cardWidth = 320.0f;
            float spacing = 24.0f;
            float availWidth = ImGui::GetContentRegionAvail().x;

            int cardsPerRow = (int)((availWidth + spacing) / (cardWidth + spacing));
            if (cardsPerRow < 1) cardsPerRow = 1;

            // Center the grid if there's space
            float totalWidth = cardsPerRow * cardWidth + (cardsPerRow - 1) * spacing;
            float offsetX = (availWidth - totalWidth) * 0.5f;
            if (offsetX > 0) ImGui::SetCursorPosX(offsetX);

            for (size_t i = 0; i < recentProjects.size(); ++i)
            {
                if (i % cardsPerRow != 0)
                    ImGui::SameLine(0, spacing);

                RenderProjectCard(recentProjects[i], cardWidth);
            }
        }

        ImGui::PopStyleVar();
    }

    void LauncherGUI::RenderBrowseTab()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16, 16));

        ImGui::Text("浏览现有的 ShineEngine 项目");
        ImGui::Separator();

        // Action buttons
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.17f, 0.21f, 1.00f));
        ImGui::BeginChild("BrowseActions", ImVec2(ImGui::GetContentRegionAvail().x, 80), false);

        ImGui::SetCursorPos(ImVec2(16, 16));

        if (ImGui::Button("浏览项目", ImVec2(180, 48)))
        {
            BrowseForProject();
        }

        ImGui::SameLine();

        if (ImGui::Button("扫描目录", ImVec2(180, 48)))
        {
            ImGui::OpenPopup("ScanDirectory");
        }

        ImGui::SameLine();

        if (ImGui::Button("刷新项目", ImVec2(180, 48)))
        {
            // Rescan all known directories
            ScanForProjects(settings.defaultProjectPath);
            fs::path documentsPath = fs::path(getenv("USERPROFILE")) / "Documents" / "ShineEngine";
            if (fs::exists(documentsPath)) {
                ScanForProjects(documentsPath.string().c_str());
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Scan directory popup
        if (ImGui::BeginPopupModal("ScanDirectory", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char scanPath[512] = "";
            if (strlen(scanPath) == 0) {
                strcpy(scanPath, settings.defaultProjectPath.c_str());
            }

            ImGui::Text("输入要扫描项目的目录：");
            ImGui::InputText("##ScanPath", scanPath, IM_ARRAYSIZE(scanPath));

            if (ImGui::Button("扫描", ImVec2(120, 0)))
            {
                if (fs::exists(scanPath)) {
                    ScanForProjects(scanPath);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("取消", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::Text("说明：");
        ImGui::BulletText("使用 '浏览项目' 来选择特定的 project.json 文件");
        ImGui::BulletText("使用 '扫描目录' 来搜索整个文件夹中的项目");
        ImGui::BulletText("使用 '刷新项目' 来从已知位置更新项目列表");

        ImGui::PopStyleVar();
    }

    void LauncherGUI::RenderLibraryTab()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16, 16));

        ImGui::Text("引擎库与内容");
        ImGui::Separator();

        if (ImGui::BeginTabBar("LibraryTabs"))
        {
            if (ImGui::BeginTabItem("引擎版本"))
            {
                RenderEngineVersionsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("插件"))
            {
                RenderPluginsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("内容"))
            {
                RenderContentTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("设置"))
            {
                RenderLibrarySettingsTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }

    void LauncherGUI::RenderEngineVersionsTab()
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.17f, 0.21f, 1.00f));
        ImGui::BeginChild("EngineVersions", ImVec2(ImGui::GetContentRegionAvail().x, 200), false);

        ImGui::SetCursorPos(ImVec2(16, 16));

        // Current engine info
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::Text("当前引擎安装");
        ImGui::PopStyleColor();

        ImGui::Indent(16);
        ImGui::Text("版本: 1.0.0");
        ImGui::Text("位置: %s", engineRootPath.c_str());
        ImGui::Text("平台: Windows x64");
        ImGui::Unindent(16);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Engine actions
        if (ImGui::Button("验证安装", ImVec2(160, 36)))
        {
            ReportError("验证安装", "验证安装功能将在后续版本中实现。");
        }

        ImGui::SameLine();

        if (ImGui::Button("修复引擎", ImVec2(160, 36)))
        {
            ReportError("修复引擎", "修复引擎功能将在后续版本中实现。");
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void LauncherGUI::RenderPluginsTab()
    {
        ImGui::Text("已安装插件");
        ImGui::Separator();

        // List installed plugins
        std::vector<std::string> plugins = {"核心引擎", "脚本系统", "渲染引擎"};

        if (plugins.empty())
        {
            ImGui::TextDisabled("未安装插件");
        }
        else
        {
            for (const auto& plugin : plugins)
            {
                ImGui::Selectable(plugin.c_str(), false);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("浏览插件", ImVec2(160, 36)))
        {
#ifdef SHINE_PLATFORM_WIN64
            ShellExecute(NULL, "open", "https://shineengine.github.io/plugins", NULL, NULL, SW_SHOW);
#endif
        }
    }

    void LauncherGUI::RenderContentTab()
    {
        ImGui::Text("内容与资源");
        ImGui::Separator();

        // Show content statistics
        ImGui::Text("内容统计：");
        ImGui::Indent(16);
        ImGui::Text("项目总数: %zu", recentProjects.size());
        ImGui::Text("可用模板: %zu", projectTemplates.size());
        ImGui::Text("引擎资源: 内置");
        ImGui::Unindent(16);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("打开内容目录", ImVec2(200, 36)))
        {
#ifdef SHINE_PLATFORM_WIN64
            ShellExecute(NULL, "open", (engineRootPath + "\\exe").c_str(), NULL, NULL, SW_SHOW);
#endif
        }

        ImGui::SameLine();

        if (ImGui::Button("验证内容", ImVec2(160, 36)))
        {
            ReportError("验证内容", "验证内容功能将在后续版本中实现。");
        }
    }

    void LauncherGUI::RenderLibrarySettingsTab()
    {
        ImGui::Text("库设置");
        ImGui::Separator();

        // Auto-scan settings
        static bool autoScanProjects = true;
        ImGui::Checkbox("启动时自动扫描项目", &autoScanProjects);

        // Default project location
        ImGui::Text("默认项目位置：");
        static char defaultPath[512];
        strcpy(defaultPath, settings.defaultProjectPath.c_str());
        if (ImGui::InputText("##DefaultPath", defaultPath, IM_ARRAYSIZE(defaultPath)))
        {
            settings.defaultProjectPath = defaultPath;
        }

        ImGui::SameLine();
        if (ImGui::Button("浏览..."))
        {
#ifdef SHINE_PLATFORM_WIN64
            BROWSEINFO bi = { 0 };
            bi.lpszTitle = "选择默认项目位置";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != 0) {
                char path[MAX_PATH];
                if (SHGetPathFromIDList(pidl, path)) {
                    settings.defaultProjectPath = path;
                    strcpy(defaultPath, path);
                }
                CoTaskMemFree(pidl);
            }
#endif
        }

        ImGui::Spacing();

        // Recent projects limit
        ImGui::Text("最大最近项目数：");
        ImGui::SliderInt("##MaxRecent", &settings.maxRecentProjects, 5, 20);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("保存设置", ImVec2(140, 36)))
        {
            SaveSettings();
        }

        ImGui::SameLine();

        if (ImGui::Button("重置为默认", ImVec2(160, 36)))
        {
            settings.showWelcomeDialog = true;
            settings.defaultProjectPath = (fs::current_path() / "Projects").string().c_str();
            settings.maxRecentProjects = 10;
            settings.autoLaunchLastProject = false;
            SaveSettings();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Error log section
        ImGui::Text("错误日志 (%zu 个错误)", errorLog.size());

        if (!errorLog.empty())
        {
            ImGui::SameLine();

            if (ImGui::Button("清除日志", ImVec2(100, 24)))
            {
                ClearErrorLog();
            }

            ImGui::SameLine();

            if (ImGui::Button("显示详情", ImVec2(120, 24)))
            {
                ImGui::OpenPopup("ErrorLogDetails");
            }

            // Error log details popup
            if (ImGui::BeginPopupModal("ErrorLogDetails", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("最近错误：");
                ImGui::Separator();

                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.15f, 0.19f, 1.00f));
                ImGui::BeginChild("ErrorList", ImVec2(600, 300), true);

                for (const auto& error : errorLog)
                {
                    char timeStr[64];
                    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&error.timestamp));

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.50f, 0.50f, 1.00f));
                    ImGui::Text("[%s]", timeStr);
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    ImGui::TextWrapped("%s", error.message.c_str());

                    if (!error.details.empty()) {
                        ImGui::Indent(20);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.72f, 0.76f, 1.00f));
                        ImGui::TextWrapped("%s", error.details.c_str());
                        ImGui::PopStyleColor();
                        ImGui::Unindent(20);
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }

                ImGui::EndChild();
                ImGui::PopStyleColor();

                if (ImGui::Button("关闭", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
            ImGui::Text("未记录错误");
            ImGui::PopStyleColor();
        }
    }

    void LauncherGUI::RenderNewProjectDialog()
    {
        if (!showNewProjectDialog) return;

        ImGui::OpenPopup("Create New Project");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(700, 600));

        if (ImGui::BeginPopupModal("Create New Project", &showNewProjectDialog,
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            // Header
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
            ImGui::Text("创建新项目");
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
            ImGui::TextWrapped("选择项目模板和位置来开始使用 ShineEngine。");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Project Details Section
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
            ImGui::Text("项目详情");
            ImGui::PopStyleColor();

            ImGui::Spacing();

            // Project Name
            ImGui::Text("名称");
            ImGui::PushItemWidth(-1);
            static char projectNameBuffer[256] = "";
            strcpy(projectNameBuffer, newProjectName.c_str());
            ImGui::InputText("##ProjectName", projectNameBuffer, IM_ARRAYSIZE(projectNameBuffer));
            newProjectName = projectNameBuffer;
            ImGui::PopItemWidth();

            ImGui::Spacing();

            // Location
            ImGui::Text("位置");
            ImGui::PushItemWidth(-80);
            static char projectPathBuffer[512] = "";
            strcpy(projectPathBuffer, newProjectPath.c_str());
            ImGui::InputText("##ProjectPath", projectPathBuffer, IM_ARRAYSIZE(projectPathBuffer));
            newProjectPath = projectPathBuffer;
            ImGui::PopItemWidth();

            ImGui::SameLine();
            if (ImGui::Button("浏览...", ImVec2(70, 0)))
            {
#ifdef SHINE_PLATFORM_WIN64
                BROWSEINFO bi = { 0 };
                bi.lpszTitle = "Select Project Location";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
                if (pidl != 0) {
                    char path[MAX_PATH];
                    if (SHGetPathFromIDList(pidl, path)) {
                        newProjectPath = path;
                    }
                    CoTaskMemFree(pidl);
                }
#endif
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Template Selection Section
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
            ImGui::Text("项目模板");
            ImGui::PopStyleColor();

            ImGui::Spacing();

            // Template grid
            float templateCardWidth = 200.0f;
            float templateCardHeight = 120.0f;
            int templatesPerRow = 2;

            for (size_t i = 0; i < projectTemplates.size(); ++i)
            {
                if (i % templatesPerRow != 0)
                    ImGui::SameLine();

                RenderTemplateCard(projectTemplates[i], templateCardWidth);

                // Selection indicator
                if (selectedTemplateIndex == (int)i)
                {
                    ImVec2 pos = ImGui::GetItemRectMin();
                    pos.x -= 3; pos.y -= 3;
                    ImVec2 size = ImVec2(templateCardWidth + 6, templateCardHeight + 6);

                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                                    IM_COL32(40, 90, 180, 255), 8.0f, 0, 3.0f);
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Action buttons
            float buttonWidth = 120.0f;
            float buttonHeight = 36.0f;

            // Right-align buttons
            float availWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(availWidth - (buttonWidth * 2 + 8));

            // Cancel button
            if (ImGui::Button("取消", ImVec2(buttonWidth, buttonHeight)))
            {
                showNewProjectDialog = false;
                newProjectName.clear();
                newProjectPath.clear();
                selectedTemplateIndex = 0;
            }

            ImGui::SameLine();

            // Create button - primary action
            bool canCreate = !newProjectName.empty() && !newProjectPath.empty() &&
                            selectedTemplateIndex >= 0 && selectedTemplateIndex < (int)projectTemplates.size();

        if (!canCreate)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 0.5f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.45f, 0.71f, 1.00f));
        }

        if (ImGui::Button("创建", ImVec2(buttonWidth, buttonHeight)) && canCreate)
        {
            CreateNewProject(newProjectName, newProjectPath.empty() ? settings.defaultProjectPath : newProjectPath, projectTemplates[selectedTemplateIndex]);
            showNewProjectDialog = false;
            newProjectName.clear();
            newProjectPath.clear();
            selectedTemplateIndex = 0;
        }

        ImGui::PopStyleColor(3);

            ImGui::EndPopup();
        }
    }

    void LauncherGUI::RenderProjectCard(const ProjectInfo& project, float cardWidth)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

        // Card background with hover effect
        bool isHovered = false;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.17f, 0.21f, 1.00f));

        ImGui::BeginChild(("ProjectCard_" + project.name).c_str(),
                          ImVec2(cardWidth, 220), true,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Thumbnail area - improved placeholder with icon-like design
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.20f, 0.24f, 1.00f));
        ImGui::BeginChild("Thumbnail", ImVec2(cardWidth - 24, 140), true);

        // Simple project icon representation
        ImVec2 center = ImGui::GetCursorScreenPos();
        center.x += (cardWidth - 24) * 0.5f;
        center.y += 70;

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Draw a simple game controller icon
        float iconSize = 40.0f;
        ImVec2 iconCenter = ImVec2(center.x, center.y);

        // Controller body
        drawList->AddRectFilled(ImVec2(iconCenter.x - 25, iconCenter.y - 15),
                               ImVec2(iconCenter.x + 25, iconCenter.y + 15),
                               IM_COL32(100, 150, 200, 255), 6.0f);

        // Left stick
        drawList->AddCircleFilled(ImVec2(iconCenter.x - 15, iconCenter.y + 5), 8, IM_COL32(150, 200, 255, 255));
        drawList->AddCircle(ImVec2(iconCenter.x - 15, iconCenter.y + 5), 8, IM_COL32(200, 220, 255, 255), 0, 2.0f);

        // Right buttons
        drawList->AddCircleFilled(ImVec2(iconCenter.x + 15, iconCenter.y - 5), 6, IM_COL32(255, 100, 100, 255));
        drawList->AddCircleFilled(ImVec2(iconCenter.x + 15, iconCenter.y + 5), 6, IM_COL32(100, 255, 100, 255));

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();

        // Project info
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
        ImGui::TextWrapped("%s", project.name.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
        ImGui::TextWrapped("%s", project.category.c_str());
        ImGui::PopStyleColor();

        // Engine version
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
        ImGui::Text("Engine: %s", project.engineVersion.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Action buttons
        float buttonWidth = (cardWidth - 32) * 0.5f - 4;

        if (ImGui::Button(("启动##" + project.name).c_str(), ImVec2(buttonWidth, 32)))
        {
            LaunchProject(project);
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.22f, 0.26f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.27f, 0.31f, 1.00f));
        if (ImGui::Button(("...##" + project.name).c_str(), ImVec2(buttonWidth, 32)))
        {
            ImGui::OpenPopup(("ProjectMenu_" + project.name).c_str());
        }
        ImGui::PopStyleColor(2);

        // Context menu
        if (ImGui::BeginPopup(("ProjectMenu_" + project.name).c_str()))
        {
            if (ImGui::MenuItem("在资源管理器中显示"))
            {
#ifdef SHINE_PLATFORM_WIN64
                ShellExecute(NULL, "open", project.path.c_str(), NULL, NULL, SW_SHOW);
#endif
            }
            if (ImGui::MenuItem("从列表中移除"))
            {
                RemoveRecentProject(project.path);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("删除项目"))
            {
                projectToDeletePath = project.path;
                showDeleteConfirm = true;
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    void LauncherGUI::RenderTemplateCard(const ProjectTemplate& template_, float cardWidth)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

        // Template card background
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.20f, 0.24f, 1.00f));

        ImGui::BeginChild(("TemplateCard_" + template_.name).c_str(),
                          ImVec2(cardWidth, 140), true,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Thumbnail area - template icon
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.22f, 0.24f, 0.28f, 1.00f));
        ImGui::BeginChild("Thumbnail", ImVec2(cardWidth - 16, 80), true);

        // Simple template icon based on type
        ImVec2 center = ImGui::GetCursorScreenPos();
        center.x += (cardWidth - 16) * 0.5f;
        center.y += 40;

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        if (template_.name == "空白游戏")
        {
            // Cube icon for blank game
            drawList->AddRectFilled(ImVec2(center.x - 20, center.y - 15),
                                   ImVec2(center.x + 20, center.y + 15),
                                   IM_COL32(100, 150, 200, 255), 4.0f);
        }
        else if (template_.name == "第一人称")
        {
            // Person silhouette for FPS
            drawList->AddCircleFilled(ImVec2(center.x, center.y - 8), 6, IM_COL32(150, 200, 255, 255));
            drawList->AddRectFilled(ImVec2(center.x - 3, center.y - 2),
                                   ImVec2(center.x + 3, center.y + 12),
                                   IM_COL32(150, 200, 255, 255), 2.0f);
        }
        else if (template_.name == "第三人称")
        {
            // Different person icon for TPS
            drawList->AddCircleFilled(ImVec2(center.x, center.y - 8), 6, IM_COL32(200, 150, 255, 255));
            drawList->AddRectFilled(ImVec2(center.x - 3, center.y - 2),
                                   ImVec2(center.x + 3, center.y + 12),
                                   IM_COL32(200, 150, 255, 255), 2.0f);
        }
        else
        {
            // Default icon
            drawList->AddCircleFilled(ImVec2(center.x, center.y), 15, IM_COL32(150, 150, 150, 255));
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();

        // Template info
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
        ImGui::TextWrapped("%s", template_.name.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
        ImGui::TextWrapped("%s", template_.category.c_str());
        ImGui::PopStyleColor();

        // Click to select
        if (ImGui::IsItemClicked())
        {
            selectedTemplateIndex = (int)(&template_ - &projectTemplates[0]);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    void LauncherGUI::LoadRecentProjects()
    {
        fs::path configPath = fs::current_path() / "launcher_config.ini";
        if (!fs::exists(configPath)) {
            // Create default config file
            SaveRecentProjects();
            return;
        }

        std::ifstream configFile(configPath);
        if (!configFile.is_open()) return;

        recentProjects.clear();
        std::string line;
        while (std::getline(configFile, line)) {
            if (line.empty() || line[0] == '#') continue;

            // Parse line format: name|path|category|engineVersion|lastModified
            std::stringstream ss(line);
            std::string name, path, category, engineVersion, lastModifiedStr;
            if (std::getline(ss, name, '|') &&
                std::getline(ss, path, '|') &&
                std::getline(ss, category, '|') &&
                std::getline(ss, engineVersion, '|') &&
                std::getline(ss, lastModifiedStr, '|')) {

                ProjectInfo project;
                project.name = name.c_str();
                project.path = path.c_str();
                project.category = category.c_str();
                project.engineVersion = engineVersion.c_str();
                project.lastModified = std::stoll(lastModifiedStr);

                // Verify project still exists
                if (fs::exists(fs::path(path) / "project.json")) {
                    recentProjects.push_back(project);
                }
            }
        }

        configFile.close();

        // If no projects loaded, add some defaults
        if (recentProjects.empty()) {
            // Create Projects directory if it doesn't exist
            fs::create_directories(fs::current_path() / "Projects");
        }
    }

    void LauncherGUI::LoadSettings()
    {
        fs::path settingsPath = fs::current_path() / "launcher_settings.ini";
        if (!fs::exists(settingsPath)) {
            // Use defaults
            settings.defaultProjectPath = (fs::current_path() / "Projects").string().c_str();
            return;
        }

        std::ifstream settingsFile(settingsPath);
        if (!settingsFile.is_open()) return;

        std::string line;
        while (std::getline(settingsFile, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string key, value;
            if (std::getline(ss, key, '=') && std::getline(ss, value)) {
                if (key == "showWelcomeDialog") {
                    settings.showWelcomeDialog = (value == "true");
                } else if (key == "defaultProjectPath") {
                    settings.defaultProjectPath = value.c_str();
                } else if (key == "maxRecentProjects") {
                    settings.maxRecentProjects = std::stoi(value);
                } else if (key == "autoLaunchLastProject") {
                    settings.autoLaunchLastProject = (value == "true");
                }
            }
        }

        settingsFile.close();

        // Ensure default project path exists
        if (settings.defaultProjectPath.empty()) {
            settings.defaultProjectPath = (fs::current_path() / "Projects").string().c_str();
        }
    }

    void LauncherGUI::SaveSettings()
    {
        fs::path settingsPath = fs::current_path() / "launcher_settings.ini";

        std::ofstream settingsFile(settingsPath);
        if (!settingsFile.is_open()) return;

        // Write header
        settingsFile << "# ShineEngine Launcher Settings\n\n";

        // Write settings
        settingsFile << "showWelcomeDialog=" << (settings.showWelcomeDialog ? "true" : "false") << "\n";
        settingsFile << "defaultProjectPath=" << settings.defaultProjectPath.c_str() << "\n";
        settingsFile << "maxRecentProjects=" << settings.maxRecentProjects << "\n";
        settingsFile << "autoLaunchLastProject=" << (settings.autoLaunchLastProject ? "true" : "false") << "\n";

        settingsFile.close();
    }

    void LauncherGUI::SaveRecentProjects()
    {
        fs::path configPath = fs::current_path() / "launcher_config.ini";

        std::ofstream configFile(configPath);
        if (!configFile.is_open()) return;

        // Write header
        configFile << "# ShineEngine Launcher Configuration\n";
        configFile << "# Format: name|path|category|engineVersion|lastModified\n\n";

        // Write recent projects
        for (const auto& project : recentProjects) {
            configFile << project.name.c_str() << "|"
                      << project.path.c_str() << "|"
                      << project.category.c_str() << "|"
                      << project.engineVersion.c_str() << "|"
                      << project.lastModified << "\n";
        }

        configFile.close();
    }

    void LauncherGUI::AddRecentProject(const ProjectInfo& project)
    {
        // Remove if already exists
        recentProjects.erase(
            std::remove_if(recentProjects.begin(), recentProjects.end(),
                          [&](const ProjectInfo& p) { return p.path == project.path; }),
            recentProjects.end());

        // Add to front
        recentProjects.insert(recentProjects.begin(), project);

        // Keep only last 10
        if (recentProjects.size() > 10)
            recentProjects.resize(10);

        SaveRecentProjects();
    }

    void LauncherGUI::RemoveRecentProject(const SString& projectPath)
    {
        recentProjects.erase(
            std::remove_if(recentProjects.begin(), recentProjects.end(),
                [&](const ProjectInfo& p) { return p.path == projectPath; }),
            recentProjects.end());
        
        SaveRecentProjects();
    }

    void LauncherGUI::DeleteProjectFiles(const SString& projectPath)
    {
        try {
            if (fs::exists(projectPath.c_str())) {
                fs::remove_all(projectPath.c_str());
                RemoveRecentProject(projectPath);
            }
        } catch (const std::exception& e) {
            ReportError("Failed to delete project", e.what());
        }
    }

    void LauncherGUI::LoadProjectTemplates()
    {
        // Add default templates
        projectTemplates.push_back({
            "空白游戏",
            "具有基本设置的空游戏项目",
            "游戏",
            "Templates/BlankGame"
        });

        projectTemplates.push_back({
            "第一人称",
            "基本的第一人称游戏模板",
            "游戏",
            "Templates/FirstPerson"
        });

        projectTemplates.push_back({
            "第三人称",
            "基本的第三人称游戏模板",
            "游戏",
            "Templates/ThirdPerson"
        });
    }

    void LauncherGUI::LaunchProject(const ProjectInfo& project)
    {
        fs::path exePath = fs::current_path() /  "MainEngine.exe";
        if (!fs::exists(exePath)) {
            exePath = fs::current_path() /  "MainEngined.exe";
        }

        if (!fs::exists(exePath)) {
            ReportError("Engine executable not found",
                      SString(fmt::format("{},path:{}", "Please build the engine first using 'build.bat run' or check your installation.", exePath.string()).c_str()));
            return;
        }

        SString command = SString("\"") + exePath.string().c_str() + "\"";
        if (!project.path.empty()) {
            command += " --project \"";
            command += project.path;
            command += "\"";
        }

#ifdef SHINE_PLATFORM_WIN64
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        if (CreateProcess(NULL, command.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            exit(0); // Exit launcher
        } else {
            fmt::println("Failed to lacunch MainEngine.exe");
        }
#endif
    }

    void LauncherGUI::CreateNewProject(const SString& name, const SString& path, const ProjectTemplate& template_)
    {
        fs::path projectPath = fs::path(path.c_str()) / name.c_str();

        try {
            // Create project directory
            fs::create_directories(projectPath);

            // Create basic project structure
            fs::create_directories(projectPath / "Content");
            fs::create_directories(projectPath / "Source");
            fs::create_directories(projectPath / "Plugins");
            fs::create_directories(projectPath / "Assets");

            // Copy template files if template exists
            fs::path templatePath = fs::current_path() / template_.templatePath.c_str();
            if (fs::exists(templatePath)) {
                CopyTemplateFiles(templatePath, projectPath);
            }

            // Create project config file
            CreateProjectConfig(projectPath, name, template_);

            // Create basic source files
            CreateBasicSourceFiles(projectPath, name);

            // Add to recent projects
            ProjectInfo newProject{name, projectPath.string().c_str(), template_.category, "1.0.0"};
            newProject.lastModified = time(nullptr);
            AddRecentProject(newProject);

            fmt::println("Successfully created new project: {}", projectPath.string());

        } catch (const std::exception& e) {
            ReportError("Failed to create project", (SString("Project: ") + name + "\nError: " + e.what()).c_str());
        }
    }

    void LauncherGUI::CopyTemplateFiles(const fs::path& templatePath, const fs::path& projectPath)
    {
        try {
            // Recursively copy all files from template to project
            for (const auto& entry : fs::recursive_directory_iterator(templatePath)) {
                if (fs::is_regular_file(entry)) {
                    fs::path relativePath = fs::relative(entry.path(), templatePath);
                    fs::path targetPath = projectPath / relativePath;
                    fs::create_directories(targetPath.parent_path());
                    fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
                }
            }
        } catch (const std::exception& e) {
            fmt::println("Warning: Failed to copy template files: {}", e.what());
        }
    }

    void LauncherGUI::CreateProjectConfig(const fs::path& projectPath, const SString& name, const ProjectTemplate& template_)
    {
        std::ofstream config(projectPath / "project.json");
        if (config.is_open()) {
            config << "{\n";
            config << "  \"name\": \"" << name.c_str() << "\",\n";
            config << "  \"engineVersion\": \"1.0.0\",\n";
            config << "  \"template\": \"" << template_.name.c_str() << "\",\n";
            config << "  \"category\": \"" << template_.category.c_str() << "\",\n";
            config << "  \"description\": \"" << template_.description.c_str() << "\",\n";
            config << "  \"created\": " << time(nullptr) << ",\n";
            config << "  \"lastModified\": " << time(nullptr) << "\n";
            config << "}\n";
            config.close();
        }
    }

    void LauncherGUI::ScanForProjects(const SString& directory)
    {
        try {
            for (const auto& entry : fs::directory_iterator(directory.c_str())) {
                if (fs::is_directory(entry)) {
                    fs::path projectJsonPath = entry.path() / "project.json";
                    if (fs::exists(projectJsonPath)) {
                        // Read project info from JSON
                        std::ifstream configFile(projectJsonPath);
                        if (configFile.is_open()) {
                            std::string content((std::istreambuf_iterator<char>(configFile)),
                                              std::istreambuf_iterator<char>());

                            // Simple JSON parsing (basic implementation)
                            ProjectInfo project;
                            project.path = entry.path().string().c_str();

                            // Extract name
                            size_t namePos = content.find("\"name\"");
                            if (namePos != std::string::npos) {
                                size_t colonPos = content.find(":", namePos);
                                size_t startQuote = content.find("\"", colonPos);
                                size_t endQuote = content.find("\"", startQuote + 1);
                                if (startQuote != std::string::npos && endQuote != std::string::npos) {
                                    project.name = content.substr(startQuote + 1, endQuote - startQuote - 1).c_str();
                                }
                            }

                            // Extract category
                            size_t catPos = content.find("\"category\"");
                            if (catPos != std::string::npos) {
                                size_t colonPos = content.find(":", catPos);
                                size_t startQuote = content.find("\"", colonPos);
                                size_t endQuote = content.find("\"", startQuote + 1);
                                if (startQuote != std::string::npos && endQuote != std::string::npos) {
                                    project.category = content.substr(startQuote + 1, endQuote - startQuote - 1).c_str();
                                }
                            }

                            // Extract engine version
                            size_t verPos = content.find("\"engineVersion\"");
                            if (verPos != std::string::npos) {
                                size_t colonPos = content.find(":", verPos);
                                size_t startQuote = content.find("\"", colonPos);
                                size_t endQuote = content.find("\"", startQuote + 1);
                                if (startQuote != std::string::npos && endQuote != std::string::npos) {
                                    project.engineVersion = content.substr(startQuote + 1, endQuote - startQuote - 1).c_str();
                                }
                            }

                            if (!project.name.empty()) {
                                project.lastModified = fs::last_write_time(entry.path()).time_since_epoch().count();
                                AddRecentProject(project);
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            fmt::println("Error scanning for projects: {}", e.what());
        }
    }

    void LauncherGUI::BrowseForProject()
    {
#ifdef SHINE_PLATFORM_WIN64
        // Use Windows file dialog
        OPENFILENAME ofn = { sizeof(OPENFILENAME) };
        char szFile[260] = { 0 };

        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Project Files\0project.json\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = settings.defaultProjectPath.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn)) {
            fs::path selectedPath = szFile;
            if (selectedPath.filename() == "project.json") {
                fs::path projectPath = selectedPath.parent_path();
                ScanForProjects(projectPath.parent_path().string().c_str());
            }
        }
#endif
    }

    void LauncherGUI::CreateBasicSourceFiles(const fs::path& projectPath, const SString& name)
    {
        // Create a basic game script file
        fs::path scriptPath = projectPath / "Source" / "main.js";
        std::ofstream script(scriptPath);
        if (script.is_open()) {
            script << "// " << name.c_str() << " - Main Game Script\n";
            script << "// This file is automatically generated by the ShineEngine Launcher\n\n";
            script << "function init() {\n";
            script << "    console.log('Initializing " << name.c_str() << "...');\n";
            script << "    // Add your initialization code here\n";
            script << "}\n\n";
            script << "function update(deltaTime) {\n";
            script << "    // Add your game logic here\n";
            script << "}\n\n";
            script << "function render() {\n";
            script << "    // Add your rendering code here\n";
            script << "}\n";
            script.close();
        }

        // Create a README file
        fs::path readmePath = projectPath / "README.md";
        std::ofstream readme(readmePath);
        if (readme.is_open()) {
            readme << "# " << name.c_str() << "\n\n";
            readme << "A game project created with ShineEngine.\n\n";
            readme << "## Getting Started\n\n";
            readme << "1. Open this project in ShineEngine\n";
            readme << "2. Modify the source files in the `Source` directory\n";
            readme << "3. Add assets to the `Content` directory\n";
            readme << "4. Run the project to test your changes\n\n";
            readme << "## Project Structure\n\n";
            readme << "- `Content/` - Game assets and resources\n";
            readme << "- `Source/` - Game scripts and logic\n";
            readme << "- `Plugins/` - Custom plugins and extensions\n";
            readme << "- `Assets/` - Additional asset files\n";
            readme.close();
        }
    }

    void LauncherGUI::ReportError(const SString& message, const SString& details)
    {
        ErrorInfo error;
        error.message = message;
        error.details = details;
        error.timestamp = time(nullptr);

        errorLog.push_back(error);

        // Keep only last 50 errors
        if (errorLog.size() > 50) {
            errorLog.erase(errorLog.begin());
        }

        // Show immediate error dialog
        currentError = error;
        showErrorDialog = true;

        // Also log to console
        fmt::println("Error: {}", message.c_str());
        if (!details.empty()) {
            fmt::println("Details: {}", details.c_str());
        }
    }

    void LauncherGUI::ClearErrorLog()
    {
        errorLog.clear();
    }

    void LauncherGUI::ShowErrorDialog(const ErrorInfo& error)
    {
        ImGui::OpenPopup("Error");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 300));

        if (ImGui::BeginPopupModal("Error", &showErrorDialog, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            // Error icon (simple red circle with exclamation)
            ImVec2 iconPos = ImGui::GetCursorScreenPos();
            iconPos.x += 20;
            iconPos.y += 20;

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddCircleFilled(ImVec2(iconPos.x + 20, iconPos.y + 20), 20, IM_COL32(220, 53, 69, 255));
            drawList->AddTriangleFilled(ImVec2(iconPos.x + 20, iconPos.y + 12),
                                      ImVec2(iconPos.x + 16, iconPos.y + 20),
                                      ImVec2(iconPos.x + 24, iconPos.y + 20), IM_COL32(255, 255, 255, 255));
            drawList->AddRectFilled(ImVec2(iconPos.x + 19, iconPos.y + 24),
                                  ImVec2(iconPos.x + 21, iconPos.y + 26), IM_COL32(255, 255, 255, 255));

            ImGui::SetCursorPos(ImVec2(80, 20));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
            ImGui::Text("An error occurred");
            ImGui::PopStyleColor();

            ImGui::SetCursorPosY(60);
            ImGui::TextWrapped("%s", error.message.c_str());

            if (!error.details.empty()) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.72f, 0.76f, 1.00f));
                ImGui::Text("详情：");
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.17f, 0.21f, 1.00f));
                ImGui::BeginChild("ErrorDetails", ImVec2(-1, 80), true);
                ImGui::TextWrapped("%s", error.details.c_str());
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            // Action buttons
            float buttonWidth = 100.0f;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth - 20);

            if (ImGui::Button("OK", ImVec2(buttonWidth, 32)))
            {
                showErrorDialog = false;
            }

            ImGui::EndPopup();
        }
    }

    void LauncherGUI::RenderErrorDialog()
    {
        if (showErrorDialog) {
            ShowErrorDialog(currentError);
        }
    }

    void LauncherGUI::RenderDeleteConfirmDialog()
    {
        if (showDeleteConfirm) {
            ImGui::OpenPopup("删除项目?");
        }

        // Center the popup
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("删除项目?", &showDeleteConfirm, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::Text("您确定要永久删除此项目吗？");
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::TextWrapped("%s", projectToDeletePath.c_str());
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::Text("此操作无法撤销！所有项目文件将被永久删除。");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons
            float buttonWidth = 120.0f;
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("删除", ImVec2(buttonWidth, 0))) {
                DeleteProjectFiles(projectToDeletePath);
                showDeleteConfirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            
            if (ImGui::Button("取消", ImVec2(buttonWidth, 0))) {
                showDeleteConfirm = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    bool LauncherGUI::IsEngineInstalled()
    {
        fs::path enginePath = fs::path(engineRootPath.c_str()) / ".." / "Engine";
        if (!fs::exists(enginePath)) {
            enginePath = fs::path(engineRootPath.c_str()) / "Engine";
        }
        return fs::exists(enginePath / "Modules");
    }

    bool LauncherGUI::InstallEngine(const SString& installPath)
    {
        try {
            installProgress = 0.0f;
            installStatus = "正在初始化...";
            fs::path targetPath = fs::path(installPath.c_str()) / "Engine";

            if (!fs::exists(targetPath)) {
                fs::create_directories(targetPath);
            }

            installProgress = 10.0f;
            installStatus = "正在安装模块...";

            fs::path modulesPath = targetPath / "Modules";
            fs::create_directories(modulesPath);

            std::vector<std::pair<fs::path, fs::path>> copyPairs = {
                {fs::current_path() / "Module" / "EngineCore", modulesPath / "EngineCore"},
                {fs::current_path() / "Module" / "Program", modulesPath / "Program"},
                {fs::current_path() / "Module" / "image", modulesPath / "image"},
                {fs::current_path() / "Module" / "loader", modulesPath / "loader"},
                {fs::current_path() / "Module" / "third", modulesPath / "third"},
                {fs::current_path() / "Module" / "util", modulesPath / "util"},
                {fs::current_path() / "Templates", targetPath / "Templates"},
            };

            int totalPairs = (int)copyPairs.size();
            for (int i = 0; i < totalPairs; ++i) {
                if (fs::exists(copyPairs[i].first)) {
                    fs::copy(copyPairs[i].first, copyPairs[i].second,
                            fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                }
                installProgress = 10.0f + (float)(i + 1) / totalPairs * 80.0f;
                installStatus = SString(fmt::format("正在安装模块... ({}/{})", i + 1, totalPairs));
            }

            installProgress = 95.0f;
            installStatus = "正在完成安装...";

            SaveSettings();
            SaveEngineSettings(targetPath / "engine_config.ini");

            installProgress = 100.0f;
            installStatus = "安装完成！";
            installSuccess = true;

            return true;
        } catch (const std::exception& e) {
            installError = e.what();
            installSuccess = false;
            return false;
        }
    }

    void LauncherGUI::SaveEngineSettings(const fs::path& configPath)
    {
        std::ofstream configFile(configPath);
        if (!configFile.is_open()) return;

        configFile << "# ShineEngine Installation Config\n";
        configFile << "enginePath=" << configPath.parent_path().string() << "\n";
        configFile << "version=1.0.0\n";
        configFile.close();
    }

    void LauncherGUI::RenderInstallWizard()
    {
        switch (installState) {
        case InstallState::SelectingLocation:
            RenderInstallLocationPage();
            break;
        case InstallState::Installing:
            RenderInstallProgressPage();
            break;
        case InstallState::Installed:
        case InstallState::Error:
            RenderInstallCompletePage();
            break;
        default:
            RenderEngineSelectPage();
            break;
        }
    }

    void LauncherGUI::RenderEngineSelectPage()
    {
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        ImVec2 windowPos = ImVec2(0, 0);

        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(windowSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("InstallWizard", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar);

        // Left panel - UE5 style blue gradient
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 1.00f));
        ImGui::BeginChild("LeftPanel", ImVec2(windowSize.x * 0.45f, windowSize.y), false);

        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.45f * 0.5f - 100, 120));

        // Logo
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::SetWindowFontScale(3.0f);
        ImGui::Text("SHINE");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(180);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.72f, 0.76f, 1.00f));
        ImGui::TextWrapped("游戏引擎启动器");
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(220);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
        ImGui::TextWrapped("版本 1.0.0");
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Right panel - content
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.15f, 0.19f, 1.00f));
        ImGui::SameLine();
        ImGui::BeginChild("RightPanel", ImVec2(windowSize.x * 0.55f, windowSize.y), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Title
        ImGui::SetCursorPos(ImVec2(40, 60));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
        ImGui::SetWindowFontScale(1.8f);
        ImGui::Text("欢迎使用 ShineEngine");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(40, 120));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
        ImGui::TextWrapped("选择引擎安装位置开始使用，或浏览已安装的引擎版本。");
        ImGui::PopStyleColor();

        // Install location card
        ImGui::SetCursorPos(ImVec2(40, 200));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.20f, 0.24f, 1.00f));
        ImGui::BeginChild("InstallCard", ImVec2(450, 180), true);

        ImGui::SetCursorPos(ImVec2(24, 20));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::Text("安装引擎");
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(24, 55));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.72f, 0.76f, 1.00f));
        ImGui::TextWrapped("首次使用需要安装引擎组件。");
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(24, 120));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.45f, 0.71f, 1.00f));
        if (ImGui::Button("安装引擎...", ImVec2(180, 44))) {
            installState = InstallState::SelectingLocation;
        }
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Existing engine versions
        ImGui::SetCursorPos(ImVec2(40, 400));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
        ImGui::Text("已安装的引擎");
        ImGui::PopStyleColor();

        // Engine version cards
        float cardWidth = 200.0f;
        int cardsPerRow = 3;

        if (engineVersions.empty()) {
            ImGui::SetCursorPos(ImVec2(40, 440));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
            ImGui::Text("未检测到已安装的引擎版本");
            ImGui::PopStyleColor();
        } else {
            for (size_t i = 0; i < engineVersions.size(); ++i) {
                if (i > 0 && i % cardsPerRow == 0) {
                    ImGui::SetCursorPosX(40);
                }
                if (i % cardsPerRow != 0) {
                    ImGui::SameLine(0, 20);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                ImGui::PushStyleColor(ImGuiCol_ChildBg,
                    selectedEngineVersion == (int)i ?
                    ImVec4(0.28f, 0.56f, 0.89f, 0.30f) :
                    ImVec4(0.18f, 0.20f, 0.24f, 1.00f));
                ImGui::BeginChild(("EngineVer" + std::to_string(i)).c_str(),
                                  ImVec2(cardWidth, 100), true);

                ImGui::SetCursorPos(ImVec2(16, 16));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
                ImGui::Text("%s", engineVersions[i].version.c_str());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(ImVec2(16, 45));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
                ImGui::TextDisabled("%s", engineVersions[i].isInstalled ? "已安装" : "未安装");
                ImGui::PopStyleColor();

                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                    selectedEngineVersion = (int)i;
                }

                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
        }

        // Bottom buttons
        float bottomY = windowSize.y - 100;
        ImGui::SetCursorPos(ImVec2(40, bottomY));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.45f, 0.71f, 1.00f));
        if (ImGui::Button("启动引擎", ImVec2(160, 44)) && !engineVersions.empty()) {
            // TODO: Launch selected engine
        }
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    void LauncherGUI::RenderInstallLocationPage()
    {
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        ImVec2 windowPos = ImVec2(0, 0);

        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(windowSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("InstallLocation", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar);

        // Left panel
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 1.00f));
        ImGui::BeginChild("LeftPanel", ImVec2(windowSize.x * 0.45f, windowSize.y), false);

        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.45f * 0.5f - 100, 60));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::SetWindowFontScale(2.5f);
        ImGui::Text("SHINE");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(100);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
        ImGui::Text("选择安装位置");
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Right panel
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.15f, 0.19f, 1.00f));
        ImGui::SameLine();
        ImGui::BeginChild("RightPanel", ImVec2(windowSize.x * 0.55f, windowSize.y), false,
                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPos(ImVec2(40, 60));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("引擎安装位置");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(40, 120));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
        ImGui::TextWrapped("选择引擎组件的安装位置。推荐使用默认位置。");
        ImGui::PopStyleColor();

        // Location input
        ImGui::SetCursorPos(ImVec2(40, 200));
        static char installPathBuffer[512] = "";
        if (strlen(installPathBuffer) == 0) {
            strcpy(installPathBuffer, "C:\\Program Files\\ShineEngine\\Engine");
        }

        ImGui::PushItemWidth(500);
        ImGui::InputText("##InstallPath", installPathBuffer, IM_ARRAYSIZE(installPathBuffer));
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("浏览...", ImVec2(100, 0))) {
#ifdef SHINE_PLATFORM_WIN64
            BROWSEINFO bi = { 0 };
            bi.lpszTitle = "选择引擎安装位置";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != 0) {
                char path[MAX_PATH];
                if (SHGetPathFromIDList(pidl, path)) {
                    strcpy(installPathBuffer, path);
                }
                CoTaskMemFree(pidl);
            }
#endif
        }

        // Space requirements
        ImGui::SetCursorPos(ImVec2(40, 280));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.56f, 1.00f));
        ImGui::Text("所需空间: ~100 MB");
        ImGui::PopStyleColor();

        // Navigation buttons
        float bottomY = windowSize.y - 100;
        ImGui::SetCursorPos(ImVec2(40, bottomY));

        if (ImGui::Button("返回", ImVec2(120, 44))) {
            installState = InstallState::NotInstalled;
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.45f, 0.71f, 1.00f));
        if (ImGui::Button("安装", ImVec2(120, 44))) {
            installPath = installPathBuffer;
            installState = InstallState::Installing;
            InstallEngine(installPath);
        }
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    void LauncherGUI::RenderInstallProgressPage()
    {
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        ImVec2 windowPos = ImVec2(0, 0);

        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(windowSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("InstallProgress", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar);

        // Left panel
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 1.00f));
        ImGui::BeginChild("LeftPanel", ImVec2(windowSize.x * 0.45f, windowSize.y), false);

        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.45f * 0.5f - 100, 60));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::SetWindowFontScale(2.5f);
        ImGui::Text("SHINE");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(100);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
        ImGui::Text("正在安装...");
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Right panel
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.15f, 0.19f, 1.00f));
        ImGui::SameLine();
        ImGui::BeginChild("RightPanel", ImVec2(windowSize.x * 0.55f, windowSize.y), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPos(ImVec2(40, 60));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.00f));
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("安装引擎组件");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        // Progress bar
        ImGui::SetCursorPos(ImVec2(40, 160));
        ImVec2 progressBarSize = ImVec2(500, 24);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.20f, 0.24f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
        ImGui::ProgressBar(installProgress / 100.0f, progressBarSize);
        ImGui::PopStyleColor(3);

        // Progress text
        ImGui::SetCursorPos(ImVec2(40, 200));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.72f, 0.76f, 1.00f));
        ImGui::Text("%s", installStatus.c_str());
        ImGui::PopStyleColor();

        // Percentage
        ImGui::SetCursorPos(ImVec2(560, 160));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
        ImGui::Text("%.0f%%", installProgress);
        ImGui::PopStyleColor();

        // Cancel button
        float bottomY = windowSize.y - 100;
        ImGui::SetCursorPos(ImVec2(40, bottomY));

        if (ImGui::Button("取消", ImVec2(120, 44))) {
            installState = InstallState::SelectingLocation;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    void LauncherGUI::RenderInstallCompletePage()
    {
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        ImVec2 windowPos = ImVec2(0, 0);

        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSize(windowSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("InstallComplete", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar);

        // Left panel
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.14f, 1.00f));
        ImGui::BeginChild("LeftPanel", ImVec2(windowSize.x * 0.45f, windowSize.y), false);

        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.45f * 0.5f - 100, 60));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
        ImGui::SetWindowFontScale(2.5f);
        ImGui::Text("SHINE");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(100);

        if (installSuccess) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.80f, 0.40f, 1.00f));
            ImGui::Text("✓ 安装成功");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.30f, 0.30f, 1.00f));
            ImGui::Text("✗ 安装失败");
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Right panel
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.15f, 0.19f, 1.00f));
        ImGui::SameLine();
        ImGui::BeginChild("RightPanel", ImVec2(windowSize.x * 0.55f, windowSize.y), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPos(ImVec2(40, 60));

        if (installSuccess) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.80f, 0.40f, 1.00f));
            ImGui::SetWindowFontScale(1.5f);
            ImGui::Text("安装完成！");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(ImVec2(40, 120));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
            ImGui::TextWrapped("ShineEngine 已成功安装到以下位置：");
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(ImVec2(40, 160));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.20f, 0.24f, 1.00f));
            ImGui::BeginChild("PathDisplay", ImVec2(500, 50), true);
            ImGui::SetCursorPos(ImVec2(16, 17));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
            ImGui::Text("%s", installPath.c_str());
            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.30f, 0.30f, 1.00f));
            ImGui::SetWindowFontScale(1.5f);
            ImGui::Text("安装失败");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(ImVec2(40, 120));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.62f, 0.66f, 1.00f));
            ImGui::TextWrapped("安装过程中发生错误：");
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(ImVec2(40, 160));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.30f, 0.15f, 0.15f, 1.00f));
            ImGui::BeginChild("ErrorDisplay", ImVec2(500, 100), true);
            ImGui::SetCursorPos(ImVec2(16, 16));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.90f, 1.00f));
            ImGui::TextWrapped("%s", installError.empty() ? "未知错误" : installError.c_str());
            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        // Navigation buttons
        float bottomY = windowSize.y - 100;
        ImGui::SetCursorPos(ImVec2(40, bottomY));

        if (ImGui::Button("返回", ImVec2(120, 44))) {
            installState = installSuccess ? InstallState::NotInstalled : InstallState::SelectingLocation;
        }

        if (installSuccess) {
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.56f, 0.89f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.63f, 0.96f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.21f, 0.45f, 0.71f, 1.00f));
            if (ImGui::Button("启动引擎", ImVec2(160, 44))) {
                // TODO: Launch engine
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar(3);
    }
}
