/******************************************************************************
 *
 * LifeLike - LifeLike computer interfaces
 * Copyright (C) 2007 Sangyoon Lee, Electronic Visualization Laboratory, 
 * University of Illinois at Chicago
 *
 * This software is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either Version 2.1 of the License, or 
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public 
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser Public License along
 * with this software; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Questions or comments about LifeLike should be directed to 
 * cavern@evl.uic.edu
 *
 *****************************************************************************/

/*
* -----------------------------------------------------------------------------
* Filename: LLCharacter.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifdef _WINDOWS
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <sapi.h>
#include <sphelper.h>
#endif

#include <boost/thread.hpp>

#include "LLCharacter.h"
#include "LifeLike.h"
#include "LLAniManager.h"
#include "LLFFTSound.h"
#include "LLUnicodeUtil.h"
#include "LLSpeechRecognizer.h"
#include "LLActivityManager.h"
#include "LLSceneManager.h"
#include "LLMicMonitor.h"
#include "LLAudioMonitor.h"
#include "LLScreenLog.h"
#include "LLFACSDB.h"
#include "LLEmotionProcessor.h"
#include "LLAffectAnalyzer.h"
#include "LLHeadController.h"
#include "LLNVBEngine.h"

#define HEADDEFAULTPITCH 0.0f
#define HEADROTRANGE 20.0f

int findposeindex(const String& poseName);

bool resettingIdleExpression = false;

using namespace tinyxml2;

//-----------------------------------------------------------------------------
LLCharacter::LLCharacter(SceneNode* ParentNode)
{
	m_pEntity   = NULL;
	m_pNode		= NULL;
	m_pHeadBone	= NULL;
	m_pAniManager = NULL;
	m_pCurrFFTVoice = NULL;
	m_pParentNode = ParentNode;
	
#ifdef _WINDOWS
	m_pVoice	= NULL;
	m_pEar		= NULL;
	m_pActivityMgr = NULL;
	m_pSoundMonitor = NULL;
	m_pMicMonitor = NULL;
#endif

	for (int i=0; i<SHAPENUMBER; i++)
	{
		m_iShape2PoseIndex[i] = -1;
		m_fSpeechShapeWeights[i] = 0.0f;
		m_fPoseWeights[i] = 0.0f;
	}

	m_BlinkAnim.minWeightL = 0.0f;
	m_BlinkAnim.maxWeightL = 1.0f;
	m_BlinkAnim.minWeightR = 0.0f;
	m_BlinkAnim.maxWeightR = 1.0f;
	m_BlinkAnim.durationMin = 0.1f;
	m_BlinkAnim.durationMax = 0.5f;
	m_BlinkAnim.frequencyMin = 2.0f;
	m_BlinkAnim.frequencyMax = 5.0f;
	m_BlinkAnim.speedrate = 1.0f;
	m_BlinkAnim.frequencyrate = 1.0f;

	m_bInitialized = false;
	m_bBlinking = false;
	m_bSpeaking = false;

	m_tSpeechType = CSP_TTS;
	m_iSpeechContext = 0;		// 0 is common dialog (conversational dialog), 1 is domain
	m_iSpeechGroup = 0;			// variation id (per paragraph bases)

#ifdef _WINDOWS
	m_iViseme2PoseIndex[SP_VISEME_0] = 0;
	m_iViseme2PoseIndex[SP_VISEME_1] = P_AAH;
	m_iViseme2PoseIndex[SP_VISEME_2] = P_BIGAAH;
	m_iViseme2PoseIndex[SP_VISEME_3] = P_OH;
	m_iViseme2PoseIndex[SP_VISEME_4] = P_EH;
	m_iViseme2PoseIndex[SP_VISEME_5] = P_EH;
	m_iViseme2PoseIndex[SP_VISEME_6] = P_I;
	m_iViseme2PoseIndex[SP_VISEME_7] = P_W;
	m_iViseme2PoseIndex[SP_VISEME_8] = P_OOHQ;
	m_iViseme2PoseIndex[SP_VISEME_9] = P_EH;
	m_iViseme2PoseIndex[SP_VISEME_10] = P_EH;
	m_iViseme2PoseIndex[SP_VISEME_11] = P_EH;
	m_iViseme2PoseIndex[SP_VISEME_12] = P_EE;
	m_iViseme2PoseIndex[SP_VISEME_13] = P_R;
	m_iViseme2PoseIndex[SP_VISEME_14] = P_EH;
	m_iViseme2PoseIndex[SP_VISEME_15] = P_CHJSH;
	m_iViseme2PoseIndex[SP_VISEME_16] = P_CHJSH;
	m_iViseme2PoseIndex[SP_VISEME_17] = P_TH;
	m_iViseme2PoseIndex[SP_VISEME_18] = P_FV;
	m_iViseme2PoseIndex[SP_VISEME_19] = P_DST;
	m_iViseme2PoseIndex[SP_VISEME_20] = P_K;
	m_iViseme2PoseIndex[SP_VISEME_21] = P_BMP;
#endif
	m_iCurrPhoneme = -1;

	m_fAnimationSpeed = 1.0f;
	
	m_tAction = CAT_IDLE;

	m_fFFTCurrLevel = 0;

	m_iMaxPoseReference = 0;

	m_bSessionClosing = false;

	m_fHeadPitch = 0.0f;
	m_fHeadXRotRange = 10.0f;
	m_fHeadYRotRange = 10.0f;
	m_fHeadXOffset = 0.0f;
	m_fHeadYOffset = 0.0f;
	m_fFaceTrackWidth = 0.2f;
	m_tEyeMode = CET_SCREEN;

	m_fLastAttractionPoint[0] = 0.5f;
	m_fLastAttractionPoint[1] = 0.5f;
	m_fLastAttractionPoint[2] = 1.0f;
	
	m_fScreenPointer[0] = 0.5f;
	m_fScreenPointer[1] = 0.5f;
	m_fScreenPointer[2] = 1.0f;

	m_fUserPosition[0] = 0.5f;
	m_fUserPosition[1] = 0.5f;
	m_fUserPosition[2] = 1.0f;

	m_fAttractionPosition[0] = 0.5f;
	m_fAttractionPosition[1] = 0.5f;
	m_fAttractionPosition[2] = 1.0f;

	m_fBlendMask = 1.0f;
	m_fBlendMaskDelta = 0.0f;
	m_bAttracted = false;
	m_bForcedAttraction = false;
	m_fAttractionExpire = 0.0f;
	m_fAttractionFactor = 0.5f;

	// 
	m_eType			= ET_CHARACTER;

	//
	m_pMoveAnimState = NULL;

	// FACS to Shape index
	for (int i=0; i<FACSNUMBER; i++)
		m_iFACS2Shape[i] = -1;

	m_pEmotion = NULL;
	m_pAffectAnalyzer = NULL;
	m_pFACSDB = NULL;

	// Emotion to Shape id 
	m_iEmotion2Shape[0] = E_ANGER;
	m_iEmotion2Shape[1] = E_ANGER;
	m_iEmotion2Shape[2] = E_DISGUST;
	m_iEmotion2Shape[3] = E_FEAR;
	m_iEmotion2Shape[4] = E_SMILE;
	m_iEmotion2Shape[5] = E_SAD;
	m_iEmotion2Shape[6] = E_SURPRISE;
	m_iEmotion2Shape[7] = E_SMILEOPEN;

	// new head controller
	m_pHeadController = NULL;
	m_pNVBEngine = NULL;

	// emotion decay factor: will multiply by AFFECTLIFETIME
	m_fEmotionDecayFactor = 1.0f;


	m_rTimeUntilNextToggle = 0.0f;

	m_iSpeechRate = 0;

	m_bUseAffectBehavior = true;
}

//-----------------------------------------------------------------------------
LLCharacter::~LLCharacter(void)
{
#ifdef _WINDOWS
	if (m_pVoice)
	{
		m_pVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, 0);
		m_pVoice->Release();
		m_pVoice = NULL;
	}
	// SAPI Uninitialize
	::CoUninitialize();
#endif
	// clean up fft voice vector
	int size = (int)m_vFFTVoiceVec.size();
	for (int i=0; i<size; i++)
	{
		LLFFTSound* s = m_vFFTVoiceVec[i];
		delete s;
		m_vFFTVoiceVec[i] = NULL;
	}

	m_vFFTVoiceVec.clear();

	// clean up fft speech pool
	int size1 = (int)m_vFFTPoolVec.size();
	for (int i=0; i<size1; i++)
	{
		sSpeechPool* sp = m_vFFTPoolVec[i];
		int size2 = sp->sVec.size();
		for (int j=0; j<size2; j++)
		{
			LLFFTSound* s = sp->sVec[j];
			delete s;
			sp->sVec[j] = NULL;
		}
		sp->sVec.clear();
		delete sp;
		m_vFFTPoolVec[i] = NULL;
	}
	m_vFFTPoolVec.clear();

	// clean up fft shape list!!!
	for(iFFTShapeList::iterator it = m_lFFTShape.begin(); it != m_lFFTShape.end(); it++)
	{
		// delete band list in each fft shape struct
		sFFTShape* sShape = *it;
		for(iFBandsList::iterator itt = sShape->bands.begin(); itt != sShape->bands.end(); itt++)
		{
			delete *itt;
		}
		sShape->bands.clear();
		delete sShape;
	} 
	m_lFFTShape.clear();

#ifdef _WINDOWS
	if (m_pEar)
		delete m_pEar;

	if (m_pActivityMgr)
		delete m_pActivityMgr;
#endif

	if (m_pAniManager)
		delete m_pAniManager;

	if (m_pFACSDB)
		delete m_pFACSDB;

	if (m_pEmotion)
		delete m_pEmotion;

	if (m_pAffectAnalyzer)
		delete m_pAffectAnalyzer;

	if (m_pHeadController)
		delete m_pHeadController;

	if (m_pNVBEngine)
		delete m_pNVBEngine;
}

//-----------------------------------------------------------------------------
bool LLCharacter::loadFromSpecFile(SceneManager* mgr, char* filename)
{
	// load character xml file and create character
	XMLElement *element, *element1, *element2;

	const char* str;
	
	// load xml file
	m_xmlDoc.LoadFile(filename);
	element = m_xmlDoc.RootElement();	// <Character id="0" name="myname" file="mymesh.mesh">
	
	// parse character info
	const char *name, *meshfile; int id;
	name = element->Attribute("name");
	meshfile = element->Attribute("file");
	id = element->IntAttribute("id");

	// create character
	createEntity(mgr, name, meshfile);

	// sub attrbute (position, headbone, pitch, animation file, fftspeech)
	// Position
	element1 = element->FirstChildElement("Position");
	if (element1)
	{
		float posx = element1->FloatAttribute("x");
		float posy = element1->FloatAttribute("y");
		float posz = element1->FloatAttribute("z");
		Vector3 vv(posx, posy, posz);		
		setPosition( vv );
	}

	// Orientation
	element1 = element->FirstChildElement("Orientation");
	if (element1)
	{
		float rot = element1->FloatAttribute("value");
		Quaternion qt;
		qt.FromAngleAxis(Degree(rot), Vector3::UNIT_Y);
		setOrientation(qt);
	}

	// Animation spec file
	element1 = element->FirstChildElement("Animation");
	if (element1)
	{
		str = element1->Attribute("file");
#ifndef _WINDOWS
		correctSlash(str);
#endif				
		initAnimationManager(str);
	}

	// Head bone
	element1 = element->FirstChildElement("Head");
	if (element1)
	{
		str = element1->Attribute("bone");
		float pitch = element1->FloatAttribute("pitch");
		setHeadBone(str);
		m_fHeadPitch = pitch;
		m_fHeadXRotRange = element1->FloatAttribute("xrot");
		m_fHeadYRotRange = element1->FloatAttribute("yrot");
		m_fHeadXOffset = element1->FloatAttribute("xoffset");
		m_fHeadYOffset = element1->FloatAttribute("yoffset");
		m_fFaceTrackWidth = element1->FloatAttribute("width");

		// should pass this information to LLHeadController
		// at least pitch value for facegen head model
		m_pHeadController->setPitchOffset(pitch);
	}

	// Voice
	element1 = element->FirstChildElement("Voice");
	if (element1)
	{
		str = element1->Attribute("name");
		int rate = element1->IntAttribute("rate");
		int vol = element1->IntAttribute("volume");
		float fftfactor =element1->FloatAttribute("fftfactor");
#ifdef _WINDOWS
		initVoice(str, rate, vol);
#endif
		element2 = element1->FirstChildElement("FFTShape");
		const char* stype;
		int shapeid, bandid;
		float factor, thresh, min, max;
		while(element2)
		{
			stype = element2->Attribute("type");
			shapeid = element2->IntAttribute("shape");
			factor = element2->FloatAttribute("factor");
			thresh = element2->FloatAttribute("threshold");
			min = element2->FloatAttribute("min");
			max = element2->FloatAttribute("max");

			if (strcmp(stype, "band") == 0)
			{
				bandid = element2->IntAttribute("band");
				addFFTShape(shapeid, 1, bandid, factor*fftfactor, thresh, min, max);
			}
			else
				addFFTShape(shapeid, 0, -1, factor*fftfactor, thresh, min, max);

			element2 = element2->NextSiblingElement("FFTShape");
		}
	}

	// Activity (Python)
	element1 = element->FirstChildElement("PythonActivity");
	if (element1)
	{
		int puse = element1->IntAttribute("use");
		if (puse)
		{
			str = element1->Attribute("dir");
#ifndef _WINDOWS
			correctSlash(str);
#endif				
			int sruse = element1->IntAttribute("useSR");
			if (sruse)
				initActivity(str, true);
			else
				initActivity(str, false);
		}
	}

	// Speech
	element1 = element->FirstChildElement("Speech");
	if (element1)
	{
		// new version for 2010 demo
		// FFT has speech string	: fSpeechPool
		// child node FFTFile has file="wave filename"	: fSpeechPool.sVec
		const char* sentence, *wavefile;
		int context = 0;
		element2 = element1->FirstChildElement("FFT");
		while (element2)
		{
			// FFT speech file
			sentence = element2->Attribute("speech");
			context = element2->IntAttribute("context");

			// create sSpeechPool, add it to vector, and add hashmap item
			sSpeechPool* spool = new sSpeechPool;
			spool->lastused = -1;
			spool->context = context;
			m_vFFTPoolVec.push_back(spool);
			m_mFFTVoiceHash[sentence] = m_vFFTPoolVec.size() - 1;

			// list wave files...
			XMLElement* element3 = element2->FirstChildElement("FFTFile");
			while (element3)
			{
				wavefile = element3->Attribute("file");
#ifndef _WINDOWS
				correctSlash(wavefile);
#endif				
				
				// create LLFFTSound
				LLFFTSound* fs = new LLFFTSound();
				
				if ( fs->loadSound(wavefile, sentence) )
				{
					// callback function
					fs->i_CharacterEventCallback.setCallback(this, &LLCharacter::eventHandler);
					
					// add this to pool
					spool->sVec.push_back(fs);

					// add nested events
					const char* evtype, *param;
					float evtime;
					XMLElement* element4 = element3->FirstChildElement("SEvent");
					while (element4)
					{
						evtime = element4->FloatAttribute("time");
						evtype = element4->Attribute("type");
						param = element4->Attribute("param");
						
						// event type
						tEVENT etype = EVT_NONE;
						if (strcmp(evtype, "EVT_SOUND")==0)
							etype = EVT_SOUND;
						else if (strcmp(evtype, "EVT_SPEAK")==0)
							etype = EVT_SPEAK;
						else if (strcmp(evtype, "EVT_FFTSPEAK")==0)
							etype = EVT_FFTSPEAK;
						else if (strcmp(evtype, "EVT_ACTION")==0)
							etype = EVT_ACTION;
						else if (strcmp(evtype, "EVT_SHAPE")==0)
							etype = EVT_SHAPE;
						else if (strcmp(evtype, "EVT_VISIBILITY")==0)
							etype = EVT_VISIBILITY;
						else if (strcmp(evtype, "EVT_ATTRACT")==0)
							etype = EVT_ATTRACT;
						else if (strcmp(evtype, "EVT_CAPTION")==0)
							etype = EVT_CAPTION;

						fs->addEvent(evtime, etype, param);		// need to modify: use hashmap to find event id

						element4 = element4->NextSiblingElement("SEvent");
					}
				}
				else
				{
					delete fs;
				}

				element3 = element3->NextSiblingElement("FFTFile");
			}

			// move to the next sibling
			element2 = element2->NextSiblingElement("FFT");
		}
	}
	
	// Neutral Expression: if character needs an initial face with some expresison enabled
	// is this really necessary? well, probably i.e. Jim's slight eye closure
	// this also may cause some conflict with facial expresison on the fly (i.e. some of these
	// is enabled by other reason. then, how handle this?)
	element1 = element->FirstChildElement("NeutralExpression");
	if (element1)
	{
		const char* sname;
		float sweight;
		element2 = element1->FirstChildElement("Shape");
		while (element2)
		{
			// each shape
			sname = element2->Attribute("name");
			sweight = element2->FloatAttribute("weight");
			addNeutralExpression(sname, sweight);
			
			element2 = element2->NextSiblingElement("Shape");
		}
	}

	// Idle Expression
	element1 = element->FirstChildElement("IdleExpression");
	if (element1)
	{
		const char* sname;
		float o0, f0, f1, d0, d1;
		int gid;

		element2 = element1->FirstChildElement("IEGroup");
		while (element2)
		{
			// each group
			sname = element2->Attribute("name");
			o0 = element2->FloatAttribute("initialOffset");
			f0 = element2->FloatAttribute("frequencymin");
			f1 = element2->FloatAttribute("frequencymax");
			d0 = element2->FloatAttribute("durationmin");
			d1 = element2->FloatAttribute("durationmax");

			gid = addIdleExpressionGroup(sname, o0, f0, f1, d0, d1);
			
			XMLElement* element3 = element2->FirstChildElement("Shape");
			const char* ssname;
			float offset;
			while (element3)
			{
				// each group
				ssname = element3->Attribute("name");
				f0 = element3->FloatAttribute("wmin");
				f1 = element3->FloatAttribute("wmax");
				offset = element3->FloatAttribute("offset");
				
				addIdleExpression(gid, ssname, f0, f1, offset);

				element3 = element3->NextSiblingElement("Shape");
			}

			element2 = element2->NextSiblingElement("IEGroup");
		}
	}
	
	// Blinking rate
	element1 = element->FirstChildElement("Bliking");
	if (element1)
	{
		element2 = element1->FirstChildElement("Speed");
		m_BlinkAnim.durationMin = element2->FloatAttribute("min");
		m_BlinkAnim.durationMax = element2->FloatAttribute("max");

		element2 = element1->FirstChildElement("Rate");
		m_BlinkAnim.frequencyMin = element2->FloatAttribute("min");
		m_BlinkAnim.frequencyMax = element2->FloatAttribute("max");
	}

	// Wrinkle Region for Shapes
	element1 = element->FirstChildElement("WrinkleRegion");
	if (element1)
	{
		// shader for Wrinkle mapping
		str = element1->Attribute("Shader");
		if (strlen(str))
		{
			try{
				Ogre::MaterialPtr mptr = (Ogre::MaterialPtr)Ogre::MaterialManager::getSingleton().getByName(str);
				m_ptrWeightParams = mptr->getTechnique(0)->getPass(1)->getFragmentProgramParameters();
				m_ptrWeightParams->setNamedConstant("wWeight", Vector4(0.0f, 0.0f, 0.0f, 0.0f));
			}
			catch (Ogre::Exception e)
			{
				// do nothing for now
				if (!m_ptrWeightParams.isNull())
					m_ptrWeightParams.setNull();
			}

			element2 = element1->FirstChildElement("Shape");
			const char* sname;
			int region, idx;
			float weight;
			bool bRegionAdded = false;
			while (element2)
			{
				// each shape
				sname = element2->Attribute("name");
				region = element2->IntAttribute("region");
				weight = element2->FloatAttribute("weight");
				idx = findposeindex(sname);

				// map for multiple wrinkle region
				if (idx != -1)
				{
					sWRegion newRegion;
					newRegion.id = region;
					newRegion.weight = weight;
					if( !m_mWrinkleMap.empty() ) {
						// see if we already have a vector for this shape
						m_mWrinkleMapItr = m_mWrinkleMap.find(idx);
						if( m_mWrinkleMapItr != m_mWrinkleMap.end() ) {
							// push anohter wrinkle region to this vector
							(*m_mWrinkleMapItr).second.push_back(newRegion);
							bRegionAdded = true;
						}
					}
				
					if (!bRegionAdded)
					{
						// add new vactor for this shape
						wrVector newVector;
						newVector.push_back(newRegion);
						m_mWrinkleMap.insert(wrMap::value_type(idx,newVector));
					}
					bRegionAdded = false;
					// end of the second approach
				}

				element2 = element2->NextSiblingElement("Shape");
			}
		}
	}

	// FACS Database
	element1 = element->FirstChildElement("FACSDB");
	if (element1)
	{
		// DB file
		str = element1->Attribute("file");
		if (strlen(str) != 0)
			m_pFACSDB = new LLFACSDB(str);

		// FACS AU to Shape index Map
		const char* sname;
		int auid, idx;
		element2 = element1->FirstChildElement("AU");
		while (element2)
		{
			sname = element2->Attribute("shape");
			auid = element2->IntAttribute("id");
			idx = findposeindex(sname);

			element2 = element2->NextSiblingElement("AU");

			// unknown shape name?
			if (idx == -1)
			{
				continue;
			}
			// does this character model has this shpae?
			if (m_iShape2PoseIndex[idx] == -1)
				continue;

			// array map
			m_iFACS2Shape[auid] = idx;
		}
	}

	// NVB motion: i.e. head nod/shake
	element1 = element->FirstChildElement("NVBMotion");
	if (element1)
	{
		element2 = element1->FirstChildElement("NVB");
		while (element2)
		{
			// name(str), type(str), file(str)
			const char	*sname, *stype, *sfile;
			sname = element2->Attribute("name");
			stype = element2->Attribute("type");
			sfile = element2->Attribute("file");

			if (sname && stype && sfile)
				m_pHeadController->loadAnimationFromFile(sname, stype, sfile);

			element2 = element2->NextSiblingElement("NVB");
		}

	}

	// NVB tagger: nod/shake data-driven model
	if (m_pNVBEngine)
	{
		element1 = element->FirstChildElement("NVBTagger");
		if (element1)
		{
			element2 = element1->FirstChildElement("MODEL");
			while (element2)
			{
				// name(str), type(str), file(str)
				const char	*sname, *stype, *sfile;
				sname = element2->Attribute("name");
				stype = element2->Attribute("type");
				sfile = element2->Attribute("file");

				if (sname && stype && sfile)
				{
					m_pNVBEngine->loadCRFTagger(sfile, stype);
				}
				element2 = element2->NextSiblingElement("MODEL");
			}
		}
	}
	// Emotion & Personality
	element1 = element->FirstChildElement("Personality");
	float p,a,d,o,c,e,n;
	if (element1)
	{
		// create emtoion processor
		m_pEmotion = new LLEmotionProcessor();
		m_pAffectAnalyzer = new LLAffectAnalyzer(this, m_pEmotion);

		// PAD initial values
		p = element1->FloatAttribute("pleasure");
		a = element1->FloatAttribute("arousal");
		d = element1->FloatAttribute("dominance");

		// or Big Five Traits?
		// can we determine if this one has 5 traits values or not
		// if not, just use PAD value.
		element2 = element1->FirstChildElement("Openness");
		if (element2)
		{
			o = element2->FloatAttribute("value");

			element2 = element1->FirstChildElement("Conscientionsness");
			c = element2->FloatAttribute("value");

			element2 = element1->FirstChildElement("Extraversion");
			e = element2->FloatAttribute("value");

			element2 = element1->FirstChildElement("Agreeableness");
			a = element2->FloatAttribute("value");

			element2 = element1->FirstChildElement("Neuroticism");
			n = element2->FloatAttribute("value");

			m_pEmotion->setPersonality(o,c,e,a,n);
		}
		else
			m_pEmotion->setPersonality(p, a, d);

		
	}

	// Emotion Engine decay
	element1 = element->FirstChildElement("EmotionEngine");
	if (element1)
	{
		m_fEmotionDecayFactor = element1->FloatAttribute("decay");
	}

	// hair simulation
	element1 = element->FirstChildElement("HairSimulation");
	if (element1)
	{
		const char* shader = element1->Attribute("shader");
		const char* technique = element1->Attribute("technique");
		const char* pass = element1->Attribute("pass");

		if (shader)
		{
			try{
				// retrieve shader parameter
				Ogre::MaterialPtr mptr = (Ogre::MaterialPtr)Ogre::MaterialManager::getSingleton().getByName(shader);
				m_ptrHairShaderParams = mptr->getTechnique(technique)->getPass(pass)->getVertexProgramParameters();
			}
			catch (Ogre::Exception e)
			{
				// do nothing for now
				if (!m_ptrHairShaderParams.isNull())
					m_ptrHairShaderParams.setNull();
			}
		}
		else
			m_ptrHairShaderParams.setNull();
	}
	else
	{
		m_ptrHairShaderParams.setNull();
	}
	return true;
}

#ifdef _WINDOWS
//-----------------------------------------------------------------------------
void LLCharacter::initVoice(const char* voice, long rate, USHORT volume)
{
	if (FAILED(::CoInitialize(NULL)))
		return;
	
	char voicename[128];
	sprintf(voicename, "%s%s", "Name=", voice);

	int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, voicename, -1, NULL, 0);
	WCHAR*	szText = new WCHAR[size];
	ConvertStringToUnicode(voicename, szText, size);

	HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&m_pVoice);
	if (SUCCEEDED(hr))
	{
		CComPtr <ISpObjectToken>	cpToken;
		CComPtr <IEnumSpObjectTokens>	cpEnum;
		hr = SpEnumTokens(SPCAT_VOICES, szText, NULL, &cpEnum);
		hr = cpEnum->Next(1, &cpToken, NULL);
		hr = m_pVoice->SetVoice(cpToken); 

		m_pVoice->SetRate(rate);	// default rate is 0
		m_pVoice->SetVolume(volume); // default is 100 (10 is good for Fraps recording)
		m_pVoice->SetPriority(SPVPRI_OVER);

		m_iSpeechRate = rate;

		// testing sapi event
		ULONGLONG ullMyEvents = SPFEI(SPEI_TTS_BOOKMARK) | SPFEI(SPEI_PHONEME) | SPFEI(SPEI_SENTENCE_BOUNDARY) | SPFEI(SPEI_WORD_BOUNDARY);
		m_pVoice->SetInterest(ullMyEvents, ullMyEvents);
		m_pVoice->SetNotifyCallbackFunction(&this->sapiTTSCallBack, 0, LPARAM(this));

		// initial speech: prevent high cpu usage from the first use of sapi tts before app starts
		int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, "  ", -1, NULL, 0);
		WCHAR*	szText1 = new WCHAR[size];
		ConvertStringToUnicode("  ", szText1, size);
		m_pVoice->Speak(szText1, SPF_ASYNC|SPF_IS_XML, NULL);
		delete [] szText1;

	}

	delete [] szText;

}

//-----------------------------------------------------------------------------
void LLCharacter::setVoice(char* voice)
{
	// this function should not be called by other thread (i.e. python module)
	// sapi voice object does not work well in that way
	// use event queue instead

	// already has one?
	if (m_pVoice)
	{
		m_pVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, 0);
		m_pVoice->Release();
		m_pVoice = NULL;
	}

	//
	char voicename[128];
	sprintf(voicename, "%s%s", "Name=", voice);

	int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, voicename, -1, NULL, 0);
	WCHAR*	szText = new WCHAR[size];
	ConvertStringToUnicode(voicename, szText, size);

	HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&m_pVoice);
	if (SUCCEEDED(hr))
	{
		CComPtr <ISpObjectToken>	cpToken;
		CComPtr <IEnumSpObjectTokens>	cpEnum;
		hr = SpEnumTokens(SPCAT_VOICES, szText, NULL, &cpEnum);
		hr = cpEnum->Next(1, &cpToken, NULL);
		hr = m_pVoice->SetVoice(cpToken); 

		m_pVoice->SetPriority(SPVPRI_OVER);

		// sapi event
		ULONGLONG ullMyEvents = SPFEI(SPEI_TTS_BOOKMARK) | SPFEI(SPEI_PHONEME) | SPFEI(SPEI_SENTENCE_BOUNDARY) | SPFEI(SPEI_WORD_BOUNDARY);
		m_pVoice->SetInterest(ullMyEvents, ullMyEvents);
		m_pVoice->SetNotifyCallbackFunction(&this->sapiTTSCallBack, 0, LPARAM(this));
	}

	delete [] szText;

}

//-----------------------------------------------------------------------------
LLFFTSound* LLCharacter::addFFTSound(char* speech, char* filename)
{
	LLFFTSound* fSound;
	fSound = new LLFFTSound();
	if ( fSound->loadSound(filename, speech) )
	{
		m_vFFTVoiceVec.push_back(fSound);
		
		// hash map...
		m_mFFTVoiceHash[speech] = m_vFFTVoiceVec.size() - 1;

		// callback function
		fSound->i_CharacterEventCallback.setCallback(this, &LLCharacter::eventHandler);
		
		return fSound;
	}
	else
	{
		delete fSound;
		return NULL;
	}
}
#endif

//-----------------------------------------------------------------------------
void LLCharacter::initActivity(const char* scriptdir, bool useSR)
{
#ifdef _WINDOWS
	// SR test
	if (useSR)
		m_pEar = new LLSpeechRecognizer(this);

	LLScreenLog::getSingleton().addText("Character init python activities in " + Ogre::String(scriptdir));

	m_pActivityMgr = new LLActivityManager(this, scriptdir);
	m_pActivityMgr->init();
	//startListening();
#endif
}

//-----------------------------------------------------------------------------
void LLCharacter::initAnimationManager(const char* filename)
{
	// animanager test
	m_pAniManager = new LLAniManager(m_pEntity, this);
	m_pAniManager->loadAnimations(filename);
	
}

//-----------------------------------------------------------------------------
void LLCharacter::setHeadBone(const char* bonename)
{
	// manual head bone control
	SkeletonInstance* skel = m_pEntity->getSkeleton();
	try {
		m_pHeadBone = skel->getBone(bonename);
	
		// commented out below 2012.4.12
		m_pHeadBone->setManuallyControlled(true);
	
		if (m_pHeadController)
			delete m_pHeadController;
		m_pHeadController = new LLHeadController(m_pHeadBone);

		if (m_pNVBEngine)
			delete m_pNVBEngine;

		m_pNVBEngine = new LLNVBEngine(this, m_pHeadController);
	}
	catch (Exception &e) {
		printf("setHeadBone excetpion caught!!!\n");
		m_pHeadBone = NULL;
	}
}

//-----------------------------------------------------------------------------
Entity* LLCharacter::createEntity(SceneManager* mgr, const String& entityName, const String& meshName)
{
	if (m_pParentNode == NULL)
		m_pParentNode = mgr->getRootSceneNode();

	preLoadMesh(meshName);

	m_pEntity = mgr->createEntity(entityName, meshName);
	m_pNode = m_pParentNode->createChildSceneNode(entityName);
	m_pNode->attachObject(m_pEntity);
	
	// enable manual control animation
	m_pEntity->getAnimationState ("LLmanual")->setEnabled(true);
	
	m_bInitialized = true;

	m_pEntity->setQueryFlags(m_eType);

	return m_pEntity;
}

//-----------------------------------------------------------------------------
void LLCharacter::addFacialExpression(int iExpression, float duration, float maxweight, float midpoint, float maxduration, float starttime)
{
	if (midpoint == -1.0f)
		midpoint = duration / 2.0f;

	// double check this shape is available
	if (m_iShape2PoseIndex[iExpression] < 0)
	{
		LLScreenLog::getSingleton().addText("addFacialExpression faile: " + Ogre::StringConverter::toString(iExpression) + " shape not found ");
		
		return;
	}

	sFExpression* sExp = new sFExpression;
	sExp->currWeight = 0.0f;
	sExp->elapeseTime = 0.0f;
	sExp->endTime = duration;	
	sExp->deltaWeight = maxweight / midpoint;
	sExp->maxWeight = maxweight;
	sExp->minWeight = 0.0f;
	sExp->midTime = midpoint;
	sExp->poseIndex = iExpression;
	sExp->startTime = starttime;
	sExp->maxDuration = maxduration;

	// any chance to conflict with neutral expression or other existing ones
	// let's just ignore it for now
	
	if (iExpression == M_BLINKRIGHT)
	{
		// adjust some parameters...
		sExp->deltaWeight = (maxweight - m_BlinkAnim.minWeightR) / midpoint;
		sExp->currWeight = sExp->minWeight = m_BlinkAnim.minWeightR;
		m_lBlinkAnimationList.push_back(sExp);
	}
	else if (iExpression == M_BLINKLEFT)
	{
		// adjust some parameters...
		sExp->deltaWeight = (maxweight - m_BlinkAnim.minWeightL) / midpoint;
		sExp->currWeight = sExp->minWeight = m_BlinkAnim.minWeightL;
		m_lBlinkAnimationList.push_back(sExp);
	}
	else
		m_lFacialAnimationList.push_back(sExp);
}

//-----------------------------------------------------------------------------
void LLCharacter::addNeutralExpression(const char* shape, float weight)
{
	sFExpression* sExp = new sFExpression;
	sExp->currWeight = weight;
	sExp->elapeseTime = 0.0f;
	sExp->endTime = 0.0f;	
	sExp->deltaWeight = 0.0f;
	sExp->maxWeight = 0.0f;
	sExp->midTime = 0.0f;
	sExp->poseIndex = findposeindex(shape);
	sExp->startTime = 0.0f;
	sExp->maxDuration = 0.0f;

	m_lNeutralExpressionList.push_back(sExp);

	m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[sExp->poseIndex], weight);

	// is this blink : trick
	if (sExp->poseIndex==M_BLINKLEFT)
		m_BlinkAnim.minWeightL = weight;
	else if (sExp->poseIndex== M_BLINKRIGHT)
		m_BlinkAnim.minWeightR = weight;
}

//-----------------------------------------------------------------------------
void LLCharacter::resetNeutralExpression()
{
    ShapeAnimationList::iterator it, itend;
    itend = m_lNeutralExpressionList.end();
    for (it = m_lNeutralExpressionList.begin(); it != itend;)
    {
        sFExpression* src = *it;

		m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], src->currWeight);
	}	
}

//-----------------------------------------------------------------------------
void LLCharacter::update(Real addedTime)
{

	if (m_rTimeUntilNextToggle >= 0)
		m_rTimeUntilNextToggle -= addedTime;

	// update head manual control blendmask
	if (m_fBlendMaskDelta != 0.0f)
	{
		m_fBlendMask += m_fBlendMaskDelta * addedTime;
		if (m_fBlendMask < 0.0f)
		{
			m_fBlendMaskDelta = 0.0f;
			m_fBlendMask = 0.0f;
		}
		else if (m_fBlendMask > 1.0f)
		{
			m_fBlendMaskDelta = 0.0f;
			m_fBlendMask = 1.0f;
		}
		m_pAniManager->setBlendMask(m_fBlendMask);
	}
	
	// expiration check
	if (m_bAttracted)
	{
		if (m_fAttractionExpire >= 0.0f)
		{
			m_fAttractionExpire -= addedTime;
			if (m_fAttractionExpire < 0.0f)
			{
				m_bAttracted = false;
				m_bForcedAttraction = false;
				m_fBlendMaskDelta = 1.0f;
				m_fAttractionExpire = 0.0f;
			}
		}
	}

	// reset shape weight accumulator
	for (int i=0; i<SHAPENUMBER; i++)
		m_fPoseWeights[i] = 0.0f;

	updateFacialAnimation(addedTime);
	updateEyeInteraction(addedTime);
	updateEyeCentering(addedTime);		// for POWER project
	updateIdleExpression(addedTime);
	updateSpeech(addedTime);

	// new head controller
	if (m_pHeadController)
		m_pHeadController->update(addedTime);
	
	// update animation
	m_pEntity->getAnimationState ("LLmanual")->getParent()->_notifyDirty();

	// skeleton animation
	m_pAniManager->update(addedTime*m_fAnimationSpeed);

#ifdef _WINDOWS
	// update activity manager
	if (m_pActivityMgr)
		m_pActivityMgr->update(addedTime);
#endif

	// trick to handle very short continuous speak
	if (m_fIdleActionTimer>0.0f && !m_bSpeaking)
	{
		m_fIdleActionTimer -= addedTime;

		if (m_fIdleActionTimer < 0.0f)
			;//setAction(CAT_IDLE);
	}
	
	// move , rotate animation
	if (m_pMoveAnimState)
	{
		if (m_pMoveAnimState->hasEnded())
		{
			// clean up
			m_pMoveAnimState = NULL;

			// change action state to idle
			setAction(CAT_IDLE);
		}
		else
			m_pMoveAnimState->addTime(addedTime);
	}

	// update emtoion
	if (m_pEmotion)
	{
		// show emotional expression?
		m_pEmotion->update(addedTime);
	}

	// update general event list
	updateEvent(addedTime);

	// hair simulation
	// get derived transformation from head controller
	if (m_pHeadBone && !m_ptrHairShaderParams.isNull())
	{
		// current transform
		Quaternion qt = m_pHeadBone->_getDerivedOrientation();

		Matrix3 rot3;
		qt.ToRotationMatrix(rot3);
		Matrix4 rot4(rot3);
			
		// set shader parameter: only use momentary orientation value for now
		// later, may consider acceleration instead of current orientation
		m_ptrHairShaderParams->setNamedConstant("rotMatrix", rot4);

		//LLScreenLog::getSingleton().addText("hair mat: " + Ogre::StringConverter::toString(qt));
	}
}

//-----------------------------------------------------------------------------
void LLCharacter::updateEvent(Real addedTime)
{
	// update general event list
	// each event has associated eTime defining start time
	char *p_token;
	char seps[] = ":";

	m_lEventList;
	LLEventList::iterator it, itend;
    itend = m_lEventList.end();
    for (it = m_lEventList.begin(); it != itend;)
	{
		// decrease eTime by addedTime and see if it's time to handle it
		(*it).eTime -= addedTime;
		if ((*it).eTime > 0.0f)
		{
			it++;
			continue;
		}
		
		// process event and pop from list
		LLEvent evt = (*it);
		
		// move event
		if (evt.type == EVT_MOVE)
		{
			// parameters already decoded?
			if (strlen(evt.param) == 0)
				moveTo(Vector3(evt.args[0], evt.args[1], evt.args[2]), evt.args[3]);
			else
			{
				// parse arguments
				// "x:y:z:ori"
				float x = atof(strtok(evt.param, seps));
				float y = atof(strtok(NULL, seps));
				float z = atof(strtok(NULL, seps));
				float ori = atof(strtok(NULL, seps));

				moveTo(Vector3(x,y,z), ori);
			}
		}
		// set attraction point
		else if (evt.type == EVT_ATTRACTION)
		{
			// parameters already decoded?
			if (strlen(evt.param) == 0)
				setAttractionPosition(evt.args[0], evt.args[1], evt.args[2], true, 5.0f, evt.args[3]);
			else
			{
				// parse arguments
				// "x:y:z:expiretime"
				float x = atof(strtok(evt.param, seps));
				float y = atof(strtok(NULL, seps));
				float z = atof(strtok(NULL, seps));
				float expire = atof(strtok(NULL, seps));

				setAttractionPosition(x, y, z, true, 5.0f, expire);
			}

		}
		// change voice
		else if (evt.type == EVT_VOICE)
		{
			if (strlen(evt.param) != 0)
				setVoice(evt.param);
		}
		// chagne action
		else if (evt.type == EVT_ACTION)
		{
			setAction((tCHARACTER_ACTION)evt.ids[0]);
		}
		// chagne action
		else if (evt.type == EVT_GUI_VISIBILITY)
		{
			g_pLLApp->setGUIVisible(evt.param, evt.args[0]);
		}
		// chagne action
		else if (evt.type == EVT_GUI_MATERIAL)
		{
			// need to parse param
			char guiname[128], matname[128];
			strcpy(guiname, strtok(evt.param, seps));
			strcpy(matname, strtok(NULL, seps));
			
			g_pLLApp->setGUITexture(guiname, matname);
		}
		// chagne action
		else if (evt.type == EVT_MAT_MESSAGE)
		{
			// need to parse param (matName:play_mode:restart)
			char matname[128], param[128];
			strcpy(matname, strtok(evt.param, seps));
			strcpy(param, strtok(NULL, seps));
			p_token = strtok(NULL, seps);
			if (p_token)
			{
				char param2[128];
				strcpy(param2, p_token);
				g_pLLApp->setMaterialMsg(matname, param, param2);
			}
			else
				g_pLLApp->setMaterialMsg(matname, param);

		}
		// add more types here
		// else if (evt.type == EVT_ACTION)

		it = m_lEventList.erase(it++);
	}
}

//-----------------------------------------------------------------------------
void LLCharacter::updateFacialAnimation(Real addedTime)
{
	// reset wrinkle weight
	memset(m_vWrinkleWeight, 0, sizeof(float)*WRINKLENUMBER);
	int widx = -1;

	// traverse animation list and update it
    ShapeAnimationList::iterator it, itend;
    itend = m_lFacialAnimationList.end();
    for (it = m_lFacialAnimationList.begin(); it != itend;)
    {
        sFExpression* src = *it;
		
		src->startTime -= addedTime;
		if (src->startTime > 0)
		{
			it++;
			continue;
		}
		
		src->elapeseTime += addedTime;

		if( src->elapeseTime > src->endTime)
		{
			// end of expression lifetime

			// remove this shape
			if (src->poseIndex == M_BLINKRIGHT)
			{
				m_bBlinking = false;
				m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], m_BlinkAnim.minWeightR);
			}
			else if (src->poseIndex == M_BLINKLEFT)
			{
				m_bBlinking = false;
				m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], m_BlinkAnim.minWeightL);
			}
			else
			{
				if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
				{
					m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], 0.0f);
				}
			}
			delete *it;
			it = m_lFacialAnimationList.erase(it++);
			continue;
		}
		else if( src->elapeseTime > src->midTime && src->deltaWeight > 0)
		{
			// after mid time

			// some tweak to support peak hold (maxDuration)
			// no change in weight until maxDuration passes.
			if (src->elapeseTime > (src->midTime + src->maxDuration) )
			{
				// shape animation just passed peak moment
				// change its delta to negative value
				src->deltaWeight = -((src->maxWeight - src->minWeight) / (src->endTime - src->midTime - src->maxDuration) );
				src->currWeight += src->deltaWeight * addedTime;

				src->currWeight = CLAMP(src->currWeight, src->minWeight, src->maxWeight);
			}

			// only update accumulator
			m_fPoseWeights[src->poseIndex] = CLAMP(m_fPoseWeights[src->poseIndex] + src->currWeight, 0.0f, 1.0f);
			if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
				m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], m_fPoseWeights[src->poseIndex]);

		}
		else
		{
			// after max period
			
			// update shape weight
			src->currWeight += src->deltaWeight * addedTime;
			
			// clamp
			src->currWeight = CLAMP(src->currWeight, src->minWeight, src->maxWeight);

			// accumulator
			m_fPoseWeights[src->poseIndex] = CLAMP(m_fPoseWeights[src->poseIndex] + src->currWeight, 0.0f, 1.0f);

			if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
				m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], m_fPoseWeights[src->poseIndex]);
		}

		// update wrinkle weight
		m_mWrinkleMapItr = m_mWrinkleMap.find(src->poseIndex);
		if (m_mWrinkleMapItr == m_mWrinkleMap.end())
		{
			it++; 
			continue;
		}
		int iNum = (*m_mWrinkleMapItr).second.size();
		for (int i=0; i<iNum;i++)
		{
			widx = (*m_mWrinkleMapItr).second[i].id;
			m_vWrinkleWeight[widx] += (m_fPoseWeights[src->poseIndex] * (*m_mWrinkleMapItr).second[i].weight);
		}
		it++;
    }

	// clamp wrinkle weight values
	for (int i=0; i< WRINKLENUMBER; i++)
		m_vWrinkleWeight[i] = CLAMP(m_vWrinkleWeight[i], 0.0f, 1.0f);

	// apply wrinkle weight varible to shader!!!
	if (!m_ptrWeightParams.isNull())
		m_ptrWeightParams->setNamedConstant("wWeight", m_vWrinkleWeight, WRINKLENUMBER/4);

	// lipsync for speech
    itend = m_lLipAnimationList.end();
    for (it = m_lLipAnimationList.begin(); it != itend;)
    {
		sFExpression* src = *it;
		src->startTime -= addedTime;
		if (src->startTime > 0)
		{
			if (!m_bSpeaking)
			{
				// this must be queued
				delete *it;
				it = m_lLipAnimationList.erase(it++);
				continue;
			}

			it++;
			continue;
		}
		
		src->elapeseTime += addedTime;

		if( src->elapeseTime > src->endTime)
		{
			// remove this shape
			if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
				m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], 0.0f);
			
			delete *it;
			it = m_lLipAnimationList.erase(it++);

			continue;
		}
		else if( src->elapeseTime > src->midTime && src->deltaWeight > 0)
		{
			// some tweak to support peak hold (maxDuration)
			// no change in weight until maxDuration passes.
			if (src->elapeseTime > (src->midTime + src->maxDuration) )
			{
				// shape animation just passed peak moment
				// change its delta to negative value
				src->deltaWeight = -((src->maxWeight - src->minWeight) / (src->endTime - src->midTime - src->maxDuration) );
				src->currWeight += src->deltaWeight * addedTime;

				src->currWeight = CLAMP(src->currWeight, src->minWeight, src->maxWeight);

				if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
					m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], src->currWeight);

			}
		}
		else
		{
			// update shape weight
			src->currWeight += src->deltaWeight * addedTime;
			
			// clamp
			src->currWeight = CLAMP(src->currWeight, src->minWeight, src->maxWeight);

			if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
				m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], src->currWeight);
		}
		it++;
    }

}

//-----------------------------------------------------------------------------
void LLCharacter::updateIdleExpression(Real addedTime)
{
	// there are two very typical idle kind expression
	// blinking and other subtle movement

	// update blinking
	if (!m_bBlinking)
	{
		// let's check if there is blink shape in the event queue
		if (m_lBlinkAnimationList.size() == 0)
		{
			m_bBlinking = true;
			// generate next blink
			// blinking interval 2 to 5 sec, blinking duration 0.1 ~ 0.5 sec
			// in fact, alex blinks a lot(18 times for 14 secs...!)
			float random_factor = ((float)rand() / (float)RAND_MAX);
			sFExpression* rightb = new sFExpression;
			sFExpression* leftb = new sFExpression;
			
			rightb->currWeight = m_BlinkAnim.minWeightR;
			rightb->elapeseTime = leftb->elapeseTime = 0.0f;
			leftb->currWeight = m_BlinkAnim.minWeightL;

			// speed of blinking...
			float speed = m_BlinkAnim.durationMin + random_factor * (m_BlinkAnim.durationMax - m_BlinkAnim.durationMin);
			speed /= m_BlinkAnim.speedrate;
			rightb->endTime = leftb->endTime = speed;
			
			rightb->deltaWeight = 2.0f * (1.0f - m_BlinkAnim.minWeightR) / rightb->endTime;
			rightb->maxWeight = m_BlinkAnim.maxWeightR;
			rightb->minWeight = m_BlinkAnim.minWeightR;
			
			leftb->deltaWeight = 2.0f * (1.0f - m_BlinkAnim.minWeightL) / leftb->endTime;
			leftb->maxWeight = m_BlinkAnim.maxWeightL;
			leftb->minWeight = m_BlinkAnim.minWeightL;

			rightb->midTime = leftb->midTime = rightb->endTime / 2.0f;
			rightb->poseIndex = M_BLINKRIGHT; leftb->poseIndex = M_BLINKLEFT;
			float interval = m_BlinkAnim.frequencyMin + random_factor * (m_BlinkAnim.frequencyMax - m_BlinkAnim.frequencyMin);
			interval /= m_BlinkAnim.frequencyrate;
			rightb->startTime = leftb->startTime = interval;
			rightb->maxDuration = leftb->maxDuration = 0.0f;

			m_lFacialAnimationList.push_back(rightb);
			m_lFacialAnimationList.push_back(leftb);
		}
		else
		{
			// dequeue items and add them to animation list.
			sFExpression* src = m_lBlinkAnimationList.front();
			m_lBlinkAnimationList.pop_front();

			m_bBlinking = true;
			
			m_lFacialAnimationList.push_back(src);

			// if there is second one...
			if (m_lBlinkAnimationList.size() != 0)
			{
				sFExpression* src = m_lBlinkAnimationList.front();
				m_lBlinkAnimationList.pop_front();
				m_lFacialAnimationList.push_back(src);
			}
		}

	}

	// update other idle expression : very subtle motion defined in character xml file
	if (m_tAction != CAT_IDLE && m_bSpeaking)
	{
		if (resettingIdleExpression)
		{
			resettingIdleExpression = false;
		    
			ShapeAnimationList::iterator it, itend;
			itend = m_lIdleFacialAnimationList.end();
			for (it = m_lIdleFacialAnimationList.begin(); it != itend;)
			{
				sFExpression* src = *it;
				m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], 0.0f);
				delete src;
				it++;
			}
			m_lIdleFacialAnimationList.clear();
		}
		return;
	}

	// what we have?
	if ( !resettingIdleExpression)
	{
		int gNum = m_vIdleExpressionVec.size();
		int eNum;
		for (int i=0; i< gNum; i++)
		{
			m_vIdleExpressionVec[i]->nextTime -= addedTime;
			if (m_vIdleExpressionVec[i]->nextTime < 0.0f)
			{
				// time to initiate new set of expression
				eNum = m_vIdleExpressionVec[i]->vExpressions.size();
			
				sFExpression *expr;
				float nmax = 0.0f;
				float random_factor;
				for (int j=0; j<eNum; j++)
				{
					random_factor = ((float)rand() / (float)RAND_MAX);

					expr = m_vIdleExpressionVec[i]->vExpressions[j];
				
					expr->currWeight = 0.0f;
					expr->elapeseTime = 0.0f;
					expr->startTime = 0.5F * expr->offset + expr->offset * random_factor;
					random_factor = ((float)rand() / (float)RAND_MAX);
					expr->endTime = m_vIdleExpressionVec[i]->durationMin + random_factor * (m_vIdleExpressionVec[i]->durationMax - m_vIdleExpressionVec[i]->durationMin);
					expr->deltaWeight = 2.0f * (expr->maxWeight) / expr->endTime;
					expr->midTime = expr->endTime * 0.5f;

					nmax = (nmax > (expr->startTime + expr->endTime))? nmax : (expr->startTime + expr->endTime);

					///////////////////
					sFExpression* sExp = new sFExpression;
					sExp->currWeight = expr->currWeight;
					sExp->elapeseTime = expr->elapeseTime;
					sExp->endTime = expr->endTime;	
					random_factor = ((float)rand() / (float)RAND_MAX);
					sExp->maxWeight = expr->minWeight + random_factor * (expr->maxWeight  - expr->minWeight);
					sExp->minWeight = 0.0f;				
					sExp->deltaWeight = 2.0f * (sExp->maxWeight) / sExp->endTime;
					sExp->midTime = expr->midTime;
					sExp->poseIndex = expr->poseIndex;
					sExp->startTime = expr->startTime;
					sExp->maxDuration = 0.0f;

					if ( !resettingIdleExpression)
						m_lIdleFacialAnimationList.push_back(sExp);

				}
				random_factor = ((float)rand() / (float)RAND_MAX);
				m_vIdleExpressionVec[i]->nextTime = nmax + m_vIdleExpressionVec[i]->frequencyMin + random_factor * (m_vIdleExpressionVec[i]->frequencyMax - m_vIdleExpressionVec[i]->frequencyMin);
			}
		}

		// Now, update idle facial animation queue list
		ShapeAnimationList::iterator it, itend;
		itend = m_lIdleFacialAnimationList.end();
		for (it = m_lIdleFacialAnimationList.begin(); it != itend;)
		{
			sFExpression* src = *it;
		
			src->startTime -= addedTime;
			if (src->startTime > 0)
			{
				it++;
				continue;
			}
		
			src->elapeseTime += addedTime;

			if( src->elapeseTime > src->endTime)
			{
				if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
					m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], 0.0f);
			
				delete *it;
				it = m_lIdleFacialAnimationList.erase(it++);
				continue;
			}
			else if( src->elapeseTime > src->midTime && src->deltaWeight > 0)
			{
				// shape animation just passed peak moment
				// change its delta to negative value
				src->deltaWeight = -((src->maxWeight - src->minWeight) / (src->endTime - src->midTime) );
				src->currWeight += src->deltaWeight * addedTime;

				if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
					m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], src->currWeight);
			}
			else
			{
				// update shape weight
				src->currWeight += src->deltaWeight * addedTime;

				if (m_iShape2PoseIndex[src->poseIndex] < m_iMaxPoseReference)
					m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], src->currWeight);
			}
			it++;
		}
	}

}

//-----------------------------------------------------------------------------
int LLCharacter::addIdleExpressionGroup(const char* name, float o0, float f0, float f1, float d0, float d1)
{
	sFIdleExpression* fexp = new sFIdleExpression;
	
	fexp->frequencyMin = f0;
	fexp->frequencyMax = f1;
	fexp->durationMin = d0;
	fexp->durationMax = d1;
	fexp->nextTime = o0;
	
	m_vIdleExpressionVec.push_back(fexp);
	int gid = m_vIdleExpressionVec.size();

	return --gid;
}

//-----------------------------------------------------------------------------
void LLCharacter::addFFTShape(int shapeid, int type, int band, float factor, float thresh, float min, float max)
{
	// search existing shape list
	sFFTShape* sShape;
	sBands* newband;
	iFFTShapeList::iterator it, itend;
	itend = m_lFFTShape.end();
	for (it = m_lFFTShape.begin(); it != itend;)
	{
		sShape = *it;
		if (sShape->id == shapeid)
		{
			// add another band
			newband = new sBands;
			newband->id = band;
			newband->factor = factor;
			newband->thresh = thresh;
			newband->min = min;
			newband->max = max;
			newband->value = 0.0f;
			sShape->bands.push_back(newband);
			return;
		}
		it++;
	}

	// add new shape
	sShape = new sFFTShape;
	sShape->id = shapeid;
	sShape->type = type;
	newband = new sBands;
	newband->id = band;
	newband->factor = factor;
	newband->thresh = thresh;
	newband->min = min;
	newband->max = max;
	newband->value = 0.0f;
	sShape->bands.push_back(newband);
	m_lFFTShape.push_back(sShape);
}

//-----------------------------------------------------------------------------
void LLCharacter::addIdleExpression(int gid, const char* name, float w0, float w1, float offset)
{
	if (gid < m_vIdleExpressionVec.size())
	{
		sFExpression* sExp = new sFExpression;
		sExp->minWeight = w0;
		sExp->currWeight = 0.0f;
		sExp->elapeseTime = 0.0f;
		sExp->endTime = 0.0f;	
		sExp->deltaWeight = 0.0f;
		sExp->maxWeight = w1;
		sExp->midTime = 0.0f;
		sExp->poseIndex = findposeindex(name);
		sExp->startTime = 0.0f;
		sExp->offset = offset;
		sExp->maxDuration = 0.0f;

		m_vIdleExpressionVec[gid]->vExpressions.push_back(sExp);
	}
}

//-----------------------------------------------------------------------------
void LLCharacter::resetIdleExpression()
{
	// set all zero so that it does not interrupt with other action
	// scan list. if found active expression... set its weight to zero
	// then clear list
    ShapeAnimationList::iterator it, itend;
    itend = m_lIdleFacialAnimationList.end();
    for (it = m_lIdleFacialAnimationList.begin(); it != itend;)
    {
        sFExpression* src = *it;
		m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[src->poseIndex], 0.0f);
		delete src;
		it++;
	}

	m_lIdleFacialAnimationList.clear();

	resettingIdleExpression = true;
}


//-----------------------------------------------------------------------------
LLFFTSound* popSpeechPool(sSpeechPool*& pool, int& context, int& group)
{
	// two exceptional case: vector is empty or has only one item
	if (pool->sVec.size() == 0)
		return NULL;
	if (pool->sVec.size() == 1)
	{
		context = pool->context;
		group = pool->lastused;
		return pool->sVec[0];
	}

	// alternatives... what if we use paragraph based speech?
	// then, need to use same sentences from a single bulk paragraph speech
	// assume that only the first sentence picked randomly and all following sentences pick from the same group
	if (context == pool->context && context != 0)
	{
		// context is not conversational and it's same as before => use same group
		// pick the same one as previous' group
		// context and group remains same
		return pool->sVec[group];
	}
	else
	{
		// all other cases are random pick
		// prevent repetition from the last use of this pool entry
		if (pool->pool.empty())
		{
			// rebuild random pool
			for (int i=0; i<pool->sVec.size(); i++)
				if (i != pool->lastused)
					pool->pool.push_back(i);
			// shuffle
			std::random_shuffle(pool->pool.begin(), pool->pool.end());

			// pop the first one
			pool->lastused = pool->pool[0];
			pool->pool.erase(pool->pool.begin());

			// set context & group
			context = pool->context;
			group = pool->lastused;

			// return speech
			return pool->sVec[pool->lastused];
		}
		else
		{
			// pop the first one
			pool->lastused = pool->pool[0];
			pool->pool.erase(pool->pool.begin());

			// set context & group
			context = pool->context;
			group = pool->lastused;
			
			// return speech
			return pool->sVec[pool->lastused];
		}
	}

}

// replaec substring with new char*
static bool _replace(std::string& str, const char* fnd, const char* repl)
{
	bool brepl = false;
	size_t pos = 0;
	for(;;)
	{
		pos = str.find(fnd, pos);
		if(pos==std::string::npos) break;
		str.replace(pos, strlen(fnd), repl);
		brepl=true;
		pos++;
	}
	return brepl;
}

#ifdef _WINDOWS

// _trimLeft
static void _trimLeft(std::string &str, LPCTSTR delims=" \r\n\t")
{str.erase(0, str.find_first_not_of(delims));}

// _trimRight
static void _trimRight(std::string &str, LPCTSTR delims=" \r\n\t")
{str.erase(str.find_last_not_of(delims) + 1);}

bool LLCharacter::speak(const String& speech, tCHARACTER_SPEECH_TYPE type)
{
	if (m_bSpeaking)
		return false;
	
	if (type != CSP_TTS_LISTEN)
		stopListening();

	//
	m_fIdleActionTimer = -1.0f;
	m_tSpeechType = type;

	// exception case: DM may send cobination of speech sentences... (2009.11.16)
	// DM will send single string to be splittled by "." or "?"
	// temporary replace delim with ".|" or "?|", then tokenize with "|"
	// remove all white space at the beginning of sentences so that we can hash properly
	m_lSpeechQueue.clear();
	string tstr = speech;
	
	// for general purpose... adler... use # as delim
	char tempSpeech[MAXSTR];
	_replace(tstr, "#", "|");
	strcpy(tempSpeech, tstr.c_str());
	char* pch = strtok(tempSpeech, "|");
	int speechcount = 0;
	while (pch != NULL)
	{
		// remove whitespace at the beginning(left side) and ending(right side)
		tstr = string(pch);
		_trimLeft(tstr, " ");
		_trimRight(tstr, " ");
		sSpeech si;
		si.speech = tstr;
		si.type = type;
		m_lSpeechQueue.push_back(si);
		pch = strtok(NULL, "|");
	}
	string newspeech(m_lSpeechQueue.front().speech);
	m_lSpeechQueue.pop_front();
	// end of exception

	// evaluate emotional aspect of given speech!!!
	// note that this might conflit with WAVE type speech because we will add some facial expresison event
	// as sapi bookmark tag
	// processSpeechAffect can receive dialogAct parameter (the second. default none)
	if (type != CSP_WAVE && m_pAffectAnalyzer)
	{
		//newspeech = processSpeechAffect(newspeech);

		// spawn thread function here, then return
		boost::thread t(boost::bind(&LLCharacter::processSpeechAffectThread, this, newspeech, DACT_NON));
		return true;
	}

	if (type == CSP_TTS_LISTEN)
	{
		if (m_pVoice == NULL)
			return false;

		setAction(CAT_SPEAK);

		m_bSpeaking = true;
		int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, newspeech.c_str(), -1, NULL, 0);
		WCHAR*	szText = new WCHAR[size];
		ConvertStringToUnicode(newspeech.c_str(), szText, size);
		
		m_pVoice->Speak(szText, SPF_ASYNC|SPF_IS_XML, NULL);
		
		delete [] szText;
		return true;
	}
	// common TTS speech (sapi event) & TTS FFT (monitor stereo mixer)
	else if (type == CSP_TTS || type == CSP_TTS_FFT)
	{
		if (m_pVoice == NULL)
			return false;

		setAction(CAT_SPEAK);

		m_bSpeaking = true;

		int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, newspeech.c_str(), -1, NULL, 0);
		WCHAR*	szText = new WCHAR[size];
		ConvertStringToUnicode(newspeech.c_str(), szText, size);
		
		m_pVoice->Speak(szText, SPF_ASYNC|SPF_IS_XML, NULL);
		
		delete [] szText;
		return true;
	}
	// recorded voice(fft)
	else if (type == CSP_WAVE)	
	{
		// fft playback: find voice wav file (brute force)
		int numFFT = m_vFFTPoolVec.size();
		
		if (m_pCurrFFTVoice != NULL)
			m_pCurrFFTVoice->stopSound();

		m_pCurrFFTVoice = NULL;

		// use map for wave file id
		std::map<std::string,int>::iterator it;
		it = m_mFFTVoiceHash.find(newspeech);
		if (it != m_mFFTVoiceHash.end() )
		{
			sSpeechPool* spool = m_vFFTPoolVec[it->second];
			m_pCurrFFTVoice = popSpeechPool(spool, m_iSpeechContext, m_iSpeechGroup);

			if (m_pCurrFFTVoice)
			{
				m_pCurrFFTVoice->playSound();
				m_bSpeaking = true;
				setAction(CAT_SPEAK);
				return true;
			}
			else
			{
				// something wrong here. let's use TTS instead.
				setAction(CAT_SPEAK);
				m_bSpeaking = true;

				return speakContinue(newspeech, CSP_TTS);
			}
		}
		else
		{
			// wave file not found. then use TTS?
			setAction(CAT_SPEAK);
			m_bSpeaking = true;

			return speakContinue(newspeech, CSP_TTS);
		}

	}
	// mic monitor(fft)
	else if (type == CSP_MIC)
	{
		// this is mic monitor type
		m_bSpeaking = true;
		setAction(CAT_SPEAK);
		return true;
	}

}

bool LLCharacter::speakAffect(const String& speech)
{
	// only called by thread function
	// it should only be TTS type
	if (m_pVoice == NULL)
		return false;

	setAction(CAT_SPEAK);

	m_bSpeaking = true;

	int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, speech.c_str(), -1, NULL, 0);
	WCHAR*	szText = new WCHAR[size];
	ConvertStringToUnicode(speech.c_str(), szText, size);
		
	m_pVoice->Speak(szText, SPF_ASYNC|SPF_IS_XML, NULL);
	
	// caption testing for thesis: need to remove all <> tags
	{
		int s,e;
		// copy string
		string tempStr = speech;
		s = tempStr.find("<");
		if (s != -1)
		{
			while (s != -1)
			{
				e = tempStr.find("/>", s);
				if (e!=-1)
					tempStr.erase(s, e-s+2);
				s = tempStr.find("<");
			}
		}
		setCaptionString((char*)tempStr.c_str());
	}

	delete [] szText;
	return true;
}

//-----------------------------------------------------------------------------
bool LLCharacter::speakContinue(const String& speech, tCHARACTER_SPEECH_TYPE type)
{
	tCHARACTER_SPEECH_TYPE oldtype = m_tSpeechType;
	m_tSpeechType = type;
	if (type == CSP_TTS_LISTEN)
	{
		if (m_pVoice == NULL)
			return false;
		
		int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, speech.c_str(), -1, NULL, 0);
		WCHAR*	szText = new WCHAR[size];
		ConvertStringToUnicode(speech.c_str(), szText, size);
		
		m_pVoice->Speak(szText, SPF_ASYNC|SPF_IS_XML, NULL);
		
		delete [] szText;
		return true;
	}
	else if (type == CSP_TTS || type == CSP_TTS_FFT)
	{
		if (m_pVoice == NULL)
			return false;
		
		if (oldtype == CSP_WAVE)
			this->resetFFTShape(true);

		int size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, speech.c_str(), -1, NULL, 0);
		WCHAR*	szText = new WCHAR[size];
		ConvertStringToUnicode(speech.c_str(), szText, size);
		
		m_pVoice->Speak(szText, SPF_ASYNC|SPF_IS_XML, NULL);
		
		delete [] szText;
		return true;
	}
	// recorded voice(fft)
	else if (type == CSP_WAVE)	
	{
		// fft playback: find voice wav file (brute force)
		int numFFT = m_vFFTPoolVec.size();
		
		if (m_pCurrFFTVoice != NULL)
			m_pCurrFFTVoice->stopSound();

		m_pCurrFFTVoice = NULL;

		// use map for wave file id
		std::map<std::string,int>::iterator it;
		it = m_mFFTVoiceHash.find(speech);
		if (it != m_mFFTVoiceHash.end() )
		{
			sSpeechPool* spool = m_vFFTPoolVec[it->second];
			m_pCurrFFTVoice = popSpeechPool(spool, m_iSpeechContext, m_iSpeechGroup);

			if (m_pCurrFFTVoice)
			{
				if (oldtype == CSP_TTS)
				{
					if (m_iCurrPhoneme >=0)
					{
						m_fSpeechShapeWeights[m_iCurrPhoneme] = 0.0f;
						if (m_iCurrPhoneme < m_iMaxPoseReference)
							m_pVPoseKeyFrame->updatePoseReference(m_iCurrPhoneme, m_fSpeechShapeWeights[m_iCurrPhoneme]);

						m_iCurrPhoneme = -1;
					}

				}
				m_pCurrFFTVoice->playSound();
				return true;
			}
			else
			{
				// something wrong here. let's just use TTS then.
				return speakContinue(speech, CSP_TTS);
			}
		}
		else
		{
			// wave file not found. then use TTS?
			return speakContinue(speech, CSP_TTS);
		}

	}
	// mic monitor(fft)
	else if (type == CSP_MIC)
	{
		// do nothing: should not be called...
		return true;
	}
	
}

//-----------------------------------------------------------------------------
bool LLCharacter::stopSpeak()
{
	if (m_bSpeaking)
	{
		m_bSpeaking = false;
		
		// exception: 2009.11.16 for composite speech strings
		if (!m_lSpeechQueue.empty())
			m_lSpeechQueue.clear();

		if (m_pVoice == NULL) {
			return false;
		}
		
		if (m_pCurrFFTVoice != NULL)
		{
			// stop playing fft speech
			m_pCurrFFTVoice->stopSound();
			m_pCurrFFTVoice = NULL;

			// resetting lip shapes
			resetFFTShape(true);

		}
		else
		{
			// this crap code for minimize remaining sapi phoneme events
			// in some voice, this purge function takes quite amount of time. i.e. all cepstral voice
			// neospeech or other ms default voice is ok.
			m_pVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, 0);
			
			
			// need to reset lipsynch too here: somewhat not natural total reset.
			if (m_iCurrPhoneme >= 0) 
			{
				m_fSpeechShapeWeights[m_iCurrPhoneme] = 0.0f;

				if (m_iCurrPhoneme < m_iMaxPoseReference)
					m_pVPoseKeyFrame->updatePoseReference(m_iCurrPhoneme, m_fSpeechShapeWeights[m_iCurrPhoneme]);
			}

			// empty or clean up all lip shape related stuff

		}

		startListening();
		
		setAction(CAT_IDLE);
		
		return true;
	}
	else
	{
		// double check if current action state is speaking
		//if (m_tAction != CAT_IDLE)
		//	setAction(CAT_IDLE);

		return false;
	}

}
#endif

//-----------------------------------------------------------------------------
void LLCharacter::updateSpeech(Real addedTime)
{
#ifdef _WINDOWS
	if (!m_bSpeaking) {
		return;
	}
	
	if (m_tSpeechType == CSP_TTS || m_tSpeechType == CSP_TTS_LISTEN)
	{
		// SAPI TTS
		if (m_pVoice == NULL) {
			return;
		}

		HRESULT hr = S_OK;

		SPVOICESTATUS eventStatus;
		hr = m_pVoice->GetStatus( &eventStatus, NULL );
		if (hr != S_OK) {
			return;
		}
	
	#ifdef _DEBUG
		float speed_factor = 5.0f;
	#else
		float speed_factor = 4.0f;
	#endif

		if (eventStatus.dwRunningState == SPRS_DONE)
		{
			// exception: 2009.11.16 for composite speech strings
			if (!m_lSpeechQueue.empty())
			{
				string speech(m_lSpeechQueue.front().speech);
				tCHARACTER_SPEECH_TYPE tt = m_lSpeechQueue.front().type;
				m_lSpeechQueue.pop_front();
				speakContinue(speech, tt);

				return;
			}
			// end of exception

			m_bSpeaking = false;
			
			// for activity manager: broadcast speech done msg
			if (m_pActivityMgr)
				m_pActivityMgr->broadcastMsg("SPD");

			// once done
			// queue the last phoneme descrese expression into list
			
			if (m_iCurrPhoneme >=0)
			{
				
				sFExpression* sExp = new sFExpression;
				sExp->currWeight = m_fSpeechShapeWeights[m_iCurrPhoneme];
				sExp->elapeseTime = m_fSpeechShapeWeights[m_iCurrPhoneme] / speed_factor;
				sExp->endTime = sExp->elapeseTime * 1.2f;
				sExp->deltaWeight = -m_fSpeechShapeWeights[m_iCurrPhoneme] / (sExp->elapeseTime * 0.3f);
				sExp->maxWeight = 1.0f;
				sExp->minWeight = 0.0f;
				sExp->midTime = sExp->endTime - sExp->elapeseTime;
				sExp->poseIndex = m_iPose2ShapeIndex[m_iCurrPhoneme];
				sExp->startTime = -sExp->elapeseTime;
				sExp->maxDuration = 0.0f;
				m_lLipAnimationList.push_back(sExp);
				m_fSpeechShapeWeights[m_iCurrPhoneme] = 0.0f;
				
			}

			if (m_bSessionClosing)
			{
				setAction(CAT_IDLE);
				m_bSessionClosing = false;
			}

			m_fIdleActionTimer = 0.5f;
			
		}
		else
		{
			if (eventStatus.VisemeId != 0 )
			{

				int poseid = m_iShape2PoseIndex[m_iViseme2PoseIndex[eventStatus.VisemeId]];
				
				if (poseid < 0 || poseid >45 || poseid > m_iMaxPoseReference) {
					return;
				}

				if (m_iCurrPhoneme == poseid)
				{
					m_fSpeechShapeWeights[poseid] += speed_factor * addedTime;
					m_fSpeechShapeWeights[poseid] = CLAMP(m_fSpeechShapeWeights[poseid], 0.0f, 1.0f);
					m_pVPoseKeyFrame->updatePoseReference(poseid, m_fSpeechShapeWeights[poseid]);
				}
				else if (m_iCurrPhoneme == -1)
				{	
					// this is the first time use
					m_iCurrPhoneme = poseid;
					m_fSpeechShapeWeights[poseid] = speed_factor * addedTime;
					m_fSpeechShapeWeights[poseid] = CLAMP(m_fSpeechShapeWeights[poseid], 0.0f, 1.0f);
					m_pVPoseKeyFrame->updatePoseReference(poseid, m_fSpeechShapeWeights[poseid]);
				}
				else
				{
					// phoneme changed here
					// quick hack for smooth decrease of lip weight
					sFExpression* sExp = new sFExpression;
					sExp->currWeight = m_fSpeechShapeWeights[m_iCurrPhoneme];
					sExp->elapeseTime = m_fSpeechShapeWeights[m_iCurrPhoneme] / speed_factor;
					sExp->endTime = sExp->elapeseTime * 1.3f;
					sExp->deltaWeight = -m_fSpeechShapeWeights[m_iCurrPhoneme] / (sExp->elapeseTime * 0.3f);
					sExp->maxWeight = 1.0f;
					sExp->minWeight = 0.0f;
					sExp->midTime = sExp->endTime - sExp->elapeseTime;
					sExp->poseIndex = m_iPose2ShapeIndex[m_iCurrPhoneme];
					sExp->startTime = -sExp->elapeseTime;
					sExp->maxDuration = 0.0f;
					m_lLipAnimationList.push_back(sExp);

					m_fSpeechShapeWeights[m_iCurrPhoneme] = 0.0f;
					m_fSpeechShapeWeights[poseid] = speed_factor * addedTime;
					m_fSpeechShapeWeights[poseid] = CLAMP(m_fSpeechShapeWeights[poseid], 0.0f, 1.0f);
					m_pVPoseKeyFrame->updatePoseReference(poseid, m_fSpeechShapeWeights[poseid]);
					m_iCurrPhoneme = poseid;
				}
			}
			else
			{
				// this is silence. treat it same as change to new phoneme
				
				if (m_iCurrPhoneme != -1)
				{
					// once done
					// queue the last phoneme descrese expression into list
					sFExpression* sExp = new sFExpression;
					sExp->currWeight = m_fSpeechShapeWeights[m_iCurrPhoneme];
					sExp->elapeseTime = m_fSpeechShapeWeights[m_iCurrPhoneme] / speed_factor;
					sExp->endTime = sExp->elapeseTime * 1.2f;
					sExp->deltaWeight = -m_fSpeechShapeWeights[m_iCurrPhoneme] / (sExp->elapeseTime * 0.3f);
					sExp->maxWeight = 1.0f;
					sExp->minWeight = 0.0f;
					sExp->midTime = sExp->endTime - sExp->elapeseTime;
					sExp->poseIndex = m_iPose2ShapeIndex[m_iCurrPhoneme];
					sExp->startTime = -sExp->elapeseTime;
					sExp->maxDuration = 0.0f;
					m_lLipAnimationList.push_back(sExp);
					m_fSpeechShapeWeights[m_iCurrPhoneme] = 0.0f;

					m_iCurrPhoneme = -1;
				}

			}
		}
	}
	else if (m_tSpeechType == CSP_TTS_FFT)
	{
		HRESULT hr = S_OK;

		SPVOICESTATUS eventStatus;
		hr = m_pVoice->GetStatus( &eventStatus, NULL );
		if (hr != S_OK) {
			return;
		}
		
		if (!m_pSoundMonitor) {
			return;
		}

		if (eventStatus.dwRunningState == SPRS_DONE)
		{
			// exception: 2009.11.16 for composite speech strings
			if (!m_lSpeechQueue.empty())
			{
				string speech(m_lSpeechQueue.front().speech);
				tCHARACTER_SPEECH_TYPE tt = m_lSpeechQueue.front().type;
				m_lSpeechQueue.pop_front();
				speakContinue(speech, tt);

				return;
			}
			// end of exception

			m_bSpeaking = false;

			// reset all associated fftshapes
			this->resetFFTShape(true);

			return;
		}
		else
		{
			// look through fft shape list
			sFFTShape* sShape;
			sBands* sBand;
			iFFTShapeList::iterator it, itend;
			itend = m_lFFTShape.end();
			for (it = m_lFFTShape.begin(); it != itend;)
			{
				sShape = *it;
				
				// for each shape...
				if (sShape->type == 0)	// using sound level
				{
					// level only has single band (level)
					float fftweight = m_pSoundMonitor->getAudioLevel();
					sBand = *sShape->bands.begin();
					if (sBand)
					{
						fftweight *= sBand->factor;
						fftweight = CLAMP(fftweight - sBand->thresh, sBand->min, sBand->max);
						sBand->value += (fftweight - sBand->value) * (15.0f * addedTime);
						sBand->value = CLAMP(sBand->value, 0.0f, 1.0f);
						setLipShapeWeight(sShape->id, sBand->value);
						sShape->value = sBand->value;
					}
				}
				else					// using fft band
				{
					// band shape can be associated with multiple bands
					float fftweight = 0.0f;
					iFBandsList::iterator iit, iitend;
					iitend = sShape->bands.end();
					for (iit = sShape->bands.begin(); iit != iitend;)
					{
						sBand = *iit;
						float value = m_pSoundMonitor->getBand(sBand->id);
						value *= sBand->factor;
						value = CLAMP(value - sBand->thresh, sBand->min, sBand->max);
						sBand->value += (value - sBand->value) * (15.0f * addedTime);
						sBand->value = CLAMP(sBand->value, 0.0f, 1.0f);
						fftweight += sBand->value;

						iit++;
					}
					fftweight = fftweight / sShape->bands.size();
					setLipShapeWeight(sShape->id, fftweight);
					sShape->value = fftweight;
				}

				it++;
			}
			
			return;
		}
	}
	// fft recorded voice
	else if (m_tSpeechType == CSP_WAVE && m_pCurrFFTVoice != NULL)
	{
		// testing for waveform FFT LipSynch
		m_pCurrFFTVoice->update(addedTime);
		if (m_pCurrFFTVoice->isPlaying())
		{
			// look through fft shape list
			sFFTShape* sShape;
			sBands* sBand;
			iFFTShapeList::iterator it, itend;
			itend = m_lFFTShape.end();
			for (it = m_lFFTShape.begin(); it != itend;)
			{
				sShape = *it;
				
				// for each shape...
				if (sShape->type == 0)	// using sound level
				{
					// level only has single band (level)
					float fftweight = m_pCurrFFTVoice->getLevel();
					sBand = *sShape->bands.begin();
					if (sBand)
					{
						fftweight *= sBand->factor;
						fftweight = CLAMP(fftweight - sBand->thresh, sBand->min, sBand->max);
						sBand->value += (fftweight - sBand->value) * (15.0f * addedTime);
						sBand->value = CLAMP(sBand->value, 0.0f, 1.0f);
						setLipShapeWeight(sShape->id, sBand->value);
						sShape->value = sBand->value;
					}
				}
				else					// using fft band
				{
					// band shape can be associated with multiple bands
					float fftweight = 0.0f;
					iFBandsList::iterator iit, iitend;
					iitend = sShape->bands.end();
					for (iit = sShape->bands.begin(); iit != iitend;)
					{
						sBand = *iit;
						float value = m_pCurrFFTVoice->getBand(sBand->id);
						value *= sBand->factor;
						value = CLAMP(value - sBand->thresh, sBand->min, sBand->max);
						sBand->value += (value - sBand->value) * (15.0f * addedTime);
						sBand->value = CLAMP(sBand->value, 0.0f, 1.0f);
						fftweight += sBand->value;

						iit++;
					}
					fftweight = fftweight / sShape->bands.size();
					setLipShapeWeight(sShape->id, fftweight);
					sShape->value = fftweight;
				}

				it++;
			}


		}
		else
		{
			// exception: 2009.11.16 for composite speech strings
			if (!m_lSpeechQueue.empty())
			{
				string speech(m_lSpeechQueue.front().speech);
				tCHARACTER_SPEECH_TYPE tt = m_lSpeechQueue.front().type;
				m_lSpeechQueue.pop_front();
				speakContinue(speech, tt);

				return;
			}
			// end of exception

			m_bSpeaking = false;

			this->resetFFTShape(true);
			
			m_pCurrFFTVoice = NULL;

			m_fIdleActionTimer = 0.5f;

		}

		return;
	}
	// mic monitor lipsync: very similar to fft case
	else if (m_tSpeechType == CSP_MIC)
	{
		// check mic
		if (!m_pMicMonitor) {
			return;
		}
		else
		{
			// look through fft shape list
			sFFTShape* sShape;
			sBands* sBand;
			iFFTShapeList::iterator it, itend;
			itend = m_lFFTShape.end();
			for (it = m_lFFTShape.begin(); it != itend;)
			{
				sShape = *it;
				
				// for each shape...
				if (sShape->type == 0)	// using sound level
				{
					// level only has single band (level)
					float fftweight = m_pMicMonitor->getMicLevel();
					sBand = *sShape->bands.begin();
					if (sBand)
					{
						fftweight *= sBand->factor;
						fftweight = CLAMP(fftweight - sBand->thresh, sBand->min, sBand->max);
						sBand->value += (fftweight - sBand->value) * (15.0f * addedTime);
						sBand->value = CLAMP(sBand->value, 0.0f, 1.0f);
						setLipShapeWeight(sShape->id, sBand->value);
						sShape->value = sBand->value;
					}
				}
				else					// using fft band
				{
					// band shape can be associated with multiple bands
					float fftweight = 0.0f;
					iFBandsList::iterator iit, iitend;
					iitend = sShape->bands.end();
					for (iit = sShape->bands.begin(); iit != iitend;)
					{
						sBand = *iit;
						float value = m_pMicMonitor->getBand(sBand->id);
						value *= sBand->factor;
						value = CLAMP(value - sBand->thresh, sBand->min, sBand->max);
						sBand->value += (value - sBand->value) * (15.0f * addedTime);
						sBand->value = CLAMP(sBand->value, 0.0f, 1.0f);
						fftweight += sBand->value;

						iit++;
					}
					fftweight = fftweight / sShape->bands.size();
					setLipShapeWeight(sShape->id, fftweight);
					sShape->value = fftweight;
				}

				it++;
			}
			return;

		}
	}
#endif
}

#ifdef _WINDOWS
//-----------------------------------------------------------------------------
void LLCharacter::resetFFTShape(bool smooth)
{
	// reset all associated fftshapes
	sFFTShape* sShape;

	iFFTShapeList::iterator it, itend;
	itend = m_lFFTShape.end();
	for (it = m_lFFTShape.begin(); it != itend;)
	{
		sShape = *it;

		// better to end lip animation smoothly...
		if (smooth)
		{
			sFExpression* sExp = new sFExpression;
			sExp->currWeight = sShape->value;
			sExp->elapeseTime = 0.1f;
			sExp->endTime = 0.2f;
			sExp->deltaWeight = -sShape->value / 0.1f;
			sExp->maxWeight = 1.0f;
			sExp->minWeight = 0.0f;
			sExp->midTime = sExp->endTime - sExp->elapeseTime;
			sExp->poseIndex = sShape->id;
			sExp->startTime = -sExp->elapeseTime;
			sExp->maxDuration = 0.0f;
			m_lLipAnimationList.push_back(sExp);
		}
		
		// reset existing values
		sShape->value = 0.0f;
		sBands* sBand;
		iFBandsList::iterator iit, iitend;
		iitend = sShape->bands.end();
		for (iit = sShape->bands.begin(); iit != iitend;)
		{
			sBand = *iit;
			sBand->value = 0.0f;
			iit++;
		}
		
		it++;
	}
	return;
	
}

//-----------------------------------------------------------------------------
void __stdcall LLCharacter::sapiTTSCallBack(WPARAM wParam, LPARAM lParam)
{
	// SAPI event callback function
	LLCharacter* pThis = (LLCharacter*)lParam;
	pThis->processVoiceEvent();
}

//-----------------------------------------------------------------------------
void LLCharacter::processVoiceEvent()
{
	// only catch two kinds of event: word boundary & bookmark
	CSpEvent event;
	
	while( event.GetFrom(m_pVoice) == S_OK )
	{
		switch( event.eEventId )
		{
/*			case SPEI_PHONEME:
				if (event.Phoneme() == 7)
				{
					if (m_iCurrPhoneme != -1)
					{
						// once done
						// queue the last phoneme descrese expression into list
						sFExpression* sExp = new sFExpression;
						sExp->currWeight = m_fSpeechShapeWeights[m_iCurrPhoneme];
						sExp->elapeseTime = m_fSpeechShapeWeights[m_iCurrPhoneme] / 4.0f;
						sExp->endTime = sExp->elapeseTime * 1.2f;
						sExp->deltaWeight = -m_fSpeechShapeWeights[m_iCurrPhoneme] / (sExp->elapeseTime * 0.3f);
						sExp->maxWeight = 1.0f;
						sExp->midTime = sExp->endTime - sExp->elapeseTime;
						sExp->poseIndex = m_iPose2ShapeIndex[m_iCurrPhoneme];
						sExp->startTime = -sExp->elapeseTime;
						sExp->maxDuration = 0.0f;
						m_lFacialAnimationList.push_back(sExp);
						m_fSpeechShapeWeights[m_iCurrPhoneme] = 0.0f;

						m_iCurrPhoneme = -1;
					}
				}
				break;
*/			
			case SPEI_TTS_BOOKMARK:
				{
					// Get the string associated with the bookmark
					USES_CONVERSION;
					WCHAR *pwszEventString = new WCHAR[ wcslen( event.BookmarkName() ) + 1];
					if ( pwszEventString )
					{
						wcscpy( pwszEventString, event.BookmarkName() );
						char cstr[256];
						wcstombs(cstr, pwszEventString, wcslen(pwszEventString)+1);
						free( pwszEventString );

						LLScreenLog::getSingleton().addText("SAPI bookmark caught: " + Ogre::String(cstr));

						// bookmark format: <bookmark mark="[type][subtype][contents]"/>
						// type: whiteboard / action / etc
						// subtype: url, image ...
						// contents: xxxxx
						// i.e. whiteboard url <bookmark mark="[whiteboard][url][http://www.evl.uic.edu]"/>

						// tokenize bookmark
						char *p_token;
						char seps[] = "[]";
						char type[64];
						char mark[256];
						char contents[256];
						p_token = strtok(cstr, seps);
						strcpy(type, p_token);
						if (p_token != NULL && strcmp(type, "whiteboard") == 0)
						{
							LLScreenLog::getSingleton().addText("whiteboard bookmark caught");
							p_token = strtok(NULL, seps);
							if (p_token != NULL)
							{
								strcpy(mark, p_token);
								if (strcmp(mark, "URL") == 0)
								{
									p_token = strtok(NULL, seps);
									strcpy(contents, p_token);
									LLScreenLog::getSingleton().addText("\twhiteboard url: " + Ogre::String(contents));

									//g_pLLApp->getLLManager()->whiteboardNavigateTo(contents);
								}
							}
						}
						else if (p_token != NULL && strcmp(type, "ACTION") == 0)
						{
							LLScreenLog::getSingleton().addText("action bookmark caught");
							p_token = strtok(NULL, seps);
							if (p_token != NULL)
							{
								strcpy(mark, p_token);
							}

						}
						else if (p_token != NULL && strcmp(type, "ACTIVITY") == 0)
						{
							// msg callback request => [ACTIVITY][CB][CMD:para:para:]
							// possible CMD: NAV, MOV, ZOM
							LLScreenLog::getSingleton().addText("activity bookmark caught");
							p_token = strtok(NULL, seps);
							if (p_token != NULL)
							{
								strcpy(mark, p_token);
								if (strcmp(mark, "BRD") == 0)
								{
									p_token = strtok(NULL, seps);
									strcpy(contents, p_token);
									LLScreenLog::getSingleton().addText("\tbroadcast msg: " + Ogre::String(contents));
									
									if (m_pActivityMgr)
										m_pActivityMgr->broadcastMsg(contents);
									
								}
							}

						}
						else if (p_token != NULL && strcmp(type, "EXPRESSION") == 0)
						{
							// add expression for this bookmark event
							// "[type][subtype][contents]"
							// shape id, duration, max, mid, maxduration
							// [EXPRESSION][starttime][shapeid,duration,max,mid,maxduration]
							// <bookmark mark="[EXPRESSION][2.0][4 2.5 1.0 0.7 0.6]"/>
							p_token = strtok(NULL, seps);
							if (p_token)
							{
								float starttime = atof(p_token);
								p_token = strtok(NULL, seps);
								
								char paramstr[128], *_token;
								char seps2[] = " ";
								strcpy(paramstr, p_token);
								_token = strtok(paramstr, seps2);
								// parameters
								int shapeid = atoi(_token);
								float duration = atof(strtok(NULL, seps2));
								float max = atof(strtok(NULL, seps2));
								float mid = atof(strtok(NULL, seps2));
								float maxd = atof(strtok(NULL, seps2));

								addFacialExpression(shapeid, duration, max, mid, maxd, starttime);
							}
						}
						else if (p_token != NULL && strcmp(type, "MOV") == 0)
						{
							// add move action
							// "[type][subtype][contents]"
							// [MOV][starttime][x y z ori_degree]
							// <bookmark mark="[MOV][2.0][300 0 0 0]"/>
							p_token = strtok(NULL, seps);
							if (p_token)
							{
								float starttime = atof(p_token);
								p_token = strtok(NULL, seps);
								
								char paramstr[128], *_token;
								char seps2[] = " ";
								strcpy(paramstr, p_token);
								_token = strtok(paramstr, seps2);
								// parameters
								float x = atof(_token);
								float y = atof(strtok(NULL, seps2));
								float z = atof(strtok(NULL, seps2));
								float ori = atof(strtok(NULL, seps2));

								// enqueue move event to character's general event list
								LLEvent levent;
								levent.type = EVT_MOVE;
								levent.eTime = starttime;
								levent.args[0] = x;
								levent.args[1] = y;
								levent.args[2] = z;
								levent.args[3] = ori;
								m_lEventList.push_back(levent);
							}
						}
						else if (p_token != NULL && strcmp(type, "ATTRACTION") == 0)
						{
							// set attraction point
							// "[type][subtype][contents]"
							// [ATTRACTION][starttime][x y z expiretime]
							// <bookmark mark="[ATTRACTION][2.0][1 0 1 -1]"/>
							p_token = strtok(NULL, seps);
							if (p_token)
							{
								float starttime = atof(p_token);
								p_token = strtok(NULL, seps);
								
								char paramstr[128], *_token;
								char seps2[] = " ";
								strcpy(paramstr, p_token);
								_token = strtok(paramstr, seps2);
								// parameters
								float x = atof(_token);
								float y = atof(strtok(NULL, seps2));
								float z = atof(strtok(NULL, seps2));
								float expire = atof(strtok(NULL, seps2));

								// enqueue move event to character's general event list
								LLEvent levent;
								levent.type = EVT_ATTRACTION;
								levent.eTime = starttime;
								levent.args[0] = x;
								levent.args[1] = y;
								levent.args[2] = z;
								levent.args[3] = expire;
								m_lEventList.push_back(levent);
							}
						}
						else if (p_token != NULL && strcmp(type, "EMOTION") == 0)
						{
							// originally this is result of user's manual injection of emotion value
							// that is processed processSpeechAffect function
							// in the form of <emo id="happy" value="0.1"/>
							// then added as bookmark to sync with speech string
							// <bookmark mark="[EMOTION][int_id][float_intensity]"/>
							// immediate start (no delay necessary)
							p_token = strtok(NULL, seps);
							if (p_token)
							{
								int eID = atoi(p_token);
								p_token = strtok(NULL, seps);
								float eValue = atof(p_token);
								
								addEmotionalStimulus(eID, eValue, AFFECTLIFETIME*m_fEmotionDecayFactor);
							}

						}
						else if (p_token != NULL && strcmp(type, "NEGATION") == 0)
						{
							// some negation word accounted (this is analyzed by affectanalyzer in advance)
							// show some kind of head shake...
							// enqueue move event to character's general event list
							if (m_pHeadController)
							{
								//m_pHeadController->activateAnimation("head_shake_short");
								m_pHeadController->activateAnimation(NVB_HEADSHAKE_SHORT);
							}
						}

					}

					// do some work here to process bookmark event
					//addFacialExpression(E_ANGER, 0.7f);
				}
				break;

			case SPEI_SENTENCE_BOUNDARY:
				{
					// this event happens right before new sentence starts.
					// if there are multiple sentence in speech string, 
					// this event will occur multiple times accordingly.
				}
				break;
			case SPEI_WORD_BOUNDARY:
				{
					// this event happens right before word starts
					// information we can get is word position
					// word position is position of the first letter of the word in speech string
					// note that any sapi tag also will be counted too.
					// this makes it hard to determine real position of word if there is any embedded spai tag in the given speech string
					// may be easier to just use counter. word is determined only by whitespace
					int wordPos = event.InputWordPos();
					if (m_pNVBEngine)
						m_pNVBEngine->notifyWordDetection(m_iWordCounter);
					m_iWordCounter++;
				}
				break;

			default:
				break;
	
		}
	}

}

