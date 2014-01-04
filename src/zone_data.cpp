
#include "zone_data.h"

ZoneData::ZoneData(Ogre::SceneManager* sceneMgr)
{
	mStaticGeometry = sceneMgr->createStaticGeometry("gfaydark_Static");
	mStaticGeometry->setRenderingDistance(1000.0f);
	mAnimState = nullptr;
	mMobInstance = nullptr;
}

void ZoneData::LoadSprites()
{
	//0x31 fragments contain a list of indices to 0x30 fragments
	//0x30 fragments contains an index to a 0x05 or 0x03 fragment
	for (auto itr = mTextureListFrags.begin(); itr != mTextureListFrags.end(); itr++)
	{
		TextureListFragment* tl = *itr;
		std::unordered_map<int16,Sprite*> spriteList;

		for (int32 i = 0; i < tl->mRefCount; ++i)
		{
			Fragment* frag = GetFragment(tl->mRefList[i]);
			if (frag && frag->mType == 0x30)
			{
				TextureRefFragment* trf = static_cast<TextureRefFragment*>(frag);
				frag = GetFragment(trf->mRef);
				if (frag)
				{
					switch (frag->mType)
					{
						case 0x03:
						{
							Sprite* sprite = new Sprite(trf->mFlags,static_cast<TextureBitmapNameFragment*>(frag)->mNameList);
							spriteList[i] = sprite;
							break;
						}
						case 0x05:
						{
							//0x05 fragments refer to 0x04 fragments (otherwise worthless, apparently)
							frag = GetFragment(static_cast<TextureBitmapRefFragment*>(frag)->mRef);
							if (frag && frag->mType == 0x04)
							{
								//0x04 fragments contain a list of references to 0x03 fragments
								//and also contain some animation info for animated sprites
								//FIXME: consider subclassing animated sprites from static sprites
								TextureBitmapFragment* tbf = static_cast<TextureBitmapFragment*>(frag);
								std::vector<const char*> textureNames;
								for (int32 j = 0; j < tbf->mNumRefs; ++j)
								{
									frag = GetFragment(tbf->mRefList[j]);
									if (frag && frag->mType == 0x03)
									{
										//assuming frag contains only 1 texture...
										textureNames.push_back(static_cast<TextureBitmapNameFragment*>(frag)->mNameList[0]);
									}
								}

								if (textureNames.size() > 0)
								{
									Sprite* sprite = new Sprite(trf->mFlags,textureNames,tbf->mParam[1]);
									spriteList[i] = sprite;
								}
							}
							break;
						}
						default:
							continue;
					}
				}
			}
		}

		//insert inner list into main list
		if (spriteList.size() > 0)
			mSpriteList[tl->mIndex + 1] = spriteList;
	}
	mTextureListFrags.clear();
}

void ZoneData::BuildMesh(MeshFragment* mesh, Ogre::SceneManager* sceneMgr, const char* model_name)
{
	std::unordered_map<int16,Sprite*>* spriteList;
	if (mSpriteList.count(mesh->mTextureListing))
		spriteList = &mSpriteList[mesh->mTextureListing];
	else
		return;

	//0x31 fragment contains a list of indices to 0x30 fragments
	//pte contains a list of indices to the 0x31 fragments' lists and shareTextureCount values
	//when shareTextureCount reaches 0, we get the 0x30 fragment index at pt_index in 0x31's list,
	//incrementing pt_index by 1 and setting shareTextureCount,
	//then retrieving the corresponding 0x30 fragment by index from the wld's lookup table
	//0x30's name is the name of our desired texture from the s3d, which is already loaded
	//with "_Material" appended to this name

	Ogre::ManualObject* manual = sceneMgr->createManualObject();
	manual->estimateVertexCount(mesh->mPolyCount * 3);
	manual->estimateIndexCount(mesh->mPolyCount * 3);

	Vector3* vert = mesh->mVertexList;
	Vector2* text = mesh->mTextureCoordList;
	uint32* clr = mesh->mColorList;
	Vector3* norm = mesh->mNormalList;
	Ogre::ColourValue cv;

	PolyTextureEntry* pte = mesh->mPolyTextureList;
	int16 pt_index = 1;
	PolyTextureEntry& pt = pte[0];
	int16 shareTextureCount = pt.mCount + 1;
	manual->begin(spriteList->at(pt.mTextureID)->mTextureNameList[0],Ogre::RenderOperation::OT_TRIANGLE_LIST);
	uint32 vert_index = 0;

	for (int16 i = 0; i < mesh->mPolyCount; ++i)
	{
		if (--shareTextureCount <= 0)
		{
			PolyTextureEntry& pt = pte[pt_index++];
			shareTextureCount = pt.mCount;
			if (spriteList->count(pt.mTextureID))
			{
				manual->end();
				manual->begin(spriteList->at(pt.mTextureID)->mTextureNameList[0],Ogre::RenderOperation::OT_TRIANGLE_LIST);
				vert_index = 0;
			}
		}
		ZEQPolygon& p = mesh->mPolyList[i];
		for (int8 j = 0; j <= 2; ++j)
		{
			uint16 idx = p.index[j];
			manual->position(vert[idx].y,vert[idx].z,vert[idx].x);
			manual->textureCoord(text[idx].u,text[idx].v);
			cv.setAsRGBA(clr[idx]);
			manual->colour(cv);
			manual->normal(norm[idx].y,norm[idx].z,norm[idx].x);
		}
		//necessary to allow conversion to a proper, reusable mesh
		manual->triangle(vert_index + 2,vert_index + 1,vert_index);
		vert_index += 3;
	}

	manual->end();
	manual->convertToMesh(model_name);
}

