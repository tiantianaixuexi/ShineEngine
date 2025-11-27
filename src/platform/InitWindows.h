#pragma once


#include "util/shine_define.h"

namespace shine::windows
{

	struct FWindowInfo
	{
		HWND*   hwnd = nullptr;
		int     width = 0;
		int     height = 0;

		FWindowInfo()
		{

		}
	};


	bool InitWindowsPlatform();

}