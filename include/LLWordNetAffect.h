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
* Filename: LLWordNetAffect.h
* -----------------------------------------------------------------------------
* Notes: 
* -----------------------------------------------------------------------------
*/

#ifndef __LLWORDNETAFFECT_H_
#define __LLWORDNETAFFECT_H_

#include "LLdefine.h"

class LLWordNetAffect
{
public:
	LLWordNetAffect();
	~LLWordNetAffect();

	void		findWordAffect(POSToken& token);

protected:

	int			loadWordNetDB();

	bool		m_bWNInitialized;

	LLMap_ls	m_nounMap;			// noun map <ID, Category>
	LLMap_ll	m_etcMap[3];		// verb,adj,adv map <ID, nounID>
	LLMap_ss	m_categoryMap;		// store WN-Affect hierarchy
	LLMap_si	m_emoWordMap;		// store base emotion word, category (happy, sad, etc)
	LLMap_sf	m_emoMap;			// noun to emotion id & intensity (x.yyy => x emotion yyy intensity)

};

#endif