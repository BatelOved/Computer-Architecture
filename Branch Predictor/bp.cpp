/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <iostream>
#include <memory>
#include <vector>
#include <list>
#include <algorithm>
#include <math.h>
using namespace std;


/*************************************************Class FSM_Table***************************************************/
class FSM_Table {
	typedef enum {SNT = 0, WNT = 1, WT = 2, ST = 3} FSM_State;

	class FSM {
		FSM_State fsmState;

	public:
		FSM(FSM_State fsmState): fsmState(fsmState) {}
		bool predict() { return (fsmState == WT || fsmState == ST); }
		void update(bool taken) {
			switch(fsmState) {
				case SNT:
					fsmState = taken ? WNT : fsmState;
					break;

				case WNT:
					fsmState = taken ? WT : SNT;
					break;

				case WT:
					fsmState = taken ? ST : WNT;
					break;

				case ST:
					fsmState = taken ? fsmState : WT;
					break;
			}
		}
	};

	vector<shared_ptr<FSM>> fsmTable;
	uint32_t fsmTableSize;

public:
	FSM_Table(uint32_t fsmTableSize, uint32_t fsmState): fsmTableSize(fsmTableSize) {
		for (uint32_t i = 0; i < fsmTableSize; i++) {
			fsmTable.push_back(make_shared<FSM>(FSM_State(fsmState)));
		}
	}

	~FSM_Table() {
		for (uint32_t i = 0; i < fsmTableSize; i++) {
			fsmTable.pop_back();
		}
	}

	bool predict(uint32_t history) {
		return fsmTable[history]->predict();
	}

	void update(uint32_t history, bool taken) {
		fsmTable[history]->update(taken);
	}
};


/***************************************************Class BTB******************************************************/
class BTB {
	struct Branch {
		uint32_t tag;
		uint32_t targetPc;
		uint32_t historySize;
		shared_ptr<list<bool>> hist;
		shared_ptr<FSM_Table> fsmTable;
		bool used;

	public:
		Branch(uint32_t tag, uint32_t targetPc, uint32_t historySize, shared_ptr<list<bool>> hist, shared_ptr<FSM_Table> fsmTable, bool used): 
					tag(tag), targetPc(targetPc), historySize(historySize), hist(hist), fsmTable(fsmTable), used(used) {}
		
		void updateHist(bool taken) {
			hist->push_front(taken);
			hist->pop_back();
		}
	};

	vector<shared_ptr<Branch>> btbTable;
	uint32_t btbSize;
	uint32_t historySize;
	uint32_t tagSize;
	uint32_t fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;

	void histInit(shared_ptr<list<bool>> hist);
	uint32_t calcShared(uint32_t pc, int Shared) const;
	uint32_t histToInt(shared_ptr<list<bool>> hist) const;
	void updateBranch(shared_ptr<BTB::Branch> targetBranch, uint32_t tag, uint32_t targetPc, uint32_t calcFsmEntry, 
							shared_ptr<list<bool>> hist, shared_ptr<FSM_Table> fsmTable, bool taken, bool used = true);

public:
	BTB(uint32_t btbSize, uint32_t historySize, uint32_t tagSize, uint32_t fsmState, bool isGlobalHist, bool isGlobalTable, int Shared);
	bool exists(uint32_t pc);
	void parsePc(uint32_t pc, uint32_t btbSize, uint32_t tagSize, uint32_t *tagIdx, uint32_t *btbIdx);
	void insertBranch(uint32_t pc, uint32_t targetPc, bool taken);
	bool predict(uint32_t pc, uint32_t *dst);
	void update(uint32_t pc, uint32_t targetPc, bool taken);
};

