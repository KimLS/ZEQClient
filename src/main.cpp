
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "Ogre.h"

#include "socket.h"
#include "zone_loader.h"


extern "C" {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
    int main(int argc, char** argv)
#endif
    {
#ifdef WIN32
		Socket::InitializeSocketLib();
#endif
        try
		{
			ZoneLoader zl;
            zl.go();
        }
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		catch (Ogre::Exception& e)
		{
            MessageBox(NULL,e.getFullDescription().c_str(),"An exception has occured!",MB_OK | MB_ICONERROR | MB_TASKMODAL);
		}
		catch (std::exception& e)
		{
			MessageBox(NULL,e.what(),"An exception has occured!",MB_OK | MB_ICONERROR | MB_TASKMODAL);
		}
#else
		catch(Ogre::Exception& e)
		{
            std::cerr << "An exception has occured: " <<
                e.getFullDescription().c_str() << std::endl;
		}
		catch (std::exception& e)
		{
			printf("%s\n",e.what());
		}
#endif
#ifdef WIN32
		Socket::CloseSocketLib();
#endif
        return 0;
    }
}
