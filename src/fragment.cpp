
#include "fragment.h"

Fragment::Fragment(int nameRef, byte* nameList, uint32 type)
{
	mType = type;
	if (nameRef >= 0)
	{
		mName = nullptr;
	}
	else
	{
		//the raw nameList block is retained in memory by the WLD
		//the strings inside are null-terminated, so we can just point to them directly
		nameRef = -nameRef; //do we need this in the frag itself at all? probably the hash table is enough
		mName = (char*)&nameList[nameRef];
	}
}

void Fragment::DecodeName(byte* name, size_t len)
{
	static byte hashval[] = {0x95,0x3A,0xC5,0x2A,0x95,0x7A,0x95,0x6A};
	for (size_t i = 0; i < len; ++i)
	{
		name[i] = name[i] ^ hashval[i & 7];
	}
}

//todo: fix all frag types to use struct pointers into the data block instead of copying members from the data block

MeshFragment::MeshFragment(int nameRef, byte* nameList, const byte* data, uint32 type, int wldVersion) :
Fragment(nameRef,nameList,type)
{
	//loooots of fixed offset stuff to start
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mTextureListing,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mAnimatedVertex,data,sizeof(int32));
	data += sizeof(int32) * 3; //skip int32 mUnknown[2]
	memcpy(&mCenterX,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mCenterY,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mCenterZ,data,sizeof(float));
	data += sizeof(float);
	memcpy(mParams,data,sizeof(int32) * 3);
	data += sizeof(int32) * 3;
	memcpy(&mMaxDist,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mMinX,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mMinY,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mMinZ,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mMaxX,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mMaxY,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mMaxZ,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mVertexCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mTextureCoordCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mNormalCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mColorCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mPolyCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mVertexPieceCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mPolyTextureCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mVertexTextureCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mSize9,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mScale,data,sizeof(int16));
	data += sizeof(int16);

#ifdef ZEQ_ENDIAN_CHECK
	mFlags = endian_uint32(mFlags);
	mTextureListing = endian_int32(mTextureListing);
	mAnimatedVertex = endian_int32(mAnimatedVertex);
	mParams[0] = endian_int32(mParams[0]);
	mParams[1] = endian_int32(mParams[1]);
	mParams[2] = endian_int32(mParams[2]);
	mVertexCount = endian_int16(mVertexCount);
	mTextureCoordCount = endian_int16(mTextureCoordCount);
	mNormalCount = endian_int16(mNormalCount);
	mColorCount = endian_int16(mColorCount);
	mPolyCount = endian_int16(mPolyCount);
	mVertexPieceCount = endian_int16(mSize6);
	mPolyTextureCount = endian_int16(mPolyTextureCount);
	mVertexTextureCount = endian_int16(mVertexTextureCount);
	mSize9 = endian_int16(mSize9);
	mScale = endian_int16(mScale);
#endif

	//vertices
	Vector3* vlist = new Vector3[mVertexCount];
	mVertexList = vlist;
	float s = 1.0f / (1 << mScale);
	for (int16 i = 0; i < mVertexCount; ++i)
	{
		Vector3& v = vlist[i];
		int16 d[3];
		memcpy(d,data,sizeof(int16) * 3);
		data += sizeof(int16) * 3;
#ifdef ZEQ_ENDIAN_CHECK
		d[0] = endian_int16(d[0]);
		d[1] = endian_int16(d[1]);
		d[2] = endian_int16(d[2]);
#endif
		v.x = mCenterX + d[0] * s;
		v.y = mCenterY + d[1] * s;
		v.z = mCenterZ + d[2] * s;
	}

	//texture coords
	Vector2* tlist = new Vector2[mTextureCoordCount];
	mTextureCoordList = tlist;
	if (wldVersion == 1)
	{
		s = 1.0f / 256.0f;
		for (int16 i = 0; i < mTextureCoordCount; ++i)
		{
			Vector2& v = tlist[i];
			int16 d[2];
			memcpy(d,data,sizeof(int16) * 2);
			data += sizeof(int16) * 2;
#ifdef ZEQ_ENDIAN_CHECK
			d[0] = endian_int16(d[0]);
			d[1] = endian_int16(d[1]);
#endif
			v.u = d[0] * s;
			v.v = d[1] * s;
		}
	}
	else
	{
		for (int16 i = 0; i < mTextureCoordCount; ++i)
		{
			Vector2& v = tlist[i];
			//memcpy(v,data,sizeof(float) * 2);
			//data += sizeof(float) * 2;
			//v->v = 0.0f - v->v;
			int32 d[2];
			memcpy(d,data,sizeof(int32) * 2);
			data += sizeof(int32);
#ifdef ZEQ_ENDIAN_CHECK
			d[0] = endian_int32(d[0]);
			d[1] = endian_int32(d[1]);
#endif
			v.u = static_cast<float>(d[0]);
			v.v = static_cast<float>(d[1]);
		}
	}

	//normals
	s = 1.0f / 127.0f;
	Vector3* nlist = new Vector3[mNormalCount];
	mNormalList = nlist;
	for (int16 i = 0; i < mNormalCount; ++i)
	{
		Vector3& v = nlist[i];
		int8 d[3];
		memcpy(d,data,sizeof(int8) * 3);
		data += sizeof(int8) * 3;
		v.x = d[0] * s;
		v.y = d[1] * s;
		v.z = d[2] * s;
	}

	//32bit colors
	mColorList = new uint32[mColorCount];
	memcpy(mColorList,data,sizeof(uint32) * mColorCount);
	data += sizeof(uint32) * mColorCount;