//-----------------------------------------------------------------------------
void LLCharacter::processListeningEvent(int gramId, int ruleId, char conf, char* listened, char* rulename)
{
	// if character is speaking, ignore this
	//if (m_bSpeaking)
	//	return;

	// this func called by Speech Recognition Engine
	// we got recognized listened string

	// gramId: FSM state id

	// ruleId: State transition (event id)
	// feed this to FSM & get the next state

	// conf: SR confidence

	// listened: recognized string
	if (m_pActivityMgr)
		m_pActivityMgr->processRecognizedEvent(gramId, ruleId, conf, listened, rulename);

	// stop user recording
	m_pMicMonitor->stopRecording();
}
#endif

//-----------------------------------------------------------------------------
int findposeindex(const String& poseName)
{
	//

	const char* name = poseName.c_str();
	if (strcmp(name, "Expression__Anger")==0)
		return E_ANGER;
	else if (strcmp(name, "Expression__Disgust")==0)
		return E_DISGUST;
	else if (strcmp(name, "Expression__Fear")==0)
		return E_FEAR;
	else if (strcmp(name, "Expression__Sad")==0)
		return E_SAD;
	else if (strcmp(name, "Expression__SmileClosed")==0)
		return E_SMILE;
	else if (strcmp(name, "Expression__SmileOpen")==0)
		return E_SMILEOPEN;
	else if (strcmp(name, "Expression__Surprise")==0)
		return E_SURPRISE;
	else if (strcmp(name, "Expression__Contempt")==0)
		return E_CONTEMPT;
	else if (strcmp(name, "Modifier__Blink_Left")==0)
		return M_BLINKLEFT;
	else if (strcmp(name, "Modifier__Blink_Right")==0)
		return M_BLINKRIGHT;
	else if (strcmp(name, "Modifier__BrowDown_Left")==0)
		return M_BROWDOWNLEFT;
	else if (strcmp(name, "Modifier__BrowDown_Right")==0)
		return M_BROWDOWNRIGHT;
	else if (strcmp(name, "Modifier__BrowIn_Left")==0)
		return M_BROWINLEFT;
	else if (strcmp(name, "Modifier__BrowIn_Right")==0)
		return M_BROWINRIGHT;
	else if (strcmp(name, "Modifier__BrowUp_Left")==0)
		return M_BROWUPLEFT;
	else if (strcmp(name, "Modifier__BrowUp_Right")==0)
		return M_BROWUPRIGHT;
	else if (strcmp(name, "Modifier__Ears_Out")==0)
		return M_EARSOUT;
	else if (strcmp(name, "Modifier__EyeSquint_Left")==0)
		return M_EYESQUINTLEFT;
	else if (strcmp(name, "Modifier__EyeSquint_Right")==0)
		return M_EYESQUINTRIGHT;
	else if (strcmp(name, "Modifier__LookDown")==0)
		return M_LOOKDOWN;
	else if (strcmp(name, "Modifier__LookLeft")==0)
		return M_LOOKLEFT;
	else if (strcmp(name, "Modifier__LookRight")==0)
		return M_LOOKRIGHT;
	else if (strcmp(name, "Modifier__LookUp")==0)
		return M_LOOKUP;
	else if (strcmp(name, "Phoneme__aah")==0)
		return P_AAH;
	else if (strcmp(name, "Phoneme__B_M_P")==0)
		return P_BMP;
	else if (strcmp(name, "Phoneme__big_aah")==0)
		return P_BIGAAH;
	else if (strcmp(name, "Phoneme__ch_J_sh")==0)
		return P_CHJSH;
	else if (strcmp(name, "Phoneme__D_S_T")==0)
		return P_DST;
	else if (strcmp(name, "Phoneme__ee")==0)
		return P_EE;
	else if (strcmp(name, "Phoneme__eh")==0)
		return P_EH;
	else if (strcmp(name, "Phoneme__F_V")==0)
		return P_FV;
	else if (strcmp(name, "Phoneme__i")==0)
		return P_I;
	else if (strcmp(name, "Phoneme__K")==0)
		return P_K;
	else if (strcmp(name, "Phoneme__N")==0)
		return P_N;
	else if (strcmp(name, "Phoneme__oh")==0)
		return P_OH;
	else if (strcmp(name, "Phoneme__ooh_Q")==0)
		return P_OOHQ;
	else if (strcmp(name, "Phoneme__R")==0)
		return P_R;
	else if (strcmp(name, "Phoneme__th")==0)
		return P_TH;
	else if (strcmp(name, "Phoneme__W")==0)
		return P_W;
	else if (strcmp(name, "xFACS__AU01_InnerBrowRaiser_")==0)		// strange this one has _ at the end
		return FACS_AU01;
	else if (strcmp(name, "xFACS__AU02_OuterBrowRaiser")==0)
		return FACS_AU02;
	else if (strcmp(name, "xFACS__AU04_BrowLowerer")==0)
		return FACS_AU04;
	else if (strcmp(name, "xFACS__AU05_UpperLidRaiser")==0)
		return FACS_AU05;
	else if (strcmp(name, "xFACS__AU06_CheeksRaiser")==0)
		return FACS_AU06;
	else if (strcmp(name, "xFACS__AU07_EyelidTightener")==0)
		return FACS_AU07;
	else if (strcmp(name, "xFACS__AU09_NoseWrinkler")==0)
		return FACS_AU09;
	else if (strcmp(name, "xFACS__AU10_UpperLipRaiser")==0)
		return FACS_AU10;
	else if (strcmp(name, "xFACS__AU12_LipCornerPuller")==0)
		return FACS_AU12;
	else if (strcmp(name, "xFACS__AU15_LipCornerDepressor")==0)
		return FACS_AU15;
	else if (strcmp(name, "xFACS__AU17_ChinRaiser")==0)
		return FACS_AU17;
	else if (strcmp(name, "xFACS__AU18_LipPuckerer")==0)
		return FACS_AU18;
	else if (strcmp(name, "xFACS__AU20_LipStretcher")==0)
		return FACS_AU20;
	else if (strcmp(name, "xFACS__AU23_LipTightener")==0)
		return FACS_AU23;
	else if (strcmp(name, "xFACS__AU24_LipPressor")==0)
		return FACS_AU24;
	else if (strcmp(name, "xFACS__AU25_LipsPart")==0)
		return FACS_AU25;
	else if (strcmp(name, "xFACS__AU28_LipSuck")==0)
		return FACS_AU28;
	else if (strcmp(name, "xFACS__AU43_EyesClosed")==0)
		return FACS_AU43;

	return -1;
}

