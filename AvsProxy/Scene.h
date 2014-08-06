#ifndef __AVS_PROXY_SCENE__
#define __AVS_PROXY_SCENE__

#include "Com.h"
#include "Room.h"
#include "Parameter.h"
#include "Termination.h"

class Scene
{
public:
	Scene() {}
	virtual ~Scene() {}

	virtual void Maintain(Parameter *, Termination *, Room *) = 0;
};

#endif
