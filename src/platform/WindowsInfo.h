#pragma once

#include <vector>


#include "util/shine_define.h"
#include "util/singleton.h"

namespace shine::windows
{
	struct FDisplayInfo
	{
		int      id = -1;
		int		 displaySize[2] = {};    //显示器的宽度和高度
		int		 workSize[2]    = {};      //显示器可用的高度和宽度

		char	 displName [32] = {};   //显示器设备名称

		bool     isMain = false; //是否为主显示器

		FDisplayInfo()
		{
			
		}
	};

	struct FWindowInfo
	{
		HWND  hwnd {};

		FWindowInfo()
		{

		}
	};

	class WindowsDeviceInfo : public util::Singleton<WindowsDeviceInfo>
	{
	public:

		std::vector<FDisplayInfo> DisplayInfos;
		FDisplayInfo MainDisplayInfo;


		void Init();


		void InitDisplayInfo();
	};



	class WindowsInfo : public util::Singleton<WindowsInfo>
	{

	public:
		FWindowInfo info;
	};

}