//-----------------------------------------------------------------------------
void LLCharacter::preLoadMesh(const String& meshName)
{
	// Pre-load the mesh so that we can tweak it with a manual animation if it has Poses
	//MeshPtr mesh = MeshManager::getSingleton().load(meshName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    MeshPtr mesh = MeshManager::getSingleton().load(meshName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
													HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY, 
													HardwareBuffer::HBU_STATIC_WRITE_ONLY, 
													true, true);
    
    unsigned short src, dest;
    if (!mesh->suggestTangentVectorBuildParams(VES_TANGENT, src, dest))
    {
        mesh->buildTangentVectors(VES_TANGENT, src, dest);
    }

	// custom stuff for LLCharacter class
	if (mesh->getPoseCount () > 0)
    {
		unsigned short numAnim = mesh->getNumAnimations ();
        bool not_found = true;
		int manualindex = -1;
        if (numAnim != 0) 
        {
            for (unsigned short i = 0; i < numAnim; i++)
            {
                if ("LLmanual" == mesh->getAnimation (i)->getName ())
                {
                    not_found = false;
					manualindex = i;
                    break;
                }
            }
        }

		Animation*				pAnimation;
		VertexAnimationTrack*	pVAnimationTrack;

		if (not_found)
		{
			pAnimation = mesh->createAnimation("LLmanual", 0);
			pVAnimationTrack = pAnimation->createVertexTrack (mesh->getPose(0)->getTarget(), VAT_POSE);
			m_pVPoseKeyFrame = pVAnimationTrack->createVertexPoseKeyFrame(0);
		}
		else
		{
			pAnimation = mesh->getAnimation("LLmanual");
			pVAnimationTrack = pAnimation->getVertexTrack(mesh->getPose(0)->getTarget());
			m_pVPoseKeyFrame = pVAnimationTrack->getVertexPoseKeyFrame(0);
		}

        // create pose references, initially zero
        Mesh::PoseIterator it = mesh->getPoseIterator ();
        unsigned short i = 0;
		int idx = -1;
        while (it.hasMoreElements ())
        {
            Pose *p = it.getNext ();
			idx = findposeindex(p->getName());
			if (idx >= 0)
			{
				m_iShape2PoseIndex[idx] = i;
				m_iPose2ShapeIndex[i] = idx;
				m_pVPoseKeyFrame->addPoseReference(i, 0.0f);
			}
			i++;
			
        }
		
		m_iMaxPoseReference = i;
    }

	// if we have modified retarget mesh shape
	// then set it to max weight. we never change this again
	if (m_iShape2PoseIndex[S_RETARGET] != -1)
		m_pVPoseKeyFrame->addPoseReference(m_iShape2PoseIndex[S_RETARGET], 1.0f);
}

