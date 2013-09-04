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
* Filename: LLFSMState.h
* -----------------------------------------------------------------------------
* Notes:    Reference source code (Game Programming Gems)
* -----------------------------------------------------------------------------
*/
#ifndef __LLFSMSTATE_H_
#define __LLFSMSTATE_H_

#include <map>
#include <list>

using namespace std;

typedef list<int> iList;
typedef list<int>::iterator iListIter;
typedef map<int,iList*> iMap;
typedef map<int,iList*>::iterator iMapIter;

class LLFSMState
{
public:
	LLFSMState(int nStateId, int nGroupId, int nTransitions);
	~LLFSMState();

	int		getStateId() { return m_iStateId;}
	int		getGroupId() { return m_iGroupId;}
	void	addTransition(int nEvent, int nState);
	int		getOutputState(int nEvent);

private:	
	int		m_iStateId;
	int		m_iGroupId;
	int		m_iNumTransitions;
	int		m_iInsertedTransitions;
	
	iListIter	m_liTransitions;
	iMap		m_mapTransitions;
	iMapIter	m_miTransitions;
};


#endif