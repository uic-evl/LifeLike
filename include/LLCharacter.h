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
* Filename: LLCharacter.h
* -----------------------------------------------------------------------------
* Notes:    Character class. Character has shape and skeletal animation
* -----------------------------------------------------------------------------
*/

#ifndef __LLCHARACTER_H_
#define __LLCHARACTER_H_

#ifdef _WINDOWS
#include <sapi.h>
#endif

#include <Ogre.h>
#include "LLdefine.h"
#include "LLEntity.h"

#define OIS_DYNAMIC_LIB
#include <OIS/OIS.h>

using namespace Ogre;

// forward declaration of main app class
class LLAniManager;
class LLFFTSound;
class LLFACSDB;
class LLEmotionProcessor;
class LLAffectAnalyzer;
class LLHeadController;
class LLNVBEngine;

#ifdef _WINDOWS
class LLSpeechRecognizer;
class LLActivityManager;
class LLMicMonitor;
class LLAudioMonitor;
#endif

#include "LLCallBack.h"
#include "LLSoundManager.h"
#include "tinyxml2.h"

// this needs change to accomodate multiple entries for single speech sentence
// randomness of speech (assume we have multip wave for single speech)
typedef struct fSpeechPool
{
	int context;
	int lastused;						// remember the last one used;
	std::vector <LLFFTSound*> sVec;		// vector holding wave file object
	std::vector <int> pool;				// temp random pool
} sSpeechPool;
typedef std::vector <LLFFTSound*>	iFFTSound_Vec;
typedef std::vector <sSpeechPool*>	iFFTSoundPool_Vec;

// fft waveform band
typedef struct fBands
{
	int id;
	float factor;				// multiplier
	float thresh;				// cutoff threshold
	float min;					// min we
	float max;
	float value;
}	sBands;
typedef std::list <fBands*> iFBandsList;

// shape for fft
typedef struct fFFTShape
{
	int id;						// shape key
	int type;					// level / bands
	float value;				// averaged value
	iFBandsList	bands;			// values
} sFFTShape;
typedef std::list <fFFTShape*> iFFTShapeList;

// speech item: single speech string composed of multiple fo this
typedef struct fSpeechItem
{
	tCHARACTER_SPEECH_TYPE type;
	std::string speech;
} sSpeech;
typedef std::list <sSpeech> iSpeechList;

// for wrinkle region map
typedef struct fWrinkleRegion
{
	int id;
	float weight;
} sWRegion;
typedef std::vector<sWRegion> wrVector;
typedef std::map<int,wrVector> wrMap;
typedef std::map<int,wrVector>::iterator wrMapIter;


// ----------------------------------------------------------------------------
class LLCharacter: public LLEntity
{
private:
	LLCharacter();

public:

	// -------------------------------------------------------------------------
	// Constructors and Destructor
	// -------------------------------------------------------------------------
	LLCharacter(SceneNode* ParentNode);
	virtual ~LLCharacter(void);
	
	// -------------------------------------------------------------------------
	// Entity Creation
	// -------------------------------------------------------------------------
	Entity*			createEntity(SceneManager* mgr, const String& entityName, const String& meshName);
	bool			loadFromSpecFile(SceneManager* mgr, char* filename);

	// -------------------------------------------------------------------------
	// Setter methods
	// -------------------------------------------------------------------------
	void			setScreenPointer(float x, float y, float depth=1.0f);
	void			setUserPosition(float x, float y, float z=1.0f);
	void			setAttractionPosition(float x, float y, float z=1.0f, bool force=false, float speed = 5.0f, float expire = 2.0f);
	void			setAnimationSpeed(float speed, bool relative = false);

	// -------------------------------------------------------------------------
	// Update methods
	// -------------------------------------------------------------------------
	virtual void	update(Real addedTime);


#ifdef _WINDOWS
	// -------------------------------------------------------------------------
	// Speech methods
	// -------------------------------------------------------------------------
	void			setSoundMonitor(LLAudioMonitor* monitor) { m_pSoundMonitor = monitor;}
	void			setMicMonitor(LLMicMonitor* monitor) { m_pMicMonitor = monitor;}
	bool			speak(const String& speech, tCHARACTER_SPEECH_TYPE type = CSP_TTS);
	bool			speakAffect(const String& speech);
	bool			stopSpeak();
	bool			isSpeaking() { return m_bSpeaking;}
	void			startListening();
	void			stopListening();
	bool			isListening();
	LLFFTSound* 	addFFTSound(char* speech, char* filename);
	static void __stdcall sapiTTSCallBack(WPARAM wParam, LPARAM lParam);
	void			processListeningEvent(int gramId, int ruleId, char conf, char* listened, char* rulename);
	LLSpeechRecognizer* getSpeechRecognizer() { return this->m_pEar;}

#endif