//-----------------------------------------------------------------------------
void LLCharacter::setScreenPointer(float x, float y, float depth)
{
	m_fScreenPointer[0] = x*2.0f;
	m_fScreenPointer[1] = y*2.0f;
	m_fScreenPointer[2] = depth;
	m_tEyeMode = CET_SCREEN;
	m_fBlendMaskDelta = -1.0f;
}

//-----------------------------------------------------------------------------
void LLCharacter::setUserPosition(float x, float y, float z)
{
	m_fUserPosition[0] = x;
	m_fUserPosition[1] = y;
	m_fUserPosition[2] = z;

	if (!m_bAttracted /*&& m_tAction != CAT_IDLE*/)
	{
		m_fBlendMaskDelta = -1.0f;
		m_bAttracted = true;
	}
	
	if (!m_bForcedAttraction) 
	{
		m_fAttractionExpire = 2.0f;		// expiration timer
		m_fAttractionFactor = 7.0f;
		m_tEyeMode = CET_USER;
	}
}

//-----------------------------------------------------------------------------
void LLCharacter::setAttractionPosition(float x, float y, float z, bool force, float speed, float expire)
{
	if (force)
	{
		m_fAttractionPosition[0] = x;
		m_fAttractionPosition[1] = y;
		m_fAttractionPosition[2] = z;

		m_bForcedAttraction = true;

		if (!m_bAttracted /*&& m_tAction != CAT_IDLE*/)
		{
			m_fBlendMaskDelta = -1.0f;
			m_bAttracted = true;
		}

		m_fAttractionExpire = expire;		// expiration timer
		m_fAttractionFactor = speed;
		m_tEyeMode = CET_MOVE;
	}
	else if (!m_bForcedAttraction)
	{
		m_fAttractionPosition[0] = x;
		m_fAttractionPosition[1] = y;
		m_fAttractionPosition[2] = z;

		if (!m_bAttracted /*&& m_tAction != CAT_IDLE*/)
		{
			m_fBlendMaskDelta = -1.0f;
			m_bAttracted = true;
		}

		m_fAttractionExpire = expire;		// expiration timer
		m_fAttractionFactor = speed;
		m_tEyeMode = CET_MOVE;

	}
	/*
	if (!m_bForcedAttraction && !force)
	{
		// this is passive blob attraction
		m_fAttractionPosition[0] = x;
		m_fAttractionPosition[1] = y;
		m_fAttractionPosition[2] = z;

		if (!m_bAttracted && m_tAction != CAT_IDLE)
		{
			m_fBlendMaskDelta = -1.0f;
			m_bAttracted = true;
			cout << "attracted by blob!\n";
		}
		m_fAttractionExpire = 2.0f;		// expiration timer
		m_tEyeMode = CET_MOVE;
		
		return;
	}

	m_fAttractionPosition[0] = x;
	m_fAttractionPosition[1] = y;
	m_fAttractionPosition[2] = z;

	if (force)
		m_bForcedAttraction = true;

	if (!m_bAttracted && m_tAction != CAT_IDLE)
	{
		m_fBlendMaskDelta = -1.0f;
		m_bAttracted = true;
		cout << "attracted by blob!\n";
	}
	m_fAttractionExpire = 2.0f;		// expiration timer
	m_tEyeMode = CET_MOVE;
*/

	// new way to interpret this attraction position in 3D space
	// find current direction (pitch, yaw)
	// compute new target direction (pitch, yaw)
	if (z==1.0f)
		int p = 1;

	if (!m_pHeadBone)
		return;

	// in fact, we need some kind of IK calculation
	
	Quaternion qd_derive = m_pHeadBone->_getDerivedOrientation();
	Quaternion qt_head = m_pHeadBone->getOrientation();
	m_fLastHeadPitch = qt_head.getPitch().valueDegrees();	// x-axis rotation
	m_fLastHeadYaw = qt_head.getYaw().valueDegrees();		// y-axis rotation

	// here need more precise calculation
	// especially, give character model has head orientation adjustment (i.e. facegen case. pitch ~17 degree)
	// should consider this in direction calculation
	// adjustment offset value is given in character xml file if manual head control is used (m_fHeadPitch)

	Quaternion qt_pitch;
	qt_pitch.FromAngleAxis(Degree(m_fHeadPitch), Vector3::UNIT_X);
	Vector3 dir0 = qt_pitch*Vector3(0,0,1);

	// parent
	Quaternion qt_parent;
	qt_parent = qd_derive * qt_head.Inverse();

	// head0
	Quaternion qt_head0 = qt_head * qt_pitch.Inverse();

	// current direction
	Quaternion qt_dir = qt_parent * qt_head0;
	Vector3 dir = qt_dir*Vector3(0,0,1);

	// target direction
	Quaternion qt;
	qt = dir.getRotationTo(Vector3(x,y,z));
	qt = qt_parent.Inverse() * qt;

	m_fTargetHeadPitch = qt.getPitch().valueDegrees() + m_fLastHeadPitch;
	m_fTargetHeadYaw = qt.getYaw().valueDegrees() + m_fLastHeadYaw;

	// last rotation
	LLScreenLog::getSingleton().addText(Ogre::StringConverter::toString(m_ID) + ": head orientation last: " 
															  + Ogre::StringConverter::toString(m_fLastHeadPitch) + ", "
															  + Ogre::StringConverter::toString(m_fLastHeadYaw));

	// rotation delta
	LLScreenLog::getSingleton().addText(Ogre::StringConverter::toString(m_ID) + ": head orientation by: " 
															  + Ogre::StringConverter::toString(qt.getPitch().valueDegrees()) + ", "
															  + Ogre::StringConverter::toString(qt.getYaw().valueDegrees()));

	// target orientation
	LLScreenLog::getSingleton().addText(Ogre::StringConverter::toString(m_ID) + ": head orientation to: " 
															  + Ogre::StringConverter::toString(m_fTargetHeadPitch) + ", "
															  + Ogre::StringConverter::toString(m_fTargetHeadYaw) + "\n");

}

