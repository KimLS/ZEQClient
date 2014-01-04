
#include "mob_manager.h"

void MobType::AddMesh(Ogre::MeshPtr& mesh)
{
	mMeshes.push_back(mesh);
}

Ogre::AnimationState* MobType::Spawn(Ogre::SceneManager* sceneMgr)
{
	Ogre::AnimationState* anim;
	Ogre::SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode();
	node->setPosition(0,0,0);

	Ogre::Entity* firstEnt = nullptr;
	for (auto itr = mMeshes.begin(); itr != mMeshes.end(); itr++)
	{
		Ogre::MeshPtr mesh = *itr;
		Ogre::Entity* ent = sceneMgr->createEntity(mesh);
		if (firstEnt)
			ent->shareSkeletonInstanceWith(firstEnt);
		else
		{
			firstEnt = ent;
			anim = ent->getAnimationState("C05");
			anim->setEnabled(true);
			anim->setLoop(true);
			//anim->addTime(0.1);
		}
		node->attachObject(ent);
	}
	return anim;
}


void MobManager::AddMobType(uint32 id, MobType* mob)
{
	if (!mMobTypes.count(id))
	{
		mMobTypes[id] = mob;
	}
}

Ogre::AnimationState* MobManager::Spawn(uint32 id, Ogre::SceneManager* sceneMgr)
{
	if (mMobTypes.count(id))
	{
		MobType* mob = mMobTypes[id];
		return mob->Spawn(sceneMgr);
	}
	return nullptr;
}