void ZoneData::BuildZoneMeshes(Ogre::SceneManager* sceneMgr)
{
	char name_buf[64];
	uint32 count = 0;
	float minZoneX = 999999, minZoneY = 999999, minZoneZ = 999999, maxZoneX = -999999, maxZoneY = -999999, maxZoneZ = -999999;

	for (auto itr = mZoneMeshFrags.begin(); itr != mZoneMeshFrags.end(); itr++)
	{
		MeshFragment* mesh = *itr;

		std::unordered_map<int16,Sprite*>* spriteList;
		if (mSpriteList.count(mesh->mTextureListing))
			spriteList = &mSpriteList[mesh->mTextureListing];
		else
			continue;

		Ogre::ManualObject* manual = sceneMgr->createManualObject();
		manual->estimateVertexCount(mesh->mPolyCount * 3);
		manual->estimateIndexCount(mesh->mPolyCount * 3);

		Vector3* vert = mesh->mVertexList;
		Vector2* text = mesh->mTextureCoordList;
		uint32* clr = mesh->mColorList;
		Vector3* norm = mesh->mNormalList;
		Ogre::ColourValue cv;

		PolyTextureEntry* pte = mesh->mPolyTextureList;
		int16 shareTextureCount = pte->mCount + 1;
		manual->begin(spriteList->at(pte->mTextureID)->mTextureNameList[0],Ogre::RenderOperation::OT_TRIANGLE_LIST);
		uint32 vert_index = 0;

		for (int16 i = 0; i < mesh->mPolyCount; ++i)
		{
			if (--shareTextureCount <= 0)
			{
				pte++;
				shareTextureCount = pte->mCount;
				if (spriteList->count(pte->mTextureID))
				{
					manual->end();
					manual->begin(spriteList->at(pte->mTextureID)->mTextureNameList[0],Ogre::RenderOperation::OT_TRIANGLE_LIST);
					vert_index = 0;
				}
			}
			ZEQPolygon& p = mesh->mPolyList[i];
			for (int8 j = 0; j <= 2; ++j)
			{
				uint16 idx = p.index[j];
				float x = vert[idx].x, y = vert[idx].y, z = vert[idx].z;
				manual->position(y,z,x);
				manual->textureCoord(text[idx].u,text[idx].v);
				cv.setAsRGBA(clr[idx]);
				manual->colour(cv);
				manual->normal(norm[idx].y,norm[idx].z,norm[idx].x);
				/*if (x < minZoneX)
					minZoneX = x;
				else if (x > maxZoneX)
					maxZoneX = x;
				if (y < minZoneY)
					minZoneY = y;
				else if (y > maxZoneY)
					maxZoneY = y;*/
				if (z < minZoneZ)
					minZoneZ = z;
				else if (z > maxZoneZ)
					maxZoneZ = z;
			}
			//some vertices are reused; we can't take advantage of this with ManualObjects though with how it's set up
			//need to work with raw vertex and index buffers
			manual->triangle(vert_index + 2,vert_index + 1,vert_index);
			vert_index += 3;
		}

		manual->end();
		snprintf(name_buf,64,"gfaydark%u",count++);
		Ogre::MeshPtr ptr = manual->convertToMesh(name_buf);
		Ogre::Entity* ent = sceneMgr->createEntity(ptr);
		mStaticGeometry->addEntity(ent,Ogre::Vector3(0,0,0));
	}
	mStaticGeometry->setOrigin(Ogre::Vector3(0,minZoneZ - 10,0));
	mStaticGeometry->setRegionDimensions(Ogre::Vector3(1000,maxZoneZ - minZoneZ + 20,1000));
	mZoneMeshFrags.clear();
}

void ZoneData::BuildObjectMeshes(Ogre::SceneManager* sceneMgr)
{
	//load model data
	//0x14 fragments contain the model's name and refer to either a set of
	//0x11 fragments (animated) or a 0x2D fragment (static)
	//0x2D fragments contain a reference to a 0x36 fragment containing the model's mesh
	//0x15 fragments then use 0x14 names to look for the 0x36 mesh data to draw
	//the triangles of the reference copy of the model
	for (auto itr = mModelFrags.begin(); itr != mModelFrags.end(); itr++)
	{
		ModelFragment* model = *itr;
		if (model->mSize[1] < 1 || !model->mName)
			continue;

		Fragment* frag = GetFragment(model->mRefList[0]);
		if (frag)
		{
			if (frag->mType == 0x2D)
			{
				frag = GetFragment(static_cast<MeshRefFragment*>(frag)->mRef);
				if (frag && frag->mType == 0x36)
				{
					BuildMesh(static_cast<MeshFragment*>(frag),sceneMgr,model->mName);
				}
			}
		}
	}

	for (auto itr = mObjLocRefFrags.begin(); itr != mObjLocRefFrags.end(); itr++)
	{
		ObjectLocRefFragment* obj = *itr;
		if (obj->mRefName)
		{
			Ogre::Entity* ent = sceneMgr->createEntity(obj->mRefName);
			Ogre::Quaternion xrot(Ogre::Degree(obj->mYRotation),Ogre::Vector3::UNIT_X);
			Ogre::Quaternion yrot(Ogre::Degree(obj->mZRotation),Ogre::Vector3::UNIT_Y);
			Ogre::Quaternion zrot(Ogre::Degree(obj->mXRotation),Ogre::Vector3::UNIT_Z);
			mStaticGeometry->addEntity(ent,
				Ogre::Vector3(obj->mY,obj->mZ,obj->mX),
				xrot * yrot * zrot,
				Ogre::Vector3(obj->mYScale,obj->mZScale,obj->mXScale));
		}
	}
}

void ZoneData::BuildMobModelMeshes(Ogre::SceneManager* sceneMgr)
{
	int n = 0;
	for (auto itr = mModelFrags.begin(); itr != mModelFrags.end(); itr++)
	{
		ModelFragment* base = *itr;

		for (int32 i = 0; i < base->mSize[1]; ++i)
		{
			Fragment* frag = GetFragment(base->mRefList[i]);
			if (frag && frag->mType == 0x11)
			{
				frag = GetFragment(static_cast<AnimationRefFragment*>(frag)->mRef);
				if (frag && frag->mType == 0x10)
				{
					SkeletonTrackSetFragment* track = static_cast<SkeletonTrackSetFragment*>(frag);
#ifdef MANUAL_SKELETONS
					SkeletonSet* skele = ReadMobModelTree(sceneMgr,track,base->mName);
					if (n == 10 && i == 0)
					{
						//skele->Test(sceneMgr);
						mMobInstance = new MobInstance(skele,"C05",sceneMgr);
					}
#else
					ReadMobModelTree(sceneMgr,track,base->mName,n);
#endif
					n++;
				}
			}
		}
	}
#ifndef MANUAL_SKELETONS
	/*mAnimState = */mMobManager.Spawn(10,sceneMgr);
#endif
}