/* 
// this is the old eye interaction function
//-----------------------------------------------------------------------------
void LLCharacter::updateEyeInteraction(Real addedTime)
{
	// reset head orientation
	if (!m_pHeadBone)
		return;
	
	// idle state?
	if (m_tAction == CAT_IDLE || m_fBlendMask == 1.0f)
	{
		m_pHeadBone->resetOrientation();
		return;
	}

	m_pHeadBone->resetOrientation();
	
	// current world transformation of head
	Quaternion headworldq = m_pHeadBone->_getDerivedOrientation();
	float cyaw = headworldq.getYaw().valueDegrees();
	float cpitch = headworldq.getPitch().valueDegrees();
	float croll = headworldq.getRoll().valueDegrees();

	// which data use?
	float x, y, d;
	if (m_tEyeMode == CET_SCREEN)	// Mouse Pointer
	{
		// tracking screen based interest point (mouse)
		x = m_fLastAttractionPoint[0];
		y = m_fLastAttractionPoint[1];
		d = m_fLastAttractionPoint[2];
		
		// delayed smoothing
		x += (m_fScreenPointer[0] - m_fLastAttractionPoint[0]) * 0.1f;
		y += (m_fScreenPointer[1] - m_fLastAttractionPoint[1]) * 0.1f;
		d += (m_fScreenPointer[2] - m_fLastAttractionPoint[2]) * 0.1f;
	
		x = (m_fScreenPointer[0]);
		y = (m_fScreenPointer[1]);
		d = (m_fScreenPointer[2]);

	}
	else if (m_tEyeMode == CET_USER)
	{
		// direct eye contact with user
		x = m_fLastAttractionPoint[0];
		y = m_fLastAttractionPoint[1];
		d = m_fLastAttractionPoint[2];
		
		// delayed smoothing
		x += (m_fUserPosition[0] - m_fLastAttractionPoint[0]) * (m_fAttractionFactor * addedTime);
		y += (m_fUserPosition[1] - m_fLastAttractionPoint[1]) * (m_fAttractionFactor * addedTime);
		
		// add some variation using the size of detected face size
		d += (m_fUserPosition[2] - m_fLastAttractionPoint[2]) * 0.1f;
		y += (d - m_fFaceTrackWidth) * 7.0f * (m_fAttractionFactor * addedTime);

#ifdef _WINDOWS
		// speaking? : use speaker output as the second effector
		if (m_bSpeaking && m_pSoundMonitor)
		{
			y+= ( (m_pSoundMonitor->getAudioLevelSmooth()-0.4f) - 0.3f ) * 0.4f * (m_fAttractionFactor * addedTime);
		}
		// listening? : 
#endif

	}
	else if (m_tEyeMode == CET_MOVE)
	{
		// detecting blob
		x = m_fLastAttractionPoint[0];
		y = m_fLastAttractionPoint[1];
		d = m_fLastAttractionPoint[2];
		
		// delayed smoothing
		x += (m_fAttractionPosition[0] - m_fLastAttractionPoint[0]) * (m_fAttractionFactor * addedTime);
		y += (m_fAttractionPosition[1] - m_fLastAttractionPoint[1]) * (m_fAttractionFactor * addedTime);

#ifdef _WINDOWS
		// speaking? : use speaker output as the second effector
		if (m_bSpeaking && m_pSoundMonitor)
		{
			y+= ( (m_pSoundMonitor->getAudioLevelSmooth()-0.4f) - 0.3f ) * 0.4f * (m_fAttractionFactor * addedTime);
		}
#endif

	}
	
	// update last point
	m_fLastAttractionPoint[0] = x;
	m_fLastAttractionPoint[1] = y;
	m_fLastAttractionPoint[2] = d;

	// apply offfset
	x += m_fHeadXOffset;
	y += m_fHeadYOffset;

	Real xrot,yrot;

	xrot = (x - 0.5f)*m_fHeadXRotRange;
	yrot = m_fHeadPitch + (y - 0.5f)*m_fHeadYRotRange;

	float ax = xrot;
	float ay = yrot - (cpitch-34.0f);

	m_pHeadBone->yaw(Degree(ax) * (1.0f - m_fBlendMask));		// y axis rotation
	m_pHeadBone->pitch(Degree(ay) * (1.0f - m_fBlendMask));		// x axis rotation

}
*/

