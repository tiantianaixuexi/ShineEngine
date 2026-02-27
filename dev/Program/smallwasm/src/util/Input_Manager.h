#pragma once


namespace shine::input
{
	struct MouseState
	{
		float x{};
		float y{};
		int   idDown {};


	};



	class InputManager
	{
	public:

		static InputManager& getInstance()
		{
			static InputManager instance;
			return instance;
		}


	};
}

#define IPMINST (shine::input::InputManager::getInstance())
