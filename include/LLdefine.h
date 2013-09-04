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
* Filename: LLDefine.h
* -----------------------------------------------------------------------------
* Notes:    Useful Definition for LifeLike application
* -----------------------------------------------------------------------------
*/

#ifndef __LLDEFINE_H_
#define __LLDEFINE_H_

#include <list>
#include <vector>
#include <map>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define MAX(x, y) ((x) > (y)) ? (x) : (y)

#define SHAPENUMBER			70
#define FACSNUMBER			50
#define WRINKLENUMBER		8

// Expressions
#define S_RETARGET			0
#define E_ANGER				1
#define E_DISGUST			2
#define E_FEAR				3
#define E_SAD				4
#define E_SMILE				5
#define E_SMILEOPEN			6
#define E_SURPRISE			7
#define E_CONTEMPT			8

// Modifiers
#define M_BLINKLEFT			10
#define M_BLINKRIGHT		11
#define M_BROWDOWNLEFT		12
#define M_BROWDOWNRIGHT		13
#define M_BROWINLEFT		14
#define M_BROWINRIGHT		15
#define M_BROWUPLEFT		16
#define M_BROWUPRIGHT		17
#define M_EARSOUT			18
#define M_EYESQUINTLEFT		19
#define M_EYESQUINTRIGHT	20	
#define M_LOOKDOWN			21
#define M_LOOKLEFT			22
#define M_LOOKRIGHT			23
#define M_LOOKUP			24

// Phonemes
#define P_AAH				30		// ?
#define P_BMP				31		// b,m,p
#define P_BIGAAH			32		// a
#define P_CHJSH				33		// c
#define P_DST				34		// d
#define P_EE				35		// e
#define P_EH				36		// 
#define P_FV				37		// f,v
#define P_I					38		// i
#define P_K					39		// k
#define P_N					40		// n
#define P_OH				41		// o
#define P_OOHQ				42		// q
#define P_R					43		// r
#define P_TH				44		// t
#define P_W					45		// w

// FACS
#define FACS_AU01			50		// InnerBrowRaiser
#define FACS_AU02			51		// OuterBrowRaiser
#define FACS_AU04			52		// BrowLowerer
#define FACS_AU05			53		// UpperLidRaiser
#define FACS_AU06			54		// CheeksRaiser
#define FACS_AU07			55		// EyelidTightener
#define FACS_AU09			56		// NoseWrinkler
#define FACS_AU10			57		// UpperLipRaiser
#define FACS_AU12			58		// LipCornerPuller
									// 14 need: dimpler
#define FACS_AU15			59		// LipCornerDepressor
									// 16 need: lower lip depressor
#define FACS_AU17			60		// ChinRaiser
#define FACS_AU18			61		// LipPuckerer
#define FACS_AU20			62		// LipStretcher
#define FACS_AU23			63		// LipTightener
#define FACS_AU24			64		// LipPressor
#define FACS_AU25			65		// LipsPart
#define FACS_AU26			66		// 26 need: jaw drop
#define FACS_AU27			67		// 27 need: mouth stretch
#define FACS_AU28			68		// LipSuck
									// 38 need: nostril dilator
									// 39 need: nostril compressor
#define FACS_AU43			69		// EyesClosed

// Emotion for FACS DB
#define EMO_ANGRY			0
#define EMO_CONTEMPT		1
#define EMO_DISGUST			2
#define EMO_FEAR			3
#define EMO_HAPPY			4		// aroused happy (open smile)
#define EMO_SADNESS			5
#define EMO_SURPRISE		6
#define EMO_HAPPY2			7		// neutral happy (closed smile)
#define EMO_PAIN			8		// neutral happy (closed smile)

// screen viewport configuration
#define LL_SINGLEVIEW		1
#define LL_DUALVIEW			2
#define LL_TRIVIEW			3

// ELIZA bot communication (shared mapping file data set)
typedef struct MEMORYDATA{
	char data[256];
} sCHATMEMORY;

// struct for facial expression
// simple linear inperpolation with start, mid, end
typedef struct fExpression
{
	int		poseIndex;		// pose shpae index
	float	elapeseTime;	// elapsed time after stating of expression
	float	startTime;		// start time based on wall clock
	float	endTime;		// duration of expression
	float	maxWeight;		// peak weight of expression
	float	minWeight;		// start weight of expression
	float	deltaWeight;	// delta amount of changes
	float	currWeight;		// store current weight value
	float	midTime;		// midTime is the moment when reaches maxWeight
	float	maxDuration;	// in case expression stays in the max weight for this value
	float	offset;
}	sFExpression;
typedef std::list<sFExpression*> ShapeAnimationList;

typedef struct fIdleExpression
{
	std::vector<sFExpression*> vExpressions;
	int gid;
	float frequencyMin;
	float frequencyMax;
	float durationMin;
	float durationMax;

	float nextTime;
}	sFIdleExpression;
typedef std::vector<sFIdleExpression*> IdleExpressionVec;