//-----------------------------------------------------------------------------
void LLCharacter::updateEyeInteraction(Real addedTime)
{
	// previous implementation is mostly based-on screen space
	// however, for more general approach, it's better to use actural 3D space
	// in case of screen space attraction, do something in different way with some type variable

	// for example if we want to implement head orienting kind of stuff (look right/left kind)
	// what we need?
	// target direction (point), transition speed, expiration time (even not expired one too)
	// if that is non expiring one, who is responsible for back to normal orientation
	// application should know about this? or based on ACT change
	
	// no head bone assigned? just return then.
	if (!m_pHeadBone)
		return;
	
	// does current state matters? not sure about this yet
	if (/*m_tAction == CAT_IDLE || */m_fBlendMask == 1.0f)
	{
		m_pHeadBone->resetOrientation();
		return;
	}

	// reset orientation first
	m_pHeadBone->resetOrientation();
	
	// current world transformation of head
	Quaternion headworldq = m_pHeadBone->_getDerivedOrientation();
	float cyaw = headworldq.getYaw().valueDegrees();		// y-axis rotation
	float cpitch = headworldq.getPitch().valueDegrees();	// x-axis rotation
	float croll = headworldq.getRoll().valueDegrees();		// z-axis rotation. roll is not an issue in orienting head/eye

	// which data use? just focus on point in 3D space first
	float x, y, z;
	
	// next angle values (delayed smoothing)
	x = m_fLastHeadPitch + (m_fTargetHeadPitch - m_fLastHeadPitch) * (m_fAttractionFactor * addedTime);
	y = m_fLastHeadYaw + (m_fTargetHeadYaw - m_fLastHeadYaw) * (m_fAttractionFactor * addedTime);
	
	// update last point
	m_fLastHeadPitch = x;
	m_fLastHeadYaw = y;

	// apply offfset
	//x += m_fHeadXOffset;
	//y += m_fHeadYOffset;

	//m_pHeadBone->yaw(Degree(y) * (1.0f - m_fBlendMask));		// y axis rotation
	//m_pHeadBone->pitch(Degree(x) * (1.0f - m_fBlendMask));		// x axis rotation

	Matrix3 rMat;
	rMat.FromEulerAnglesYXZ(Degree(y* (1.0f - m_fBlendMask)), Degree(x* (1.0f - m_fBlendMask)), Radian(0));
	headworldq.FromRotationMatrix(rMat);
	m_pHeadBone->setOrientation(headworldq);

}

