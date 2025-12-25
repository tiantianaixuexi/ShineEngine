#pragma once
#include "selectionData.h"

#include <unordered_set>

namespace shine::gameplay
{
	class SObject;
}


namespace shine::editor::selection
{
	class SelectionManager
	{


	public:


		inline std::vector<SelectionData>& GetSelectionData() noexcept
		{
			return _selectData;
		}

		

		void selct(SelectionData& _Data)
		{
			if (_selectDataSet.insert(&_Data).second)
			{
				_selectData.push_back(_Data);
			}
		}

		void remove(SelectionData& _Data)
		{
			auto it = _selectDataSet.find(&_Data);
			if (it != _selectDataSet.end())
			{
				_selectDataSet.erase(it);
				std::erase_if(_selectData,
				              [&_Data](const SelectionData& data) { return data.objectId == _Data.objectId; });
			}
		}

		bool isSelect(SelectionData& _Data)
		{
			return _selectDataSet.contains(&_Data);
		}


	private:

		std::unordered_set<SelectionData*> _selectDataSet;
		std::vector<SelectionData> _selectData;
	};
}
