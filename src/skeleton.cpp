
#include "skeleton.h"

SkeletonSet::SkeletonSet(Skeleton* baseSkele, uint8 numMeshes)
{
	mBaseSkeleton = baseSkele;
	mMeshArray = new MeshData[numMeshes];
	mMeshNum = numMeshes;
	mBoneAssignments = new std::vector<BoneAssignment>[numMeshes];
}

Skeleton::Skeleton(uint16 numBones)
{
	mBoneArray = new Bone*[numBones];
	mBoneNum = numBones;
}

Skeleton::~Skeleton()
{
	for (uint16 i = 0; i < mBoneNum; ++i)
	{
		delete mBoneArray[i];
	}
	delete[] mBoneArray;
}

SkeletonSet::~SkeletonSet()
{
	for (uint8 i = 0; i < mMeshNum; ++i)
	{
		MeshData& d = mMeshArray[i];
		delete[] d.baseVertexData;
		delete[] d.baseNormalData;
		delete[] d.targetVertexBuffer;
		delete[] d.targetNormalBuffer;
		//ogre handles getting rid of the meshes themselves
	}
	delete[] mMeshArray;
	delete[] mBoneAssignments;
}

bool SkeletonSet::HasAnimation(const char* name)
{
	return (mAnimations.count(name) == 1);
}

Animation* SkeletonSet::GetAnimation(const char* name)
{
	if (mAnimations.count(name) == 1)
		return mAnimations[name];
	return nullptr;
}

void SkeletonSet::AddAnimation(Animation* anim, const char* name)
{
	if (mAnimations.count(name) == 0)
		mAnimations[name] = anim;
}

Bone* Skeleton::GetBone(uint16 id) const
{
	if (id >= mBoneNum)
		throw ZEQException("Attempt to retrieve out-of-bounds bone id from skeleton");
	return mBoneArray[id];
}

void Skeleton::AddBone(Bone* bone, uint16 index)
{
	if (index >= mBoneNum)
		throw ZEQException("Attempt to add out-of-bounds bone id to skeleton");
	mBoneArray[index] = bone;
}

void SkeletonSet::AddBoneAssignment(uint16 bone_id, uint16 count, uint8 mesh_id)
{
	BoneAssignment ba;
	ba.boneIndex = bone_id;
	ba.vertexCount = count;
	mBoneAssignments[mesh_id].push_back(ba);
}

void SkeletonSet::AddMesh(Ogre::MeshPtr& mesh, uint8 pos, float* vertices, float* normals, uint16 numVertices)
{
	if (pos >= mMeshNum)
		throw ZEQException("Attempt to add out-of-bound mesh to skeleton");
	MeshData& data = mMeshArray[pos];
	data.mesh = mesh;
	data.baseVertexData = vertices;
	data.baseNormalData = normals;
	data.targetVertexBuffer = new float[numVertices * 3];
	data.targetNormalBuffer = new float[numVertices * 3];
}

void SkeletonSet::Complete()
{
	for (uint8 i = 0; i < mMeshNum; ++i)
	{
		mBoneAssignments[i].shrink_to_fit();
	}
}

