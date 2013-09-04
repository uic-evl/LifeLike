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
* Filename: LLFACSDB.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLFACSDB.h"

vFACS usedfacs (100);

//-----------------------------------------------------------------------------
LLFACSDB::LLFACSDB()
{
}

//-----------------------------------------------------------------------------
LLFACSDB::LLFACSDB(const char* filename)
{
	loadData(filename);
}

//-----------------------------------------------------------------------------
LLFACSDB::~LLFACSDB()
{
	// clear db & pool
	for (int i=0; i<7; i++)
	{
		m_lFACSPool[i].clear();
		m_lFACS[i].clear();
	}

}

//-----------------------------------------------------------------------------
void LLFACSDB::loadData(const char* filename, bool append)
{
	// open file
	FILE *fp;
	if (!(fp=fopen(filename,"r"))) 
	{
		printf("Can't open FACS database file: %s\n", filename);
		return;
	}

	if (!append)
	{
		// clear all existing db entries
		for (int i=0; i<7; i++)
		{
			m_lFACSPool[i].clear();
			m_lFACS[i].clear();
		}
	}

	// parse contents of FACS DB file
	char line[256], austr[16];
	char* p_token, *pch;
	char seps[] =" \t";
	int emotion, au;
	while( fgets(line, sizeof(line), fp) )
	{
		// each line in custom FACS DB txt file
		// index emotionid action units
		// 0 1 1.0 2.0 ...
		// action unit x.y  => AUx with intensity y
		
		// index
		p_token = strtok(line, seps);
		if (!p_token)
			continue;

		// emotion
		p_token = strtok(NULL, seps);
		if (!p_token)
			continue;
		emotion = atoi(p_token) - 1;		// emotion 0 ~ 6
		if (emotion < 0 || emotion > 6)
			continue;

		// Action Units
		// note that we do not use intensity level defined in Cohn FACS DB
		vFACS newfacs;
		p_token = strtok(NULL, seps);
		while (p_token)
		{
			// find AU id (i.e 12.0 is AU12)
			pch = strchr(p_token, '.');
			memset(austr, 0, sizeof(char)*16);
			if (pch == NULL)
				strcpy(austr, p_token);
			else
				strncpy(austr, p_token, pch-p_token);
			
			// check if this is the last one with only return character
			if (strcmp(austr, "\n") != 0)
			{
				au = atoi(austr);						// AU 1 ~ 4x
				newfacs.push_back(au);

				usedfacs[au] = usedfacs[au] + 1;
			}
			p_token = strtok(NULL, seps);
		}

		m_lFACS[emotion].push_back(newfacs);
	}

	// close file
	fclose(fp);

	// create DB pool
	for (int i=0; i<7; i++)
		m_lFACSPool[i] = m_lFACS[i];
}

//-----------------------------------------------------------------------------
vFACS LLFACSDB::sampleEmotion(int expression)
{
	vFACS facs;

	// check emtoion range first
	if (expression < 0 || expression > 6)
		// this is out of range. just return empty vector
		return facs;

	// pick random index number
	int poolsize = m_lFACSPool[expression].size() - 1;
	double a = (double)rand() / (double)RAND_MAX;
	int idx = (int)(a * poolsize);

	// get matching FACS vector
	ilFACS itr = m_lFACSPool[expression].begin();
	for (int i=0; i< idx; i++)
		itr++;
		
	facs = *itr;		// vFACS

	// pop it
	m_lFACSPool[expression].erase(itr);

	// check if pool is empty, then refill it
	if (poolsize == 0)
		m_lFACSPool[expression] = m_lFACS[expression];

	return facs;
}
