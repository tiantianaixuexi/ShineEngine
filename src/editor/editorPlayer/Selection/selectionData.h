#pragma once
#include <vector>


namespace shine::gameplay
{
	class SObject;
}


namespace shine::editor::selection
{
	struct SelectionData
	{
	public:


		int objectId = -1;
		int parentId = -1;


		std::vector<SelectionData> Children;
	};
}
