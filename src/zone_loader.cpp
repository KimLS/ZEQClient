
#include "zone_loader.h"

ZoneLoader::ZoneLoader()
{

}

void ZoneLoader::Load(const char* shortname)
{
	mZoneData = new ZoneData(mSceneMgr);
	char name_buf[256];

	//check if there is a main S3D file (original flavor zones)
	snprintf(name_buf,256,"%s.s3d",shortname);
	FILE* fp = fopen(name_buf,"rb");
	if (fp) {
		S3D mainS3D(fp,mZoneData,mSceneMgr,true);
		fclose(fp);
		snprintf(name_buf,256,"%s_obj.s3d",shortname);
		fp = fopen(name_buf,"rb");
		if (fp) {
			S3D objS3D(fp,mZoneData,mSceneMgr,false,true);
			fclose(fp);
		}
		snprintf(name_buf,256,"%s_chr.s3d",shortname);
		fp = fopen(name_buf,"rb");
		if (fp) {
			S3D chrS3D(fp,mZoneData,mSceneMgr);
			fclose(fp);
		}
	}
}

ZoneLoader::~ZoneLoader()
{

}

void ZoneLoader::createCamera()
{
	mCamera = mSceneMgr->createCamera("PlayerCam");
	mCamera->setPosition(Ogre::Vector3(0,10,500));
	mCamera->lookAt(Ogre::Vector3(0,0,0));
	mCamera->setNearClipDistance(0.1);
	mCamera->setFarClipDistance(1000);

	mCameraMan = new OgreBites::SdkCameraMan(mCamera);
}

void ZoneLoader::createViewports()
{
	Ogre::Viewport* vp = mWindow->addViewport(mCamera);

	vp->setBackgroundColour(Ogre::ColourValue(0,0,0));
	mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));
}

void ZoneLoader::createScene()
{
	Load("C:\\Users\\Sam\\Desktop\\Custom_Quest_EQ\\gfaydark");
	mSceneMgr->setAmbientLight(Ogre::ColourValue(1.0,1.0,1.0));

	mZoneData->mStaticGeometry->build();

	//pointless as nothing is reflective
	/*Ogre::Light* light = mSceneMgr->createLight("MainLight");
	light->setType(Ogre::Light::LT_DIRECTIONAL);
	light->setDirection(Ogre::Vector3(0,-1,0.75));
	light->setDiffuseColour(1,1,1);*/

	/*Ogre::Entity* ent = mSceneMgr->createEntity("WIL_ACTORDEF0");
	Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	
	node->attachObject(ent);
	node->setPosition(0,0,0);*/
}

bool ZoneLoader::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if(mWindow->isClosed())
        return false;

    if(mShutDown)
        return false;

	//Need to capture/update each device
	mInputContext.capture();

    mTrayMgr->frameRenderingQueued(evt);

    if (!mTrayMgr->isDialogVisible())
    {
        mCameraMan->frameRenderingQueued(evt);   // if dialog isn't up, then update the camera
        if (mDetailsPanel->isVisible())   // if details panel is visible, then update its contents
        {
            mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
            mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
            mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
            mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().w));
            mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().x));
            mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().y));
            mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().z));
        }
    }

	//if (mZoneData->mAnimState)
	//	mZoneData->mAnimState->addTime(evt.timeSinceLastFrame);
	if (mZoneData->mMobInstance)
		mZoneData->mMobInstance->AddAnimTime(evt.timeSinceLastFrame);

	Sleep(10);

    return true;
}