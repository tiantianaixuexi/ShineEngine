#include "scene.h"

void shine::gameplay::scene::Scene::OnInit()
{
	SObject::OnInit();

	_objectIds = wasm::HashArray<u16>(sizeof(u16));
}
