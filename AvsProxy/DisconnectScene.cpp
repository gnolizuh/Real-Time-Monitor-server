#include "DisconnectScene.h"

scene_opt_t DisconnectScene::Maintain(Termination *termination)
{
	if(termination)
	{
		delete termination;
		termination = nullptr;
	}

	return SCENE_OPT_NONE;
}
