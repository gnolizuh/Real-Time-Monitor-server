#include "DisconnectScene.h"

void DisconnectScene::Maintain(Termination *termination)
{
	if(termination)
	{
		delete termination;
		termination = nullptr;
	}
}
