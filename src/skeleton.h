
#ifndef ZEQ_SKELETON_H
#define ZEQ_SKELETON_H

#include <math.h>
#include <vector>
#include <unordered_map>
#include "type.h"
#include "exception.h"

class Bone;
class SkeletonSet;
class MobInstance;

class Skeleton
{
public:
	Skeleton(uint16 numBones);
	~Skeleton();
	Bone*	GetBone(uint16 id) const;
	void	AddBone(Bone* bone, uint16 id);
	uint16	GetNumBones() const { return mBoneNum; }
private:
	Bone**	mBoneArray;
	uint16	mBoneNum;
};

class Bone
{
public:
	Bone(float xRot, float yRot, float zRot, float xTrans, float yTrans, float zTrans, Bone* parent = nullptr);
	virtual bool IsAttachmentBone() { return false; }
private:
	float mXRotation;
	float mYRotation;
	float mZRotation;
	float mXShift;
	float mYShift;
	float mZShift;
	friend class SkeletonSet;
	friend class MobInstance;
};


class AttachmentBone : public Bone
{
public:
	AttachmentBone();
	bool IsAttachmentBone() override { return true; }
private:
	Ogre::SceneNode* mAttachNode;
};


struct AnimationKeyframe
{
	Skeleton* skeleton;
	float time;
};

class Animation
{
public:
	void	AddSkeleton(Skeleton* skele, float time);
	Skeleton* GetSkeletonByKeyframe(uint16 frameNum);
	AnimationKeyframe* GetKeyframe(uint16 frameNum);
private:
	std::vector<AnimationKeyframe> mKeyframes;
};


struct BoneAssignment
{
	uint16 boneIndex;
	uint16 vertexCount;
};

struct MeshData
{
	Ogre::MeshPtr mesh;
	float* baseVertexData;
	float* baseNormalData;
	float* targetVertexBuffer;
	float* targetNormalBuffer;
};

class SkeletonSet
{
public:
	SkeletonSet(Skeleton* baseSkele, uint8 numMeshes);
	~SkeletonSet();
	void	AddMesh(Ogre::MeshPtr& mesh, uint8 pos, float* vertices, float* normals, uint16 numVertices);
	void	AddBoneAssignment(uint16 bone_id, uint16 count, uint8 mesh_id);
	void	Complete();
	void	Test(Ogre::SceneManager* sceneMgr);
	bool	HasAnimation(const char* name);
	Animation* GetAnimation(const char* name);
	void	AddAnimation(Animation* anim, const char* name);
	uint8	GetMeshNum() const { return mMeshNum; }
	MeshData* GetMeshData(uint8 num) const;
	std::vector<BoneAssignment>* GetBoneAssignments(uint8 meshNum);
private:
	Skeleton* mBaseSkeleton;
	std::unordered_map<std::string,Animation*> mAnimations;
	std::vector<BoneAssignment>* mBoneAssignments;
	MeshData* mMeshArray;
	uint8	mMeshNum;
	Ogre::SceneNode* mNode;
};


class MobInstance
{
public:
	MobInstance(SkeletonSet* skeleSet, const char* animName, Ogre::SceneManager* sceneMgr);
	void	AddAnimTime(float time);
private:
	SkeletonSet* mSkeletonSet;
	Animation* mCurAnim;
	AnimationKeyframe* mCurKeyframe;
	AnimationKeyframe* mNextKeyframe;
	uint16 mNextFrameNum;
	float mAnimTime;
	Ogre::SceneNode* mNode;
};

#endif