void SkeletonSet::Test(Ogre::SceneManager* sceneMgr)
{
	mNode = sceneMgr->getRootSceneNode()->createChildSceneNode();
	mNode->setPosition(0,0,0);
	Skeleton* skele = mBaseSkeleton;

	for (uint8 i = 0; i < mMeshNum; ++i)
	{
		MeshData& meshdata = mMeshArray[i];
		Ogre::Entity* ent = sceneMgr->createEntity(meshdata.mesh);
		mNode->attachObject(ent);

		float* vdata = meshdata.baseVertexData;
		float* ndata = meshdata.baseNormalData;
		float* vtarget = meshdata.targetVertexBuffer;
		float* ntarget = meshdata.targetNormalBuffer;

		for (auto itr = mBoneAssignments[i].begin(); itr != mBoneAssignments[i].end(); itr++)
		{
			BoneAssignment& ba = *itr;

			Bone* bone = skele->GetBone(ba.boneIndex);
			float xrot = bone->mXRotation;
			float yrot = bone->mYRotation;
			float zrot = bone->mZRotation;
			float xtrans = bone->mXShift;
			float ytrans = bone->mYShift;
			float ztrans = bone->mZShift;
			for (uint16 j = 0; j < ba.vertexCount; ++j)
			{
				//order is y, z, x
				float tempY = *vdata++;
				float z = *vdata++;
				float tempX = *vdata++;
				float x, y;
				//x axis
				y = (cos(xrot) * tempY) - (sin(xrot) * z);
				z = (sin(xrot) * tempY) + (cos(xrot) * z);
				//y axis
				x = (cos(yrot) * tempX) + (sin(yrot) * z);
				z = -(sin(yrot) * tempX) + (cos(yrot) * z);
				//z axis
				tempX = x;
				x = (cos(zrot) * tempX) - (sin(zrot) * y);
				y = (sin(zrot) * tempX) + (cos(zrot) * y);
				//add shifts
				x += xtrans;
				y += ytrans;
				z += ztrans;
				
				//write them into the vertex buffer
				*vtarget++ = y;
				*vtarget++ = z;
				*vtarget++ = x;

				//normals
				tempY = *ndata++;
				z = *ndata++;
				tempX = *ndata++;
				//x axis
				y = (cos(xrot) * tempY) - (sin(xrot) * z);
				z = (sin(xrot) * tempY) + (cos(xrot) * z);
				//y axis
				x = (cos(yrot) * tempX) + (sin(yrot) * z);
				z = -(sin(yrot) * tempX) + (cos(yrot) * z);
				//z axis
				tempX = x;
				x = (cos(zrot) * tempX) - (sin(zrot) * y);
				y = (sin(zrot) * tempX) + (cos(zrot) * y);
				//add shifts
				x += xtrans;
				y += ytrans;
				z += ztrans;

				//write them into the normal buffer
				*ntarget++ = y;
				*ntarget++ = z;
				*ntarget++ = x;
			}
		}

		Ogre::VertexBufferBinding* bind = meshdata.mesh->sharedVertexData->vertexBufferBinding;
		Ogre::HardwareVertexBufferSharedPtr vbuf = bind->getBuffer(0);
		Ogre::HardwareVertexBufferSharedPtr nbuf = bind->getBuffer(1);
		vbuf->writeData(0,vbuf->getSizeInBytes(),meshdata.targetVertexBuffer,true);
		nbuf->writeData(0,nbuf->getSizeInBytes(),meshdata.targetNormalBuffer,true);
	}
}


Bone::Bone(float xRot, float yRot, float zRot, float xTrans, float yTrans, float zTrans, Bone* parent)
{
	if (parent)
	{
		//shifts are rotated according to the parent's rotation
		float x, y, z;
		//x axis
		float rot = parent->mXRotation;
		y = (cos(rot) * yTrans) - (sin(rot) * zTrans);
		z = (sin(rot) * yTrans) + (cos(rot) * zTrans);
		//y axis
		rot = parent->mYRotation;
		x = (cos(rot) * xTrans) + (sin(rot) * z);
		z = -(sin(rot) * xTrans) + (cos(rot) * z);
		//z axis
		rot = parent->mZRotation;
		xTrans = x;
		x = (cos(rot) * xTrans) - (sin(rot) * y);
		y = (sin(rot) * xTrans) + (cos(rot) * y);
		//add parent's shifts
		mXShift = x + parent->mXShift;
		mYShift = y + parent->mYShift;
		mZShift = z + parent->mZShift;
		//add parent's rotations - should we check for > 2 * pi here?
		mXRotation = xRot + parent->mXRotation;
		mYRotation = yRot + parent->mYRotation;
		mZRotation = zRot + parent->mZRotation;
	}
	else
	{
		mXRotation = xRot;
		mYRotation = yRot;
		mZRotation = zRot;
		mXShift = xTrans;
		mYShift = yTrans;
		mZShift = zTrans;
	}
}

