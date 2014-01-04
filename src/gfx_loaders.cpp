
#include "gfx_loaders.h"

void Decompress(byte* src, byte* dst, uint32 slen, uint32 dlen)
{
	z_stream z;

	z.zalloc = nullptr;
	z.zfree = nullptr;
	z.opaque = nullptr;

	z.next_in = src;
	z.avail_in = slen;
	z.next_out = dst;
	z.avail_out = dlen;

	if (inflateInit(&z) != Z_OK || inflate(&z,Z_NO_FLUSH) != Z_STREAM_END || inflateEnd(&z) != Z_OK)
	{
		throw ZEQException("ZLib inflation failed");
	}
}

//Headers only used during initial loading
struct S3DHeader
{
	uint32 dirOffset;
	char magicWord[4];
	uint32 unknown;
};

struct S3DBlockHeader
{
	uint32 deflatedLen;
	uint32 inflatedLen;
};

struct S3DDirEntry
{
	uint32 CRC;
	uint32 offset;
	uint32 inflatedLen;
};

S3D::S3D(FILE* fp, ZoneData* zone_data, Ogre::SceneManager* sceneMgr, bool is_main, bool is_obj)
{
	S3DHeader header;
	fread(&header,sizeof(S3DHeader),1,fp);
	if (header.magicWord[0] != 'P' || header.magicWord[1] != 'F' || header.magicWord[2] != 'S' || header.magicWord[3] != ' ')
	{
		fclose(fp);
		throw ZEQException("Invalid S3D file header");
	}

	uint32 nDirEntries;
	fseek(fp,header.dirOffset,SEEK_SET);
	fread(&nDirEntries,sizeof(uint32),1,fp);
#ifdef ZEQ_ENDIAN_CHECK
	nDirEntries = endian_uint32(nDirEntries);
#endif

	//S3D Directory Entries are listed in order of CRC, but the list of file names are in order of entry offset
	//Load entries into a map sorted by offset
	std::map<uint32,S3DDirEntry> entryMap;
	for (uint32 i = 0; i < nDirEntries; ++i)
	{
		S3DDirEntry entry;
		fread(&entry,sizeof(S3DDirEntry),1,fp);
#ifdef ZEQ_ENDIAN_CHECK
		entry.mInflatedLen = endian_uint32(entry.mInflatedLen);
		entry.mOffset = endian_uint32(entry.mOffset);
#endif
		entryMap[entry.offset] = entry; //copy construction
	}

	std::vector<S3DFileEntry> fileEntries;

	//retrieve contained files
	for (auto itr = entryMap.begin(); itr != entryMap.end(); itr++)
	{
		S3DDirEntry& entry = itr->second;
		S3DFileEntry fileEntry;

		fileEntry.mDataSize = 0;
		uint32 offset = entry.offset;

		uint32 infLenRead = 0;
		uint32 toRead = entry.inflatedLen;
		StackBuffer buf;

		while (infLenRead < toRead)
		{
			S3DBlockHeader blockHeader;
			fseek(fp,offset,SEEK_SET);
			fread(&blockHeader,sizeof(S3DBlockHeader),1,fp);
#ifdef ZEQ_ENDIAN_CHECK
			blockHeader.mDeflatedLen = endian_uint32(blockHeader.mDeflatedLen);
			blockHeader.mInflatedLen = endian_uint32(blockHeader.mInflatedLen);
#endif

			byte* compressed = new byte[blockHeader.deflatedLen];
			byte* decompressed = new byte[blockHeader.inflatedLen];

			fread(compressed,sizeof(byte),blockHeader.deflatedLen,fp);
			Decompress(compressed,decompressed,blockHeader.deflatedLen,blockHeader.inflatedLen);
			buf.AddWithoutCopying(decompressed,blockHeader.inflatedLen);
			delete[] compressed;

			offset += blockHeader.deflatedLen + sizeof(S3DBlockHeader);
			infLenRead += blockHeader.inflatedLen;
			fileEntry.mDataSize += blockHeader.inflatedLen;
		}

		//put data into file entry structure
		fileEntry.mData = buf.TakeData();
		fileEntries.push_back(fileEntry);
	}

	//final file entry contains file names
	byte* fileNameData = fileEntries.back().mData;
	uint32 nNames;
	memcpy(&nNames,fileNameData,sizeof(uint32));
#ifdef ZEQ_ENDIAN_CHECK
	nNames = endian_uint32(nNames);
#endif
	uint32 pos = sizeof(uint32);
	for (uint32 i = 0; i < nNames; ++i)
	{
		//pascal-style strings with 32bit len field
		uint32 len;
		memcpy(&len,&fileNameData[pos],sizeof(uint32));
#ifdef ZEQ_ENDIAN_CHECK
		len = endian_uint32(len);
#endif
		pos += sizeof(uint32);
		char* nameptr = (char*)&fileNameData[pos];
		pos += len;

		//make sure name is all lowercase
		strlwr(nameptr);

		fileEntries[i].mFileName = nameptr;
	}

	LoadContents(zone_data,&fileEntries,sceneMgr,is_main,is_obj);
	//deallocate mDatas
	for (auto itr = fileEntries.begin(); itr != fileEntries.end(); itr++)
	{
		S3DFileEntry& entry = *itr;
		delete[] entry.mData;
	}
}