	// -------------------------------------------------------------------------
	// Animation Handle method
	// -------------------------------------------------------------------------
	void			setAction(tCHARACTER_ACTION action, bool force=false);
	void			addFacialExpression(int iExpression, float duration, 
						float maxweight = 1.0f, float midpoint = -1.0f, float maxduration = 0.0f, float starttime=0.0f);

	// -------------------------------------------------------------------------
	// GUI method
	// -------------------------------------------------------------------------
	void			setTipString(char* tip, float duration=0.0f);
	void			showTipString(float duration=0.0f);
	void			setCaptionString(char* tip, float duration=0.0f);

	// -------------------------------------------------------------------------
	// Event method
	// -------------------------------------------------------------------------
	bool			eventHandler(void *param);	// animation and fft speech related
	void			addEvent(LLEvent evt) { m_lEventList.push_back(evt); }	// general purpose
	TCallback<LLSoundManager> i_SoundEventCallback;	// sound event callback interface

	// -------------------------------------------------------------------------
	// Process Keyboard Input methods
	// -------------------------------------------------------------------------
	void			processKeyInput(OIS::Keyboard* keyboard);
	
	// move character to destination
	void			moveTo(Vector3 pos, float ori=0.0f);

	LLActivityManager* getActivityManager() { return m_pActivityMgr;}

	// -------------------------------------------------------------------------
	// emotion related
	// -------------------------------------------------------------------------
	float			addEmotionalStimulus(int emotion, float intensity, float lifetime=1.0f);
	std::string		processSpeechAffect(std::string speech, int dialogAct=DACT_NON);
	void			processSpeechAffectThread(std::string speech, int dialogAct=DACT_NON);		// try thread
	LLEmotionProcessor* getEmotionEngine() { return m_pEmotion; }
	float			getEmotionFactor() { return m_fEmotionDecayFactor; }
	void			setUseAffectBehavior(bool bUse);
	void			resetMood();

protected:
	
	// -------------------------------------------------------------------------
	// protected Mesh load method
	// -------------------------------------------------------------------------
	virtual void	preLoadMesh(const String& meshName);

	void			updateSpeech(Real addedTime);
	void			updateEvent(Real addedTime);
	void			updateFacialAnimation(Real addedTime);
	void			updateEyeInteraction(Real addedTime);
	void			updateEyeCentering(Real addedTime);
	void			updateIdleExpression(Real addedTime);
	void			resetNeutralExpression();
	void			resetIdleExpression();

	// -------------------------------------------------------------------------
	// Activity method
	// -------------------------------------------------------------------------
	void			initActivity(const char* scriptdir, bool useSR = false);

	void			initAnimationManager(const char* filename);
	void			setLipShapeWeight(int shape, float weight);
	void			setHeadBone(const char* bonename);
	void			setEyeTrackMode(tCHARACTER_EYE_TRACK mode) { m_tEyeMode = mode;}
	void			addNeutralExpression(const char* shape, float weight);
	int				addIdleExpressionGroup(const char* name, float o0, float f0, float f1, float d0, float d1);
	void			addIdleExpression(int gid, const char* name, float w0, float w1, float offset);
	void			addFFTShape(int shapeid, int type, int band, float factor, float thresh, float min, float max);

	void			setClosing() { m_bSessionClosing = true;}

#ifdef _WINDOWS
	void			initVoice(const char* voice = "Microsoft Mike", long rate = 0, USHORT volume = 100);
	float			getFFTLevel() { return m_fFFTCurrLevel;}
	void			resetFFTShape(bool smooth = false);
	void			processVoiceEvent();
	void			setVoice(ISpVoice* voice) { m_pVoice = voice;}
	void			setVoice(char* voice);
#endif


	tCHARACTER_ACTION m_tAction;
	
	bool			m_bSpeaking;
	bool			m_bSessionClosing;
	
	tCHARACTER_SPEECH_TYPE m_tSpeechType;

#ifdef _WINDOWS
	ISpVoice *		m_pVoice;
	int				m_iSpeechRate;
#endif

	iFFTSound_Vec	m_vFFTVoiceVec;
	iFFTSoundPool_Vec m_vFFTPoolVec;
	LLMap_si		m_mFFTVoiceHash;
	LLFFTSound*		m_pCurrFFTVoice;
	float			m_fFFTCurrLevel;
	int				m_iViseme2PoseIndex[22];
	int				m_iCurrPhoneme;
	iSpeechList		m_lSpeechQueue;
	int				m_iSpeechContext;		// conversational or domain
	int				m_iSpeechGroup;			// picked random group
	
#ifdef _WINDOWS
	LLSpeechRecognizer*		m_pEar;
	LLActivityManager*		m_pActivityMgr;
#endif