void Animation::AddSkeleton(Skeleton* skele, float time)
{
	AnimationKeyframe key;
	key.skeleton = skele;
	key.time = time;
	mKeyframes.push_back(key);
}

Skeleton* Animation::GetSkeletonByKeyframe(uint16 frameNum)
{
	if (mKeyframes.size() >= frameNum)
		return mKeyframes[frameNum].skeleton;
	return nullptr;
}

AnimationKeyframe* Animation::GetKeyframe(uint16 frameNum)
{
	if (mKeyframes.size() >= frameNum)
		return &mKeyframes[frameNum];
	return nullptr;
}


MeshData* SkeletonSet::GetMeshData(uint8 num) const
{
	if (num <= mMeshNum)
		return &mMeshArray[num];
	return nullptr;
}

std::vector<BoneAssignment>* SkeletonSet::GetBoneAssignments(uint8 meshNum)
{
	if (meshNum <= mMeshNum)
		return &mBoneAssignments[meshNum];
	return nullptr;
}


MobInstance::MobInstance(SkeletonSet* skeleSet, const char* animName, Ogre::SceneManager* sceneMgr)
{
	mSkeletonSet = skeleSet;
	mCurAnim = skeleSet->GetAnimation(animName);
	mCurKeyframe = mCurAnim->GetKeyframe(0);
	mNextKeyframe = mCurAnim->GetKeyframe(1);
	mAnimTime = 0;
	mNextFrameNum = 1;
	mNode = sceneMgr->getRootSceneNode()->createChildSceneNode();
	mNode->setPosition(0,0,0);
	uint8 meshNum = skeleSet->GetMeshNum();
	for (uint8 i = 0; i < meshNum; ++i)
	{
		Ogre::Entity* ent = sceneMgr->createEntity(skeleSet->GetMeshData(i)->mesh);
		mNode->attachObject(ent);
	}
	AddAnimTime(0);
}