void S3D::LoadContents(ZoneData* zone_data, std::vector<S3DFileEntry>* fileEntries, Ogre::SceneManager* sceneMgr, bool is_main, bool is_obj)
{
	Ogre::TextureManager* texMgr = Ogre::TextureManager::getSingletonPtr();
	Ogre::MaterialManager* matMgr = Ogre::MaterialManager::getSingletonPtr();
	std::vector<S3DFileEntry*> WLDList;

	for (auto itr = fileEntries->begin(); itr != fileEntries->end(); itr++)
	{
		S3DFileEntry& entry = *itr;
		const char* extension = strstr(entry.mFileName,".");

		if (extension)
		{
			//most files in S3Ds are bmp
			if (strcmp(extension,".bmp") == 0)
			{
				Ogre::TexturePtr tex;
				char magic = (char)*entry.mData;
				if (magic == 'D')
				{
					//need to alter DDS headers to exclude bad mipmaps
					uint32 temp = 1;
					memcpy(&entry.mData[28],&temp,sizeof(uint32));
					temp = 0x1000;
					memcpy(&entry.mData[108],&temp,sizeof(uint32));
				}
				else
				{
					//need to manually add an alpha channel to palettized BMPs
					BITMAPINFOHEADER* bmpHeader = (BITMAPINFOHEADER*)&entry.mData[14];
					if (bmpHeader->biBitCount == 8)
					{
						BITMAPFILEHEADER* bmpFile = (BITMAPFILEHEADER*)entry.mData;
						RGBQUAD* palette = (RGBQUAD*)&entry.mData[14 + sizeof(BITMAPINFOHEADER)];
						uint8* entries = (uint8*)&entry.mData[bmpFile->bfOffBits];
						uint8 tb = palette[0].rgbBlue, tg = palette[0].rgbGreen, tr = palette[0].rgbRed;
						//create manual texture
						tex = texMgr->createManual(entry.mFileName,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,Ogre::TEX_TYPE_2D,
							bmpHeader->biWidth,bmpHeader->biHeight,0,Ogre::PF_BYTE_BGRA,Ogre::TU_DEFAULT);
						Ogre::HardwarePixelBufferSharedPtr pixelptr = tex->getBuffer();
						pixelptr->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY);
						const Ogre::PixelBox& pixel = pixelptr->getCurrentLock();

						uint8* ptr = static_cast<uint8*>(pixel.data);

						//write data
						uint32 width = bmpHeader->biWidth;
						for (int32 i = (bmpHeader->biHeight - 1) * 4; i >= 0; i -= 4)
						{
							uint8* output = ptr + i * width; //set this up better
							for (int32 j = bmpHeader->biWidth; j > 0; --j)
							{
								RGBQUAD& data = palette[*entries++];
								*output++ = data.rgbBlue;
								*output++ = data.rgbGreen;
								*output++ = data.rgbRed;
								//alpha
								if (data.rgbRed == tr && data.rgbGreen == tg && data.rgbBlue == tb)
									*output++ = 0;
								else
									*output++ = 255;
							}
							output += pixel.getRowSkip() * Ogre::PixelUtil::getNumElemBytes(pixel.format); //no-op?
						}

						pixelptr->unlock();
					}
				}

				if (tex.isNull())
				{
					Ogre::DataStreamPtr data(new Ogre::MemoryDataStream(entry.mData,entry.mDataSize));
					Ogre::Image img;
					img.load(data);
					tex = texMgr->loadImage(entry.mFileName,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,img);
				}

				//regardless of format, we're ready to set up the material now
				char mat_name[64];
				snprintf(mat_name,64,"%s_Material",entry.mFileName);
				Ogre::MaterialPtr mat = matMgr->create(mat_name,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
				pass->createTextureUnitState(entry.mFileName);
				pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
				pass->setAlphaRejectSettings(Ogre::CMPF_GREATER,254); //comparison is to *display*, not reject
			}
			else if (strcmp(extension,".wld") == 0)
			{
				//need to defer wld loading until after all textures they may refer to are loaded
				WLDList.push_back(&entry);
			}
		}
	}
	
	for (auto itr = WLDList.begin(); itr != WLDList.end(); itr++)
	{
		S3DFileEntry* entry = *itr;
		WLD wld(entry,zone_data,sceneMgr,is_main,is_obj);
		zone_data->LoadSprites();
	}

	if (is_main)
		zone_data->BuildZoneMeshes(sceneMgr);
	else if (is_obj)
		zone_data->BuildObjectMeshes(sceneMgr);
	else
		zone_data->BuildMobModelMeshes(sceneMgr);

	zone_data->mFragsByIndex.clear();
	zone_data->mFragsByName.clear();
}


