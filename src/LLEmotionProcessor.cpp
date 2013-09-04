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
* Filename: LLEmotionProcessor.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLEmotionProcessor.h"
#include "LLScreenLog.h"

#define PADWIDTH 512
#define PADHEIGHT 512
#define MONITORCYCLE 25.0

float fPAD::length()
{
	return sqrt(p*p + a*a + d*d);
}

void fPAD::normalise()
{
	float length = this->length();
	if (length != 0.0f)
	{
		p /= length;
		a /= length;
		d /= length;
	}
}
//-----------------------------------------------------------------------------
LLEmotionProcessor::LLEmotionProcessor()
{
	// initial Mood
	m_sPAD.p = 0.0f;
	m_sPAD.a = 0.0f;
	m_sPAD.d = 0.0f;

	// Personality
	m_sPersonality.p = 0.0f;
	m_sPersonality.a = 0.0f;
	m_sPersonality.d = 0.0f;

	// initial mood: same as personality
	// this initialization is for blending with targetMood
	m_sMood = m_sPersonality;

	// evaluatino co-efficient
	m_fPrimaryFactor = 0.5f;
	m_fSecondaryFactor = 0.25f;

	m_iEmotion = -1;
	memset(m_fEmotions, 0, sizeof(float)*8);

	//-------------------------------------------------------------------------
	// Emotion PAD value from Zhang 2010 (Table8 Original PAD of emotion words)
	//-------------------------------------------------------------------------
	// Angry			(-0.40 0.22 0.12)
	// Contempt			missing
	// Disgust			(-0.36 0.08 0.13)
	// Fear (Scared)	(-0.19 0.26 -0.13)
	// Happy			(0.55 0.24 0.28)
	// Sad				(-0.18 0.03 -0.14)
	// Surprise			(0.34 0.34 0.04)

	//-------------------------------------------------------------------------
	// TODO: this needs some adjustments
	//-------------------------------------------------------------------------
	// Emotion center coordinate
	// angry(-80,80,100)
	m_sEmotion[0].p = -0.8f;
	m_sEmotion[0].a = 0.8f;
	m_sEmotion[0].d = 1.0f;
	m_sEmotion[0].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[0].dspand = false;

	// comtempt	missing
	m_sEmotion[1].p = 0.0f;
	m_sEmotion[1].a = 0.0f;
	m_sEmotion[1].d = 0.0f;
	m_sEmotion[1].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[1].dspand = false;

	// disgust	missing: this is just my own
	m_sEmotion[2].p = -0.4f;
	m_sEmotion[2].a = 0.1f;
	m_sEmotion[2].d = 0.1f;
	m_sEmotion[2].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[2].dspand = false;

	// fear(-80,80,-100)
	m_sEmotion[3].p = -0.8f;
	m_sEmotion[3].a = 0.8f;
	m_sEmotion[3].d = -1.0f;
	m_sEmotion[3].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[3].dspand = false;

	// happy1(80,80,+/-100) (50,0,+/-100)
	m_sEmotion[4].p = 0.8f;
	m_sEmotion[4].a = 0.8f;
	//m_sEmotion[4].d = 0.0f;
	m_sEmotion[4].d = 0.2f;
	m_sEmotion[4].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[4].dspand = true;

	// sad(-50,0,+/-100)
	m_sEmotion[5].p = -0.5f;
	//m_sEmotion[5].a = 0.0f;
	m_sEmotion[5].a = -0.2f;
	//m_sEmotion[5].d = 0.0f;
	m_sEmotion[5].d = -0.3f;
	m_sEmotion[5].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[5].dspand = true;

	// surprise(10,80,+/-100)
	//m_sEmotion[6].p = 0.1f;
	m_sEmotion[6].p = 0.2f;
	m_sEmotion[6].a = 0.8f;
	//m_sEmotion[6].d = 0.0f;
	m_sEmotion[6].d = 0.1f;
	m_sEmotion[6].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[6].dspand = true;

	// happy2(50,0,+/-100)
	m_sEmotion[7].p = 0.5f;
	m_sEmotion[7].a = 0.0f;
	m_sEmotion[7].d = 0.0f;
	m_sEmotion[7].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[7].dspand = true;

	// pain(-80,30,50), this is guess work
	m_sEmotion[8].p = -0.8f;
	m_sEmotion[8].a = 0.3f;
	m_sEmotion[8].d = 0.5f;
	m_sEmotion[8].radius = 0.5f;	// categorical boundary radius
	m_sEmotion[8].dspand = true;

	// emotion name to id mapping utility
	m_mEmotionMap["ANGRY"] = 0;
	m_mEmotionMap["CONTEMPT"] = 1;
	m_mEmotionMap["DISGUST"] = 2;
	m_mEmotionMap["FEAR"] = 3;
	m_mEmotionMap["HAPPY"] = 4;
	m_mEmotionMap["SADNESS"] = 5;
	m_mEmotionMap["SAD"] = 5;
	m_mEmotionMap["SURPRISE"] = 6;
	m_mEmotionMap["PAIN"] = 8;

	//
	m_logFile = NULL;
	startLogFile();

	// control case
	m_bUseEmotionalExpression = true;
	m_bActive = true;
}