#ifdef ZEQ_ENDIAN_CHECK
	if (!isLittleEndian())
	{
		for (int16 i = 0; i < mColorCount; ++i)
		{
			mColorList[i] = endian_uint32(mColorList[i]);
		}
	}
#endif

	//polygons (triangles)
	ZEQPolygon* plist = new ZEQPolygon[mPolyCount];
	mPolyList = plist;
	for (int16 i = 0; i < mPolyCount; ++i)
	{
		//skipping a 2 byte flag
		memcpy(&plist[i],&data[2],sizeof(uint16) * 3);
		data += sizeof(uint16) * 3 + 2;
#ifdef ZEQ_ENDIAN_CHECK
		plist[i].index[0] = endian_uint16(plist[i].index[0]);
		plist[i].index[1] = endian_uint16(plist[i].index[1]);
		plist[i].index[2] = endian_uint16(plist[i].index[2]);
#endif
	}

	//vertex pieces (skeleton-related)
	mVertexPieceList = new VertexPiece[mVertexPieceCount];
	memcpy(mVertexPieceList,data,4 * mVertexPieceCount);
	data += 4 * mVertexPieceCount;
#ifdef ZEQ_ENDIAN_CHECK
	if (!isLittleEndian())
	{
		for (int16 i = 0; i < mVertexPieceCount; ++i)
		{
			VertexPiece& p = mVertexPieceList[i];
			p.mCount = endian_int16(p.mCount);
			p.mIndex = endian_int16(p.mIndex);
		}
	}
#endif

	//polygon texture assignment
	mPolyTextureList = new PolyTextureEntry[mPolyTextureCount];
	memcpy(mPolyTextureList,data,sizeof(PolyTextureEntry) * mPolyTextureCount);
#ifdef ZEQ_ENDIAN_CHECK
	if (!isLittleEndian())
	{
		for (int16 i = 0; i < mPolyTextureCount; ++i)
		{
			PolyTextureEntry& p = mPolyTextureList[i];
			p.mCount = endian_int16(p.mCount);
			p.mTextureID = endian_int16(p.mTextureID);
		}
	}
#endif
}

MeshFragment::~MeshFragment()
{
	delete[] mVertexList;
	delete[] mTextureCoordList;
	delete[] mNormalList;
	delete[] mColorList;
	delete[] mPolyList;
	delete[] mVertexPieceList;
	delete[] mPolyTextureList;
}

TextureListFragment::TextureListFragment(int nameRef, byte* nameList, const byte* data, uint32 type, uint32 index) :
Fragment(nameRef,nameList,type)
{
	mIndex = index;
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mRefCount,data,sizeof(int32));
	data += sizeof(int32);

#ifdef ZEQ_ENDIAN_CHECK
	mFlags = endian_uint32(mFlags);
	mRefCount = endian_int32(mRefCount);
#endif

	mRefList = new int32[mRefCount];
	memcpy(mRefList,data,sizeof(int32) * mRefCount);
#ifdef ZEQ_ENDIAN_CHECK
	if (!isLittleEndian())
	{
		for (int32 i = 0; i < mRefCount; ++i)
		{
			mRefList[i] = endian_int32(mRefList[i]);
		}
	}