BTB::BTB(uint32_t btbSize, uint32_t historySize, uint32_t tagSize, uint32_t fsmState, bool isGlobalHist, bool isGlobalTable, int Shared):
	btbTable(), btbSize(btbSize), historySize(historySize), tagSize(tagSize), fsmState(fsmState), isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), Shared(Shared) {
	if (!isGlobalHist && !isGlobalTable) {
		for (uint32_t i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, historySize, nullptr, nullptr, false));
		}
	}
	else if (isGlobalHist && isGlobalTable) {
		shared_ptr<list<bool>> global_hist = make_shared<list<bool>>();
		histInit(global_hist);
		shared_ptr<FSM_Table> global_fsm_table = make_shared<FSM_Table>(pow(2,historySize), fsmState);
		for (uint32_t i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, historySize, global_hist, global_fsm_table, false));
		}
	}
	else if (!isGlobalHist && isGlobalTable) {
		shared_ptr<FSM_Table> global_fsm_table = make_shared<FSM_Table>(pow(2,historySize), fsmState);
		for (uint32_t i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, historySize, nullptr, global_fsm_table, false));
		}
	}
	else {
		shared_ptr<list<bool>> global_hist = make_shared<list<bool>>();
		histInit(global_hist);
		for (uint32_t i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, historySize, global_hist, nullptr, false));
		}
	}
}

uint32_t BTB::calcShared(uint32_t pc, int Shared) const {
	if (isGlobalTable && Shared) {
		return ((uint32_t)pow(2, historySize) - 1) & (Shared == 1 ? (pc >> 2) : (pc >> 16)); 
	}
	return 0;
}

uint32_t BTB::histToInt(shared_ptr<list<bool>> hist) const {
	uint32_t histIdx = 0;
	uint32_t i;
	list<bool>::iterator histIt;
	for (i = 0, histIt = hist->begin(); histIt != hist->end(); ++histIt, ++i) {
		histIdx += (*histIt) * pow(2,i);
	}
	return histIdx;
}

void BTB::histInit(shared_ptr<list<bool>> hist) {
	for (uint32_t i = 0; i < historySize; i++) {
		hist->push_front(0);
	}
}