	// -------------------------------------------------------------------------
	// Animation related variables
	// -------------------------------------------------------------------------
	LLAniManager*			m_pAniManager;
	VertexPoseKeyFrame*		m_pVPoseKeyFrame;
	int						m_iMaxPoseReference;
	bool					m_bInitialized;

	int						m_iShape2PoseIndex[SHAPENUMBER];	//
	int						m_iPose2ShapeIndex[SHAPENUMBER];	//
	int						m_iFACS2Shape[FACSNUMBER];			// FACS idx start from 1 not 0 (based on FACS DB)
	int						m_iEmotion2Shape[8];				// Emotion to matching shape id
	float					m_fSpeechShapeWeights[SHAPENUMBER];	// this is only for speech related shape
	float					m_fPoseWeights[SHAPENUMBER];		// this is for general shape (i.e. expression)
	
	sFBlinkExpression		m_BlinkAnim;
	bool					m_bBlinking;
	float					m_fAnimationSpeed;
	Bone*					m_pHeadBone;
	
	ShapeAnimationList		m_lFacialAnimationList;			// general facial animation queue
	ShapeAnimationList		m_lLipAnimationList;			// speech lipsync animation queue
	ShapeAnimationList		m_lNeutralExpressionList;		// list of neutral expression
	ShapeAnimationList		m_lIdleFacialAnimationList;		// idle facial animation queue
	ShapeAnimationList		m_lBlinkAnimationList;			// blink animation queue
	IdleExpressionVec		m_vIdleExpressionVec;			// storage of idle animation set
	
	float					m_fHeadPitch;

	float					m_fIdleActionTimer;
	
	iFFTShapeList			m_lFFTShape;

#ifdef _WINDOWS
	LLMicMonitor*			m_pMicMonitor;			// microphone monitor
	LLAudioMonitor*			m_pSoundMonitor;		// audio stereo mixer monitor
#endif

	// -------------------------------------------------------------------------
	// Eye Attraction (Head)
	// -------------------------------------------------------------------------
	float					m_fLastAttractionPoint[3];
	float					m_fScreenPointer[3];
	float					m_fUserPosition[3];
	float					m_fAttractionPosition[3];

	float					m_fHeadXRotRange;
	float					m_fHeadYRotRange;
	float					m_fHeadXOffset;
	float					m_fHeadYOffset;
	float					m_fFaceTrackWidth;
	tCHARACTER_EYE_TRACK	m_tEyeMode;
	float					m_fBlendMask;
	float					m_fBlendMaskDelta;
	bool					m_bAttracted;
	bool					m_bForcedAttraction;
	float					m_fAttractionExpire;
	float					m_fAttractionFactor;

	// for the continuing speech: it's only for internal invoke
	bool	speakContinue(const String& speech, tCHARACTER_SPEECH_TYPE type = CSP_TTS);

	// wrinkle map weight for expression: total 8 regions for now
	// each shape can have multiple wrinkle region assignment with weight value
	float					m_vWrinkleWeight[WRINKLENUMBER];
	wrMap					m_mWrinkleMap;
	wrMapIter				m_mWrinkleMapItr;
	GpuProgramParametersSharedPtr m_ptrWeightParams;
	GpuProgramParametersSharedPtr m_ptrHairShaderParams;

	// facial expression database (Cohn+ FACS DB)
	LLFACSDB*				m_pFACSDB;

	// FACS AU id to shape index map
	// shape id is not pose id in model data. it's the one defined in LLdefine.h
	// so, once we got FACS, need to use m_iShape2PoseIndex[shape_id] 
	// to get pose index for blendshape animation. Just same to use addFacialAnimation interface
	// in fact, we can do this same way as m_iShape2PoseIndex instead of using map structure
	// i.e. int m_iFACS2Shape[50];
	// this would be much faster as it's int array. changed to use this one.
	//facsMap					m_mFACS2Shape;

	// emotion
	LLEmotionProcessor*				m_pEmotion;
	LLAffectAnalyzer*				m_pAffectAnalyzer;
	float							m_fEmotionDecayFactor;

	// walk
	AnimationState*			m_pMoveAnimState;

	// character general event list with start time defined (eTime)
	// use for event encoded in sapi bookmark with time delay in general
	LLEventList				m_lEventList;

	// new way to orient head by attraction
	float					m_fLastHeadPitch;
	float					m_fLastHeadYaw;
	float					m_fTargetHeadPitch;
	float					m_fTargetHeadYaw;
	
	// tinyxml document
	tinyxml2::XMLDocument	m_xmlDoc;

	// new manual head controller
	LLHeadController*		m_pHeadController;

	// NVB Engine
	int						m_iWordCounter;
	LLNVBEngine*			m_pNVBEngine;

	// for keyboard event to prevent from processing too often
	Ogre::Real		m_rTimeUntilNextToggle;

	//
	bool			m_bUseAffectBehavior;

};

#endif