#endif
}

TextureListFragment::~TextureListFragment()
{
	delete[] mRefList;
}

TextureRefFragment::TextureRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mParamA,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mParamB,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(mParamC,data,sizeof(float) * 2);
	data += sizeof(float) * 2;
	memcpy(&mRef,data,sizeof(int32));

#ifdef ZEQ_ENDIAN_CHECK
	mFlags = endian_uint32(mFlags);
	mParamA = endian_uint32(mParamA);
	mParamB = endian_int32(mParamB);
	mRef = endian_int32(mRef);
#endif
}

MeshRefFragment::MeshRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mRef,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mFlags,data,sizeof(uint32));

#ifdef ZEQ_ENDIAN_CHECK
	mRef = endian_int32(mRef);
	mFlags = endian_uint32(mFlags);
#endif
}

ObjectLocRefFragment::ObjectLocRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mRef,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mRefB,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mX,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mY,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mZ,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mZRotation,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mYRotation,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mXRotation,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mZScale,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mYScale,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mXScale,data,sizeof(float));
	data += sizeof(float);
	memcpy(&mRefC,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mParam,data,sizeof(int32));
	data += sizeof(int32);

#ifdef ZEQ_ENDIAN_CHECK
	mRef = endian_int32(mRef);
	mFlags = endian_uint32(mFlags);
	mRefB = endian_int32(mRefB);
	mRefC = endian_int32(mRefC);
#endif

	mXRotation = mXRotation / 512.0f * 360.0f;// * 3.14159f / 180.0f;
	mYRotation = mYRotation / 512.0f * 360.0f;// * 3.14159f / 180.0f;
	mZRotation = mZRotation / 512.0f * 360.0f;// * 3.14159f / 180.0f;
	mZScale = mYScale;

	if (mRefC == 0)
		mParam = 0;
#ifdef ZEQ_ENDIAN_CHECK
	else
		mParam = endian_int32(mParam);
#endif

	if (mRef >= 0)
	{
		mRefName = nullptr;
	}
	else
	{
		//the raw nameList block is retained in memory by the WLD
		//the strings inside are null-terminated, so we can just point to them directly
		mRef = -mRef;
		mRefName = (char*)&nameList[mRef];
	}
}

ModelFragment::ModelFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mRef,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(mSize,data,sizeof(int32) * 2);
	data += sizeof(int32) * 2;
	memcpy(&mRefB,data,sizeof(int32));
	data += sizeof(int32);

#ifdef ZEQ_ENDIAN_CHECK
	mFlags = endian_uint32(mFlags);
	mRef = endian_int32(mRef);
	mSize[0] = endian_int32(mSize[0]);
	mSize[1] = endian_int32(mSize[1]);
	mRefB = endian_int32(mRefB);
#endif

	//skip optional fields if they exist
	if (mFlags & (1 << 0))
	{
		data += 4;
	}
	if (mFlags & (1 << 1))
	{
		data += 4;
	}

	//skipping of various size
	for (int32 i = 0; i < mSize[0]; ++i)
	{
		int32 size;
		memcpy(&size,data,sizeof(int32));
		data += size * 8 + sizeof(int32);
	}

	mRefList = new int32[mSize[1]];
	memcpy(mRefList,data,sizeof(int32) * mSize[1]);
#ifdef ZEQ_ENDIAN_CHECK
	if (!isLittleEndian())
	{
		for (int32 i = 0; i < mSize[1]; ++i)
		{
			mRefList[i] = endian_int32(mRefList[i]);
		}
	}
#endif
}

ModelFragment::~ModelFragment()
{
	delete[] mRefList;
}

SkeletonPieceRefFragment::SkeletonPieceRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mRef,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mParam,data,sizeof(int32));

#ifdef ZEQ_ENDIAN_CHECK
	mRef = endian_int32(mRef);
	mParam = endian_int32(mParam);
#endif
}

SkeletonPieceTrackFragment::SkeletonPieceTrackFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mSize,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mRotationDenominator,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mRotationX,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mRotationY,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mRotationZ,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mShiftX,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mShiftY,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mShiftZ,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mShiftDenominator,data,sizeof(int16));
	data += sizeof(int16);

