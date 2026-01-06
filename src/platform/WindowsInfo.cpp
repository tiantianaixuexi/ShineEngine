#include "WindowsInfo.h"
#include <fmt/format.h>


namespace shine::windows
{

    BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {

        WindowsDeviceInfo* device = reinterpret_cast<WindowsDeviceInfo*>(dwData);
        if (!device) return FALSE;

        MONITORINFOEX monitorInfo;
        monitorInfo.cbSize = sizeof(monitorInfo);

        if (GetMonitorInfo(hMonitor, &monitorInfo)) {

            FDisplayInfo Info;

			// 检查是否为主显示器
            const bool isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;

			// 计算显示器的宽度和高度
            int width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
            int height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

            // 计算可用工作区域大小
            int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
            int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;


            Info.isMain = isPrimary;

            Info.displaySize[0] = width;
            Info.displaySize[1] = height;

			Info.workSize[0] = workWidth;
			Info.workSize[1] = workHeight;



			memcpy(Info.displName, monitorInfo.szDevice, sizeof(monitorInfo.szDevice));

			Info.id = static_cast<int>(device->DisplayInfos.size());
            device->DisplayInfos.push_back(Info);

            if (isPrimary)
            {
                device->MainDisplayInfo = device->DisplayInfos.back();
            }
        }
		return TRUE;
    }





	void WindowsDeviceInfo::Init()
	{
        InitDisplayInfo();
	}

	void WindowsDeviceInfo::InitDisplayInfo()
	{
        EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(this));
        for (auto& c : DisplayInfos)
        {
	        fmt::println("显示器 ID: {}, 设备名称: {}, 大小: {}x{}, 工作区大小: {}x{}, 是否是主屏幕: {}",
                c.id,
                c.displName,
                c.displaySize[0], c.displaySize[1],
                c.workSize[0], c.workSize[1],
				c.isMain);
        }
        
	}
}
