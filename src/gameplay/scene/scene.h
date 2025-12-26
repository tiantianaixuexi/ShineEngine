#pragma once


#include "../object.h"
#include "wasm/SArray.h"

namespace shine::gameplay::scene
{

	//场景
	class Scene : public SObject
	{
		
	public:

		void OnInit() override;


	private:
		wasm::HashArray<u16> _objectIds;
		
	};
}