void LLCharacter::updateEyeCentering(Real addedTime)
{
	if (!m_pHeadBone)
	{
		//SkeletonInstance* skel = m_pEntity->getSkeleton();
		//m_pHeadBone = skel->getBone("Head_1");
		//if (!m_pHeadBone)
			return;

	}

	// when, not in idle, speak, listen, don't do centering
	if (m_tAction != CAT_IDLE && m_tAction != CAT_LISTEN && m_tAction != CAT_SPEAK)
		return;

	// current world transformation of head
	// rotaion values in degree
	Quaternion headworldq = m_pHeadBone->_getDerivedOrientation();
	float cyaw = headworldq.getYaw().valueDegrees();		// rotation around y-axis
	float cpitch = headworldq.getPitch().valueDegrees();	// rotation around x-axis
	float croll = headworldq.getRoll().valueDegrees();		// rotation around z-axis

	// cyaw, cpitch, croll
	float eh, ev;
	
	//cyaw += 5.0f;
	if (cyaw > 0.0)
		m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[M_LOOKRIGHT], CLAMP(cyaw*0.05, 0.0, 0.3));
	else
		m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[M_LOOKLEFT], CLAMP(-cyaw*0.05, 0.0, 0.3));
	
	cpitch -= 2.0f;
	if (cpitch > 0.0)
		m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[M_LOOKUP], CLAMP(cpitch*0.05, 0.0, 0.5));
	else
		m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[M_LOOKDOWN], CLAMP(-cpitch*0.05, 0.0, 0.5));
	
#ifdef DEBUG
	/*
	LLScreenLog::getSingleton().addText("Eye Centering (yaw,pitch,roll): " + Ogre::StringConverter::toString(cyaw) + ", "
															  + Ogre::StringConverter::toString(cpitch) + ", "
															  + Ogre::StringConverter::toString(croll));
	*/
#endif

}

//-----------------------------------------------------------------------------
void LLCharacter::setAnimationSpeed(float speed, bool relative)
{
	if (relative)
	{
		m_fAnimationSpeed += speed;
		m_fAnimationSpeed = CLAMP(m_fAnimationSpeed, 0.0f, 2.0f);
	}
	else
		m_fAnimationSpeed = speed;
}

//-----------------------------------------------------------------------------
void LLCharacter::setLipShapeWeight(int shape, float weight)
{
	m_fSpeechShapeWeights[shape] = weight;

	if (m_iShape2PoseIndex[shape] < m_iMaxPoseReference)
		m_pVPoseKeyFrame->updatePoseReference(m_iShape2PoseIndex[shape], m_fSpeechShapeWeights[shape]);
}

//-----------------------------------------------------------------------------
void LLCharacter::setAction(tCHARACTER_ACTION action, bool force)
{
	// for thesis testing: start speech from animation event CAT_SIMUL
	if (action == CAT_SIMUL)
	{
		m_pActivityMgr->broadcastMsg("KEY:Space");
		return;
	}

	// since there is idle relate expression... need to check them
	if (m_tAction == CAT_IDLE)
	{
		if (m_tAction != action)
			resetIdleExpression();
	}
	else if (action == CAT_IDLE)		// non idle to idle
	{
		m_fBlendMaskDelta = 0.5f;
		m_bAttracted = false;
		m_bForcedAttraction = false;
	}

	m_tAction = action;
	m_pAniManager->setAction(action, force);
}

//-----------------------------------------------------------------------------
void LLCharacter::setTipString(char* tip, float duration)
{
	g_pLLApp->setTipEntry(tip, duration);
}

//-----------------------------------------------------------------------------
void LLCharacter::setCaptionString(char* tip, float duration)
{
	g_pLLApp->setCaptionEntry(tip, duration);
}

//-----------------------------------------------------------------------------
void LLCharacter::showTipString(float duration)
{
	g_pLLApp->showTipEntry(duration);
}

//-----------------------------------------------------------------------------
bool LLCharacter::eventHandler(void *param)
{
	if (param==NULL)
		return false;

	// this function called by LLFFSound & LLAnimation object 
	// when it process associated event
	LLEvent* se = (LLEvent*)param;

	char *p_token;
	char seps[] = ":";
	char name[32];

	// invoke proper event here
	// what kind of event?
	switch(se->type)
	{
	case EVT_NONE:
		break;
	case EVT_SOUND:			// sound file id: play sound effect
		i_SoundEventCallback.execute(param);
		break;
	case EVT_SPEAK:			// TTS speaking
#ifdef _WINDOWS
		speak(se->param);	
#endif
		break;
	case EVT_FFTSPEAK:		// FFT lipsynch speaking
#ifdef _WINDOWS
		speak(se->param, CSP_WAVE);	

#endif
		break;
	case EVT_SHAPE:			// shape, duration, max weight, midpoint, maxduration: str (float, float, float)
		addFacialExpression(se->ids[1], se->args[0], se->args[1], se->args[2], se->args[3]);
		break;
	case EVT_ACTION:		// action name
		setAction(tCHARACTER_ACTION(se->ids[0]));
		break;
	case EVT_VISIBILITY:	// toggle visibility
		char *p_token;
		char name[32];
		p_token = strtok(se->param, seps);
		if (p_token == NULL)
			break;
		strcpy(name, p_token);
		p_token = strtok(NULL, seps);
		if (p_token == NULL)
			break;
		if ( atoi(p_token) == 0)
			setVisible(name, false);
		else
			setVisible(name, true);
		break;
	case EVT_ATTRACT:		// set attraction point
		setAttractionPosition(se->args[0], se->args[1], 1.0f, true, se->args[2]);
		printf("attraction event processed...\n");
		break;
	case EVT_CAPTION:
		// show caption string...
		setCaptionString(se->param, 0.0f);
		break;
	}
	
	// do not delete event param here since caller reuse it

	return true;
}

#ifdef _WINDOWS
//-----------------------------------------------------------------------------
void LLCharacter::startListening()
{
	if (m_pEar)
	{
		m_pEar->startListening();
		m_pMicMonitor->startRecording();
		g_pLLApp->setMicVisible(true);
	}
}

//-----------------------------------------------------------------------------
void LLCharacter::stopListening()
{
	if (m_pEar)
	{
		m_pEar->stopListening();
		m_pMicMonitor->stopRecording();
		g_pLLApp->setMicVisible(false);
	}
}

//-----------------------------------------------------------------------------
bool LLCharacter::isListening()
{
	if (m_pEar)
	{
		return m_pEar->isListening();
	}
	else
		return false;
}

#endif


//-----------------------------------------------------------------------------
void LLCharacter::moveTo(Vector3 pos, float ori)
{

	String trackname = "MoveTrack_" + getName();

	if (g_pLLApp->getSceneManager()->hasAnimation(trackname))
	{
		g_pLLApp->getSceneManager()->destroyAnimationState(trackname);
		g_pLLApp->getSceneManager()->destroyAnimation(trackname);
	}

	// current postion
	Vector3 curPos = getPosition();
	Vector3 dist = pos - curPos;
	
	// current orientation
	Quaternion curQt = getOrientation();
	Vector3 dir = curQt*Vector3(0,0,1);

	Quaternion mqt, dqt;
	
	// dest orientation
	dqt.FromAngleAxis(Degree(ori), Vector3::UNIT_Y);
	
	// only consider xz plane movement. no vertical move
	Vector3 dir0 = dir; dir0.y = 0;
	Vector3 dist0 = dist; dist0.y = 0;

	// moving orientation
	mqt = curQt * dir0.getRotationTo(dist0, Vector3::UNIT_Y);

	// three direction (angle) values
	float cOrientation = curQt.getYaw().valueDegrees();
	float mOrientation = mqt.getYaw().valueDegrees();
	float dOrientation = dqt.getYaw().valueDegrees();

	// time for rotation
	// change rotation time based on amount of rotation angle value
	// say... 1 sec for 180 degree turn
	float rotTime0 = abs(mOrientation - cOrientation) / 180.0f;
	float rotTime1 = abs(dOrientation - mOrientation) / 180.0f;

	// determine total animation track time
	// traslation speed is about 100cm/sec
	// if distance is pretty close, then do not use intermideate rotation
	float distance = dist.length();	// centimeter scale
	float anitime, speed;

	// some variation depend on distance to move
	// if it's short, then use slow walk animation
	if (distance < 150.0f)
	{
		// determin which animation for this short move
		// side walk (L/R), slow forward, slow backward
		// divide moving direction in 4 category based on current direction
		float movingAngle = mOrientation - cOrientation;	// range from -180 to 180

		if (movingAngle >= -45.0f && movingAngle < 45.0f)
			// forward
			setAction(CAT_WALKSLOW);
		else if (movingAngle >= 45.0f && movingAngle < 135.0f)
			// left
			setAction(CAT_WALKL);
		else if (movingAngle >= -135.0f && movingAngle < -45.0f)
			// right
			setAction(CAT_WALKR);
		else
			// backward
			setAction(CAT_WALKBACK);

		speed = m_pAniManager->getMovingSpeed();
		if (speed == 0.0f)
			anitime = distance * 0.01f;
		else
			anitime = distance / speed;
	}
	else
	{
		setAction(CAT_WALK);
		speed = m_pAniManager->getMovingSpeed();
		if (speed == 0.0f)
			anitime = distance * 0.0096f + rotTime0 + rotTime1;
		else
			anitime = distance / speed + rotTime0 + rotTime1;
	}

	Animation* anim = g_pLLApp->getSceneManager()->createAnimation(trackname, anitime);
	
	// create a track to animate the node
	NodeAnimationTrack* track = anim->createNodeTrack(0, m_pNode);

	// create keyframes for our track
	TransformKeyFrame* key;
	key = track->createNodeKeyFrame(0);
	key->setTranslate(curPos);
	key->setRotation(curQt);
	
	// if long distance moving, set intermediate moving direction
	if (distance >= 150.0f)
	{
		key = track->createNodeKeyFrame(rotTime0);
		key->setTranslate(curPos + dist*(rotTime0/anitime));
		key->setRotation(mqt);
	
		key = track->createNodeKeyFrame(anitime - rotTime1);
		key->setTranslate(curPos + dist*(anitime-rotTime1)/anitime);
		key->setRotation(mqt);
	}

	key = track->createNodeKeyFrame(anitime);
	key->setTranslate(pos);
	key->setRotation(dqt);

    // create a new animation state to track this
    m_pMoveAnimState = g_pLLApp->getSceneManager()->createAnimationState(trackname);
	m_pMoveAnimState->setLoop(false);
    m_pMoveAnimState->setEnabled(true);

}