#ifdef ZEQ_ENDIAN_CHECK
	mFlags = endian_uint32(mFlags);
	mSize = endian_int32(mSize);
	mRotationDenominator = endian_uint16(mRotationDenominator);
	mRotationX = endian_uint16(mRotationX);
	mRotationY = endian_uint16(mRotationY);
	mRotationZ = endian_uint16(mRotationZ);
	mShiftX = endian_uint16(mShiftX);
	mShiftY = endian_uint16(mShiftY);
	mShiftZ = endian_uint16(mShiftZ);
	mShiftDenominator = endian_uint16(mShiftDenominator);
#endif

	int32 size = mSize * 4;
	mData = new int32[size];
	memcpy(mData,data,sizeof(int32) * size);
#ifdef ZEQ_ENDIAN_CHECK
	if (!isLittleEndian())
	{
		for (int32 i = 0; i < size; ++i)
		{
			mData[i] = endian_int32(mData[i]);
		}
	}
#endif
}

SkeletonPieceTrackFragment::~SkeletonPieceTrackFragment()
{
	delete[] mData;
}

AnimationRefFragment::AnimationRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mRef,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mParam,data,sizeof(int32));

#ifdef ZEQ_ENDIAN_CHECK
	mRef = endian_int32(mRef);
	mParam = endian_int32(mParam);
#endif
}

SkeletonTrackSetFragment::SkeletonTrackSetFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mSizeA,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mRefA,data,sizeof(int32));
	data += sizeof(int32);

#ifdef ZEQ_ENDIAN_CHECK
	mFlags = endian_uint32(mFlags);
	mSizeA = endian_int32(mSizeA);
	mRefA = endian_int32(mRefA);
#endif

	if (mFlags & (1 << 0))
	{
		memcpy(mParamA,data,sizeof(int32) * 3);
		data += sizeof(int32) * 3;
#ifdef ZEQ_ENDIAN_CHECK
		mParamA[0] = endian_int32(mParamA[0]);
		mParamA[1] = endian_int32(mParamA[1]);
		mParamA[2] = endian_int32(mParamA[2]);
#endif
	}
	else
	{
		mParamA[0] = mParamA[1] = mParamA[2] = 0;
	}
	if (mFlags & (1 << 1))
	{
		memcpy(&mParamB,data,sizeof(float));
		data += sizeof(float);
	}

	mEntries = new SkeletonTrackSet[mSizeA];
	for (int32 i = 0; i < mSizeA; ++i)
	{
		memcpy(&mEntries[i],data,20);
		data += 20;
		SkeletonTrackSet& entry = mEntries[i];

#ifdef ZEQ_ENDIAN_CHECK
		entry.mNameRef = endian_int32(entry.mNameRef);
		entry.mFlags = endian_uint32(entry.mFlags);
		entry.mRef[0] = endian_int32(entry.mRef[0]);
		entry.mRef[1] = endian_int32(entry.mRef[1]);
		entry.mSize = endian_int32(entry.mSize);
#endif

		entry.mIndexList = new int32[entry.mSize];
		memcpy(entry.mIndexList,data,sizeof(int32) * entry.mSize);
		data += sizeof(int32) * entry.mSize;

#ifdef ZEQ_ENDIAN_CHECK
		if (!isLittleEndian())
		{
			for (int32 i = 0; i < entry.mSize; ++i)
			{
				entry.mIndexList[i] = endian_int32(entry.mIndexList[i]);
			}
		}
#endif
	}

	if (mFlags & (1 << 9))
	{
		memcpy(&mSizeB,data,sizeof(int32));
		data += sizeof(int32);
#ifdef ZEQ_ENDIAN_CHECK
		mSizeB = endian_int32(mSizeB);
#endif

		mRefList = new int32[mSizeB];
		memcpy(mRefList,data,sizeof(int32) * mSizeB);
		data += sizeof(int32) * mSizeB;

		mData = new int32[mSizeB];
		memcpy(mData,data,sizeof(int32) * mSizeB);

#ifdef ZEQ_ENDIAN_CHECK
		if (!isLittleEndian())
		{
			for (int32 i = 0; i < mSizeB; ++i)
			{
				mRefList[i] = endian_int32(mRefList[i]);
				mData[i] = endian_int32(mData[i]);
			}
		}
#endif
	}
	else
	{
		mRefList = nullptr;
		mData = nullptr;
		mSizeB = 0;
	}
}

