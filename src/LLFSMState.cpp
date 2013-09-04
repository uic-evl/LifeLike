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
* Filename: LLFSMState.cpp
* -----------------------------------------------------------------------------
* Notes:    Reference source code (Game Programming Gems)
* -----------------------------------------------------------------------------
*/
#include <time.h>
#include "LLFSMState.h"
#include <stdlib.h>

//-----------------------------------------------------------------------------
LLFSMState::LLFSMState(int nStateId, int nGroupId, int nTransitions)
{
	m_iStateId = nStateId;
	m_iNumTransitions = nTransitions;
	m_iGroupId = nGroupId;
	m_iInsertedTransitions = 0;
}

//-----------------------------------------------------------------------------
LLFSMState::~LLFSMState()
{
}

//-----------------------------------------------------------------------------
void LLFSMState::addTransition(int nEvent, int nState)
{
	// event id is the primary key of transitions
	// we have multiple transition for single event
	// e.g.) ready to idle0, idle2, idle3 ...

	int nTempEvent = 0;
	iList* tempList = NULL;
	if( !m_mapTransitions.empty() ) {
		m_miTransitions = m_mapTransitions.find(nEvent);
		if( m_miTransitions != m_mapTransitions.end() ) {
			tempList = (*m_miTransitions).second;
		}
	}
	
	if (tempList!=NULL) {
		// we already got list, so just add new transition into it
		if(!tempList->empty()) {
			
			// do we need to check its existence?
		    iListIter it, itend;
			itend = tempList->end();
			for (it = tempList->begin(); it != itend; it++)
			{
				if (nState == *it)
					return;	
			}
		}
		// ok. we do not have it already. so add it.
		tempList->push_back(nState);
	}
	else
	{
		tempList = new iList;
		tempList->push_back(nState);
		m_mapTransitions.insert(iMap::value_type(nEvent,tempList));
	}
	
	m_iInsertedTransitions++;
}

//-----------------------------------------------------------------------------
int LLFSMState::getOutputState(int nEvent)
{
	m_miTransitions = m_mapTransitions.find(nEvent);
	
	if (m_miTransitions == m_mapTransitions.end())
		return -1;
	
	iList* tlist = (*m_miTransitions).second;

	// now we have multiple of output transition
	// let's select it totally randomly
	int a = (int)tlist->size() -1;
	double b = (double)rand() / (double)RAND_MAX;
	int outIdx = a * b + 0.5;	// add 0.5 for proper rounding
	m_liTransitions = tlist->begin();
	for (int i=0; i< outIdx; i++)
		//m_liTransitions = m_liTransitions++;
		m_liTransitions++;
	
	// in fact, it would be better if can remove any possible repetition
	// for instance, 5 idle state outputs and currently idle
	// clearly we do not want to get the same output again
	// partially, it is implemented in FSM spec file as it does not have transition to itself.

	return *m_liTransitions;

}
