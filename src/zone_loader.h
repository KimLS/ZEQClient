
#ifndef ZEQ_ZONE_LOADER_H
#define ZEQ_ZONE_LOADER_H

#include <stdio.h>
#include <list>
#include <vector>
#include <unordered_map>
#include "type.h"
#include "exception.h"
#include "gfx_loaders.h"
#include "fragment.h"
#include "zone_data.h"

#include "TutorialFramework.h"

class ZoneLoader : public BaseApplication
{
public:
	ZoneLoader();
	~ZoneLoader();
	void Load(const char* shortname);
	void createScene() override;
	void createCamera() override;
	void createViewports() override;
	bool frameRenderingQueued(const Ogre::FrameEvent& evt) override;
private:
	ZoneData* mZoneData;
};

#endif
