#include "LLScreenLog.h"

using namespace Ogre;

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR > 7 // If you're using a version newer than 1.7.
	template<> LLScreenLog* Singleton<LLScreenLog>::msSingleton = 0;
#else
	template<> LLScreenLog* Singleton<LLScreenLog>::ms_Singleton = 0;
#endif

LLScreenLog::LLScreenLog()
{

	struct tm *pTime;
	char logFileName[256];
	time_t ctTime; time(&ctTime);
	pTime = localtime( &ctTime );
#ifdef _WINDOWS
	sprintf(logFileName, "log\\LLScreenLog_%i\-%02i\-%02i_%02i\-%02i\-%02i.log", (1900+pTime->tm_year), (1+pTime->tm_mon), pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
#else
	sprintf(logFileName, "log/LLScreenLog_%i\-%02i\-%02i_%02i\-%02i\-%02i.log", (1900+pTime->tm_year), (1+pTime->tm_mon), pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
#endif
	if (m_logFile = fopen(logFileName, "w"))
	{
		fprintf(m_logFile, "==========================================================================\n");
		fprintf(m_logFile, "LifeLike Screen Log. Ver.0.1\n");
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime (&rawtime);
		fprintf(m_logFile, "%s", asctime(timeinfo));
		fprintf(m_logFile, "==========================================================================\n");
	}
}

LLScreenLog::~LLScreenLog()
{
	if (m_logFile)
		fclose(m_logFile);
}

void LLScreenLog::init(int w, int h)
{
	// max lines = 42 lines for 768 height => 17.2 per line
	mMaxLine = h / 18.3;
	//mMaxLine = h / 20.3;
	mNextLine = mMaxLine -1;
	mDebugLines = new Ogre::String[mMaxLine];
	for (int i=0; i<mMaxLine; i++)
		mDebugLines[i] = "\n";

	OGRE_LOCK_AUTO_MUTEX
	OverlayManager& om = OverlayManager::getSingleton();
	mOverlay = om.create("LifeLike/LogOverlay");
	mOverlay->setZOrder(500);

	mPanel = static_cast<OverlayContainer*>(om.createOverlayElement("Panel", "LifeLike/LogOverlay/Panel"));
	mPanel->setMetricsMode(GMM_PIXELS);
	mPanel->setDimensions(w, h);
	mPanel->setPosition(15, 0);

	// now create the text...
	String name;
	mDebugBox = static_cast<Ogre::TextAreaOverlayElement*>(om.createOverlayElement("TextArea", "LifeLike/LogOverlay/Panel/Text"));
	mDebugBox->setMetricsMode(Ogre::GMM_PIXELS);
	mDebugBox->setDimensions(20, 5);		
	mDebugBox->setWidth(1.0f);
	mDebugBox->setHeight(0.5f);
	mDebugBox->setPosition(POSITION_TOP_X, -10);
	mDebugBox->setFontName("BlueHighway");
	mDebugBox->setCharHeight(18);
	mDebugBox->setColour(ColourValue(0.9, 0.9, 0.95, 1.0));
	mDebugBox->setCaption(mDebugString);
	mDebugBox->getMaterial();	// this fix bug on font loading. without this, font won't loaded
	mPanel->addChild(mDebugBox);

	mOverlay->add2D(mPanel);
}

void LLScreenLog::addText(const String& text) 
{
	OGRE_LOCK_AUTO_MUTEX

	// time stamp
	char chtime[12];
	struct tm *pTime;
	time_t ctTime; time(&ctTime);
	pTime = localtime( &ctTime );
	sprintf(chtime, "%2i:%2i:%2i: ", pTime->tm_hour, pTime->tm_min, pTime->tm_sec);

	int head = mNextLine;
	mDebugLines[mNextLine] = Ogre::String(chtime) + text + "\n";
	if (m_logFile)
		fprintf(m_logFile, "%s", mDebugLines[mNextLine].c_str());
	
	printf("%s", mDebugLines[mNextLine].c_str());
	mNextLine = (mNextLine+1) % mMaxLine;
	

//#ifdef _DEBUG
	mDebugString = "";
	for (int i=0; i<mMaxLine; i++)
	{		
		
		mDebugString.append(mDebugLines[(mNextLine+i)%mMaxLine]);
	}
//#endif

	mDebugBox->setCaption(mDebugString);
}

void LLScreenLog::showOverlay(bool show)
{
	if (show)
		mOverlay->show();
	else
		mOverlay->hide();
}