void BTB::update(uint32_t pc, uint32_t targetPc, bool taken) {
	uint32_t btbIdx, tagIdx;
	parsePc(pc, btbSize, tagSize, &tagIdx, &btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[btbIdx];
	uint32_t fsmEntry = histToInt(targetBranch->hist);

	targetBranch->fsmTable->update(fsmEntry ^ calcShared(pc, Shared), taken);
	targetBranch->updateHist(taken);
	targetBranch->targetPc = targetPc;
}

bool BTB::predict(uint32_t pc, uint32_t *dst) {
	uint32_t btbIdx, tagIdx;
	parsePc(pc, btbSize, tagSize, &tagIdx, &btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[btbIdx];
	uint32_t fsmEntry = histToInt(targetBranch->hist);
	
	if (targetBranch->fsmTable->predict(fsmEntry ^ calcShared(pc, Shared))) {
		*dst = targetBranch->targetPc;
		return true;
	}
	*dst = pc + 4;
	return false;
}

void BTB::updateBranch(shared_ptr<BTB::Branch> targetBranch, uint32_t tag, uint32_t targetPc, uint32_t calcFsmEntry, 
								shared_ptr<list<bool>> hist, shared_ptr<FSM_Table> fsmTable, bool taken, bool used) {
	targetBranch->tag = tag;
	targetBranch->used = used;
	targetBranch->hist = hist;
	targetBranch->fsmTable = fsmTable;
	targetBranch->targetPc = targetPc;
	targetBranch->historySize = historySize;
	targetBranch->fsmTable->update(calcFsmEntry, taken);
	targetBranch->updateHist(taken);
}

void BTB::insertBranch(uint32_t pc, uint32_t targetPc, bool taken) {
	uint32_t btbIdx, tagIdx;
	parsePc(pc, btbSize, tagSize, &tagIdx, &btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[btbIdx];
	uint32_t globalFsmEntry = histToInt(targetBranch->hist);

	if (isGlobalHist && isGlobalTable) {
		updateBranch(targetBranch, tagIdx, targetPc, globalFsmEntry ^ calcShared(pc, Shared), targetBranch->hist, targetBranch->fsmTable, taken);
	}
	else if (!isGlobalHist && isGlobalTable) {
		shared_ptr<list<bool>> local_hist = make_shared<list<bool>>();
		histInit(local_hist);
		updateBranch(targetBranch, tagIdx, targetPc, calcShared(pc, Shared), local_hist, targetBranch->fsmTable, taken);
	}
	else if (isGlobalHist && !isGlobalTable) {
		updateBranch(targetBranch, tagIdx, targetPc, globalFsmEntry, targetBranch->hist, make_shared<FSM_Table>(pow(2,historySize), fsmState), taken);
	}
	else {
		shared_ptr<list<bool>> local_hist = make_shared<list<bool>>();
		histInit(local_hist);
		updateBranch(targetBranch, tagIdx, targetPc, 0, local_hist, make_shared<FSM_Table>(pow(2,historySize), fsmState), taken);
	}
}

bool BTB::exists(uint32_t pc) {
	uint32_t btbIdx, tagIdx;
	parsePc(pc, btbSize, tagSize, &tagIdx, &btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[btbIdx];

	return targetBranch->used && tagIdx == targetBranch->tag;
}

void BTB::parsePc(uint32_t pc, uint32_t btbSize, uint32_t tagSize, uint32_t *tagIdx, uint32_t *btbIdx) {
	*tagIdx = (pc >> (2 + (uint32_t)log2(btbSize))) & ((uint32_t)pow(2, tagSize) - 1);
	*btbIdx = (pc >> 2) & (btbSize - 1);
}


/*****************************************************Class BP*******************************************************/
class BP {
	shared_ptr<BTB> btb;
	SIM_stats stats;
	uint32_t historySize;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;

public:
	BP(uint32_t btbSize, uint32_t historySize, uint32_t tagSize, uint32_t fsmState, bool isGlobalHist, bool isGlobalTable, int Shared);
	bool predict(uint32_t pc, uint32_t *dst);
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	void getStats(SIM_stats *curStats);
};

BP::BP(uint32_t btbSize, uint32_t historySize, uint32_t tagSize, uint32_t fsmState, bool isGlobalHist, bool isGlobalTable, int Shared):
										historySize(historySize), isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), Shared(Shared) {
	if (btbSize != 1 && btbSize != 2 && btbSize != 4 && btbSize != 8 && btbSize != 16 && btbSize != 32) {
		cout << "BTB size is not valid! valid values are: {1, 2, 4, 8, 16, 32}" << endl;
		throw "Error: BTB size";
	}
	if (historySize > 8 || historySize < 1) {
		cout << "History size is not valid! valid values are: [1, 8]" << endl;
		throw "Error: History size";
	}
	if (tagSize > (30 - log2(btbSize)) || tagSize < 0) {
		cout << "Tag size is not valid! valid values are: [0, 30 - log2(btbSize)]" << endl;
		throw "Error: Tag size";
	}

	btb = make_shared<BTB>(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	stats.br_num = 0;
	stats.flush_num = 0;
	stats.size = btbSize * (tagSize + 30 + 1) + historySize * (isGlobalHist ? 1 : btbSize) + 2 * pow(2, historySize) * (isGlobalTable ? 1 : btbSize);
}

bool BP::predict(uint32_t pc, uint32_t *dst) {
	if (btb->exists(pc)) {
		return btb->predict(pc, dst);
	}
	*dst = pc + 4;
	return false;
}

void BP::update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
	btb->exists(pc) ? btb->update(pc, targetPc, taken) : btb->insertBranch(pc, targetPc, taken);
	stats.flush_num += taken ? pred_dst != targetPc : pred_dst != pc + 4;
	++stats.br_num;
}

void BP::getStats(SIM_stats *curStats) {
	*curStats = stats;
}

/********************************************************************************************************************/

shared_ptr<BP> bp = nullptr;


int BP_init(uint32_t btbSize, uint32_t historySize, uint32_t tagSize, uint32_t fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared) {
	bp = make_shared<BP>(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	return bp == nullptr ? -1 : 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst) {
	return bp->predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
	bp->update(pc, targetPc, taken, pred_dst);
}

void BP_GetStats(SIM_stats *curStats) {
	bp->getStats(curStats);
}
