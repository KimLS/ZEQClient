
#ifndef ZEQ_MOB_MANAGER_H
#define ZEQ_MOB_MANAGER_H

#include <vector>
#include <unordered_map>
#include "type.h"

class MobType
{
public:
	void AddMesh(Ogre::MeshPtr& mesh);
	Ogre::AnimationState* Spawn(Ogre::SceneManager* sceneMgr);
private:
	std::vector<Ogre::MeshPtr> mMeshes;
};

class MobManager
{
public:
	void AddMobType(uint32 id, MobType* mob);
	Ogre::AnimationState* Spawn(uint32 id, Ogre::SceneManager* sceneMgr);
private:
	std::unordered_map<uint32,MobType*> mMobTypes;
};

#endif
