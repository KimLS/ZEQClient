
#include "sprite.h"

Sprite::Sprite(uint32 flags, std::vector<const char*>& namelist, int32 anim_delay)
{
	mFlags = flags;
	mTextureNameList = namelist;
	mAnimDelay = anim_delay;
}