#ifndef MANUAL_SKELETONS
struct TreeEntry
{
	uint16 index;
	uint16 loop_pos;
	Ogre::Bone* bone;
	float xRot;
	float yRot;
	float zRot;
	float xTrans;
	float yTrans;
	float zTrans;
};

void ZoneData::ReadMobModelTree(Ogre::SceneManager* sceneMgr, SkeletonTrackSetFragment* track, const char* model_name, uint32 id)
{
	Ogre::LogManager* logMgr = Ogre::LogManager::getSingletonPtr();
	char log[128];
	Ogre::HardwareBufferManager* hardwareMgr = Ogre::HardwareBufferManager::getSingletonPtr();
	Ogre::MeshManager* meshMgr = Ogre::MeshManager::getSingletonPtr();

	std::stack<TreeEntry> stack;
	SkeletonTrackSet* set = track->mEntries;
	TreeEntry root;
	root.index = 0;
	root.loop_pos = 0;
	TreeEntry& cur_entry = root;

	Ogre::SkeletonPtr skele = Ogre::SkeletonManager::getSingleton().create(model_name,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Animation* anim = skele->createAnimation("base",60.0f);

	std::vector<SkeletonPieceRefFragment*> animSets;
	Ogre::Bone* root_bone;

	//root piece
	SkeletonTrackSet* cur_piece = &set[0];
	Fragment* frag = GetFragment(cur_piece->mRef[0]);
	if (frag && frag->mType == 0x13)
	{
		SkeletonPieceRefFragment* pr = static_cast<SkeletonPieceRefFragment*>(frag);
		snprintf(log,128,"MODEL ROOT: %s -> %s",model_name ? model_name : "<none>",pr->mName);
		logMgr->logMessage(log);
		/*const char* match = pr->mName;
		bool found_cur = false;
		//search for animation keyframe skeletons
		for (auto itr = mSkelePieceRefFrags.begin(); itr != mSkelePieceRefFrags.end(); itr++)
		{
			std::pair<const std::string,SkeletonPieceRefFragment*>& entry = *itr;
			if (entry.first.find(match) != std::string::npos)
			{
				SkeletonPieceRefFragment* next = entry.second;
				if (!found_cur && entry.first.compare(match) == 0)
				{
					found_cur = true; //we're already processing this one, no need to do it twice
				}
				else
				{
					snprintf(log,128,"ALTERNATE: %s",next->mName);
					logMgr->logMessage(log);
					animSets.push_back(next);
				}
			}
		}*/
		frag = GetFragment(pr->mRef);
		if (frag && frag->mType == 0x12)
		{
			SkeletonPieceTrackFragment* piece = static_cast<SkeletonPieceTrackFragment*>(frag);
			float shift_denom;
			if (piece->mShiftDenominator == 0)
			{
				shift_denom = 1.0f;
				piece->mShiftX = piece->mShiftY = piece->mShiftZ = 0;
			}
			else
			{
				shift_denom = static_cast<float>(piece->mShiftDenominator);
			}
			float rot_denom;
			if (piece->mRotationDenominator == 0)
			{
				rot_denom = 1.0f;
				piece->mRotationX = piece->mRotationY = piece->mRotationZ = 0;
			}
			else
			{
				rot_denom = static_cast<float>(piece->mRotationDenominator);
			}
			/*Ogre::Bone**/ root_bone = skele->createBone(0);
			cur_entry.xTrans = piece->mShiftX / shift_denom;
			cur_entry.yTrans = piece->mShiftY / shift_denom;
			cur_entry.zTrans = piece->mShiftZ / shift_denom;
			Ogre::Vector3 translation(cur_entry.yTrans,cur_entry.zTrans,cur_entry.xTrans);
			//Ogre::Vector3 translation(piece->mShiftX / shift_denom,piece->mShiftY / shift_denom,piece->mShiftZ / shift_denom);
			cur_entry.xRot = piece->mRotationX / rot_denom * 3.14159f * 0.5f;
			cur_entry.yRot = piece->mRotationY / rot_denom * 3.14159f * 0.5f;
			cur_entry.zRot = piece->mRotationZ / rot_denom * 3.14159f * 0.5f;
			Ogre::Quaternion xr(Ogre::Radian(cur_entry.yRot),Ogre::Vector3::UNIT_X);
			Ogre::Quaternion yr(Ogre::Radian(cur_entry.zRot),Ogre::Vector3::UNIT_Y);
			Ogre::Quaternion zr(Ogre::Radian(cur_entry.xRot),Ogre::Vector3::UNIT_Z);
			Ogre::Quaternion rotation = xr * yr * zr;
			root_bone->setPosition(translation);
			root_bone->setOrientation(rotation);
			cur_entry.bone = root_bone;

			Ogre::NodeAnimationTrack* track = anim->createNodeTrack(0,root_bone);
			Ogre::TransformKeyFrame* frame = track->createNodeKeyFrame(0);
			frame->setTranslate(root_bone->getPosition());
			frame->setRotation(root_bone->getOrientation());

			LoadBoneAnimations(root_bone,skele,pr->mName,0);
		}
	}

	//tree recursion
	for (;;)
	{
		if (cur_entry.loop_pos < cur_piece->mSize)
		{
			//get next piece
			TreeEntry next;
			next.index = cur_piece->mIndexList[cur_entry.loop_pos];
			next.loop_pos = 0;
			cur_piece = &set[next.index];
			//process new piece
			frag = GetFragment(cur_piece->mRef[0]);
			if (frag && frag->mType == 0x13)
			{
				SkeletonPieceRefFragment* pr = static_cast<SkeletonPieceRefFragment*>(frag);
				frag = GetFragment(pr->mRef);
				if (frag && frag->mType == 0x12)
				{
					SkeletonPieceTrackFragment* piece = static_cast<SkeletonPieceTrackFragment*>(frag);
					float shift_denom;
					if (piece->mShiftDenominator == 0)
					{
						shift_denom = 1.0f;
						piece->mShiftX = piece->mShiftY = piece->mShiftZ = 0;
					}
					else
					{
						shift_denom = static_cast<float>(piece->mShiftDenominator);
					}
					float rot_denom;
					if (piece->mRotationDenominator == 0)
					{
						rot_denom = 1.0f;
						piece->mRotationX = piece->mRotationY = piece->mRotationZ = 0;
					}
					else
					{
						rot_denom = static_cast<float>(piece->mRotationDenominator);
					}
					//if the piece has "POINT" in its name, it's an attachment bone
					Ogre::Bone* add_bone = skele->createBone(next.index);
					//cur_entry.bone->addChild(add_bone);
					root_bone->addChild(add_bone);

					//apply parent rotations
					float xTrans = piece->mShiftX / shift_denom;
					float yTrans = piece->mShiftY / shift_denom;
					float x,y,z = piece->mShiftZ / shift_denom;
					//x axis
					y = (cos(cur_entry.xRot) * yTrans) - (sin(cur_entry.xRot) * z);
					z = (sin(cur_entry.xRot) * yTrans) + (cos(cur_entry.xRot) * z);
					//y axis
					x = (cos(cur_entry.yRot) * xTrans) + (sin(cur_entry.yRot) * z);
					z = -(sin(cur_entry.yRot) * xTrans) + (cos(cur_entry.yRot) * z);
					//z axis
					xTrans = x;
					x = (cos(cur_entry.zRot) * xTrans) - (sin(cur_entry.zRot) * y);
					y = (sin(cur_entry.zRot) * xTrans) + (cos(cur_entry.zRot) * y);

					//add parent shifts
					next.xTrans = x + cur_entry.xTrans;
					next.yTrans = y + cur_entry.yTrans;
					next.zTrans = z + cur_entry.zTrans;

					//apply own rotations (added to parent's)
					next.xRot = cur_entry.xRot + piece->mRotationX / rot_denom * 3.14159f * 0.5f;
					next.yRot = cur_entry.yRot + piece->mRotationY / rot_denom * 3.14159f * 0.5f;
					next.zRot = cur_entry.zRot + piece->mRotationZ / rot_denom * 3.14159f * 0.5f;

					Ogre::Vector3 translation(next.yTrans,next.zTrans,next.xTrans);
					Ogre::Quaternion xr(Ogre::Radian(next.yRot),Ogre::Vector3::UNIT_X);
					Ogre::Quaternion yr(Ogre::Radian(next.zRot),Ogre::Vector3::UNIT_Y);
					Ogre::Quaternion zr(Ogre::Radian(next.xRot),Ogre::Vector3::UNIT_Z);
					Ogre::Quaternion rotation = xr * yr * zr;
					//add_bone->setInheritOrientation(false);
					//add_bone->_setDerivedOrientation(rotation);
					//add_bone->_setDerivedPosition(translation);
					//add_bone->setOrientation(rotation);
					add_bone->translate(translation,Ogre::Node::TS_LOCAL);
					add_bone->pitch(Ogre::Radian(next.yRot));
					add_bone->yaw(Ogre::Radian(next.zRot));
					add_bone->roll(Ogre::Radian(next.xRot));
					//add_bone->translate(translation);
					next.bone = add_bone;

					Ogre::NodeAnimationTrack* track = anim->createNodeTrack(next.index,add_bone);
					Ogre::TransformKeyFrame* frame = track->createNodeKeyFrame(0);
					frame->setTranslate(add_bone->getPosition());
					frame->setRotation(add_bone->getOrientation());

					LoadBoneAnimations(add_bone,skele,pr->mName,next.index);
				}
			}
			//put current piece on the stack so we can return to it
			cur_entry.loop_pos++;
			if (cur_piece->mSize == 0)
			{
				//don't bother recursing into the child if we know it is terminal
				cur_piece = &set[cur_entry.index];
			}
			else
			{
				stack.push(cur_entry);
				cur_entry = next; //copy construction
			}
		}
		else
		{
			//hit a terminal node, move backward
			if (stack.empty())
				break;
			cur_entry = stack.top(); //copy construction
			stack.pop();
			cur_piece = &set[cur_entry.index];
		}
	}

	MobType* mobType = new MobType;
	//time to create the geometry and associate vertices to bones
	for (int32 i = 0; i < track->mSizeB; ++i)
	{
		frag = GetFragment(track->mRefList[i]);
		if (frag && frag->mType == 0x2D)
		{
			frag = GetFragment(static_cast<MeshRefFragment*>(frag)->mRef);
			if (frag && frag->mType == 0x36)
			{
				MeshFragment* mesh = static_cast<MeshFragment*>(frag);

				std::unordered_map<int16,Sprite*>* spriteList;
				if (mSpriteList.count(mesh->mTextureListing))
					spriteList = &mSpriteList[mesh->mTextureListing];
				else
					continue;

				char name_buf[64];
				snprintf(name_buf,64,"%s",mesh->mName);
				Ogre::MeshPtr mainMesh = meshMgr->createManual(name_buf,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

				//vertex declaration - should store this somewhere and reuse...
				Ogre::VertexDeclaration* decl = hardwareMgr->createVertexDeclaration();
				decl->addElement(0,0,Ogre::VET_FLOAT3,Ogre::VES_POSITION);
				decl->addElement(1,0,Ogre::VET_FLOAT3,Ogre::VES_NORMAL);
				decl->addElement(2,0,Ogre::VET_FLOAT2,Ogre::VES_TEXTURE_COORDINATES);
				//ignoring colors for now (are they even used?)

				//create buffers
				//todo: shove normals into the first buffer (seems to crash DX9?)
				Ogre::HardwareVertexBufferSharedPtr vbuf = hardwareMgr->createVertexBuffer(decl->getVertexSize(0),mesh->mVertexCount,Ogre::HardwareBuffer::HBU_DYNAMIC);
				Ogre::HardwareVertexBufferSharedPtr nbuf = hardwareMgr->createVertexBuffer(decl->getVertexSize(1),mesh->mVertexCount,Ogre::HardwareBuffer::HBU_DYNAMIC);
				Ogre::HardwareVertexBufferSharedPtr tbuf = hardwareMgr->createVertexBuffer(decl->getVertexSize(2),mesh->mVertexCount,Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

				//get data pointers
				float* vdata = static_cast<float*>(vbuf->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));
				float* ndata = static_cast<float*>(nbuf->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));
				float* tdata = static_cast<float*>(tbuf->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));

				//fill in data
				Vector3* vert = mesh->mVertexList;
				Vector3* norm = mesh->mNormalList;
				Vector2* text = mesh->mTextureCoordList;
				//the mesh fragment gives us min and max coord information; that information is wrong
				float minX = 99999, minY = 99999, minZ = 99999, maxX = -99999, maxY = -99999, maxZ = -99999;

				for (int16 j = 0; j < mesh->mVertexCount; ++j)
				{
					float x = vert[j].x, y = vert[j].y, z = vert[j].z;
					vdata[0] = y;
					vdata[1] = z;
					vdata[2] = x;
					ndata[0] = norm[j].y;
					ndata[1] = norm[j].z;
					ndata[2] = norm[j].x;
					tdata[0] = text[j].u;
					tdata[1] = -text[j].v;
					vdata += 3;
					ndata += 3;
					tdata += 2;
					if (x < minX)
						minX = x;
					else if (x > maxX)
						maxX = x;
					if (y < minY)
						minY = y;
					else if (y > maxY)
						maxY = y;
					if (z < minZ)
						minZ = z;
					else if (z > maxZ)
						maxZ = z;
				}

				vbuf->unlock();
				nbuf->unlock();
				tbuf->unlock();

				//bind vertex data
				Ogre::VertexBufferBinding* vbind = hardwareMgr->createVertexBufferBinding();
				vbind->setBinding(0,vbuf);
				vbind->setBinding(1,nbuf);
				vbind->setBinding(2,tbuf);

				Ogre::VertexData* data = new Ogre::VertexData(decl,vbind);
				mainMesh->sharedVertexData = data;
				data->vertexCount = mesh->mVertexCount; //are these filled in automatically from the binding?
				data->vertexStart = 0;

				//create index buffer
				Ogre::HardwareIndexBufferSharedPtr ibuf = hardwareMgr->createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT,mesh->mPolyCount * 3,Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
				uint16* idata = static_cast<uint16*>(ibuf->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));

				//create submeshes and fill associated indices
				PolyTextureEntry* pte = mesh->mPolyTextureList;
				int16 shareTextureCount = pte->mCount + 1;

				Ogre::SubMesh* subMesh = mainMesh->createSubMesh();
				subMesh->setMaterialName(spriteList->at(pte->mTextureID)->mTextureNameList[0]);
				subMesh->useSharedVertices = true;
				subMesh->indexData->indexBuffer = ibuf;
				subMesh->indexData->indexStart = 0;
				subMesh->indexData->indexCount = pte->mCount * 3;
				uint32 indexOffset = pte->mCount * 3;

				for (int16 j = 0; j < mesh->mPolyCount; ++j)
				{
					if (--shareTextureCount <= 0)
					{
						pte++;
						shareTextureCount = pte->mCount;
						if (spriteList->count(pte->mTextureID))
						{
							subMesh = mainMesh->createSubMesh();
							subMesh->setMaterialName(spriteList->at(pte->mTextureID)->mTextureNameList[0]);
							subMesh->useSharedVertices = true;
							subMesh->indexData->indexBuffer = ibuf;
							subMesh->indexData->indexStart = indexOffset;
							subMesh->indexData->indexCount = pte->mCount * 3;
							indexOffset += pte->mCount * 3;
						}
					}
					ZEQPolygon& p = mesh->mPolyList[j];
					idata[0] = p.index[2];
					idata[1] = p.index[1];
					idata[2] = p.index[0];
					idata += 3;
				}

				ibuf->unlock();

				mainMesh->_setBounds(Ogre::AxisAlignedBox(minY,minZ,minX,maxY,maxZ,maxX));
				mainMesh->_setBoundingSphereRadius(std::max(maxX - minX,std::max(maxY - minY,maxZ - minZ)) / 2.0f);

				//associate bones with vertices - shared geometry lets us do this independently (thankfully)
				mainMesh->_notifySkeleton(skele);
				VertexPiece* vp = mesh->mVertexPieceList;
				uint16 vp_count = 0;
				for (uint16 v = 0; v < mesh->mVertexCount; ++v)
				{
					Ogre::VertexBoneAssignment vba;
					vba.boneIndex = vp->mIndex;
					vba.vertexIndex = v;
					vba.weight = 1.0f;
					mainMesh->addBoneAssignment(vba);
					if (++vp_count == vp->mCount)
					{
						vp++;
						vp_count = 0;
					}
				}

				mainMesh->_compileBoneAssignments();
				mainMesh->load();
				mobType->AddMesh(mainMesh);
			}
		}
	}

	mMobManager.AddMobType(id,mobType);
}