typedef struct fBlinkExpression
{
	float minWeightL;		// mimimun opening
	float maxWeightL;		// maximun closing
	float minWeightR;		// mimimun opening
	float maxWeightR;		// maximun closing
	float frequencyMin;		// min period
	float frequencyMax;		// max period
	float durationMin;		// min blink speed
	float durationMax;		// max blink speed
	float speedrate;		// constant factor: use to apply emotional state?
	float frequencyrate;	// constant factor: use to apply emotional state?
} sFBlinkExpression;

// Event type
enum tEVENT
{
	EVT_NONE		= 0,	// no args								: str
	EVT_SOUND		= 1,	// sound file name						: str
	EVT_SPEAK		= 2,	// speech text							: str
	EVT_FFTSPEAK	= 3,	// wav filename							: str
	EVT_SHAPE		= 4,	// shape, duration, max weight, midpoint: str (float, float, float)
	EVT_ACTION		= 5,	// action name							: str
	EVT_VISIBILITY	= 6,	// mesh name:ON/OFF						: str
	EVT_ATTRACT		= 7,	// set attraction point: x, y
	EVT_CAPTION		= 8,	// set caption text
	EVT_MOVE		= 9,	// move avatar (mainly for sapi bookmark embeded) : x, y, z, orientation
	EVT_ATTRACTION	= 10,	// give attention to attractio point	: x,y,z,exptime
	EVT_VOICE		= 11,	// change voice
	EVT_GUI_VISIBILITY	= 12,	// change gui window visibility
	EVT_GUI_MATERIAL	= 13,	// change gui window material
	EVT_MAT_MESSAGE		= 14,	// message to material manager (for dynamic material related. i.e. movie control)
};

// Event structure
typedef struct sEvent
{
	float	eTime;				// 
	tEVENT	type;				// type of event
	char	param[4096];		// general purpose string
	int		ids[2];				// actionid, shapeid
	float	args[5];			// three float: duration, weight, midpoint, maxduration...
	bool	simUsed;			// for thesis testing
	sEvent()
	{
		eTime = 0.0f;
		type = EVT_NONE;
		ids[0] = ids[1] = -1;
		args[0] = args[3] = args[4] = 0.0f;
		args[1] = 1.0f;		// weight
		args[2] = -1.0f;	// midpoint
		simUsed = false;
		memset(param, 0, sizeof(char)*4096);
	}
} LLEvent;

typedef std::list<LLEvent> LLEventList;

// Character Actions
enum tCHARACTER_ACTION
{
	CAT_IDLE		= 0,
	CAT_LISTEN		= 1,
	CAT_SPEAK		= 2,
	CAT_READ		= 3,
	CAT_WRITE		= 4,
	CAT_DRINK		= 5,
	CAT_LOOKL		= 6,
	CAT_LOOKR		= 7,
	CAT_POINTL		= 8,
	CAT_POINTR		= 9,
	CAT_YES			= 10,
	CAT_NO			= 11,
	CAT_WALK		= 12,
	CAT_WALKSLOW	= 13,
	CAT_WALKBACK	= 14,
	CAT_WALKL		= 15,
	CAT_WALKR		= 16,
	CAT_RUN			= 17,
	CAT_TURN		= 18,	
	CAT_PAIN		= 19,	// for pain app testing: play predefined animation for pain app
	CAT_SMILE		= 20,	// for pain app
	CAT_SIMUL		= 21	// for thesis testing: use in animation event to start speech 
};

// Character Eye track mode
enum tCHARACTER_EYE_TRACK
{
	CET_SCREEN		= 0,		// point of interest on screen space
	CET_USER		= 1,		// track user position & rotation
	CET_MOVE		= 2,		// track blob detection
	CET_AUTO		= 3,		// some random behavior
	CET_NONE		= 4			// no move
};

// Speach type
enum tCHARACTER_SPEECH_TYPE
{
	CSP_TTS			= 0,		// synthesize voice and use SAPI phoneme events
	CSP_TTS_FFT		= 1,		// synthesize voice and monitor stereo mixer
	CSP_WAVE		= 2,		// playback recorded file and monitor waveform
	CSP_MIC			= 3,			// monitor mic: how to know when to stop speaking?
	CSP_TTS_LISTEN  = 4
};

#ifndef _WINDOWS
typedef unsigned long DWORD;

void correctSlash(char* path);

#endif


#define MAXSTR	2048


//-----------------------------------------------------------------------------
// some common std template
//-----------------------------------------------------------------------------
typedef std::map<unsigned long, std::string>	LLMap_ls;
typedef std::map<unsigned long, unsigned long>	LLMap_ll;
typedef std::map<std::string, std::string>		LLMap_ss;
typedef std::map<std::string, int>				LLMap_si;
typedef std::map<int, std::string>				LLMap_is;
typedef std::map<std::string, float>			LLMap_sf;
typedef std::string::iterator					LLItr_s;

//-----------------------------------------------------------------------------
// for stanford parser class
//-----------------------------------------------------------------------------

