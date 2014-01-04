
#ifndef ZEQ_SPRITE_H
#define ZEQ_SPRITE_H

#include "type.h"
#include <vector>

class Sprite //make this a struct
{
public:
	Sprite(uint32 flags, std::vector<const char*>& namelist, int32 anim_delay = 0);
	std::vector<const char*> mTextureNameList;
private:
	uint32 mFlags;
	int32 mAnimDelay;
};

#endif