void ZoneData::LoadBoneAnimations(Ogre::Bone* bone, Ogre::SkeletonPtr& skele, const char* name, uint16 index)
{
	Ogre::LogManager* logMgr = Ogre::LogManager::getSingletonPtr();
	bool found_cur = false;
	for (auto itr = mSkelePieceRefFrags.begin(); itr !=  mSkelePieceRefFrags.end(); itr++)
	{
		std::pair<const std::string,SkeletonPieceRefFragment*>& entry = *itr;

		if (entry.first.find(name) != std::string::npos)
		{
			if (!found_cur && entry.first.compare(name) == 0)
			{
				found_cur = true; //we've already processed the base position, no need to do it again
				continue;
			}
		
			std::string anim_name = entry.first.substr(0,3);
			SkeletonPieceRefFragment* pr = entry.second;
			float total_duration = 10.0f;//pr->mParam / 1000.0f;

			Ogre::Animation* anim;
			if (index == 0)
			{
				//some root bones have two animations with the same name - whyyyy
				if (skele->hasAnimation(anim_name))
					continue;
				anim = skele->createAnimation(anim_name,total_duration);
			}
			else
				anim = skele->getAnimation(anim_name);

			Ogre::NodeAnimationTrack* track = anim->createNodeTrack(index,bone);

			//first frame: base position
			Ogre::TransformKeyFrame* frame = track->createNodeKeyFrame(0.0f);
			frame->setTranslate(bone->getPosition());
			frame->setRotation(bone->getOrientation());

			//second frame: target position
			frame = track->createNodeKeyFrame(total_duration / 2.0f);
			Fragment* frag = GetFragment(pr->mRef);
			if (frag && frag->mType == 0x12)
			{
				SkeletonPieceTrackFragment* piece = static_cast<SkeletonPieceTrackFragment*>(frag);
				float shift_denom;
				if (piece->mShiftDenominator == 0)
				{
					shift_denom = 1.0f;
					piece->mShiftX = piece->mShiftY = piece->mShiftZ = 0;
				}
				else
				{
					shift_denom = static_cast<float>(piece->mShiftDenominator);
				}
				float rot_denom;
				if (piece->mRotationDenominator == 0)
				{
					rot_denom = 1.0f;
					piece->mRotationX = piece->mRotationY = piece->mRotationZ = 0;
				}
				else
				{
					rot_denom = static_cast<float>(piece->mRotationDenominator);
				}
				
				Ogre::Vector3 translation(piece->mShiftY / shift_denom,piece->mShiftZ / shift_denom,piece->mShiftX / shift_denom);
				Ogre::Quaternion rotation(Ogre::Radian(piece->mRotationY / rot_denom * 3.14159f * 0.5f),Ogre::Vector3::UNIT_X);
				rotation = rotation * Ogre::Quaternion(Ogre::Radian(piece->mRotationZ / rot_denom * 3.14159f * 0.5f),Ogre::Vector3::UNIT_Y);
				rotation = rotation * Ogre::Quaternion(Ogre::Radian(piece->mRotationX / rot_denom * 3.14159f * 0.5f),Ogre::Vector3::UNIT_Z);
				frame->setRotation(rotation);
				frame->setTranslate(translation);

				//char log[256];
				//snprintf(log,256,"ANIM %s %s %u\r\nTRANS %g,%g,%g denom %i\r\nROTATE %g,%g,%g denom %i",anim_name.c_str(),name,pr->mParam,
				//	translation.x,translation.y,translation.z,piece->mShiftDenominator,rotation.getPitch().valueRadians(),
				//	rotation.getYaw().valueRadians(),rotation.getRoll().valueRadians(),piece->mRotationDenominator);
				//logMgr->logMessage(log);
			}

			//third frame: back to base position
			frame = track->createNodeKeyFrame(total_duration);
			frame->setTranslate(bone->getPosition());
			frame->setRotation(bone->getOrientation());
		}
	}
}
#endif

