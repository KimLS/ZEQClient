
#ifndef ZEQ_FRAGMENT_H
#define ZEQ_FRAGMENT_H

#include <vector>
#include "type.h"
#include "exception.h"
#include "byte_order.h"

struct Vector3
{
	float x, y, z;
};

struct Vector2
{
	float u, v;
};

struct ZEQPolygon
{
	uint16 index[3];
};

struct PolyTextureEntry
{
	int16 mCount;
	int16 mTextureID;
};

struct VertexPiece
{
	int16 mCount;
	int16 mIndex;
};

class Fragment
{
public:
	Fragment(int nameRef, byte* nameList, uint32 type);
	static void DecodeName(byte* src, size_t len);

	const char* mName;
	uint32 mType;
};

class MeshFragment : public Fragment //0x36
{
public:
	MeshFragment(int nameRef, byte* nameList, const byte* data, uint32 type, int wldVersion);
	~MeshFragment();

	uint32 mFlags;
	int32 mTextureListing;
	int32 mAnimatedVertex;
	int32 mUnknown[2];
	float mCenterX;
	float mCenterZ;
	float mCenterY;
	int32 mParams[3];
	float mMaxDist;
	float mMinX;
	float mMinY;
	float mMinZ;
	float mMaxX;
	float mMaxY;
	float mMaxZ;
	int16 mVertexCount;
	int16 mTextureCoordCount;
	int16 mNormalCount;
	int16 mColorCount;
	int16 mPolyCount;
	int16 mVertexPieceCount;
	int16 mPolyTextureCount;
	int16 mVertexTextureCount;
	int16 mSize9;
	int16 mScale;

	Vector3* mVertexList;
	Vector2* mTextureCoordList;
	Vector3* mNormalList;
	uint32* mColorList;
	ZEQPolygon* mPolyList;
	VertexPiece* mVertexPieceList;
	PolyTextureEntry* mPolyTextureList;
};

class TextureListFragment : public Fragment //0x31
{
public:
	TextureListFragment(int nameRef, byte* nameList, const byte* data, uint32 type, uint32 index);
	~TextureListFragment();

	uint32 mFlags;
	int32 mRefCount;

	uint32 mIndex;
	int32* mRefList;
};

class TextureRefFragment : public Fragment //0x30
{
public:
	TextureRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	uint32 mFlags;
	uint32 mParamA;
	int32 mParamB;
	float mParamC[2];
	int32 mRef;
};

class MeshRefFragment : public Fragment //0x2D
{
public:
	MeshRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	int32 mRef;
	uint32 mFlags;
};

class ObjectLocRefFragment : public Fragment //0x15
{
public:
	ObjectLocRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	int32 mRef;
	uint32 mFlags;
	int32 mRefB;
	float mX;
	float mZ;
	float mY;
	float mYRotation;
	float mZRotation;
	float mXRotation;
	float mYScale;
	float mZScale;
	float mXScale;
	int32 mRefC;
	int32 mParam;

	const char* mRefName;
};

class ModelFragment : public Fragment //0x14
{
public:
	ModelFragment(int nameRef, byte* nameList, const byte* data, uint32 type);
	~ModelFragment();

	uint32 mFlags;
	int32 mRef;
	int32 mSize[2];
	int32 mRefB;

	int32* mRefList;
};

class SkeletonPieceRefFragment : public Fragment //0x13
{
public:
	SkeletonPieceRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	int32 mRef;
	int32 mParam;
};

class SkeletonPieceTrackFragment : public Fragment //0x12
{
public:
	SkeletonPieceTrackFragment(int nameRef, byte* nameList, const byte* data, uint32 type);
	~SkeletonPieceTrackFragment();

	uint32 mFlags;
	int32 mSize;
	int16 mRotationDenominator;
	int16 mRotationX;
	int16 mRotationY;
	int16 mRotationZ;
	int16 mShiftX;
	int16 mShiftY;
	int16 mShiftZ;
	int16 mShiftDenominator;

	int32* mData;
};

class AnimationRefFragment : public Fragment //0x11
{
public:
	AnimationRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	int32 mRef;
	int32 mParam;
};

struct SkeletonTrackSet
{
	int32 mNameRef;
	uint32 mFlags;
	int32 mRef[2];
	int32 mSize;
	int32* mIndexList;
};

class SkeletonTrackSetFragment : public Fragment //0x10
{
public:
	SkeletonTrackSetFragment(int nameRef, byte* nameList, const byte* data, uint32 type);
	~SkeletonTrackSetFragment();

	uint32 mFlags;
	int32 mSizeA;
	int32 mRefA;
	int32 mParamA[3];
	float mParamB;
	int32 mSizeB;

	SkeletonTrackSet* mEntries;
	int32* mRefList;
	int32* mData;
};

class TextureBitmapRefFragment : public Fragment //0x05
{
public:
	TextureBitmapRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	uint32 mRef;
	int32 mFlags;
};

class TextureBitmapFragment : public Fragment //0x04
{
public:
	TextureBitmapFragment(int nameRef, byte* nameList, const byte* data, uint32 type);
	~TextureBitmapFragment();

	uint32 mFlags;
	int32 mNumRefs;
	int32 mParam[2];

	uint32* mRefList;
};

class TextureBitmapNameFragment : public Fragment //0x03
{
public:
	TextureBitmapNameFragment(int nameRef, byte* nameList, const byte* data, uint32 type);
	~TextureBitmapNameFragment();

	int32 mNumNames;

	std::vector<const char*> mNameList;
};

class AnimatedMeshFragment : public Fragment //0x37
{
public:
	AnimatedMeshFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	uint32 mFlags;
	int16 mVertexCount;
	int16 mFrameCount;
	int16 mParam1;
	int16 mParam2;
	int16 mScale;

	Vector3** mVertexData; //array of arrays of vertex data, mFrameCount * mVertexCount
};

class AnimatedMeshRefFragment : public Fragment //0x2F
{
public:
	AnimatedMeshRefFragment(int nameRef, byte* nameList, const byte* data, uint32 type);

	int32 mRef;
	uint32 mFlags;
};

#endif