SkeletonTrackSetFragment::~SkeletonTrackSetFragment()
{
	for (int32 i = 0; i < mSizeA; ++i)
	{
		delete[] mEntries->mIndexList;
	}
	delete[] mEntries;
	delete[] mRefList;
	delete[] mData;
}

TextureBitmapRefFragment::TextureBitmapRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mRef,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mFlags,data,sizeof(int32));

#ifdef ZEQ_ENDIAN_CHECK
	mRef = endian_uint32(mRef);
	mFlags = endian_int32(mFlags);
#endif
}

TextureBitmapFragment::TextureBitmapFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mNumRefs,data,sizeof(int32));
	data += sizeof(int32);

#ifdef ZEQ_ENDIAN_CHECK
	mFlags = endian_uint32(mFlags);
	mNumRefs = endian_int32(mNumRefs);
#endif

	if (mFlags & (1 << 2))
	{
		memcpy(&mParam[0],data,sizeof(int32));
		data += sizeof(int32);
#ifdef ZEQ_ENDIAN_CHECK
		mParam[0] = endian_int32(mParam[0]);
#endif
	}
	else
	{
		mParam[0] = 0;
	}
	if (mFlags & (1 << 3))
	{
		memcpy(&mParam[1],data,sizeof(int32));
		data += sizeof(int32);
#ifdef ZEQ_ENDIAN_CHECK
		mParam[1] = endian_int32(mParam[1]);
#endif
	}
	else
	{
		mParam[1] = 0;
	}

	mRefList = new uint32[mNumRefs];
	memcpy(mRefList,data,sizeof(uint32) * mNumRefs);
#ifdef ZEQ_ENDIAN_CHECK
	if (!isLittleEndian())
	{
		for (int32 i = 0; i < mNumRefs; ++i)
		{
			mRefList[i] = endian_uint32(mRefList[i]);
		}
	}
#endif
}

TextureBitmapFragment::~TextureBitmapFragment()
{
	delete[] mRefList;
}

TextureBitmapNameFragment::TextureBitmapNameFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mNumNames,data,sizeof(int32));
	data += sizeof(int32);
#ifdef ZEQ_ENDIAN_CHECK
	mNumNames = endian_int32(mNumNames);
#endif

	if (mNumNames == 0)
		mNumNames = 1;

	for (int32 i = 0; i < mNumNames; ++i)
	{
		uint16 len;
		memcpy(&len,data,sizeof(uint16));
		data += sizeof(uint16);
#ifdef ZEQ_ENDIAN_CHECK
		len = endian_uint16(len);
#endif

		if (len > 0)
		{
			//we append "_Material" to texture names to distinguish the processed textures from the raw source images
			byte* name = new byte[len + 9];
			memcpy(name,data,len - 1); //ignore the '\0'
			data += len;

			DecodeName(name,len);
			char* cname = (char*)name;
			strlwr(cname);
			memcpy(&cname[len - 1],"_Material",10);
			mNameList.push_back(cname);
		}
	}
}

TextureBitmapNameFragment::~TextureBitmapNameFragment()
{
	for (auto itr = mNameList.begin(); itr != mNameList.end(); itr++)
	{
		const char* name = *itr;
		delete[] name;
	}
}

AnimatedMeshFragment::AnimatedMeshFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mFlags,data,sizeof(uint32));
	data += sizeof(uint32);
	memcpy(&mVertexCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mFrameCount,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mParam1,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mParam2,data,sizeof(int16));
	data += sizeof(int16);
	memcpy(&mScale,data,sizeof(int16));
	data += sizeof(int16);

	
	float s = 1.0f / (1 << mScale);
	//for each frame...
	mVertexData = new Vector3*[mFrameCount];
	for (int16 i = 0; i < mFrameCount; ++i)
	{
		Vector3* frame = mVertexData[i];
		frame = new Vector3[mVertexCount];
		//for each vertex in a frame...
		for (int16 j = 0; j < mVertexCount; ++j)
		{
			Vector3& v = frame[j];
			int16 d[3];
			memcpy(d,data,sizeof(int16) * 3);
			data += sizeof(int16) * 3;
			v.x = d[0] * s;
			v.y = d[1] * s;
			v.z = d[2] * s;
		}
	}
}

AnimatedMeshRefFragment::AnimatedMeshRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type) :
Fragment(nameRef,nameList,type)
{
	memcpy(&mRef,data,sizeof(int32));
	data += sizeof(int32);
	memcpy(&mFlags,data,sizeof(uint32));
}


