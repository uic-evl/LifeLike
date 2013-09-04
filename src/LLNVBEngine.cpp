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
* Filename: LLNVBEngine.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLNVBEngine.h"
#include "LLCharacter.h"
#include "LLEmotionProcessor.h"
#include "LLHeadController.h"
#include "LLAffectAnalyzer.h"
#include "LLScreenLog.h"
#include "LLCRFTagger.h"

using namespace std;


bool compare_event(NVBSpeechEvent* first, NVBSpeechEvent* second)
{
	if (first->wordpos >= second->wordpos)
		return false;
	else
		return true;
}

LLNVBEngine::LLNVBEngine(LLCharacter* owner, LLHeadController* hcontrol)
{
	m_pOwner = owner;
	m_pHeadController = hcontrol;
	m_bUseEmotionalNVB = true;

	// CRF head nod tagger: need to load trained model for nod & shake
	m_pCRFTagger = NULL;

}

LLNVBEngine::~LLNVBEngine()
{

}

void LLNVBEngine::loadCRFTagger(const char* modelfile, int type)
{
	if (m_pCRFTagger == NULL)
		m_pCRFTagger = new LLCRFTagger();

	m_pCRFTagger->loadModel(modelfile, type);
}

void LLNVBEngine::loadCRFTagger(const char* modelfile, const char* type)
{
	if (m_pCRFTagger == NULL)
		m_pCRFTagger = new LLCRFTagger();

	// which type
	int itype = 0;
	if (strcmp(type, "CRFMODEL_NOD")==0)
		itype = CRFMODEL_NOD;
	else if (strcmp(type, "CRFMODEL_SHAKE")==0)
		itype = CRFMODEL_SHAKE;
	else
		return;

	m_pCRFTagger->loadModel(modelfile, itype);
}

