
#ifndef ZEQ_ZONE_DATA_H
#define ZEQ_ZONE_DATA_H

#define MANUAL_SKELETONS

#include "type.h"
#include "fragment.h"
#include "sprite.h"
#include "skeleton.h"
#include "mob_manager.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <algorithm>
#include <math.h>

struct ZoneData
{
	ZoneData(Ogre::SceneManager* sceneMgr);
	void LoadSprites();
	void BuildMesh(MeshFragment* mesh, Ogre::SceneManager* sceneMgr, const char* model_name = nullptr);
	void BuildZoneMeshes(Ogre::SceneManager* sceneMgr);
	void BuildObjectMeshes(Ogre::SceneManager* sceneMgr);
	void BuildMobModelMeshes(Ogre::SceneManager* sceneMgr);
#ifdef MANUAL_SKELETONS
	SkeletonSet* ReadMobModelTree(Ogre::SceneManager* sceneMgr, SkeletonTrackSetFragment* track, const char* model_name);
	void LoadBoneAnimations(Bone* bone, Skeleton* skele, SkeletonSet* skeleSet, const char* name, uint16 index, uint16 parent_index = 0);
#else
	void ReadMobModelTree(Ogre::SceneManager* sceneMgr, SkeletonTrackSetFragment* track, const char* model_name, uint32 id);
	void LoadBoneAnimations(Ogre::Bone* bone, Ogre::SkeletonPtr& skele, const char* name, uint16 index);
#endif

	Fragment* GetFragment(int32 index);
	Fragment* GetFragment(const char* name);

	byte* mNameData; //raw fragment name block, fragments point to entries in here so we need to maintain it
	std::unordered_map<uint32,Fragment*> mFragsByIndex;
	std::unordered_map<std::string,Fragment*> mFragsByName;
	std::unordered_map<uint32,std::unordered_map<int16,Sprite*>> mSpriteList;
	std::unordered_map<std::string,MeshFragment*> mModelMeshList;
	std::vector<MeshFragment*> mZoneMeshFrags;
	//std::vector<MeshFragment*> mObjMeshFrags;
	std::vector<TextureListFragment*> mTextureListFrags;
	std::vector<ObjectLocRefFragment*> mObjLocRefFrags;
	std::vector<ModelFragment*> mModelFrags;
	std::vector<AnimatedMeshRefFragment*> mAnimMeshFrags;
	std::unordered_map<std::string,SkeletonPieceRefFragment*> mSkelePieceRefFrags;

	Ogre::StaticGeometry* mStaticGeometry;
	MobManager mMobManager;
	Ogre::AnimationState* mAnimState;
	MobInstance* mMobInstance;
};

#endif
