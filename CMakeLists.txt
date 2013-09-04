#/*
#-----------------------------------------------------------------------------
#Filename:    CMakeLists.txt
#-----------------------------------------------------------------------------
#
#This source file is part of the
#   ___                 __    __ _ _    _ 
#  /___\__ _ _ __ ___  / / /\ \ (_) | _(_)
# //  // _` | '__/ _ \ \ \/  \/ / | |/ / |
#/ \_// (_| | | |  __/  \  /\  /| |   <| |
#\___/ \__, |_|  \___|   \/  \/ |_|_|\_\_|
#      |___/                              
#      Tutorial Framework
#      http://www.ogre3d.org/tikiwiki/
#-----------------------------------------------------------------------------
#*/
cmake_minimum_required(VERSION 2.6)

project(LifeLike)
 
if(WIN32)
	set(CMAKE_MODULE_PATH "$ENV{OGRE_HOME}/CMake/;${CMAKE_MODULE_PATH}")
	set(OGRE_SAMPLES_INCLUDEPATH
		$ENV{OGRE_HOME}/Samples/include
	)
endif(WIN32)
 
if(UNIX)
  	set(CMAKE_MODULE_PATH "$ENV{OGRE_HOME}/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
	set(OGRE_SAMPLES_INCLUDEPATH "$ENV{OGRE_HOME}/Samples/Common/include/")
endif(UNIX)
 
if (CMAKE_BUILD_TYPE STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

message("build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_DEBUG_POSTFIX "_d")

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

find_package(OGRE REQUIRED)
 
if(NOT "${OGRE_VERSION_NAME}" STREQUAL "Cthugha")
  message(SEND_ERROR "You need Ogre 1.7 Cthugha to build this.")
endif()
 
find_package(OIS REQUIRED)
 
if(NOT OIS_FOUND)
	message(SEND_ERROR "Failed to find OIS.")
endif()

# CEGUI
set(CEGUI_INCLUDE_DIRS "$ENV{CEGUI_HOME}/include;$ENV{CEGUI_HOME}/include/CEGUI")
set(CEGUI_LIBRARIES "optimized;$ENV{CEGUI_HOME}/lib/libCEGUIBase.so;debug;$ENV{CEGUI_HOME}/lib/libCEGUIBase.so")
set(CEGUIOGRE_LIBRARIES "optimized;$ENV{CEGUI_HOME}/lib/libCEGUIOgreRenderer.so;debug;$ENV{CEGUI_HOME}/lib/libCEGUIOgreRenderer.so")

#xerces
set(XERCES_INCLUDE_DIRS "$ENV{XERCES_HOME}/include;$ENV{XERCES_HOME}/include/xercesc")
set(XERCES_LIBRARIES "optimized;$ENV{XERCES_HOME}/lib/libxerces-c.so;debug;$ENV{XERCES_HOME}/lib/libxerces-c.so")


# Find Boost
if (NOT OGRE_BUILD_PLATFORM_IPHONE)
	if (WIN32 OR APPLE)
		set(Boost_USE_STATIC_LIBS TRUE)
	else ()
		# Statically linking boost to a dynamic Ogre build doesn't work on Linux 64bit
		set(Boost_USE_STATIC_LIBS ${OGRE_STATIC})
	endif ()
	if (MINGW)
		# this is probably a bug in CMake: the boost find module tries to look for
		# boost libraries with name libboost_*, but CMake already prefixes library
		# search names with "lib". This is the workaround.
		set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")
	endif ()
	set(Boost_ADDITIONAL_VERSIONS "1.44" "1.44.0" "1.42" "1.42.0" "1.41.0" "1.41" "1.40.0" "1.40" "1.39.0" "1.39" "1.38.0" "1.38" "1.37.0" "1.37" )
	# Components that need linking (NB does not include header-only components like bind)
	set(OGRE_BOOST_COMPONENTS thread date_time)
	find_package(Boost COMPONENTS ${OGRE_BOOST_COMPONENTS} QUIET)
	if (NOT Boost_FOUND)
		# Try again with the other type of libs
		set(Boost_USE_STATIC_LIBS NOT ${Boost_USE_STATIC_LIBS})
		find_package(Boost COMPONENTS ${OGRE_BOOST_COMPONENTS} QUIET)
	endif()
	find_package(Boost QUIET)
 
	# Set up referencing of Boost
	include_directories(${Boost_INCLUDE_DIR})
	add_definitions(-DBOOST_ALL_NO_LIB)
	set(OGRE_LIBRARIES ${OGRE_LIBRARIES} ${Boost_LIBRARIES})
endif()

 
set(HDRS
	./include/BaseApplication.h
	./include/LifeLike.h
	./include/LLActivityBase.h
	./include/LLActivityManager.h
	./include/LLAniManager.h
	./include/LLAnimationBlender.h
	./include/LLAnimationEntity.h
	./include/LLAnimation.h
	./include/LLAudioMonitor.h
	./include/LLCallBack.h
	./include/LLCharacter.h
	./include/LLdefine.h
	./include/LLEntity.h
	./include/LLFFTSound.h
	./include/LLFSM.h
	./include/LLFSMState.h
	./include/LLGUIManager.h
	./include/LLItem.h
	./include/LLMicMonitor.h
	./include/LLSceneManager.h
	./include/LLScreenLog.h
	./include/LLSound.h
	./include/LLSoundManager.h
	./include/LLSpeechRecognizer.h
	./include/LLUnicodeUtil.h
	./include/LLXMLParser.h
	./include/LLXStr.h
	./include/LLNavigator.h
)
 
set(SRCS
	./src/BaseApplication.cpp
	./src/LifeLike.cpp
	./src/LLActivityBase.cpp
	./src/LLActivityManager.cpp
	./src/LLAniManager.cpp
	./src/LLAnimationBlender.cpp
	./src/LLAnimation.cpp
	./src/LLAnimationEntity.cpp
	./src/LLAudioMonitor.cpp
	./src/LLCharacter.cpp
	./src/LLdefine.cpp
	./src/LLEntity.cpp
	./src/LLFFTSound.cpp
	./src/LLFSM.cpp
	./src/LLFSMState.cpp
	./src/LLGUIManager.cpp
	./src/LLItem.cpp
	./src/LLMicMonitor.cpp
	./src/LLSceneManager.cpp
	./src/LLScreenLog.cpp
	./src/LLSound.cpp
	./src/LLSoundManager.cpp
	./src/LLSpeechRecognizer.cpp
	./src/LLUnicodeUtil.cpp
	./src/LLXMLParser.cpp
	./src/LLNavigator.cpp
)
 
include_directories( ${OIS_INCLUDE_DIRS}
	${OGRE_INCLUDE_DIRS}
	${OGRE_SAMPLES_INCLUDEPATH}
	${CEGUI_INCLUDE_DIRS}
	${XERCES_INCLUDE_DIRS}
	./include
)
 
add_executable(LifeLike WIN32 ${HDRS} ${SRCS})
 
set_target_properties(LifeLike PROPERTIES DEBUG_POSTFIX _d)
 
#target_link_libraries(LifeLike ${OGRE_LIBRARIES} ${OIS_LIBRARIES})
target_link_libraries(LifeLike ${OGRE_LIBRARIES} ${OIS_LIBRARIES} ${CEGUI_LIBRARIES} ${CEGUIOGRE_LIBRARIES} ${XERCES_LIBRARIES} )

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# post-build copy for win32
if(WIN32 AND NOT MINGW)
	add_custom_command( TARGET LifeLike PRE_BUILD
		COMMAND if not exist .\\bin mkdir .\\bin )
	add_custom_command( TARGET LifeLike POST_BUILD
		COMMAND copy \"$(TargetPath)\" .\\bin )
endif(WIN32 AND NOT MINGW)

if(MINGW OR UNIX)
	set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
endif(MINGW OR UNIX)
 
if(WIN32)
 
	install(TARGETS LifeLike
		RUNTIME DESTINATION bin
		CONFIGURATIONS All)
 
	install(FILES ${CMAKE_SOURCE_DIR}/dist/bin/plugins.cfg
		${CMAKE_SOURCE_DIR}/dist/bin/resources.cfg
		${CMAKE_SOURCE_DIR}/dist/bin/stereo.cfg
		DESTINATION ./
		CONFIGURATIONS Release RelWithDebInfo
	)
 
	install(FILES ${CMAKE_SOURCE_DIR}/dist/bin/plugins_d.cfg
		${CMAKE_SOURCE_DIR}/dist/bin/resources_d.cfg
		${CMAKE_SOURCE_DIR}/dist/bin/stereo_d.cfg
		DESTINATION ./
		CONFIGURATIONS Debug
	)
 
        # NOTE: for the 1.7.1 sdk the OIS dll is called OIS.dll instead of libOIS.dll
        # so you'll have to change that to make it work with 1.7.1
	install(FILES ${OGRE_PLUGIN_DIR_REL}/OgreMain.dll
		${OGRE_PLUGIN_DIR_REL}/RenderSystem_Direct3D9.dll
		${OGRE_PLUGIN_DIR_REL}/RenderSystem_GL.dll
		${OGRE_PLUGIN_DIR_REL}/libOIS.dll
		DESTINATION ./
		CONFIGURATIONS Release RelWithDebInfo
	)
 
	install(FILES ${OGRE_PLUGIN_DIR_DBG}/OgreMain_d.dll
		${OGRE_PLUGIN_DIR_DBG}/RenderSystem_Direct3D9_d.dll
		${OGRE_PLUGIN_DIR_DBG}/RenderSystem_GL_d.dll
		${OGRE_PLUGIN_DIR_DBG}/libOIS_d.dll
		DESTINATION ./
		CONFIGURATIONS Debug
	)
 
   # as of sdk 1.7.2 we need to copy the boost dll's as well
   # because they're not linked statically (it worked with 1.7.1 though)
   install(FILES ${Boost_DATE_TIME_LIBRARY_RELEASE}
      ${Boost_THREAD_LIBRARY_RELEASE}
      DESTINATION ./
      CONFIGURATIONS Release RelWithDebInfo
   )
 
   install(FILES ${Boost_DATE_TIME_LIBRARY_DEBUG}
      ${Boost_THREAD_LIBRARY_DEBUG}
      DESTINATION ./
      CONFIGURATIONS Debug
   )
endif(WIN32)

if(UNIX)
 
	install(TARGETS LifeLike
		RUNTIME DESTINATION bin
		CONFIGURATIONS All)
 
	install(FILES ${CMAKE_SOURCE_DIR}/res/unix/plugins.cfg
		${CMAKE_SOURCE_DIR}/res/unix/resources.cfg
		${CMAKE_SOURCE_DIR}/res/unix/stereo.cfg
		${CMAKE_SOURCE_DIR}/res/unix/LifeLike.cfg
		DESTINATION ./
		CONFIGURATIONS Release RelWithDebInfo
	)
 
	install(FILES ${CMAKE_SOURCE_DIR}/res/unix/plugins_d.cfg
		${CMAKE_SOURCE_DIR}/res/unix/resources.cfg
		${CMAKE_SOURCE_DIR}/res/unix/stereo.cfg
		${CMAKE_SOURCE_DIR}/res/unix/LifeLike.cfg
		DESTINATION ./
		CONFIGURATIONS Debug
	)
	
endif(UNIX)
 
