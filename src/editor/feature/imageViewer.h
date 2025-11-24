#pragma once

#include "image/texture.h"

namespace shine::editor::imageViewer
{

	class SImageViewer
	{
	public:

		SImageViewer();
		~SImageViewer();
		   

	public:

		void Render();


		// 图片
		image::STexture* texture = nullptr;

		bool open = false;



	private:

	};


}
