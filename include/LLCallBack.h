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
* Filename: LLCallBack.h
* -----------------------------------------------------------------------------
* Notes:    Base class of Callbakc utility.
* -----------------------------------------------------------------------------
*/

#ifndef _LLCALLBACK_H_
#define _LLCALLBACK_H_

class LLCallback
{
public:
	virtual bool execute(void *Param) const = 0;
};

template <class cInstance>
class TCallback : public LLCallback
{
public:
	TCallback()
	{
		pFunction = 0;
	}

	typedef bool (cInstance::*tFunction)(void *Param);

	virtual bool execute(void *Param) const
	{
		if (pFunction) return (cInst->*pFunction)(Param);
		else printf("ERROR : the callback function has not been defined.");

		return false;
	}

	void setCallback(cInstance *cInstancePointer, tFunction pFunctionPointer)
	{
		cInst = cInstancePointer;
		pFunction = pFunctionPointer;
	}

private:
	cInstance *cInst;
	tFunction pFunction;

};


#endif