void MobInstance::AddAnimTime(float time)
{
	mAnimTime += time;
	//advance frame if needed
	while (mAnimTime >= mNextKeyframe->time)
	{
		AnimationKeyframe* temp = mCurAnim->GetKeyframe(++mNextFrameNum);
		if (temp)
		{
			mCurKeyframe = mNextKeyframe;
			mNextKeyframe = temp;
		}
		else
		{
			//mAnimTime -= mNextKeyframe->time;
			mAnimTime = 0;
			mCurKeyframe = mCurAnim->GetKeyframe(0);
			mNextKeyframe = mCurAnim->GetKeyframe(1);
			mNextFrameNum = 1;
		}
	}

	//interpolate between cur frame skeleton and next frame skeleton
	float curtime = mCurKeyframe->time;
	float percent = (mAnimTime - curtime) / (mNextKeyframe->time - curtime);
	SkeletonSet* skeleSet = mSkeletonSet;
	Skeleton* curSkele = mCurKeyframe->skeleton;
	Skeleton* nextSkele = mNextKeyframe->skeleton;
	for (uint8 i = 0; i < skeleSet->GetMeshNum(); ++i)
	{
		MeshData* meshdata = skeleSet->GetMeshData(i);
		std::vector<BoneAssignment>* vba = skeleSet->GetBoneAssignments(i);

		float* vdata = meshdata->baseVertexData;
		float* ndata = meshdata->baseNormalData;
		float* vtarget = meshdata->targetVertexBuffer;
		float* ntarget = meshdata->targetNormalBuffer;

		for (auto itr = vba->begin(); itr != vba->end(); itr++)
		{
			BoneAssignment& ba = *itr;

			Bone* curBone = curSkele->GetBone(ba.boneIndex);
			Bone* nextBone = nextSkele->GetBone(ba.boneIndex);
			float xrot = curBone->mXRotation + (nextBone->mXRotation - curBone->mXRotation) * percent;
			float yrot = curBone->mYRotation + (nextBone->mYRotation - curBone->mYRotation) * percent;
			float zrot = curBone->mZRotation + (nextBone->mZRotation - curBone->mZRotation) * percent;
			float xtrans = curBone->mXShift + (nextBone->mXShift - curBone->mXShift) * percent;
			float ytrans = curBone->mYShift + (nextBone->mYShift - curBone->mYShift) * percent;
			float ztrans = curBone->mZShift + (nextBone->mZShift - curBone->mZShift) * percent;
			//try interpolating the final x y z values instead ...
			//or are target keyframes not complete positions, but just differences already ?
			for (uint16 j = 0; j < ba.vertexCount; ++j)
			{
				//order is y, z, x
				float tempY = *vdata++;
				float z = *vdata++;
				float tempX = *vdata++;

				/*float ay = tempY;
				float az = z;
				float ax = tempX;*/

				float x, y;
				//x axis
				y = (cos(xrot) * tempY) - (sin(xrot) * z);
				z = (sin(xrot) * tempY) + (cos(xrot) * z);
				//y axis
				x = (cos(yrot) * tempX) + (sin(yrot) * z);
				z = -(sin(yrot) * tempX) + (cos(yrot) * z);
				//z axis
				tempX = x;
				x = (cos(zrot) * tempX) - (sin(zrot) * y);
				y = (sin(zrot) * tempX) + (cos(zrot) * y);
				//add shifts
				x += xtrans;
				y += ytrans;
				z += ztrans;

				/*tempY = ay;
				tempX = ax;
				float rot = nextBone->mXRotation;
				//x axis
				ay = (cos(rot) * tempY) - (sin(rot) * az);
				az = (sin(rot) * tempY) + (cos(rot) * az);
				//y axis
				rot = nextBone->mYRotation;
				ax = (cos(rot) * tempX) + (sin(rot) * az);
				az = -(sin(rot) * tempX) + (cos(rot) * az);
				//z axis
				rot = nextBone->mZRotation;
				tempX = ax;
				ax = (cos(rot) * tempX) - (sin(rot) * ay);
				ay = (sin(rot) * tempX) + (cos(rot) * ay);
				//add shifts
				ax += nextBone->mXShift;
				ay += nextBone->mYShift;
				az += nextBone->mZShift;

				x += (ax - x) * percent;
				y += (ay - y) * percent;
				z += (az - z) * percent;*/
				
				//write them into the vertex buffer
				*vtarget++ = y;
				*vtarget++ = z;
				*vtarget++ = x;

				//normals
				tempY = *ndata++;
				z = *ndata++;
				tempX = *ndata++;
				//x axis
				y = (cos(xrot) * tempY) - (sin(xrot) * z);
				z = (sin(xrot) * tempY) + (cos(xrot) * z);
				//y axis
				x = (cos(yrot) * tempX) + (sin(yrot) * z);
				z = -(sin(yrot) * tempX) + (cos(yrot) * z);
				//z axis
				tempX = x;
				x = (cos(zrot) * tempX) - (sin(zrot) * y);
				y = (sin(zrot) * tempX) + (cos(zrot) * y);
				//add shifts
				x += xtrans;
				y += ytrans;
				z += ztrans;

				//write them into the normal buffer
				*ntarget++ = y;
				*ntarget++ = z;
				*ntarget++ = x;
			}
		}

		Ogre::VertexBufferBinding* bind = meshdata->mesh->sharedVertexData->vertexBufferBinding;
		Ogre::HardwareVertexBufferSharedPtr vbuf = bind->getBuffer(0);
		Ogre::HardwareVertexBufferSharedPtr nbuf = bind->getBuffer(1);
		vbuf->writeData(0,vbuf->getSizeInBytes(),meshdata->targetVertexBuffer,true);
		nbuf->writeData(0,nbuf->getSizeInBytes(),meshdata->targetNormalBuffer,true);
	}
}