struct WLDHeader
{
	uint32 magicWord;
	uint32 version;
	uint32 maxFragment;
	uint32 unknownA[2];
	uint32 nameHashLen;
	uint32 unknownB;
};

struct FragHeader
{
	uint32 len;
	uint32 type;
	int32 nameRef;
};

WLD::WLD(S3DFileEntry* entry, ZoneData* zone_data, Ogre::SceneManager* sceneMgr, bool is_main, bool is_obj)
{
	byte* data = entry->mData;

	WLDHeader* header = reinterpret_cast<WLDHeader*>(data);
	uint32 pos = sizeof(WLDHeader);

#ifdef ZEQ_ENDIAN_CHECK
	header->magicWord = endian_uint32(header.magicWord);
	header->version = endian_uint32(header.version);
#endif
	header->version &= 0xFFFFFFFE;
	if (header->magicWord != WLD::MAGIC || (header->version != WLD::VERSION1 && header->version != WLD::VERSION2))
	{
		throw ZEQException("Invalid WLD header");
	}
	int version = (header->version == WLD::VERSION1) ? 1 : 2;

#ifdef ZEQ_ENDIAN_CHECK
	header->nameHashLen = endian_uint32(header.nameHashLen);
#endif
	byte* names = new byte[header->nameHashLen]; //shouldn't need to copy this probably
	memcpy(names,&data[pos],header->nameHashLen); //need to structure post-wld loading better first though
	zone_data->mNameData = names;

	Fragment::DecodeName(names,header->nameHashLen);
	pos += header->nameHashLen;

	//fragment loop
#ifdef ZEQ_ENDIAN_CHECK
	header->maxFragment = endian_uint32(header.maxFragment);
#endif
	for (uint32 i = 0; i < header->maxFragment; ++i)
	{
		FragHeader* fragHeader = reinterpret_cast<FragHeader*>(&data[pos]);
		pos += sizeof(FragHeader);
#ifdef ZEQ_ENDIAN_CHECK
		fragHeader->len = endian_uint32(fragHeader->len);
		fragHeader->nameRef = endian_int32(fragHeader->nameRef);
		fragHeader->type = endian_uint32(fragHeader->type);
#endif

		Fragment* frag;

		switch (fragHeader->type)
		{
			case 0x03:
			{
				TextureBitmapNameFragment* add = new TextureBitmapNameFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				frag = add;
				break;
			}
			case 0x04:
			{
				frag = new TextureBitmapFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				break;
			}
			case 0x05:
			{
				TextureBitmapRefFragment* add = new TextureBitmapRefFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				frag = add;
				break;
			}
			case 0x10:
			{
				frag = new SkeletonTrackSetFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				break;
			}
			case 0x11:
			{
				frag = new AnimationRefFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				break;
			}
			case 0x12:
			{
				frag = new SkeletonPieceTrackFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				break;
			}
			case 0x13:
			{
				SkeletonPieceRefFragment* add = new SkeletonPieceRefFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				zone_data->mSkelePieceRefFrags[add->mName] = add;
				frag = add;
				break;
			}
			case 0x14:
			{
				ModelFragment* add = new ModelFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				zone_data->mModelFrags.push_back(add);
				frag = add;
				break;
			}
			case 0x15:
			{
				ObjectLocRefFragment* add = new ObjectLocRefFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				zone_data->mObjLocRefFrags.push_back(add);
				frag = add;
				break;
			}
			case 0x2D:
			{
				frag = new MeshRefFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				break;
			}
			case 0x2F:
			{
				AnimatedMeshRefFragment* add = new AnimatedMeshRefFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				zone_data->mAnimMeshFrags.push_back(add);
				frag = add;
				break;
			}
			case 0x30:
			{
				TextureRefFragment* add = new TextureRefFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				frag = add;
				break;
			}
			case 0x31:
			{
				TextureListFragment* add = new TextureListFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type,i);
				zone_data->mTextureListFrags.push_back(add);
				frag = add;
				break;
			}
			case 0x36:
			{
				MeshFragment* add = new MeshFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type,version);
				uint32 flag = add->mFlags;
				if (flag == MESH_ZONE)
					zone_data->mZoneMeshFrags.push_back(add);
				frag = add;
				break;
			}
			case 0x37:
			{
				AnimatedMeshFragment* add = new AnimatedMeshFragment(fragHeader->nameRef,names,&data[pos],fragHeader->type);
				frag = add;
				break;
			}
			default:
				goto UNKNOWN_TYPE;
		}

		zone_data->mFragsByIndex[i] = frag;
		if (frag->mName)
			zone_data->mFragsByName[frag->mName] = frag;

UNKNOWN_TYPE:
		pos += fragHeader->len - 4;
	}
}