#ifdef MANUAL_SKELETONS
struct TreeEntry
{
	uint16 index;
	uint16 loop_pos;
	Bone* bone;
};

SkeletonSet* ZoneData::ReadMobModelTree(Ogre::SceneManager* sceneMgr, SkeletonTrackSetFragment* track, const char* model_name)
{
	Ogre::LogManager* logMgr = Ogre::LogManager::getSingletonPtr();
	char log[128];
	Ogre::HardwareBufferManager* hardwareMgr = Ogre::HardwareBufferManager::getSingletonPtr();
	Ogre::MeshManager* meshMgr = Ogre::MeshManager::getSingletonPtr();

	std::stack<TreeEntry> stack;
	SkeletonTrackSet* set = track->mEntries;
	TreeEntry root;
	root.index = 0;
	root.loop_pos = 0;
	TreeEntry& cur_entry = root;

	Skeleton* skele = new Skeleton(track->mSizeA);
	SkeletonSet* meshSkele = new SkeletonSet(skele,track->mSizeB);

	//root piece
	SkeletonTrackSet* cur_piece = &set[0];
	Fragment* frag = GetFragment(cur_piece->mRef[0]);
	if (frag && frag->mType == 0x13)
	{
		SkeletonPieceRefFragment* pr = static_cast<SkeletonPieceRefFragment*>(frag);
		snprintf(log,128,"MODEL ROOT: %s -> %s",model_name ? model_name : "<none>",pr->mName);
		logMgr->logMessage(log);
		/*if (!animation)
		{
			const char* match = pr->mName;
			bool found_cur = false;
			//search for animation keyframe skeletons
			for (auto itr = mSkelePieceRefFrags.begin(); itr != mSkelePieceRefFrags.end(); itr++)
			{
				std::pair<const std::string,SkeletonPieceRefFragment*>& entry = *itr;
				if (entry.first.find(match) != std::string::npos)
				{
					SkeletonPieceRefFragment* next = entry.second;
					if (!found_cur && entry.first.compare(match) == 0)
					{
						found_cur = true; //we're already processing this one, no need to do it twice
					}
					else
					{
						snprintf(log,128,"ALTERNATE: %s",next->mName);
						logMgr->logMessage(log);
					}
				}
			}
		}*/
		frag = GetFragment(pr->mRef);
		if (frag && frag->mType == 0x12)
		{
			SkeletonPieceTrackFragment* piece = static_cast<SkeletonPieceTrackFragment*>(frag);
			float shift_denom;
			if (piece->mShiftDenominator == 0)
			{
				shift_denom = 1.0f;
				piece->mShiftX = piece->mShiftY = piece->mShiftZ = 0;
			}
			else
			{
				shift_denom = static_cast<float>(piece->mShiftDenominator);
			}
			float rot_denom;
			if (piece->mRotationDenominator == 0)
			{
				rot_denom = 1.0f;
				piece->mRotationX = piece->mRotationY = piece->mRotationZ = 0;
			}
			else
			{
				rot_denom = static_cast<float>(piece->mRotationDenominator);
			}
			Bone* root_bone = new Bone(
				piece->mRotationX / rot_denom * 3.14159f * 0.5f,
				piece->mRotationY / rot_denom * 3.14159f * 0.5f,
				piece->mRotationZ / rot_denom * 3.14159f * 0.5f,
				piece->mShiftX / shift_denom,
				piece->mShiftY / shift_denom,
				piece->mShiftZ / shift_denom);
			cur_entry.bone = root_bone;
			skele->AddBone(root_bone,0);
			LoadBoneAnimations(root_bone,skele,meshSkele,pr->mName,0);
		}
	}

	//tree recursion
	for (;;)
	{
		if (cur_entry.loop_pos < cur_piece->mSize)
		{
			//get next piece
			TreeEntry next;
			next.index = cur_piece->mIndexList[cur_entry.loop_pos];
			next.loop_pos = 0;
			cur_piece = &set[next.index];
			//process new piece
			frag = GetFragment(cur_piece->mRef[0]);
			if (frag && frag->mType == 0x13)
			{
				SkeletonPieceRefFragment* pr = static_cast<SkeletonPieceRefFragment*>(frag);
				frag = GetFragment(pr->mRef);
				if (frag && frag->mType == 0x12)
				{
					SkeletonPieceTrackFragment* piece = static_cast<SkeletonPieceTrackFragment*>(frag);
					float shift_denom;
					if (piece->mShiftDenominator == 0)
					{
						shift_denom = 1.0f;
						piece->mShiftX = piece->mShiftY = piece->mShiftZ = 0;
					}
					else
					{
						shift_denom = static_cast<float>(piece->mShiftDenominator);
					}
					float rot_denom;
					if (piece->mRotationDenominator == 0)
					{
						rot_denom = 1.0f;
						piece->mRotationX = piece->mRotationY = piece->mRotationZ = 0;
					}
					else
					{
						rot_denom = static_cast<float>(piece->mRotationDenominator);
					}
					//if the piece has "POINT" in its name, it's an attachment bone
					Bone* add_bone = new Bone(
						piece->mRotationX / rot_denom * 3.14159f * 0.5f,
						piece->mRotationY / rot_denom * 3.14159f * 0.5f,
						piece->mRotationZ / rot_denom * 3.14159f * 0.5f,
						piece->mShiftX / shift_denom,
						piece->mShiftY / shift_denom,
						piece->mShiftZ / shift_denom,
						cur_entry.bone);
					next.bone = add_bone;
					skele->AddBone(add_bone,next.index);
					LoadBoneAnimations(add_bone,skele,meshSkele,pr->mName,next.index,cur_entry.bone);
				}
			}
			//put current piece on the stack so we can return to it
			cur_entry.loop_pos++;
			if (cur_piece->mSize == 0)
			{
				//don't bother recursing into the child if we know it is terminal
				cur_piece = &set[cur_entry.index];
			}
			else
			{
				stack.push(cur_entry);
				cur_entry = next; //copy construction
			}
		}
		else
		{
			//hit a terminal node, move backward
			if (stack.empty())
				break;
			cur_entry = stack.top(); //copy construction
			stack.pop();
			cur_piece = &set[cur_entry.index];
		}
	}

	//time to create the geometry and associate vertices to bones
	for (int32 i = 0; i < track->mSizeB; ++i)
	{
		frag = GetFragment(track->mRefList[i]);
		if (frag && frag->mType == 0x2D)
		{
			frag = GetFragment(static_cast<MeshRefFragment*>(frag)->mRef);
			if (frag && frag->mType == 0x36)
			{
				MeshFragment* mesh = static_cast<MeshFragment*>(frag);

				std::unordered_map<int16,Sprite*>* spriteList;
				if (mSpriteList.count(mesh->mTextureListing))
					spriteList = &mSpriteList[mesh->mTextureListing];
				else
					continue;

				char name_buf[64];
				snprintf(name_buf,64,"%s%i",mesh->mName,i);
				Ogre::MeshPtr mainMesh = meshMgr->createManual(name_buf,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

				//vertex declaration - should store this somewhere and reuse...
				Ogre::VertexDeclaration* decl = hardwareMgr->createVertexDeclaration();
				decl->addElement(0,0,Ogre::VET_FLOAT3,Ogre::VES_POSITION);
				decl->addElement(1,0,Ogre::VET_FLOAT3,Ogre::VES_NORMAL);
				decl->addElement(2,0,Ogre::VET_FLOAT2,Ogre::VES_TEXTURE_COORDINATES);
				//ignoring colors for now (are they even used?)

				//create buffers
				//todo: shove normals into the first buffer (seems to crash DX9?)
				Ogre::HardwareVertexBufferSharedPtr vbuf = hardwareMgr->createVertexBuffer(decl->getVertexSize(0),mesh->mVertexCount,Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				Ogre::HardwareVertexBufferSharedPtr nbuf = hardwareMgr->createVertexBuffer(decl->getVertexSize(1),mesh->mVertexCount,Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				Ogre::HardwareVertexBufferSharedPtr tbuf = hardwareMgr->createVertexBuffer(decl->getVertexSize(2),mesh->mVertexCount,Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

				//get data pointers
				float* vertices = new float[mesh->mVertexCount * 3];
				float* normals = new float[mesh->mVertexCount * 3];
				float* vdata = vertices;
				float* ndata = normals;
				float* tdata = static_cast<float*>(tbuf->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));

				//fill in data
				Vector3* vert = mesh->mVertexList;
				Vector3* norm = mesh->mNormalList;
				Vector2* text = mesh->mTextureCoordList;
				//the mesh fragment gives us min and max coord information; that information is wrong
				float minX = 99999, minY = 99999, minZ = 99999, maxX = -99999, maxY = -99999, maxZ = -99999;

				for (int16 j = 0; j < mesh->mVertexCount; ++j)
				{
					float x = vert[j].x, y = vert[j].y, z = vert[j].z;
					*vdata++ = y;
					*vdata++ = z;
					*vdata++ = x;
					*ndata++ = norm[j].y;
					*ndata++ = norm[j].z;
					*ndata++ = norm[j].x;
					*tdata++ = text[j].u;
					*tdata++ = -text[j].v;
					if (x < minX)
						minX = x;
					else if (x > maxX)
						maxX = x;
					if (y < minY)
						minY = y;
					else if (y > maxY)
						maxY = y;
					if (z < minZ)
						minZ = z;
					else if (z > maxZ)
						maxZ = z;
				}

				tbuf->unlock();

				//bind vertex data
				Ogre::VertexBufferBinding* vbind = hardwareMgr->createVertexBufferBinding();
				vbind->setBinding(0,vbuf);
				vbind->setBinding(1,nbuf);
				vbind->setBinding(2,tbuf);

				Ogre::VertexData* data = new Ogre::VertexData(decl,vbind);
				mainMesh->sharedVertexData = data;
				data->vertexCount = mesh->mVertexCount; //are these filled in automatically from the binding?
				data->vertexStart = 0;

				//create index buffer
				Ogre::HardwareIndexBufferSharedPtr ibuf = hardwareMgr->createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT,mesh->mPolyCount * 3,Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
				uint16* idata = static_cast<uint16*>(ibuf->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));

				//create submeshes and fill associated indices
				PolyTextureEntry* pte = mesh->mPolyTextureList;
				int16 shareTextureCount = pte->mCount + 1;

				Ogre::SubMesh* subMesh = mainMesh->createSubMesh();
				subMesh->setMaterialName(spriteList->at(pte->mTextureID)->mTextureNameList[0]);
				subMesh->useSharedVertices = true;
				subMesh->indexData->indexBuffer = ibuf;
				subMesh->indexData->indexStart = 0;
				subMesh->indexData->indexCount = pte->mCount * 3;
				uint32 indexOffset = pte->mCount * 3;

				for (int16 j = 0; j < mesh->mPolyCount; ++j)
				{
					if (--shareTextureCount <= 0)
					{
						pte++;
						shareTextureCount = pte->mCount;
						if (spriteList->count(pte->mTextureID))
						{
							subMesh = mainMesh->createSubMesh();
							subMesh->setMaterialName(spriteList->at(pte->mTextureID)->mTextureNameList[0]);
							subMesh->useSharedVertices = true;
							subMesh->indexData->indexBuffer = ibuf;
							subMesh->indexData->indexStart = indexOffset;
							subMesh->indexData->indexCount = pte->mCount * 3;
							indexOffset += pte->mCount * 3;
						}
					}
					ZEQPolygon& p = mesh->mPolyList[j];
					*idata++ = p.index[2];
					*idata++ = p.index[1];
					*idata++ = p.index[0];
				}

				ibuf->unlock();

				mainMesh->_setBounds(Ogre::AxisAlignedBox(minY,minZ,minX,maxY,maxZ,maxX));
				mainMesh->_setBoundingSphereRadius(std::max(maxX - minX,std::max(maxY - minY,maxZ - minZ)) / 2.0f);

				//associate bones with vertices - shared geometry lets us do this independently (thankfully)
				VertexPiece* vp = mesh->mVertexPieceList;
				uint16 vertex_num = 0;
				while (vertex_num < mesh->mVertexCount)
				{
					meshSkele->AddBoneAssignment(vp->mIndex,vp->mCount,i);
					vertex_num += vp->mCount;
					vp++;
				}

				mainMesh->load();
				meshSkele->AddMesh(mainMesh,i,vertices,normals,mesh->mVertexCount);
			}
		}
	}

	meshSkele->Complete();
	return meshSkele;
}

void ZoneData::LoadBoneAnimations(Bone* bone, Skeleton* skele, SkeletonSet* skeleSet, const char* name, uint16 index, Bone* parent)
{
	bool found_cur = false;
	for (auto itr = mSkelePieceRefFrags.begin(); itr !=  mSkelePieceRefFrags.end(); itr++)
	{
		std::pair<const std::string,SkeletonPieceRefFragment*>& entry = *itr;

		if (entry.first.find(name) != std::string::npos)
		{
			if (!found_cur && entry.first.compare(name) == 0)
			{
				found_cur = true; //we've already processed the base position, no need to do it again
				continue;
			}
		
			std::string anim_name = entry.first.substr(0,3);
			SkeletonPieceRefFragment* pr = entry.second;
			float total_duration = 10.0f;//pr->mParam / 1000.0f;

			Skeleton* target_skele;
			Animation* anim;
			if (index == 0)
			{
				//some root bones have two animations with the same name - whyyyy
				if (skeleSet->HasAnimation(anim_name.c_str()))
					continue;
				anim = new Animation();
				skeleSet->AddAnimation(anim,anim_name.c_str());
				//first frame: base position
				anim->AddSkeleton(skele,0.0f);
				//second frame: target position
				target_skele = new Skeleton(skele->GetNumBones());
				anim->AddSkeleton(target_skele,total_duration / 2.0f);
				//third frame: back to base position
				anim->AddSkeleton(skele,total_duration);
			}
			else
			{
				anim = skeleSet->GetAnimation(anim_name.c_str());
				target_skele = anim->GetSkeletonByKeyframe(1);
			}

			//second frame: target position
			Fragment* frag = GetFragment(pr->mRef);
			if (frag && frag->mType == 0x12)
			{
				SkeletonPieceTrackFragment* piece = static_cast<SkeletonPieceTrackFragment*>(frag);
				float shift_denom;
				if (piece->mShiftDenominator == 0)
				{
					shift_denom = 1.0f;
					piece->mShiftX = piece->mShiftY = piece->mShiftZ = 0;
				}
				else
				{
					shift_denom = static_cast<float>(piece->mShiftDenominator);
				}
				float rot_denom;
				if (piece->mRotationDenominator == 0)
				{
					rot_denom = 1.0f;
					piece->mRotationX = piece->mRotationY = piece->mRotationZ = 0;
				}
				else
				{
					rot_denom = static_cast<float>(piece->mRotationDenominator);
				}

				Bone* add_bone = new Bone(
					piece->mRotationX / rot_denom * 3.14159f * 0.5f,
					piece->mRotationY / rot_denom * 3.14159f * 0.5f,
					piece->mRotationZ / rot_denom * 3.14159f * 0.5f,
					piece->mShiftX / shift_denom,
					piece->mShiftY / shift_denom,
					piece->mShiftZ / shift_denom/*,
					parent*/);
				target_skele->AddBone(add_bone,index);

				/*char log[128];
				snprintf(log,128,"ANIM BONE %s rot %g, %g, %g shift %g, %g, %g",pr->mName,
					piece->mRotationX / rot_denom * 3.14159f * 0.5f,
					piece->mRotationY / rot_denom * 3.14159f * 0.5f,
					piece->mRotationZ / rot_denom * 3.14159f * 0.5f,
					piece->mShiftX / shift_denom,
					piece->mShiftY / shift_denom,
					piece->mShiftZ / shift_denom);
				Ogre::LogManager::getSingleton().logMessage(log);*/
			}
		}
	}
}
#endif

Fragment* ZoneData::GetFragment(int32 index)
{
	if (index > 0)
	{
		uint32 check = static_cast<uint32>(index - 1);
		if (mFragsByIndex.count(check))
			return mFragsByIndex[check];
	}
	else
	{
		index = -(index - 1);
		const char* name = (char*)&mNameData[index];
		return GetFragment(name);
	}
	return nullptr;
}

Fragment* ZoneData::GetFragment(const char* name)
{
	if (mFragsByName.count(name))
		return mFragsByName[name];
	return nullptr;
}