void LLNVBEngine::processSpeechInput(fSpeechAffect& affect)
{
	// build internal speech related rule result
	// this will be uses later when NVB receives word detection notification
	// for head motion (nod,shake), it may need adjust word position so that it start a bit early

	int nWord = affect.token.size();

	// 2012.12.04
	// adding data-driven method too. use crf machine model trained with SEMAINE DB
	// how to fuse this result with rule-based parsing result?
	// need some priority model and conflict resolution mechanism
	char tchars[128];
	string dstr = "CRF label: ";
	if (m_pCRFTagger)
	{
		// tag result stored in token.crf_label: NOD / NA
		// only tag nodding at this point (shake is unreliable)
		m_pCRFTagger->tagSentence(affect.token);

#ifdef DEBUG
		// debug
		for (int i=0; i<nWord; i++)
		{
			sprintf(tchars, "%s (%d) ", affect.token[i].str.c_str(), affect.token[i].crf_label);
			dstr.append(tchars);
		}
		LLScreenLog::getSingleton().addText(dstr); 
#endif
	}

	
	// all these rule-based NVB generation is based on literature
	// Jina Lee's NVB rules (www-scf.usc.edu/~csci597/Slides/NVB%20.pdf)
	// Evelyn2000, Linguistic functions of head moveemnts in the context of speech
	// in fact lee changes two of function to the different behavior rule (uncentainty & intensification to nod instead of shake)
	/*
	[Rules from Evelyn]
		without speech
			nod -> negation
			shake -> affirmation
			listening nod -> backchannel

		with speech
			shake
				self correction
				uncertainty (I guess, I think ...) => Lee put this in head nod rule
					I guess, I think, I suppose, maybe, probably, perhaps, could
				negative expression (negation. no, not, nothing, cannot, none)
				superative or intensified expression (very, really) also lower brows on intensifying word
				(Lee found head nod and lower brows in video on intensifying word. she use this as head nod behavior)
					really, very, quite, great, absolutely, gorgeous
			sweep
				inclusivity (everyone, all, whole, several, plenty, full, ...
			nod
				affirmation
					yes, yeah, I do, we have, it's true, ok... with brow raise on phrase

	[Rules from Lee]
		shake
			self correction
			negation
		sweep
			inclusivity
		nod
			affirmattion (with brow raise)
			uncentainty
			intensification (with brow frown) => I guess this must be a signle nod motion
		
		~ there are more however these seem most fundamental

	*/

	// clear existing speech related event
	while(!m_lSpeechEvents.empty()) 
	{
		delete m_lSpeechEvents.front();
		m_lSpeechEvents.pop_front();
	}

	// scan given affect struct
	// need to re-align a bit
	// remove period and other non-word components
	// merge combined two words(i.e. I'm, don't ...)
	NVBSpeechEvent* evt;
	int begin(0), offset(0), wpos(0);
	int* lookup = new int[nWord];
	lookup[0] = 0;
	for (int i=1; i<nWord; i++)
	{
		begin = affect.sentence.find(affect.token[i].str, begin);
		if (affect.sentence[begin-1] != ' ')
			offset--;

		lookup[i] = i + offset;
	}            

	// pre-process rule-based behavior label
	// this fusion algorithm gives higher prority on rule system rather than crf
	// crf is more like supplemental to rule-based only when 
	// rule-based does not detect label for a given word
	// some issue
	//		"sure, why not"	: negation will overwrite affirmation
	//		"I really don't like it."	: negation will not set due to intensification
	//		=> first check affirmative and negation, then intensifier & inclusion (only if none set)
	int b,e;
	for (int i=0; i<nWord; i++)
	{
		switch (affect.token[i].func_type)
		{
			case WORDFN_NEGATION:		// SHAKE
				// mark three words as shake (-1, 0, 1)
				// high priority
				b = max(0, i-1);
				e = min(nWord, i+2);
				for (int k=b; k<e; k++)
				{
					// 
					if (affect.token[k].rule_label != CRFLABEL_NONE)
						break;

					affect.token[k].rule_label = CRFLABEL_SHAKE;
				}

				break;

			case WORDFN_AFFIRMATIVE:
				// mark three words as nod (0, 1, 2)
				// high priority
				b = max(0, i);
				e = min(nWord, i+3);
				for (int k=b; k<e; k++)
					affect.token[k].rule_label = CRFLABEL_NOD;
				
				break;

			default:
				// if label is not set? then use crf label
				if (affect.token[i].rule_label == CRFLABEL_NONE)
					affect.token[i].rule_label = affect.token[i].crf_label;
				
				break;
		}

	}
	for (int i=0; i<nWord; i++)
	{
		switch (affect.token[i].func_type)
		{
			case WORDFN_INTENSE:
				// mark two word as nod (0, 1)
				// medium priority
				b = max(0, i);
				e = min(nWord, i+2);
				for (int k=b; k<e; k++)
					if (affect.token[k].rule_label == CRFLABEL_NONE)
						affect.token[k].rule_label = CRFLABEL_NOD;

				break;

			case WORDFN_LISTING:
				// mark two word as nod (0, 1)
				// medium priority
				if (affect.token[i].rule_label == CRFLABEL_NONE)
					affect.token[i].rule_label = CRFLABEL_NOD;

				break;

			case WORDFN_INCLUSIVE:		// short SHAKE (sweep)
				// mark one word as shake (0)
				// this is weakest shake. if crf tagged this as NOD, use NOD
				if (affect.token[i].rule_label == CRFLABEL_NONE)
					affect.token[i].rule_label = CRFLABEL_SHAKE;

				break;
			
			default:
				break;
		}

	}

	// debug
#ifdef DEBUG
	dstr = "NVB fused: ";
	for (int i=0; i<nWord; i++)
	{
		sprintf(tchars, "%s (%d) ", affect.token[i].str.c_str(), affect.token[i].rule_label);
		dstr.append(tchars);
	}
	LLScreenLog::getSingleton().addText(dstr); 
#endif

	// post-filtering: fused behavior may has some abnormal tag or too sudden changes
	// let's use simple interpolation to filter those out.
	// only consider adjacent word label
	float *feval = new float[nWord];
	for (int i=0; i<nWord; i++)
	{
		feval[i] = 0.0f;

		if (i==0)
			feval[i] += affect.token[i].rule_label;
		else
			feval[i] += affect.token[i-1].rule_label;

		if (i == nWord-1)
			feval[i] += affect.token[i].rule_label;
		else
			feval[i] += affect.token[i+1].rule_label;

		feval[i] /= 2.0f;
	}
	// set label upon filtered value with adjacent label
	for (int i=0; i<nWord; i++)
	{
		// same direction: do nothing
		if ( (affect.token[i].rule_label * feval[i]) > 0.0f)
			;
		// opposite direction
		else if ( (affect.token[i].rule_label * feval[i]) < 0.0f)
			affect.token[i].rule_label = CRFLABEL_NONE;
		// zero
		else
		{
			// there are three cases: me(0), sum(0), me(0) & sum(0)
			// here what matters is me(0) and sum (1.0 or -1.0)
			if (affect.token[i].rule_label == CRFLABEL_NONE)
			{
				if (abs(feval[i]) == 1.0f)
					affect.token[i].rule_label = (int) feval[i];
			}
		}

	}
	delete feval;

	// debug
#ifdef _DEBUG
	dstr = "NVB filtered: ";
	for (int i=0; i<nWord; i++)
	{
		sprintf(tchars, "%s (%d) ", affect.token[i].str.c_str(), affect.token[i].rule_label);
		dstr.append(tchars);
	}
	LLScreenLog::getSingleton().addText(dstr); 
#endif

	// now iterate affect list to generate emotion, head nod/shake event
	//		two step iteration
	//		0. find all affect and equeue emotion event
	//		1. find nod/shake group, then queue NVB event for those
	for (int i=0; i<nWord; i++)
	{
		if (affect.token[i].func_type == WORDFN_AFFECT)
		{
			// if this word is main affect word in sentence
			// don't check it. assume affect analyzer already set func_type to none for unnecessary affect word
			// this was necessary for manual emotional annotation to show up no matter what
			//if (affect.token[i].emotion == affect.emotion && affect.token[i].intensity == affect.intensity)
			evt = new NVBSpeechEvent();
			evt->type = NVBEVT_SEMOTION;
			evt->wordpos = lookup[i];
			evt->values[0] = affect.token[i].emotion;								// affect id
			//evt->values[1] = CLAMP(affect.token[i].intensity*1.5f, 0.0f, 1.0f);		// affect intensity
			evt->values[1] = CLAMP(affect.token[i].intensity, 0.0f, 1.0f);		// affect intensity	(why *1.5??? remove it. 2013.7.6)
			evt->values[2] = AFFECTLIFETIME * m_pOwner->getEmotionFactor();			// default lifetime 300 sec

			// start it a bit early
			if (evt->wordpos>0)
				evt->wordpos--;

			m_lSpeechEvents.push_back(evt);
		}
	}
	for (int i=0; i<nWord; i++)
	{
		if (affect.token[i].rule_label == CRFLABEL_NOD)
		{
			// find all consequent nod labels
			//		check if there is intensifier within to increase intensity
			//		check if this is affirmative phrase from rule-based so that add facial expression too
			int k=i;
			bool affFound(false), intFound(false);
			int iAffirmative(-1), iIntensity(-1);		// fucntion word index
			float fAffirmative(0.0f), fIntensity(0.0f);		// function word intensity
			for (; k<nWord; )
			{
				// same label?
				if (affect.token[k].rule_label != CRFLABEL_NOD)
					break;

				// found affirmative? (affirmative intensity)
				if (affect.token[k].func_type == WORDFN_AFFIRMATIVE)
				{
					affFound = true;
					iAffirmative = k;
					
					// affirmative intensity affect intensity of nod & facial expression
					// intensity value range 0.333f ~ 1.0f => reduce to 0.0f ~ 0.5f
					fAffirmative = affect.token[k].intensity * 0.8f;
				}

				// found intensifier? (intensifier intensity)
				if (affect.token[k].func_type == WORDFN_INTENSE)
				{
					intFound = true;
					iIntensity = k;

					// intensifier intensity affect intensity of nod & facial expression
					// intensity value range 0.0f ~ 2.0f (1.0f is neutral. it's polarity)
					// so only consider distance from 1.0f and reduce intensity range from 0.0f to 0.5f
					//fIntensity = affect.token[k].intensity;
					fIntensity = CLAMP(abs(affect.token[k].intensity - 1.0f)*0.8f, 0.0f, 0.8f);

				}

				// listing item?
				if (affect.token[k].func_type == WORDFN_LISTING)
				{
					fIntensity = max(fIntensity, 0.5f);
				}

				k++;
			}

			// how many words in this nod instance = k - i
			// this decides how long this nod behavior should be
			// can we enforce nod animation to end at a word rank?
			// otherwise, use a few discrete variation in nod animation
			//	let's say three differetn types
			//		short one: 1~2 words 
			//		mid one: 3 ~ 4 words
			//		long: 4 ~ 6 words
			//		more than 6 words? then, add anohter nod event there
			int nodWords = k - i;

			// add head shake behavior here in case we support
			evt = new NVBSpeechEvent();
			evt->type = NVBEVT_SHEADNOD;
			evt->wordpos = lookup[i];

			// nod type?
			if (nodWords > 3)
				evt->values[0] = 3.0f;		// means long nod
			else if (nodWords > 2)
				evt->values[0] = 2.0f;		// means mid nod
			else
				evt->values[0] = 1.0f;		// means short nod

			// nod intensity? range should be from 0.2f to 1.0f;
			evt->values[1] = CLAMP( 0.2f + fAffirmative + fIntensity, 0.2f, 1.0f);

			// start it a bit early
			if (evt->wordpos>0)
				evt->wordpos--;

			m_lSpeechEvents.push_back(evt);

			// add expression raise brow if affirmative word found
			// not yet consider exact location of aff word in this case
			// just start expression at the beginning of nodding
			if (affFound)
			{
				wpos = lookup[iAffirmative];
				if (wpos > 0)
					wpos--;
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SEXPRESSION;
				evt->wordpos = wpos;
				evt->values[0] = FACS_AU01;					// expresison id : InnerBrowRaiser for affirmative
				evt->values[2] = fAffirmative*1.5f;			// expressison intensity
				evt->values[1] = evt->values[2]*3.0f;		// expression length
				evt->values[3] = evt->values[1]*0.3f;		// expression mid point

				m_lSpeechEvents.push_back(evt);
			}

			// intensifier expression
			if (intFound)
			{
				// add expression lower brow for intensification
				wpos = lookup[iIntensity];
				if (wpos > 0)
					wpos--;
				wpos = evt->wordpos;
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SEXPRESSION;
				evt->wordpos = wpos;
				evt->values[0] = FACS_AU04;					// expresison id : BrowLowerer for intensifier
				evt->values[2] = CLAMP((affect.token[iIntensity].intensity - 1.0f)*0.3f, 0.0f, 1.0f);	// expressison intensity
				evt->values[1] = evt->values[2]*3.0f;		// expression length
				evt->values[3] = evt->values[1]*0.3f;		// expression mid point

				m_lSpeechEvents.push_back(evt);
			}

			i = k;
		}
		else if (affect.token[i].rule_label == CRFLABEL_SHAKE)
		{
			// find all consequent shake labels
			int k=i;
			bool intFound(false), negFound(false);
			int iNegation(-1), iIntensity(-1);				// fucntion word index
			float fNegation(0.0f), fIntensity(0.0f);		// function word intensity
			for (; k<nWord; )
			{
				// same label?
				if (affect.token[k].rule_label != CRFLABEL_SHAKE)
					break;

				// found affirmative? (affirmative intensity)
				if (affect.token[k].func_type == WORDFN_NEGATION)
				{
					negFound = true;
					iNegation = k;
					
					// affirmative intensity affect intensity of nod & facial expression
					// in fact, negation does not have variable intensity (all 1.0f);
					fNegation = affect.token[k].intensity*0.3f;
				}

				// found intensifier?
				if (affect.token[k].func_type == WORDFN_INTENSE)
				{
					intFound = true;
					iIntensity = k;

					// intensifier intensity affect intensity of nod & facial expression
					// intensity value range 0.0f ~ 2.0f (1.0f is neutral. it's polarity)
					// so only consider distance from 1.0f and reduce intensity range from 0.0f to 0.5f
					//fIntensity = affect.token[k].intensity;
					fIntensity = CLAMP(abs(affect.token[k].intensity - 1.0f)*0.5f, 0.0f, 0.5f);
				}

				k++;
			}

			int shakeWords = k - i;

			// add head shake behavior here in case we support
			evt = new NVBSpeechEvent();
			evt->type = NVBEVT_SHEADSHAKE;
			evt->wordpos = lookup[i];
			
			// shake type?
			if (shakeWords > 3)
				evt->values[0] = 3.0f;		// means long shake
			else if (shakeWords > 2)
				evt->values[0] = 2.0f;		// means mid shake
			else
				evt->values[0] = 1.0f;		// means short shake

			// shake intensity?
			evt->values[1] = CLAMP(0.2f + fNegation + fIntensity, 0.2f, 1.0f);

			// start it a bit early
			if (evt->wordpos>0)
				evt->wordpos--;

			m_lSpeechEvents.push_back(evt);

			i = k;
		}
	}

	// sort all new event based on word position
	m_lSpeechEvents.sort(compare_event);

	// TODO: conflict resoltion in NVB events for the given sentence
	// priority: negation > affirmation > possibility > intensification/inclusion
	// for now, just blend each other. seems not much conflict happening in general
}