//-----------------------------------------------------------------------------
LLEmotionProcessor::~LLEmotionProcessor()
{
	// clean up list
	while(!m_lPADList.empty()) 
	{
		delete m_lPADList.front();
		m_lPADList.pop_front();
	}

	if (m_logFile)
	{
		fclose(m_logFile);
		TextureManager::getSingleton().remove("PADImage");
	}
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::startLogFile()
{
	struct tm *pTime;
	char logFileName[256];
	time_t ctTime; time(&ctTime);
	pTime = localtime( &ctTime );
#ifdef _WINDOWS
	sprintf(logFileName, "log\\LLAffectLog_%i\-%02i\-%02i_%02i\-%02i\-%02i.log", (1900+pTime->tm_year), (1+pTime->tm_mon), pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
#else
	sprintf(logFileName, "log/LLAffectLog_%i\-%02i\-%02i_%02i\-%02i\-%02i.log", (1900+pTime->tm_year), (1+pTime->tm_mon), pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
#endif
	if (m_logFile = fopen(logFileName, "w"))
	{
		fprintf(m_logFile, "==========================================================================\n");
		fprintf(m_logFile, "LifeLike Affect Log. Ver.0.1\n");
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime (&rawtime);
		fprintf(m_logFile, "%s", asctime(timeinfo));
		fprintf(m_logFile, "==========================================================================\n");
		fprintf(m_logFile, "time	Pleasure	Arousal	Dominance\n");
		fprintf(m_logFile, "==========================================================================\n");
		m_fLogTimer = 0.0f;
	}

	// create our dynamic texture with 8-bit luminance texels
	TexturePtr tex = TextureManager::getSingleton().createManual("PADImage", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			TEX_TYPE_2D, PADWIDTH, PADHEIGHT, 0, PF_BYTE_RGBA, TU_DYNAMIC_WRITE_ONLY);
	
	m_pTexBuf = tex->getBuffer();  // save off the texture buffer
	// initialise the texture to have full luminance
	m_pTexBuf->lock(HardwareBuffer::HBL_DISCARD);
	memset(m_pTexBuf->getCurrentLock().data, 0x00, m_pTexBuf->getSizeInBytes());
	m_pTexBuf->unlock();
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::recordLog(int emotion, float intensity, float lifetime)
{
	if (!m_logFile)
		return;

	// timestamp (sec)	PAD		p	a	d
	// timestamp		EMO		id	intensity	lifetime	

	// time stamp
	char chtime[12];
	struct tm *pTime;
	time_t ctTime; time(&ctTime);
	pTime = localtime( &ctTime );

	
	if (emotion == -1)
		//fprintf(m_logFile, "%2i:%2i:%2i\tPAD\t%f\t%f\t%f\n", pTime->tm_hour, pTime->tm_min, pTime->tm_sec, m_sMood.p, m_sMood.a, m_sMood.d);
		fprintf(m_logFile, "%f\t%f\t%f\t%f\n", m_fLogTimer, m_sMood.p, m_sMood.a, m_sMood.d);
	//else
		//fprintf(m_logFile, "%f\tEMO\t%d\t%f\t%f\n", m_fLogTimer, emotion, intensity, lifetime);


	// update screen monitor image (1024 x 1024 texture)
	// animation texture with uv coordinate

	// lock texture
	m_pTexBuf->lock(HardwareBuffer::HBL_NORMAL);

	// get access to raw texel data
	Ogre::uint8* data = (Ogre::uint8*)m_pTexBuf->getCurrentLock().data;

	// clean up the column data (set alpha 0), then draw PAD value in there
	// which column? calculate current time for uv animation, then column
	int cycles = ((int)floor(m_fLogTimer)) / MONITORCYCLE;		// cycles
	float time = m_fLogTimer - cycles * MONITORCYCLE;
	int col = CLAMP(PADWIDTH * time / MONITORCYCLE, 0, PADWIDTH-1);

	int idx = col * 4;
	for (int i=0; i<PADHEIGHT; i++)
	{
		data[idx] = 0;	// alpha
		data[idx+1] = 0;	// alpha
		data[idx+2] = 0;	// alpha
		data[idx+3] = 0;	// alpha
		idx += PADWIDTH*4;
	}
	
	// reorder for color scheme: P(red), A(green), D(blue)
	int value[3]; // 0 ~ PADHEIGHT
	value[2] = ( -(float)m_sMood.p + 1.0f) * PADHEIGHT * 0.5f; value[2] = CLAMP(value[2], 0, PADHEIGHT-3);
	value[1] = ( -(float)m_sMood.a + 1.0f) * PADHEIGHT * 0.5f; value[1] = CLAMP(value[1], 0, PADHEIGHT-3);
	value[0] = ( -(float)m_sMood.d + 1.0f) * PADHEIGHT * 0.5f; value[0] = CLAMP(value[0], 0, PADHEIGHT-3);
	for (int i=0; i<3; i++)
	{
		// paint three pixels along the column
		idx = col * 4 + (value[i]+1) * PADWIDTH * 4 - 4;
		//idx = col * 4 + (value[i]) * PADWIDTH * 4;
		data[idx+3] = 255; data[idx+i] = 255;
		
		idx += PADWIDTH * 4;
		data[idx+3] = 255; data[idx+i] = 255;
		
		idx += PADWIDTH * 4;
		data[idx+3] = 255; data[idx+i] = 255;
	}

	// unlock texture
	m_pTexBuf->unlock();
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::setPersonality(float p, float a, float d)
{
	m_sPersonality.p = p;
	m_sPersonality.a = a;
	m_sPersonality.d = d;

	// reset current mood by new personality
	m_sMood = m_sPersonality;
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::setPersonality(float o, float c, float e, float a, float n)
{
	// Big Five Traits model parameters to PAD vector
	// {Openness, Consciousness, Extraversion, Agreeableness, Neuroticism}
	// based on Mehrabian96b
	m_sPersonality.p = 0.21f*e + 0.59f*a + 0.19f*n;
	m_sPersonality.a = 0.15f*o + 0.3f*a + 0.57f*n;
	m_sPersonality.d = 0.25f*o + 0.17f*c + 0.6f*e + 0.32f*a;

	// reset current mood by new personality
	m_sMood = m_sPersonality;
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::processStimulus(sPAD* stimul)
{
	// add stimulus to active list
	m_lPADList.push_back(stimul);
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::processStimulus(int& emotion, float& intensity, float& lifetime)
{
	// convert given an emotion value to PAD vector
	// how to determine lifetime for this stimulus?
	// internal emotion engine does not have any reference for lifetime
	// so, caller must decide how long it should last.
	// reference: Russel & Mehrabian 1977, Asano 2008 (IVA08)
	
	// center of emotional category

	// 0:Angry(-80,80,100), 1:Contempt, 2:Disgust, 3:Fear(-80,80,-100), 4:Happy(80,80,+/-100)(50,0,+/-100), 5:Annoyed/Sadness(-50,0,+/-100), 6:Surprise(10,80,+/-100)
	// missing 1,2 in Ekman's category

	// Zhang(2010)
	// 0:Angry(-59,8,47), 1:Contempt, 2:Disgust(-59,-1,40), 3:Fear(-8,18,-39), 4:Happy(63,40,29), 5:Sadness(-28,-12,-37), 6:Surprise(41,55,19)

	// a few more things need attention here
	// if we use unit vector for each emotion category, what happen in reality?
	// accumulated effect, may never get to the right emotion
	// for example, one whos personality is happy, cannot be sad because there is no unit vector that has negative arousal
	// therefore, we may need to use relative vector from personality in PAD space to given category center
	// in above example, sad stimulus translated to (-P-A-D) instead of (-P00)
	// what if give emotonal stimulus is the same as personality in PAD? zero PAD vector?
	// i.e. happy person got one happy and one sad, then? still should be happy.
	// however, if use zero vector for happy stimulus, it goes close to sad state!!!
	
	// range check just in case
	if (emotion < 0 || emotion > 8)
		return;

	// some exception for contempt and disgust (missing values)
	if (emotion == 1)// || emotion == 2)
		return;

	// for control case
	float oldintensity = intensity;

	// create new PAD vecotor
	sPAD* stimul = new sPAD;
	stimul->emotion = emotion;

	float p,a,d;

	// set vector value
	// nothing fancy here. just use standard emotion PAD vector (center of each emotion in PAD space)
	// may need some complicate alogrithm later
	stimul->intensity = intensity;
	stimul->p = p = m_sEmotion[emotion].p * intensity;
	stimul->a = a = m_sEmotion[emotion].a * intensity;
	stimul->d = d = m_sEmotion[emotion].d * intensity;
	
	// lifetime?
	stimul->lifetime = lifetime;
	stimul->elapsed = 0.0f;
	
	// add stimulus to active list
	m_lPADList.push_back(stimul);
	
	// some consideration for short-term emtoion processing
	// according to ALMA model, he defines short-term emotion as instant stimulus
	// with some factors associated.
	// then, it is used to determine facial expression
	// in my work, i believe it should take current mood into account even though
	// its short-term definition
	// for instance, if a person is very happy mood, he should show higher degree 
	// of happiness on his face in some degree
	// otherwise, its facial expression will be same all the time regardless of
	// current mood
	// therefore, this function should modify input emotion accordingly
	// pseudo equation for this is following
	// EmotionOut = Emotion + c1 x Personality x |Emotion| + c2 x Sum (active emotions)
	// the second term is augmentation by Personality
	// the third term is current mood factor

	
	// should we convert this p,a,d back to emotion category?
	// or somehow compute intensity changing factor out of this?
	//   project p,a,d to stimulus vector: project u onto v  = (v dot u)*v    v is unit vector
	//   determine polarity & length (same direction -> intensify by length, opposite -> decrease by length)
	//   calculate changing factor
	// a bit of issue in this model is all emotioanl categories PAD vector has zero or positive Arousal value in 
	// Mehrabian's paper. therefore, mood vector will always have positive affect to momentary emotion vector
	sPAD v,u;
	v.p = p;
	v.a = a;
	v.d = d;
	v.normalise();

	u.p = m_fPrimaryFactor * m_sPersonality.p + m_fSecondaryFactor * m_sPAD.p;
	u.a = m_fPrimaryFactor * m_sPersonality.a + m_fSecondaryFactor * m_sPAD.a;
	u.d = m_fPrimaryFactor * m_sPersonality.d + m_fSecondaryFactor * m_sPAD.d;
	float moodLength = u.length();

	float dot = (v.p * u.p + v.a * u.a + v.d * u.d);
	v.p *= dot;
	v.a *= dot;
	v.d *= dot;

	float projectLength = v.length();

	// just in case if mood value is all zero
	// or it's perpendicular to emotion
	if (moodLength==0 || projectLength==0.0f)
	{
		// avatar is in very neutral mood status
		// let's cut the intensity by half
		intensity *= 0.5f;

		sprintf(m_cDebugString, "Emotion Value: %d intensityA(%.3f) intensityB(%.3f) PAD(%.2f %.2f %.2f) factor=1.0f", emotion, 0.0f, intensity, p, a, d);
		LLScreenLog::getSingleton().addText(m_cDebugString);

		// thesis
		sprintf(m_cDebugString, "*** Emotion Value0: %f %d %f", m_fLogTimer, emotion, intensity);
		LLScreenLog::getSingleton().addText(m_cDebugString);

		return;
	}

	// apply augmented emotion intensity
	// note that this will only affect current momentary emotion for facial expression and else
	// however, in mood computation, only original emotion stimulus value will be store
	// as mood itself has accumulated augmentation factors in its calculation
	// range of mood factor to the original intensity is 0.25 ~ 1.5
	float factor = projectLength / moodLength;	// 0 ~ 1.0f

	// same direction or opposite?
	if ((v.p * p)  < 0.0f || (v.a * a)  < 0.0f || (v.d * d)  < 0.0f)
	{
		// this is opposite direction
		// larger factor, lesser intensity
		// decrease intensity upto by 75% (0.25)
		// (1.0 - factor) * 0.5 + 0.25    => 0.25 ~ 0.75
		factor = (1.0f - factor) * 0.5f + 0.25f;
		intensity = CLAMP(intensity*factor, 0.0f, 1.0f);
	}
	else
	{
		// increase intensity
		// however double check if it's vector is within a centain range (angle 45 degree) => length > 0.7
		if (factor > 0.7f)
		{
			// upto 50% increase of intensity by mood factor
			// (factor - 0.7)*0.5/0.3 + 1.0   => 1.0 ~ 1.5
			factor = (factor - 0.7f)*1.67f + 1.0f;
			intensity = CLAMP(intensity*factor, 0.0f, 1.0f);
		}
		else
		{
			// about same direction with angle > 45 degree
			// slightly decrease upon its length
			// decrease intensity upto by 25%
			// (factor/0.7)*0.25 + 0.75    => 0.75 ~ 1.0
			factor = factor * 0.357f + 0.75f;
			intensity = CLAMP(intensity*factor, 0.0f, 1.0f);
		}
	}
	
	// the final factor may be considered
	// accumulated same emotions. i.e. happy, happy, happy => creater happy
	// does it matter if it's alive or not?
	// if matters, need to use active emotion list instead
	// using active list
	factor = 1.0f;
	int counter = 0;
	iPADRIter it, itrend;
	it = m_lPADList.rbegin(); 
	it++;		// skip the one just added
	itrend = m_lPADList.rend();
	for (; it != itrend;)
	{
		sPAD* src = *it;
		if (src->emotion == emotion)
			//factor *= 1.1f;
			factor *= 1.05f;		// 2013.7.6
		else
			break;

		// only for the three latest active emotions
		counter++;
		if (counter > 2)
			break;

		it++;
	}
	// apply repeated emotion factor: upto 33% increase in addition to mood factor
	intensity = CLAMP(intensity*factor, 0.0f, 1.0f);

//#ifdef _DEBUG
	//sprintf(m_cDebugString, "Emotion Value: %d intensityA(%f) intensityB(%f) PAD(%f %f %f) factor=%f", emotion, oldintensity, intensity, p, a, d, factor);
	//LLScreenLog::getSingleton().addText(m_cDebugString);
//#endif

	recordLog(emotion, intensity, lifetime);

	// for control case: emotion does not alter the original intensity value for facial expression
	//if (!m_bUseEmotionalExpression)
	//	intensity = 0.5f;	// strictly uniform expression

	// manual control: thesis testing purpose (0.5 or commented out)
	sprintf(m_cDebugString, "*** Emotion Value1: %f %d %f %f", m_fLogTimer, emotion, oldintensity, intensity);
	LLScreenLog::getSingleton().addText(m_cDebugString);
	//intensity = 0.5f;
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::update(float elapsedTime)
{
	// suspended?
	if (!m_bActive)
		return;
	
	// iterlate all current active stimulus and sum them up
	// to determine current emotonal state
	// evaluateEmotion() will account for personality values
	iPADIter it, itend;
	itend = m_lPADList.end();

	float p(0.0f), a(0.0f), d(0.0f), ratio(0.0f);
    int numActive = 0;
	for (it = m_lPADList.begin(); it != itend;)
    {
        sPAD* src = *it;

		src->elapsed += elapsedTime;

		// is it alive?
		if (src->lifetime > src->elapsed)
		{
			// any way to smooth in / out of new stimulus?
			// at least, each pad vector should decay over lifetime
			ratio = 1.0f - (src->elapsed / src->lifetime);
			p += src->p * ratio;
			a += src->a * ratio;
			d += src->d * ratio;

			numActive++;
		}
		else
		{
			// delete this one!
			delete src;
			it = m_lPADList.erase(it++);
			continue;
		}

		it++;
	}

	// update current value: may need some normalization here
	if (numActive)
	{
		//m_sPAD.p = p / (float)numActive;
		//m_sPAD.a = a / (float)numActive;
		//m_sPAD.d = d / (float)numActive;
		m_sPAD.p = p;
		m_sPAD.a = a;
		m_sPAD.d = d;
	}

	// convert PAD value to conventional emtotion category
	// -1: Neutral, 0: Angry, 1:Contempt, 2:Disgust, 3:Fear, 4:Happy, 5:Sadness, 6:Surprise
	// compute m_iEmotion (most dominant one) and m_fEmotions (intensity for each categories)
	evaluateMood(elapsedTime);
	
	// Mood = Personality + Averaged Sum of active emotions
	// in fact, we need some blending/smoothing here
	// when new stimulus comes in, Mood gets a sudden change
	// therefore, here we set the target point and interpolate it from current mood
	// let's try 5 second delay style interpolation to minimize discreet change at the moment of new stimulus entry
	m_sTargetMood.p = m_sPAD.p + m_sPersonality.p;
	m_sTargetMood.a = m_sPAD.a + m_sPersonality.a;
	m_sTargetMood.d = m_sPAD.d + m_sPersonality.d;

	// update current mood: interpolation
	// as this is aka dealyed method, it may never catch up to target point
	// because each active emotions change over time based on its lifetime 
	// (refer to the beginning of this func)
	m_sMood.p += (m_sTargetMood.p - m_sMood.p)*elapsedTime*0.2f;
	m_sMood.a += (m_sTargetMood.a - m_sMood.a)*elapsedTime*0.2f;
	m_sMood.d += (m_sTargetMood.d - m_sMood.d)*elapsedTime*0.2f;


	// need some logging file for testing
	// timestamp (sec)	PAD		p	a	d
	// timestamp		EMO		id	value
	m_fLogTimer += elapsedTime;
	recordLog();
	//sprintf(m_cDebugString, "Target Mood: P(%f) A(%f) D(%f)", m_sTargetMood.p, m_sTargetMood.a, m_sTargetMood.d);
	//LLScreenLog::getSingleton().addText(m_cDebugString);
	//sprintf(m_cDebugString, "Emotion Mood: P(%f) A(%f) D(%f)", m_sMood.p, m_sMood.a, m_sMood.d);
	//LLScreenLog::getSingleton().addText(m_cDebugString);

}

//-----------------------------------------------------------------------------
int LLEmotionProcessor::getEmotions(float val[])
{
	// val array should have 7 elements for emtotion category
	val[0] = m_fEmotions[0];
	val[1] = m_fEmotions[1];
	val[2] = m_fEmotions[2];
	val[3] = m_fEmotions[3];
	val[4] = m_fEmotions[4];
	val[5] = m_fEmotions[5];
	val[6] = m_fEmotions[6];
	val[7] = m_fEmotions[7];

	// maybe also return the significant emotion among 7 categories
	return m_iEmotion;
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::getPAD(sPAD & pad)
{
	pad = m_sPAD;
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::getMood(sPAD & mood)
{
	mood = m_sMood;
}

//-----------------------------------------------------------------------------
void LLEmotionProcessor::getPersonality(sPAD & personality)
{
	personality = m_sPersonality;
}

//-----------------------------------------------------------------------------
int LLEmotionProcessor::evaluateMood(float elapsedTime)
{
	// convert PAD value to emotional category
	
	// some literature
	// Becker (2004). Becker's seminar pdf (http://www.newton.ac.uk/programmes/SAS/seminars/031316301.pdf)
	// Russel & Mehrabian 1977
	// Zhang10

	float p0 = m_sPAD.p + m_sPersonality.p;
	float a0 = m_sPAD.a + m_sPersonality.a;
	float d0 = m_sPAD.d + m_sPersonality.d;
	float p,a,d;


	// we need to make pad value kinda normalize so that it stays within PAD cubic space
	// along the vector direction, shrink it to cubic space
	// use p,a,d variable as temp storage
	p = abs(p0); a = abs(a0); d = abs(d0);
	if (p > 1.0f || a > 1.0f || d > 1.0f)
	{
		// find intersection point on cubic surface
		// 0. find max axis value (abs) => determine which plane will hit the vector
		// 1. divide p,a,d values by max abs value to shrink it down to cubic space
		if (p > a)
		{
			if (p > d)
			{
				p0 /= p;
				a0 /= p;
				d0 /= p;
			}
			else
			{
				p0 /= d;
				a0 /= d;
				d0 /= d;
			}
		}
		else if (a > d)
		{
			p0 /= a;
			a0 /= a;
			d0 /= a;
		}
		else
		{
			p0 /= d;
			a0 /= d;
			d0 /= d;
		}
	}

	float distance;

	// emotion evaluation
	// computer distance from current to each emotion's center point
	// emotion value (intensity) = 1.0 - distance
	// when distnace is 0.0 (very center of an emotion), its intensity is 1.0

	// neutral		(0.0,0.0,0.0)
	distance = sqrt(p0*p0 + a0*a0 + d0*d0);

	// loop for 7 emotions + happy2
	// there are two cases. 1. sphere model, 2. cylinder model (d value in range)
	for (int i=0; i<8; i++)
	{
		p = p0 - m_sEmotion[i].p;
		a = a0 - m_sEmotion[i].a;
		d = d0 - m_sEmotion[i].d;
			
		// is this category's d is in range?
		// then, compute distance in 2D space with p & a values only
		// this is kind of cylinder volumn category space
		// otherwise, simple sphere model
		if (m_sEmotion[i].dspand)
			distance = sqrt(p*p + a*a);
		else
			distance = sqrt(p*p + a*a + d*d);
		
		// should consider some kind of threshold
		// what's the boundary for emotional category in PAD space
		// if it's out of boundary, its value is 0.0
		// then, what if the position in PAD where no emtoion boundary covers
		// is that pseudo neutral state? assume it is for now
		distance = CLAMP(m_sEmotion[i].radius - distance, 0.0f, 1.0f);
		
		// emotion intensity = inverse distance ratio to center of category space
		// distance == radius => intensity 0.0
		// distance == 0.0    => intensity 1.0
		m_fEmotions[i] = distance * (1.0f / m_sEmotion[i].radius);

		// above intensity model may not suitable
		// category space in PAD basically means kind of distribution
		// so, closer to center does not mean it is stronger in intensity
	}

	// dirty hack for missing emotions
	m_fEmotions[1] = 0.0f;
	m_fEmotions[2] = 0.0f;

	// which category is the most significant emotion?
	int cEmotion = -1;
	float eMax = 0.0f;
	for (int i=0; i<8; i++)
	{
		if (m_fEmotions[i] > eMax)
		{
			cEmotion = i;
			eMax = m_fEmotions[i];
		}
	}

	// if current emotion is changed, then reset elapsed time
	// this elapsed time value may be useful to determine emotional strength
	// in PAD space, the intensity of emotion is less obvious
	// so, assume.... if avatar stays in one emotional state for long time... it could intensify it.
	/*if (cEmotion != m_iEmotion)
	{
		m_fElapsedTime = 0.0f;
		m_iEmotion = cEmotion;
	}
	else
		m_fElapsedTime += elapsedTime;

	//m_fEmotions[m_iEmotion] = CLAMP(m_fElapsedTime*0.1, 0.0f, 1.0f);
	*/

	// some rollback mechanism for exceeding accumulated pad vector value
	// when pad vector pass the center of one emotional category, does it always mean decreasing intensity?
	// this is the case if we use distance factor as intensity value
	// simple check if the length of pad vector is longer than category center
	// then, put the max value for it
	float centerDistance;
	if (m_iEmotion!=-1)
	{
		if (m_sEmotion[m_iEmotion].dspand)
		{
			distance = sqrt(p0*p0 + a0*a0);
			centerDistance = sqrt(m_sEmotion[m_iEmotion].p * m_sEmotion[m_iEmotion].p + 
									m_sEmotion[m_iEmotion].a * m_sEmotion[m_iEmotion].a);
		}
		else
		{
			distance = sqrt(p0*p0 + a0*a0 + d0*d0);
			centerDistance = sqrt(m_sEmotion[m_iEmotion].p * m_sEmotion[m_iEmotion].p + 
									m_sEmotion[m_iEmotion].a * m_sEmotion[m_iEmotion].a + 
									m_sEmotion[m_iEmotion].d * m_sEmotion[m_iEmotion].d);
		}
		if (distance > centerDistance)
			m_fEmotions[m_iEmotion] = 1.0f;
	}

	// well above one does not work well becasue it does not model vector characteristics
	// need to do a bit more complicate calculation
	// say, determine pad vector exceed a plane that has center of category as normal vector (plane locate at the end of center)
	// test if pad vector is outside this plane, then intensity max



#ifdef _DEBUG
/*
if (m_iEmotion>-1)
	LLScreenLog::getSingleton().addText("Emotion Value: " + Ogre::StringConverter::toString(m_iEmotion) +
							"  (" + Ogre::StringConverter::toString(m_fEmotions[m_iEmotion], 2,3) + ")" +
							"  (" + Ogre::StringConverter::toString(p0, 2,3) +
							"  " + Ogre::StringConverter::toString(a0, 2,3) +
							"  " + Ogre::StringConverter::toString(d0, 2,3) + ")" +
							"  " + Ogre::StringConverter::toString(m_fEmotions[0], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[1], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[2], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[3], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[4], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[5], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[6], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[7], 2)
							);
else
	LLScreenLog::getSingleton().addText("Emotion Value: " + Ogre::StringConverter::toString(m_iEmotion) +
							"  (" + Ogre::StringConverter::toString(Ogre::Real(0.0), 2,3) + ")" +
							"  (" + Ogre::StringConverter::toString(p0, 2,3) +
							"  " + Ogre::StringConverter::toString(a0, 2,3) +
							"  " + Ogre::StringConverter::toString(d0, 2,3) + ")" +
							"  " + Ogre::StringConverter::toString(m_fEmotions[0], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[1], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[2], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[3], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[4], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[5], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[6], 2) +
							"  " + Ogre::StringConverter::toString(m_fEmotions[7], 2)
							);
*/
#endif

	return cEmotion;
}

int LLEmotionProcessor::getEmotinoID(char* emo)
{
	std::string str = emo;
	std::transform(str.begin(), str.end(),str.begin(), ::toupper);

	std::map<std::string,int>::iterator it;
	it = m_mEmotionMap.find(str);
	if (it != m_mEmotionMap.end())
	{
		return it->second;
	}

	return -1;
}

void LLEmotionProcessor::resetMood()
{
	// clear up all active emotions, reset mood
	iPADIter it, itend;
	itend = m_lPADList.end();
	for (it = m_lPADList.begin(); it != itend;)
    {
        sPAD* src = *it;
		delete src;
		it = m_lPADList.erase(it++);
	}

	m_lPADList.clear();

	m_sPAD.p = 0.0f;
	m_sPAD.a = 0.0f;
	m_sPAD.d = 0.0f;

	m_sMood = m_sPersonality;
}
