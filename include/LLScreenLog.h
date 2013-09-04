#pragma once

#include <Ogre.h>
#include <OgreTextAreaOverlayElement.h>

class LLScreenLog : public Ogre::Singleton<LLScreenLog>
{
public:
	OGRE_AUTO_MUTEX
	LLScreenLog();
	~LLScreenLog();
	void init(int w=800, int h=600);
	void addText(const Ogre::String& text);
	void showOverlay(bool show);

protected:
	Ogre::OverlayContainer* mPanel;
	Ogre::Overlay* mOverlay;
	Ogre::TextAreaOverlayElement* mDebugBox;
	Ogre::String mDebugString;
	Ogre::String* mDebugLines;
	int mNextLine;
	int mMaxLine;

	// all are in pixel
	enum { POSITION_TOP_X = 5, POSITION_TOP_Y = 5, SPACING = 30 };
	
	FILE*			m_logFile;

};