void LLNVBEngine::processSpeechInput0(fSpeechAffect& affect)
{
	// build internal speech related rule result
	// this will be uses later when NVB receives word detection notification
	// for head motion (nod,shake), it may need adjust word position so that it start a bit early

	// all these rule-based NVB generation is based on literature
	// Jina Lee's NVB rules (www-scf.usc.edu/~csci597/Slides/NVB%20.pdf)
	// Evelyn2000, Linguistic functions of head moveemnts in the context of speech
	// in fact lee changes two of function to the different behavior rule (uncentainty & intensification to nod instead of shake)
	/*
	[Rules from Evelyn]
		without speech
			nod -> negation
			shake -> affirmation
			listening nod -> backchannel

		with speech
			shake
				self correction
				uncertainty (I guess, I think ...) => Lee put this in head nod rule
					I guess, I think, I suppose, maybe, probably, perhaps, could
				negative expression (negation. no, not, nothing, cannot, none)
				superative or intensified expression (very, really) also lower brows on intensifying word
				(Lee found head nod and lower brows in video on intensifying word. she use this as head nod behavior)
					really, very, quite, great, absolutely, gorgeous
			sweep
				inclusivity (everyone, all, whole, several, plenty, full, ...
			nod
				affirmation
					yes, yeah, I do, we have, it's true, ok... with brow raise on phrase

	[Rules from Lee]
		shake
			self correction
			negation
		sweep
			inclusivity
		nod
			affirmattion (with brow raise)
			uncentainty
			intensification (with brow frown) => I guess this must be a signle nod motion
		
		~ there are more however these seem most fundamental

	*/

	// clear existing speech related event
	while(!m_lSpeechEvents.empty()) 
	{
		delete m_lSpeechEvents.front();
		m_lSpeechEvents.pop_front();
	}

	// scan given affect struct
	// need to re-align a bit
	// remove period and other non-word components
	// merge combined two words(i.e. I'm, don't ...)
	NVBSpeechEvent* evt;
	int begin(0), offset(0), wpos(0);
	int nWord = affect.token.size();
	int* lookup = new int[nWord];
	lookup[0] = 0;
	for (int i=1; i<nWord; i++)
	{
		begin = affect.sentence.find(affect.token[i].str, begin);
		if (affect.sentence[begin-1] != ' ')
			offset--;

		lookup[i] = i + offset;
	}

	// now iterate affect list to generate emotion, head nod/shake event
	// simply iterate all tokens in affect

	// resolve conflict between affirmative(nod) and negation(shake)
	// if affirmative presents, do not use shake for negation.
	bool affirmative = false;		
	for (int i=0; i<nWord; i++)
	{
		evt = NULL;
		switch (affect.token[i].func_type)
		{
			// affect word
			case WORDFN_AFFECT:
				// if this word is main affect word in sentence
				// don't check it. assume affect analyzer already set func_type to none for unnecessary affect word
				// this was necessary for manual emotional annotation to show up no matter what
				//if (affect.token[i].emotion == affect.emotion && affect.token[i].intensity == affect.intensity)
				{
					evt = new NVBSpeechEvent();
					evt->type = NVBEVT_SEMOTION;
					evt->wordpos = lookup[i];
					evt->values[0] = affect.token[i].emotion;							// affect id
					evt->values[1] = CLAMP(affect.token[i].intensity, 0.0f, 1.0f);		// affect intensity
					evt->values[2] = AFFECTLIFETIME * m_pOwner->getEmotionFactor();		// default lifetime 300 sec

					// start it a bit early
					if (evt->wordpos>0)
						evt->wordpos--;
				}
				break;
			case WORDFN_NEGATION:

				// already got affirmative phrase? then skip negation
				if (affirmative)
					break;

				// add head shake behavior here in case we support
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SHEADSHAKE;
				evt->wordpos = lookup[i];
				evt->values[0] = 1.0f;		// means short shake. when to use long nod?

				// start it a bit early
				if (evt->wordpos>0)
					evt->wordpos--;
				if (evt->wordpos>0)
					evt->wordpos--;

				break;
			case WORDFN_INTENSE:
				
				// add head nod behavior here in case we support
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SHEADNOD;
				evt->wordpos = lookup[i];
				evt->values[0] = 1.0f;		// means short nodding (or single nod)
				evt->values[1] = CLAMP((affect.token[i].intensity - 1.0f)*2.0f, 0.0f, 1.0f); // original value 0.0f ~ 2.0f (1.0f is neutral)

				// start it a bit early
				if (evt->wordpos>0)
					evt->wordpos--;
				if (evt->wordpos>0)
					evt->wordpos--;

				m_lSpeechEvents.push_back(evt);

				// add expression lower brow for intensification
				wpos = evt->wordpos;
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SEXPRESSION;
				evt->wordpos = wpos;
				evt->values[0] = FACS_AU04;	// expresison id : BrowLowerer
				evt->values[2] = CLAMP((affect.token[i].intensity - 1.0f)*0.5f, 0.0f, 1.0f);	// expressison intensity
				evt->values[1] = evt->values[2]*3.0f;	// expression length
				evt->values[3] = evt->values[1]*0.35f;	// expression mid point

				// if there is close following word has negative meaning... this should not be used
				// i.e. it is really impossible.

				break;
			case WORDFN_AFFIRMATIVE:
				// add head shake behavior here in case we support
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SHEADNOD;
				evt->wordpos = lookup[i];
				evt->values[0] = 2.0f;		// means long shake

				// start it a bit early
				if (evt->wordpos>0)
					evt->wordpos--;

				affirmative = true;

				m_lSpeechEvents.push_back(evt);

				// add expression raise brow
				wpos = evt->wordpos;
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SEXPRESSION;
				evt->wordpos = wpos;
				evt->values[0] = FACS_AU01;		// expresison id : InnerBrowRaiser
				evt->values[2] = affect.token[i].intensity;	// expressison intensity
				evt->values[1] = evt->values[2]*2.0f;	// expression length
				evt->values[3] = evt->values[1]*0.35f;	// expression mid point

				break;
			case WORDFN_INCLUSIVE:
				// add head sweep behavior here in case we support
				evt = new NVBSpeechEvent();
				evt->type = NVBEVT_SHEADSWEEP;
				evt->wordpos = lookup[i];

				// start it a bit early
				if (evt->wordpos>0)
					evt->wordpos--;

				break;

			default:
				break;
		}

		if (evt)
			m_lSpeechEvents.push_back(evt);
	}

	// sort all new event based on word position
	m_lSpeechEvents.sort(compare_event);

	// TODO: conflict resoltion in NVB events for the given sentence
	// priority: negation > affirmation > possibility > intensification/inclusion
}