//-----------------------------------------------------------------------------
float LLCharacter::addEmotionalStimulus(int emotion, float intensity, float lifetime)
{
	// this function is likely called by speech processor, interaction detector (user response)
	// or something else that may elicit avatar's emtoional change
	// then, question is how/when an avatar displays facial expression?
	// and how to use facial expression database with this?

	// say... speech processor possibly detect the main keyword elicit this emotion
	// then, maybe insert this facial expression synch with that word
	// which mean caller (speech evaluator) need to receive intensity information from this call
	// assume we do not chance the original emtoion. only change intensity

	// the reason we cannot directly enqueue this facial expression event here is
	// we don't know the synch point. it has to be embeded in TTS speech string as bookmark
	// to synchronize with the work or phrases
	if (m_pEmotion)
	{
		// emotion processing for PAD model
		m_pEmotion->processStimulus(emotion, intensity, lifetime);
	}

	
	// well, even without emotion engine, let's just do facial expression with given parameter
	if (emotion != -1)
	{
		// TODO: need to use FACS DB later
		int shapeid = m_iEmotion2Shape[emotion];
		
		// manual tweak: angry expression is too strong
		if (emotion == EMO_ANGRY)
			addFacialExpression(shapeid, 15.0 * intensity, intensity * 0.7f, 10.0 * intensity*0.1, 4.0 * intensity*0.2);
		else if (emotion == EMO_SADNESS)	
		{
			// sad is too weak. increase a bit
			intensity *= 1.3f;
			addFacialExpression(shapeid, 15.0 * intensity, intensity, 10.0 * intensity*0.1, 4.0 * intensity*0.2);
			intensity /= 1.3f;
		}
		else if (emotion == EMO_PAIN)
		{
			// this is not the basic category expression.
			// use pain related individual FACS to do so...
			// FACS: 19, 20, 52 &  54, 55, 56, (62, 65 => lip related. not to use for now)
			addFacialExpression(M_EYESQUINTLEFT, 15.0 * intensity, intensity*0.15, 10.0 * intensity*0.1, 4.0 * intensity*0.2);
			addFacialExpression(M_EYESQUINTRIGHT, 15.0 * intensity, intensity*0.15, 10.0 * intensity*0.1, 4.0 * intensity*0.2);
			addFacialExpression(FACS_AU04, 15.0 * intensity, intensity*0.05, 10.0 * intensity*0.1, 4.0 * intensity*0.2);

			addFacialExpression(FACS_AU06, 15.0 * intensity, intensity*0.5, 10.0 * intensity*0.1, 4.0 * intensity*0.2, intensity);
			addFacialExpression(FACS_AU07, 15.0 * intensity, intensity*0.05, 10.0 * intensity*0.1, 4.0 * intensity*0.2, intensity);
			addFacialExpression(FACS_AU09, 15.0 * intensity, intensity*0.25, 10.0 * intensity*0.1, 4.0 * intensity*0.2, intensity);

			addFacialExpression(FACS_AU20, 15.0 * intensity, intensity*0.15, 10.0 * intensity*0.1, 4.0 * intensity*0.2, intensity);
			addFacialExpression(FACS_AU25, 15.0 * intensity, intensity*0.1, 10.0 * intensity*0.1, 4.0 * intensity*0.2, intensity);

			//<AEvent type="EVT_SHAPE" time="6.0" param="19 2.08 0.6 1.08 0.17"/>
  			//<AEvent type="EVT_SHAPE" time="6.0" param="20 2.08 0.6 1.08 0.17"/>
  			//<AEvent type="EVT_SHAPE" time="6.0" param="52 2.08 0.2 1.08 0.17"/>

			//<AEvent type="EVT_SHAPE" time="6.0" param="54 3.29 1.0 1.08 0.17"/>
  			//<AEvent type="EVT_SHAPE" time="6.0" param="55 3.29 0.15 1.08 0.17"/>
  			//<AEvent type="EVT_SHAPE" time="6.0" param="56 3.29 0.9 1.08 0.17"/>
  			//<AEvent type="EVT_SHAPE" time="6.0" param="62 3.29 0.83 1.08 0.17"/>
  			//<AEvent type="EVT_SHAPE" time="6.0" param="65 3.29 0.4 1.08 0.17"/>
		}
		else
			// parameter tweak: refer to processSpeechAffect function
			addFacialExpression(shapeid, 15.0 * intensity, intensity, 10.0 * intensity*0.1, 4.0 * intensity*0.2);
	}
	// manual control: thesis testing purpose. make it much faster for the worst senario
	/*if (emotion != -1)
	{
		// TODO: need to use FACS DB later
		int shapeid = m_iEmotion2Shape[emotion];
		
		// manual tweak: angry expression is too strong
		if (emotion == EMO_ANGRY)
			addFacialExpression(shapeid, 5.0 * intensity, intensity * 0.7f, 3.0 * intensity*0.1, 1.0 * intensity*0.2);
		else if (emotion == EMO_SADNESS)	
		{
			// sad is too weak. increase a bit
			intensity *= 1.3f;
			addFacialExpression(shapeid, 5.0 * intensity, intensity, 3.0 * intensity*0.1, 1.0 * intensity*0.2);
			intensity /= 1.3f;
		}
		else if (emotion == EMO_PAIN)
		{
			// this is not the basic category expression.
			// use pain related individual FACS to do so...
			// FACS: 19, 20, 52 &  54, 55, 56, (62, 65 => lip related. not to use for now)
			addFacialExpression(M_EYESQUINTLEFT, 5.0 * intensity, intensity*0.15, 3.0 * intensity*0.1, 1.0 * intensity*0.2);
			addFacialExpression(M_EYESQUINTRIGHT, 5.0 * intensity, intensity*0.15, 3.0 * intensity*0.1, 1.0 * intensity*0.2);
			addFacialExpression(FACS_AU04, 5.0 * intensity, intensity*0.05, 3.0 * intensity*0.1, 1.0 * intensity*0.2);

			addFacialExpression(FACS_AU06, 5.0 * intensity, intensity*0.5, 3.0 * intensity*0.1, 1.0 * intensity*0.2, intensity);
			addFacialExpression(FACS_AU07, 5.0 * intensity, intensity*0.05, 3.0 * intensity*0.1, 1.0 * intensity*0.2, intensity);
			addFacialExpression(FACS_AU09, 5.0 * intensity, intensity*0.25, 3.0 * intensity*0.1, 1.0 * intensity*0.2, intensity);

			addFacialExpression(FACS_AU20, 5.0 * intensity, intensity*0.15, 3.0 * intensity*0.1, 1.0 * intensity*0.2, intensity);
			addFacialExpression(FACS_AU25, 5.0 * intensity, intensity*0.1, 3.0 * intensity*0.1, 1.0 * intensity*0.2, intensity);
		}
		else
			// parameter tweak: refer to processSpeechAffect function
			addFacialExpression(shapeid, 5.0 * intensity, intensity, 3.0 * intensity*0.1, 1.0 * intensity*0.2);
	}*/

	// return updated intensity of emotion to be used for facial expression

	return intensity;


	// Expressino from FACS DB
	/*		
	if (m_pFACSDB)
	{
		// 0:Angry, 1:Contempt, 2:Disgust, 3:Fear, 4:Happy, 5:Sadness, 6:Surprise
		vFACS facs = m_pFACSDB->sampleEmotion(affect.emotion);
		int shapeid,vsize;
		vsize = facs.size();
		for (int i=0; i<vsize; i++)
		{
			// covert FACS AU id to shape id
			shapeid = m_iFACS2Shape[facs[i]];

			// exception 1: AU26 (jaw drop)		=> AU25 (lip parts with small weight)
			if (facs[i] == FACS_AU26)
			{
				shapeid = m_iFACS2Shape[FACS_AU25];
				intensity *= 0.5f;
			}

			if (shapeid != -1)
			{
				// add expression
				tagStr = "<bookmark mark=\"[EXPRESSION]";
				sprintf(tchars, "[%f][%d %f %f %f %f]\"/>", starttime,
											shapeid, 
											duration, 
											intensity, 
											duration*0.3, 
											duration*0.2);
				tagStr.append(tchars);
				//speech.append(tagStr);
			}
			// exception 2: AU27 (mouth stretch)	=> AU25 (lip parts) + AU20 (lip stretcher)
			else if (facs[i] == FACS_AU27)
			{
				// add expression
				shapeid = m_iFACS2Shape[FACS_AU25];
				tagStr = "<bookmark mark=\"[EXPRESSION]";
				sprintf(tchars, "[%f][%d %f %f %f %f]\"/>", starttime,
											shapeid, 
											duration, 
											intensity, 
											duration*0.3, 
											duration*0.2);
				tagStr.append(tchars);
				//speech.append(tagStr);

				shapeid = m_iFACS2Shape[FACS_AU20];
				tagStr = "<bookmark mark=\"[EXPRESSION]";
				sprintf(tchars, "[%f][%d %f %f %f %f]\"/>", starttime,
											shapeid, 
											duration, 
											intensity, 
											duration*0.3, 
											duration*0.2);
				tagStr.append(tchars);
				//speech.append(tagStr);
			}
		}
	}
	*/

}

//-----------------------------------------------------------------------------
// when avatar receives new speech string
// feed it to affect analyzer to extract suface text affect
// and evaluate it in emotion processor: compute intensity
// then, embed facial expression tag in the original speech string
// assume that the speech string does not include any expression at the beginning
// it soley depends on emotion processing instead of manual injection
// do not use this one any more. use thread version
std::string LLCharacter::processSpeechAffect(std::string speech, int dialogAct)
{
	if (!m_pAffectAnalyzer)
		return speech;
	
	// speech affect storage struct
	sSpeechAffect affect;

	// reset word counter: used in NVBEngine
	m_iWordCounter = 0;

	// evaluate sentence affect
	// dialogAct is non by default. if character knows act, then use it.
	//	two possible way to set dialog act
	//		1. pass it as parameter in processSpeechInput method of Affect Analyzer
	//		2. act is embeded in speech string in the form of parenthesis
	//			at the beginning of sentence or within it
	//			YES (y), AGREE (a), NO (n), DISAGREE (r)
	//			if not set, then NONE by default
	affect = m_pAffectAnalyzer->processSpeechInput(speech, dialogAct);
	
	// generate non-verbal behavior (NVB) from analyzied affect
	if (m_pNVBEngine)
		m_pNVBEngine->processSpeechInput(affect);

	// experimental speed control via rate tag
	//		joy,anger,fear(panic) =>faster
	//		sad, disgust	=> slower
	// Speaking Tempo in Emotional Speech–a Cross-Cultural Study Using Dubbed Speech
	//	faster: joy and anger, panic fear [1]. 
	//	slow: sadness, disgust and anxiety [19].
	// another reference: Determinism in speech pitch relation to emotion
	// happy, fear => high
	// sad, anger, disgust => low
	int irate(0), ipitch(0);
	if (affect.emotion == EMO_ANGRY)
	{
		irate = affect.intensity * 3.0f;
		ipitch = -1;
	}
	else if (affect.emotion == EMO_CONTEMPT)
	{
		irate = affect.intensity * -3.0f;
		ipitch = affect.intensity * 0.0f;
	}
	else if (affect.emotion == EMO_DISGUST)
	{
		irate = affect.intensity * -3.0f;
		ipitch = -1;
	}
	else if (affect.emotion == EMO_HAPPY)
	{
		irate = affect.intensity * 3.0f;
		ipitch = 1;
	}
	else if (affect.emotion == EMO_SADNESS)
	{
		irate = affect.intensity * -3.0f;
		ipitch = -1;
	}

	char tchars[128];
	if (irate != 0)
	{
		sprintf(tchars, "<rate speed=\"%d\">", irate+m_iSpeechRate);
		string rateTag = tchars;
		rateTag.append(speech);
		rateTag.append("</rate><rate speed=\"0\"/>");
		speech = rateTag;
	}
	if (ipitch != 0)
	{
		sprintf(tchars, "<pitch middle=\"%d\">", ipitch);
		string rateTag = tchars;
		rateTag.append(speech);
		rateTag.append("</pitch><pitch middle=\"0\"/>");
		speech = rateTag;
	}

	sprintf(tchars, "Affect Speech Rate(%d), Pitch(%d)", irate, ipitch);
	LLScreenLog::getSingleton().addText(tchars); 

	return speech;
}

// thread version
// this is thread function
void LLCharacter::processSpeechAffectThread(std::string speech, int dialogAct)
{
	// speech affect storage struct
	sSpeechAffect affect;

	// reset word counter: used in NVBEngine
	m_iWordCounter = 0;

	// evaluate sentence affect
	// dialogAct is non by default. if character knows act, then use it.
	//	two possible way to set dialog act
	//		1. pass it as parameter in processSpeechInput method of Affect Analyzer
	//		2. act is embeded in speech string in the form of parenthesis
	//			at the beginning of sentence or within it
	//			YES (y), AGREE (a), NO (n), DISAGREE (r)
	//			if not set, then NONE by default
	affect = m_pAffectAnalyzer->processSpeechInput(speech, dialogAct);
	
	// generate non-verbal behavior (NVB) from analyzied affect
	if (m_pNVBEngine)
		m_pNVBEngine->processSpeechInput(affect);

	// experimental speed control via rate tag
	//		joy,anger,fear(panic) =>faster
	//		sad, disgust	=> slower
	// Speaking Tempo in Emotional Speech–a Cross-Cultural Study Using Dubbed Speech
	//	faster: joy and anger, panic fear [1]. 
	//	slow: sadness, disgust and anxiety [19].
	// another reference: Determinism in speech pitch relation to emotion
	// happy, fear => high
	// sad, anger, disgust => low
	int irate(0), ipitch(0);
	if (affect.emotion == EMO_ANGRY)
	{
		irate = affect.intensity * 3.0f;
		ipitch = -1;
	}
	else if (affect.emotion == EMO_CONTEMPT)
	{
		irate = affect.intensity * -3.0f;
		ipitch = affect.intensity * 0.0f;
	}
	else if (affect.emotion == EMO_DISGUST)
	{
		irate = affect.intensity * -3.0f;
		ipitch = -1;
	}
	else if (affect.emotion == EMO_HAPPY)
	{
		irate = affect.intensity * 3.0f;
		ipitch = 1;
	}
	else if (affect.emotion == EMO_SADNESS)
	{
		irate = affect.intensity * -3.0f;
		ipitch = -1;
	}

	/*
	if (m_bUseAffectBehavior)
	{
		char tchars[128];
		if (irate != 0)
		{
			sprintf(tchars, "<rate speed=\"%d\">", irate+m_iSpeechRate);
			string rateTag = tchars;
			rateTag.append(speech);
			rateTag.append("</rate><rate speed=\"0\"/>");
			speech = rateTag;
		}
		if (ipitch != 0)
		{
			sprintf(tchars, "<pitch middle=\"%d\">", ipitch);
			string rateTag = tchars;
			rateTag.append(speech);
			rateTag.append("</pitch><pitch middle=\"0\"/>");
			speech = rateTag;
		}

#ifdef _DEBUG
		sprintf(tchars, "Affect Speech Rate(%d), Pitch(%d)", irate, ipitch);
		LLScreenLog::getSingleton().addText(tchars);
#endif
	}*/

	// call speak function with speech
	speakAffect(speech);

}

//-----------------------------------------------------------------------------
void LLCharacter::setUseAffectBehavior(bool bUse)
{
	m_bUseAffectBehavior = bUse;

	if (m_pNVBEngine)
		m_pNVBEngine->setUseEmotionalNVB(bUse);
	if (m_pEmotion)
		m_pEmotion->setUseEmotionalExpression(bUse);
}

//-----------------------------------------------------------------------------
void LLCharacter::resetMood()
{
	// reset mood in emotion engine
	if (m_pEmotion)
		m_pEmotion->resetMood();

	// clean all active facial expressions

}

//-----------------------------------------------------------------------------
// not often used... restored from old version to send msg to python activity
void LLCharacter::processKeyInput(OIS::Keyboard* keyboard)
{
	if (!m_bInitialized || m_bSpeaking)
		return;

	// send keybaord event to python script
	// unfortunately keyboard is the status of all keys not a single key evnet!!!
	// hard to deal with unbuffered key event as we get call every frame without knowing what key event it is...
	// have to do the same if statement to pass keycode to python. duh
	if (m_pActivityMgr)
	{
		// which key pressed?
		for (int i=0; i<60; i++)
		{
			if (keyboard->isKeyDown(OIS::KeyCode(i)) && m_rTimeUntilNextToggle <= 0)
			{
				m_rTimeUntilNextToggle = 0.5f;
				string msg = "KEY:" + keyboard->getAsString(OIS::KeyCode(i));
				m_pActivityMgr->broadcastMsg((char*)msg.c_str());
				break;
			}
		}

	}

}
