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
* Filename: LLFACSDB.h
* -----------------------------------------------------------------------------
* Notes: 
* -----------------------------------------------------------------------------
*/

#ifndef __LLFACSDB_H_
#define __LLFACSDB_H_

#include <list>
#include <vector>
#include <map>

//-----------------------------------------------------------------------------
typedef struct fFACS
{
	int emotionID;			// emotion: angry, comtempt, disgust, fear, happy, sadness, surprise
	std::vector<int> vAU;	// vector of AUs
}	sFACS;
typedef std::vector<int> vFACS;
typedef std::list<vFACS> lFACS;
typedef std::list<vFACS>::iterator ilFACS;

//-----------------------------------------------------------------------------
class LLFACSDB
{

public:
	LLFACSDB();
	LLFACSDB(const char* filename);
	~LLFACSDB();

	// load FACS database
	void loadData(const char* filename, bool append=true);

	// sample FACS code for a expression
	vFACS sampleEmotion(int expression);

protected:

	// FACS database list: store vectors for each emotion
	// 0: Angry, 1:Contempt, 2:Disgust, 3:Fear, 4:Happy, 5:Sadness, 6:Surprise
	lFACS	m_lFACS[7];

	// FACS database pool: temp storage not to repeat a certain FACS AU sets for a given emotion
	lFACS	m_lFACSPool[7];

	// need to map FACS AU index to Facial Animaiton Shape id somehow!
	// however, this class must keep the original values 
	// since animation shape depends on graphical data so can be changed per character
	// therefore, translation should be handled in character class with some sort of dictionary(map)
};

#endif