// NVB related function type (refer to Jina Lee's presentation 07)
#define WORDFN_NONE			0		// none
#define WORDFN_PERSON		1
#define WORDFN_AFFECT		2		// emotional facial expression
#define WORDFN_NEGATION		3		// head shake on phrase
#define WORDFN_INTENSE		4		// head nod, brow frown on word
#define WORDFN_AFFIRMATIVE	5		// head nod, brow raise on phrase
#define WORDFN_ASSUME		6		// head nods on phrase
#define WORDFN_CONTRAST		7		// head mvoed to side and brow raise
#define WORDFN_INTERJECTION	8		// head node on word
#define WORDFN_RESREQUEST	9		// head move to side and brow raise on word
#define WORDFN_LISTING		10		// head moved to one side and to the other on word
#define WORDFN_INCLUSIVE	11		// latral head sweep, brwo flash on word
#define WORDFN_OBLIGATION	12		// head nod once on phrase

// person pronoun type
#define PERSON_FIRST		1
#define PERSON_SECOND		2
#define PERSON_THIRD		3

// behavior machine model type
#define CRFMODEL_NOD		0		// CRF model id for Nodding
#define CRFMODEL_SHAKE		1		// CRF model id for Shaking
#define CRFLABEL_NONE		0		// CRF label for none
#define CRFLABEL_NOD		1		// CRF label for nod
#define CRFLABEL_SHAKE		-1		// CRF lable for shake

// dialog act type
#define DACT_NON			0		// none
#define DACT_YES			1		// yes answer
#define DACT_NO				2		// no answer
#define DACT_AGR			3		// agree, affirmative answer without yes
#define DACT_NAGR			4		// disagree, negative answer without no

// Token for Penn Tree POS Tag: Word in general
typedef struct tWord
{
	std::string str;	// word string
	std::string POS;	// penn tree POS tag string
	std::string phrase;	// penn tree phrase tag

	int			act;	// dialog act related. not all words in sentence need to have this set. let's assume the first word has act value set

	int			loc;	// nth word in sentence

	int		emotion;	// emotion id
	float	intensity;	// emotion intensity
	int		wnPOS;		// wordnet affect pos (noun1, verb2, adj3, adv4)
	int		func_type;	// functional type
	int		person;		// 1,2,3

	std::vector<std::string> crf_feature;	// store [affirmative intensity inclusive] string for CRF machine
	int		crf_label;	// CRF NOD tagging result
	int		rule_label;	// rule-based NOD/SHAKE label

	tWord() : str(""), POS(""), phrase(""), act(DACT_NON), loc(-1), emotion(-1), intensity(0.0f), wnPOS(0), func_type(WORDFN_NONE), person(0), crf_label(0), rule_label(0) {}
	tWord(std::string s, std::string p) : str(s), POS(p), phrase(""), act(DACT_NON), loc(-1), emotion(-1), intensity(0.0f), wnPOS(0), func_type(WORDFN_NONE), person(0), crf_label(0), rule_label(0) {}
} POSToken;

// Tree node for Sentence Parsing
struct tNode
{
	std::vector<tNode> child;

	std::string label;		// ROOT S NP VP...
	std::string phrase;		// combined string of child words
	tWord word;

	tNode() {}
};


//-----------------------------------------------------------------------------
// for emotion engine
//-----------------------------------------------------------------------------
typedef struct fPADCategory
{
	float p,a,d;	// center of category location
	float radius;	// boundary radius (sphere model)
	bool dspand;	// when d value is in range (i.e. +/-100)
} sPADCategory;

typedef struct fPAD
{
	int emotion;	// stimulus id
	float p;		// pleasure		-1.0 ~ 1.0
	float a;		// arousal		-1.0 ~ 1.0
	float d;		// dominance	-1.0 ~ 1.0
	float intensity;
	float lifetime;
	float elapsed;
	
	float length();
	void normalise();
}	sPAD;
typedef std::list <sPAD*> iFPADList;
typedef std::list<sPAD*>::iterator iPADIter;
typedef std::list<sPAD*>::reverse_iterator iPADRIter;


//-----------------------------------------------------------------------------
// for affect analyzer
//-----------------------------------------------------------------------------
#define AFFECTLIFETIME 60.0f
typedef struct fSpeechAffect
{
	int					emotion;		// evaluated emotion
	float				intensity;		// its intensity value
	float				lifetime;		// its lifetime

	std::string			sentence;		// clean speech sentence (no tag): a bit redundent
	std::vector<POSToken> token;		// parsed & evaluated word tokens

	fSpeechAffect() : emotion(-1), intensity(0.0f), lifetime(AFFECTLIFETIME) {}
} sSpeechAffect;


extern char *LLDictionaryFile;	// LifeLike custom dictional file path


//-----------------------------------------------------------------------------
// NVB type
//-----------------------------------------------------------------------------
#define NVB_NONE			0		// 
#define NVB_HEADSHAKE_SHORT	1		// 
#define NVB_HEADSHAKE_MID	2		// 
#define NVB_HEADSHAKE_LONG	3		// 
#define NVB_HEADNOD_SHORT	4		// short(single) nod
#define NVB_HEADNOD_MID		5		// short(single) nod
#define NVB_HEADNOD_LONG	6		// long (multiple) nod
#define NVB_HEADSWEEP		7		// single motion from left to right

#endif
