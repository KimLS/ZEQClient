
#ifndef ZEQ_GFX_LOADERS_H
#define ZEQ_GFX_LOADERS_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
//unix bitmap headers?
#endif

#include <stdio.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <string>
#include <utility>
#include "zlib.h"
#include "type.h"
#include "exception.h"
#include "buffer.h"
#include "fragment.h"
#include "sprite.h"
#include "byte_order.h"
#include "zone_data.h"


struct S3DFileEntry
{
	const char* mFileName;
	uint32 mDataSize;
	byte* mData;
};

class S3D
{
public:
	S3D(FILE* fp, ZoneData* zone_data, Ogre::SceneManager* sceneMgr, bool is_main = false, bool is_obj = false);
	void LoadContents(ZoneData* zone_data, std::vector<S3DFileEntry>* fileEntries, Ogre::SceneManager* sceneMgr, bool is_main = false, bool is_obj = false);
};

class WLD
{
public:
	WLD(FILE* fp);
	WLD(S3DFileEntry* entry, ZoneData* zone_data, Ogre::SceneManager* sceneMgr, bool is_main = false, bool is_obj = false);

	enum MagicNumbers {
		MAGIC = 0x54503D02,
		VERSION1 = 0x00015500,
		VERSION2 = 0x1000C800,
		MESH_ZONE = 0x00018003,
		//MESH_OBJECT = 0x00014003,
	};
};


#endif
