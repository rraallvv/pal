#include "factoryconfig.h"
#include "os.h"
//(c) Adrian Boeing 2004, see liscence.txt (BSD liscence)
/*
	Abstract:
		The "configuration" setting for the factory templates I choose to use.
	Author: 
		Adrian Boeing
	Revision History:
		Version 1.2  :06/12/07 MGF merge
		Version 1.1	 :28/06/07 Group DLL reimplementation
		Version 1.0.4:18/08/04 PAL modifications
	TODO:
*/

myFactory* myFactory::m_pInstance = 0;

typedef void   (*pt2SetFactory) (void*, void*);
typedef void *(*pt2CreateFunction) (void);
typedef void *(*pt2CreateGroupFunction) (int);
typedef UINT32 (*pt2PropertiesFunction) (void);
//typedef void (*pt2StatusTrackerFunction) (StatusTracker<StatusInfo> *pInstance);

#ifndef NDEBUG
#include <stdio.h>
#endif

#include "osfs.h"
#include "errorlog.h"

#if defined (OS_LINUX) || defined (OS_OSX)
#include <dlfcn.h>
#endif

static VECTOR<DYNLIB_HANDLE> svDlls;
//class myAbstractFOS : public myFactoryObject, public Serializable  {};
static VECTOR<myFactoryObject *> svDllObjects;

void myFactory::FreeObjects() {
	unsigned int i;
	for (i=0;i<svDllObjects.size();i++) {
		delete svDllObjects[i];
	}
#if defined (WIN32)
#pragma warning( disable : 4552) //disable warning from os portable DYNLIB macro
	for (i=0;i<svDlls.size();i++) {
		DYNLIB_UNLOAD(svDlls[i]);
	}
#pragma warning( default : 4552)
#endif
}

void myFactory::LoadObjects(char *szPath , void * factoryPointer, void *factoryInfoPointer) {

	char current_directory[4096];
	if (szPath != NULL) {
	GetCurrentDir(4096,current_directory);
	SetCurrentDir(szPath);
	}
	VECTOR<STRING> filesfound;
#if defined (WIN32)
	FindFiles("*.dll",filesfound);
#else
	FindFiles("*.so",filesfound);
#endif
	VECTOR<STRING>::size_type i;
	for (i=0;i<filesfound.size();i++) {

			const char *filename = filesfound[i].c_str();
				//printf("found : '%s'\n",filename);
				
				//build full location for *nix systems
				char full_location[4096];
				GetCurrentDir(4096,full_location);
				strcat(full_location,"/");
				strcat(full_location,filename);
				
				//load the dll
			DYNLIB_HANDLE hInst=DYNLIB_LOAD(full_location);
			if (hInst==NULL) {
				STATIC_SET_ERROR("Could not load DLL library %s",filename);
				#if defined (OS_LINUX) 
				STATIC_SET_ERROR("DLL/SO error: %s",dlerror());
				#endif
				continue;
			}
			#ifndef NDEBUG
			printf("Found dll %s\n",filename);
			#endif
			svDlls.push_back(hInst);
/*			pt2StatusTrackerFunction sfp = (pt2StatusTrackerFunction) GetProcAddress((HMODULE)hInst,"SetStatusTrackerInstance");
			if (sfp==NULL) {
//				LOG(SWARNING,"%s does not contain a valid status tracking component\n",filename);
				//continue;
			} else {
				//connect the status trackers.
				sfp(StatusTracker<StatusInfo>::GetInstance());
				printf("st connected\n");
			}
			*/

			//group creation functions
			pt2CreateGroupFunction fpg = (pt2CreateGroupFunction) DYNLIB_GETSYM(/*(HMODULE)*/hInst,"Group_CreateComponent");
			if (fpg == NULL) {
//				LOG(SWARNING,"%s does not contain a valid factory group component",filename);
			} else {

				pt2SetFactory fps = (pt2SetFactory) DYNLIB_GETSYM(hInst,"Group_SetFactory");
				if (fps)
					fps(factoryPointer,factoryInfoPointer);
				int *num = (int *)fpg(-1); //get the number of components in here.
#ifndef NDEBUG
				printf("%d components found in group\n",*num);
#endif
				for (int i=0;i<*num;i++) {
					void *vo=fpg(i); //void object pointer, construct a copy of the object for registration purposes
					myFactoryObject *fo= (myFactoryObject *) vo;
					fo->RegisterWithFactory(myFactory::sInfo());
					svDllObjects.push_back(fo);	
				}
			}

			//individual dll component creation functions
			pt2CreateFunction fp = (pt2CreateFunction) DYNLIB_GETSYM(/*(HMODULE)*/hInst,"CreateComponent");
			if (fp==NULL) {
//				LOG(SWARNING,"%s does not contain a valid factory component",filename);
			} else {

				void *vo=fp(); //void object pointer, construct a copy of the object for registration purposes
				myFactoryObject *fo= (myFactoryObject *) vo;
				fo->RegisterWithFactory(myFactory::sInfo());
				svDllObjects.push_back(fo);
#ifndef NDEBUG
				printf("constructor connected and registered\n");
#endif

				bool serializable;
				serializable=false;
				pt2PropertiesFunction pfp = (pt2PropertiesFunction) DYNLIB_GETSYM(/*(HMODULE)*/hInst,"GetProperties");
				if (pfp==NULL) {
					//serialization is not supported
				} else {
					if (pfp()&1) serializable = true;
				}
				if (serializable) {
#ifndef NDEBUG
					printf("this is serializable!\n");
#endif
/*					myAbstractFOS *s=(myAbstractFOS *)vo;
					SerialFactory::SerialRegister(s);*/
				}

			}


	}
	if (szPath != NULL) {
	SetCurrentDir(current_directory);
	}
}


myFactoryObject *myFactory::Construct(STRING ClassName) {
	myFactoryObject *ret;
	ret=newObject(ClassName);
	#ifdef INTERNAL_DEBUG
	printf("%s:%d:newobj:%x\n",__FILE__,__LINE__,ret);
	#endif
	if (ret==NULL) return NULL;
	Add(ret);
	return ret;
}

#ifdef INTERNAL_DEBUG
void myFactory::DisplayAllObjects() {
	//typename 
	VECTOR<myFactoryInfo>::iterator itv;
	itv=myFactory::sInfo().begin();
	printf("sInfo (%d : %d entries) contents:\n",&myFactory::sInfo(),myFactory::sInfo().size());
	while (itv!=myFactory::sInfo().end()) {
		printf("sInfo entry:%s\n",itv->mUniqueName.c_str());
		itv++;
	}
	printf("Current registry contents:\n");
	DisplayContents();
}
#endif
