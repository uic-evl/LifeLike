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
* Filename: LLCRFTagger.cpp
* -----------------------------------------------------------------------------
* Notes:    basic interface to crfsuite lib
*			load pre-processed model, take surface text, and then label words
*			to generate behavior model output (nod/shake)
*			assume surface text is processed with associated feature
*			tagger should parse feature tokens into proper format for crf model
* -----------------------------------------------------------------------------
*/

#include "LLCRFTagger.h"

using namespace std;

#define    SAFE_RELEASE(obj)    if ((obj) != NULL) { (obj)->release(obj); (obj) = NULL; }

LLCRFTagger::LLCRFTagger()
{
	m_pModel[0] = NULL;
	m_pModel[1] = NULL;
	m_pTagger[0] = NULL;
	m_pTagger[1] = NULL;
	m_pAttrs[0] = NULL;
	m_pAttrs[1] = NULL;
	m_pLabels[0] = NULL;
	m_pLabels[1] = NULL;

	m_bInitialized = false;

	m_actString[DACT_NON] = "NA";
	m_actString[DACT_YES] = "YES";
	m_actString[DACT_NO] = "NO";
	m_actString[DACT_AGR] = "AGR";
	m_actString[DACT_NAGR] = "NAG";
}


LLCRFTagger::~LLCRFTagger()
{
	if (m_pTagger[0])
		SAFE_RELEASE(m_pTagger[0]);
	if (m_pTagger[1])
		SAFE_RELEASE(m_pTagger[1]);
	if (m_pAttrs[0])
		SAFE_RELEASE(m_pAttrs[0]);
	if (m_pAttrs[1])
		SAFE_RELEASE(m_pAttrs[1]);
	if (m_pLabels[0])
		SAFE_RELEASE(m_pLabels[0]);
	if (m_pLabels[1])
		SAFE_RELEASE(m_pLabels[1]);
	if (m_pModel[0])
		SAFE_RELEASE(m_pModel[0]);
	if (m_pModel[1])
		SAFE_RELEASE(m_pModel[1]);
}

void LLCRFTagger::loadModel(const char* modelfile, int type)
{
	m_bInitialized = false;

	if (type > 1 || type < 0)
		return;

	int ret;
	// model
	if (ret = crfsuite_create_instance_from_file(modelfile, (void**)&m_pModel[type])) {
		// error
		return;
	}
	// tagger
	if (ret = m_pModel[type]->get_tagger(m_pModel[type], &m_pTagger[type])) {
        // error
		return;
    }
	// attribute dictionary
	if (ret = m_pModel[type]->get_attrs(m_pModel[type], &m_pAttrs[type])) {
        // error
		return;
    }
	// label dictionary
	if (ret = m_pModel[type]->get_labels(m_pModel[type], &m_pLabels[type])) {
        // error
		return;
    }

	if (m_pLabels[0] && m_pLabels[1])
		m_bInitialized = true;
}