void LLNVBEngine::notifyWordDetection(int wordpos)
{
	// this function called upon TTS word boundary event
	// to tell NVB to process word-level behavior if any presents

	// emotion (facial expression) only be used to visualize it
	// (its effect in emotion processor is already done after affect analyzer parsed speech input)

	// nothing to process?
	if (m_lSpeechEvents.empty())
		return;

	// TODOs: variation of head gesture
	// character mental status could affect the intensity of head gesture
	//		claim is very intuitive, however need to find some literature on this claim
	//		when avatar generate gestuer, refer to character's PAD value and modify intensity
	//		let's say...
	//		Pleasure:	high -> strong nod, low  -> strong shake
	//		Arousal:	high ->, low -> 
	//		Dominance:	high ->, low ->
	// reference
	//		Cowie10: relationship between energy of the gesture and the Arousal level

	// PAD arousal value from -1.0 to 1.0
	//	use it as multiplier for intensity of weight in animation (in conjunction with 
	//	given intensity from affect analyzer. factor from 0.5x to 2.0x
	//  in fact, arousal value is mostly 0 ~ 1.0 unless personality cause negative initial value
	float aEnergy = 0.0f;
	if (m_pOwner->getEmotionEngine() != NULL)
		aEnergy = m_pOwner->getEmotionEngine()->getMoodArousal();
	
	// negative energy => reduce intensity of gesture x(0.25 ~ 0.5)
	// positive energy => increase intensity of gesture x(0.5 ~ 2.0)
	if (aEnergy < 0.0f)	// 0.25 ~ 0.5
		//aEnergy = CLAMP((aEnergy * 0.5f + 1.0f), 0.25f, 0.5f);
		aEnergy = CLAMP((aEnergy * 0.25f + 0.5f), 0.25f, 0.5f);
	else				// 1.0 ~ 2.0
		//aEnergy = CLAMP((aEnergy + 1.0f), 1.0f, 2.0f);
		//aEnergy = CLAMP((aEnergy*1.25f + 0.75f), 0.75f, 2.0f);
		aEnergy = CLAMP((aEnergy*1.5f + 0.5f), 0.5f, 2.0f);		// more aggresive affect (0.5 ~ 2.0)
	
	NVBSpeechEvent* nvbE = m_lSpeechEvents.front();

	// compute weight value for gesture motion
	float weight = nvbE->values[1]*0.5f;	// control case: always use half intensity from motion data
	float weight0 = weight;
	if (m_bUseEmotionalNVB)
		weight = CLAMP(aEnergy*nvbE->values[1]*0.5f, 0.0f, 1.0f);	// emotional head gesture control: varying weight

	char tchars[128];
#ifdef DEBUG
	
	sprintf(tchars, "NVE weight: %f", weight);
	LLScreenLog::getSingleton().addText(tchars); 
#endif

	while (nvbE->wordpos == wordpos)
	{
		// process this event
		switch (nvbE->type)
		{
		case NVBEVT_SEMOTION:
			// 
			m_pOwner->addEmotionalStimulus(nvbE->values[0], nvbE->values[1], nvbE->values[2]);
			break;
		case NVBEVT_SEXPRESSION:
			// expression unrelated to affect: id, duration, max, midpoint, max_duration, starttime
			m_pOwner->addFacialExpression(nvbE->values[0], nvbE->values[1], nvbE->values[2], nvbE->values[3]);
			break;
		case NVBEVT_SHEADNOD:
			{
				//sprintf(tchars, "*** Head Gesture Energy: %f", aEnergy);
				//LLScreenLog::getSingleton().addText(tchars);

				// three different types: short, mid, long in terms of length of gesture
				if (nvbE->values[0] == 3.0f)
					m_pHeadController->activateAnimation(NVB_HEADNOD_LONG, weight, weight0);
				else if (nvbE->values[0] == 2.0f)
					m_pHeadController->activateAnimation(NVB_HEADNOD_MID, weight, weight0);
				else
					m_pHeadController->activateAnimation(NVB_HEADNOD_SHORT, weight, weight0);
			}
			break;
		case NVBEVT_SHEADSHAKE:
			{
				//sprintf(tchars, "*** Head Gesture Energy: %f", aEnergy);
				//LLScreenLog::getSingleton().addText(tchars);

				// three different types: short, mid, long in terms of length of gesture
				if (nvbE->values[0] == 3.0f)
					m_pHeadController->activateAnimation(NVB_HEADSHAKE_LONG, weight, weight0);
				else if (nvbE->values[0] == 2.0f)
					m_pHeadController->activateAnimation(NVB_HEADSHAKE_MID, weight, weight0);
				else
					m_pHeadController->activateAnimation(NVB_HEADSHAKE_SHORT, weight, weight0);
			}
			break;
		case NVBEVT_SHEADSWEEP:
			{
				// head sweep from left to right: this is very short shake so to speak
				// currently we do not use this one (2012.12.12)
				m_pHeadController->activateAnimation(NVB_HEADSWEEP);
			}
			break;
		default:
			break;
		}

		// clean up this event
		delete nvbE;
		m_lSpeechEvents.pop_front();

		// done?
		if (m_lSpeechEvents.empty())
			break;

		nvbE = m_lSpeechEvents.front();
	}
}