// final version tagger
void LLCRFTagger::tagSentence(std::vector<POSToken>& tokens)
{
	// need to run two tagger (NOD, SHAKE)
	// to determine NOD label in comparision with SAHKE model
	
	// features used: pos, phrase, act, lex_keyword (total 4 attributes)
	// feature template for the trained machine
	//	(('pos', 0),),
	//	(('act', 0),),
	//	(('phr', 0),),
	//
	//	(('pos', -1), ('pos',  0), ('pos',  1)),
	//	(('phr', -1), ('phr',  0), ('phr',  1)),
	//
	//	(('pos', -1), ('phr',  -1), ('key', -1)),
	//	(('pos',  0), ('phr',   0), ('key',  0)),
	//	(('pos',  1), ('phr',   1), ('key',  1)),

	if (!m_bInitialized)
		return;

	int nWords = tokens.size();
	
	// CRF variables
	crfsuite_instance_t inst0, inst1;
	crfsuite_item_t item0, item1;
	crfsuite_attribute_t cont0, cont1;
	int aid=0, ret=0, L0=0, L1=0;

	// temp tokens for punctuation exclusion
	std::vector<POSToken> ntokens;

	// pre-process: generate crf token feature
	for (int i=0; i<nWords; i++)
	{
		// word pos affirmative intensity inclusive nod/shake
		// pos & phrase is already defined in token.pos, token.phrase
		// act is defined in the first token.act
		// so, additionaly store [affirmative/negative], [intensify/inclusive] here
		tokens[i].crf_label = CRFLABEL_NONE;	// 0:none, 1: nod, 2: shake

		// Intensification & Inclusive in the same rank
		if (tokens[i].func_type == WORDFN_INTENSE)
		{
			tokens[i].crf_feature.push_back("INT");
		}
		else if (tokens[i].func_type == WORDFN_INCLUSIVE)
			tokens[i].crf_feature.push_back("INC");
		else
			tokens[i].crf_feature.push_back("NA");
	
	
		// Affirmative: current model does not use this attribute
		/*
		if (tokens[i].func_type == WORDFN_AFFIRMATIVE)
			tokens[i].crf_feature.push_back("AF");
		else if (tokens[i].func_type == WORDFN_NEGATION)
			tokens[i].crf_feature.push_back("NE");
		else
			tokens[i].crf_feature.push_back("NA");
		*/

		// exclude punctuation (comma)
		if (tokens[i].str.compare(",") != 0 && tokens[i].str.compare(".") != 0)
		{
			ntokens.push_back(tokens[i]);
		}
	
	}

	//
	L0 = m_pLabels[CRFMODEL_NOD]->num(m_pLabels[CRFMODEL_NOD]);
	L1 = m_pLabels[CRFMODEL_SHAKE]->num(m_pLabels[CRFMODEL_SHAKE]);

	// init instance
	crfsuite_instance_init(&inst0);
	crfsuite_instance_init(&inst1);

	// generated crfsuite format feature
	char att[256];
	string attr;
	int b,e;
	nWords = ntokens.size();
	for (int i=0; i<nWords; i++)
	{
		//	(('pos', 0),),
		//	(('act', 0),),
		//	(('phr', 0),),
		//
		//	(('pos', -1), ('pos',  0), ('pos',  1)),
		//	(('phr', -1), ('phr',  0), ('phr',  1)),
		//
		//	(('pos', -1), ('phr',  -1), ('key', -1)),
		//	(('pos',  0), ('phr',   0), ('key',  0)),
		//	(('pos',  1), ('phr',   1), ('key',  1)),

		// init item as it begins here
		crfsuite_item_init(&item0);
		crfsuite_item_init(&item1);

		// unigram (pos, act, phr)
		{
			// pos
			sprintf(att, "pos[0]=%s", ntokens[i].POS.c_str());
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont0, aid, 1.0),
				crfsuite_item_append_attribute(&item0, &cont0);

			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont1, aid, 1.0),
				crfsuite_item_append_attribute(&item1, &cont1);

			// act
			sprintf(att, "act[0]=%s", m_actString[ntokens[0].act].c_str());
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont0, aid, 1.0),
				crfsuite_item_append_attribute(&item0, &cont0);

			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont1, aid, 1.0),
				crfsuite_item_append_attribute(&item1, &cont1);

			// phrase
			sprintf(att, "phr[0]=%s", ntokens[i].phrase.c_str());
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont0, aid, 1.0),
				crfsuite_item_append_attribute(&item0, &cont0);

			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont1, aid, 1.0),
				crfsuite_item_append_attribute(&item1, &cont1);
		}

		// trigram with same feature (all three valuse should be in range
		if (i > 0 && i < (nWords-1) )
		{
			// POS (-1, 0, 1)
			sprintf(att, "pos[-1]|pos[0]|pos[1]=%s|%s|%s", ntokens[i-1].POS.c_str(), ntokens[i].POS.c_str(), ntokens[i+1].POS.c_str());
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont0, aid, 1.0),
				crfsuite_item_append_attribute(&item0, &cont0);
			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], att);
			if (0 <= aid)
				crfsuite_attribute_set(&cont1, aid, 1.0),
				crfsuite_item_append_attribute(&item1, &cont1);

			// phrase (-1, 0, 1)
			sprintf(att, "phr[-1]|phr[0]|phr[1]=%s|%s|%s", ntokens[i-1].phrase.c_str(), ntokens[i].phrase.c_str(), ntokens[i+1].phrase.c_str());
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], att);
			if (0 <= aid) {
				crfsuite_attribute_set(&cont0, aid, 1.0),
				crfsuite_item_append_attribute(&item0, &cont0);
			}
			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], att);
			if (0 <= aid) {
				crfsuite_attribute_set(&cont1, aid, 1.0),
				crfsuite_item_append_attribute(&item1, &cont1);
			}
		}

		// trigram with mixed (pos, phrase, key) for rank i-1, i, i+1
		b = max(i-1, 0);
		e = min(i+2, nWords);
		for (int k=b; k<e; k++)	// i(1), k(0)  i(1), k(1)  i(1), k(2)
		{
			// pos, phrase, key (-1,-1,-1)(0,0,0)(1,1,1)
			sprintf(att, "pos[%d]|phr[%d]|key[%d]=%s|%s|%s", k-i, k-i, k-i, ntokens[k].POS.c_str(), ntokens[k].phrase.c_str(), ntokens[k].crf_feature[0].c_str());
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], att);
			if (0 <= aid) {
				crfsuite_attribute_set(&cont0, aid, 1.0),
				crfsuite_item_append_attribute(&item0, &cont0);
			}
			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], att);
			if (0 <= aid) {
				crfsuite_attribute_set(&cont1, aid, 1.0),
				crfsuite_item_append_attribute(&item1, &cont1);
			}
		}

		// two special case
		// BOS & EOS: beginning and ending of sentence
		if (i == 0)
		{
			// add __BOS__ attribute
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], "__BOS__");
			crfsuite_attribute_set(&cont0, aid, 1.0);
			crfsuite_item_append_attribute(&item0, &cont0);

			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], "__BOS__");
			crfsuite_attribute_set(&cont1, aid, 1.0);
			crfsuite_item_append_attribute(&item1, &cont1);
		}
		else if (i == (nWords-1))
		{
			// add __EOS__ attribute
			aid = m_pAttrs[CRFMODEL_NOD]->to_id(m_pAttrs[CRFMODEL_NOD], "__EOS__");
			crfsuite_attribute_set(&cont0, aid, 1.0);
			crfsuite_item_append_attribute(&item0, &cont0);

			aid = m_pAttrs[CRFMODEL_SHAKE]->to_id(m_pAttrs[CRFMODEL_SHAKE], "__EOS__");
			crfsuite_attribute_set(&cont1, aid, 1.0);
			crfsuite_item_append_attribute(&item1, &cont1);
		}

		// need to append item to instance
		crfsuite_instance_append(&inst0, &item0, L0);
		crfsuite_item_finish(&item0);
		crfsuite_instance_append(&inst1, &item1, L1);
		crfsuite_item_finish(&item1);
	}
	
	// now all items added

	// tag label
	if ((ret = m_pTagger[CRFMODEL_NOD]->set(m_pTagger[CRFMODEL_NOD], &inst0))) {
		// error
		crfsuite_instance_finish(&inst0);
		crfsuite_instance_finish(&inst1);
		return;
	}
	if ((ret = m_pTagger[CRFMODEL_SHAKE]->set(m_pTagger[CRFMODEL_SHAKE], &inst1))) {
		// error
		crfsuite_instance_finish(&inst0);
		crfsuite_instance_finish(&inst1);
		return;
	}

	// Obtain the viterbi label sequence
	floatval_t score = 0;
	int *output0 = (int*)calloc(sizeof(int), inst0.num_items);
	int *output1 = (int*)calloc(sizeof(int), inst1.num_items);
	if ((ret = m_pTagger[CRFMODEL_NOD]->viterbi(m_pTagger[CRFMODEL_NOD], output0, &score))) {
		// error
		free(output0);
		free(output1);
		crfsuite_instance_finish(&inst0);
		crfsuite_instance_finish(&inst1);
		return;
	}
	if ((ret = m_pTagger[CRFMODEL_SHAKE]->viterbi(m_pTagger[CRFMODEL_SHAKE], output1, &score))) {
		// error
		free(output0);
		free(output1);
		crfsuite_instance_finish(&inst0);
		crfsuite_instance_finish(&inst1);
		return;
	}

	// set tag label in tokens
	// should consider excluded punctions
	int k = 0;
	for (int i=0; i<tokens.size(); i++)
	{
		// exclude punctuation (comma)
		if (tokens[i].str.compare(",") == 0)
			continue;
		if (tokens[i].str.compare(".") == 0)
			continue;

		// label NOD
		const char *label0 = NULL;
		m_pLabels[CRFMODEL_NOD]->to_string(m_pLabels[CRFMODEL_NOD], output0[k], &label0);
		if (strcmp(label0, "NOD") == 0)
		{
			// merge two result: does not matter what label SHAKE model got. just compare probability of it
			// 0. gab < 0.2 => NA
			// 1. p_nod > p_shake => NA
			
			// may not use this merge process if trigram word level tagging is good enough
			// james0 train model is precision, recall, f-score (0.347, 0.506, 0.412)
			// determine this later...

			// retrieve probability
			floatval_t prob0, prob1;
			m_pTagger[CRFMODEL_NOD]->marginal_point(m_pTagger[CRFMODEL_NOD], output0[k], k, &prob0);
			m_pTagger[CRFMODEL_SHAKE]->marginal_point(m_pTagger[CRFMODEL_SHAKE], output1[k], k, &prob1);

			// compare probability
			if (prob0 > prob1 && abs(prob0-prob1) > 0.02 )
				tokens[i].crf_label = CRFLABEL_NOD;

		}
		k++;
	}

	free(output0);
	free(output1);
	crfsuite_instance_finish(&inst0);
	crfsuite_instance_finish(&inst1);
	
}


