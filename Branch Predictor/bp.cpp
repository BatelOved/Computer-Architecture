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
					if (taken) {
						fsmState = WNT;
					}

					break;

				case WNT:
					if (!taken) {
						fsmState = SNT;
					}
					else {
						fsmState = WT;
					}

					break;

				case WT:
					if (!taken) {
						fsmState = WNT;
					}
					else {
						fsmState = ST;
					}

					break;

				case ST:
					if (!taken) {
						fsmState = WT;
					}

					break;
			}
		}
	};

	vector<shared_ptr<FSM>> fsmTable;
	unsigned fsmTableSize;

public:
	FSM_Table(unsigned fsmTableSize, unsigned fsmState): fsmTableSize(fsmTableSize) {
		for (int i = 0; i < fsmTableSize; i++) {
			fsmTable.push_back(make_shared<FSM>(FSM_State(fsmState)));
		}
	}

	~FSM_Table() {
		for (int i = 0; i < fsmTableSize; i++) {
			fsmTable.pop_back();
		}
	}

	bool predict(unsigned history) {
		return fsmTable[history]->predict();
	}

	void update(unsigned history, bool taken) {
		fsmTable[history]->update(taken);
	}
};


/***************************************************Class BTB******************************************************/
class BTB {
public:
	struct Branch {
		uint32_t pc;
		uint32_t tag;
		uint32_t targetPc;
		unsigned historySize;
		shared_ptr<list<bool>> hist;
		shared_ptr<FSM_Table> fsmTable;
		bool used;

	public:
		Branch(uint32_t pc, uint32_t tag, uint32_t targetPc, unsigned historySize, shared_ptr<list<bool>> hist, shared_ptr<FSM_Table> fsmTable, bool used): 
					pc(pc), tag(tag), targetPc(targetPc), historySize(historySize), hist(hist), fsmTable(fsmTable), used(used) {}
		
		void updateHist(bool taken) {
			hist->push_front(taken);
			hist->pop_back();
		}
	};

private:
	vector<shared_ptr<Branch>> btbTable;
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;

	static unsigned histToInt(shared_ptr<list<bool>> hist);
	unsigned calcShared(uint32_t pc, int Shared);

public:
	BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared);
	bool exists(uint32_t pc);
	void parsePc(uint32_t pc, unsigned btbSize, unsigned tagSize, unsigned *tagIdx, unsigned *btbIdx);
	void insertBranch(uint32_t pc, uint32_t targetPc, bool taken);
	bool predict(uint32_t pc, uint32_t *dst);
	void update(uint32_t pc, uint32_t targetPc, bool taken);
};

BTB::BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared):
	btbTable(), btbSize(btbSize), historySize(historySize), tagSize(tagSize), fsmState(fsmState), isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), Shared(Shared) {
	if (!isGlobalHist && !isGlobalTable) {
		for (int i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, 0, historySize, nullptr, nullptr, false));
		}
	}
	else if (isGlobalHist && isGlobalTable) {
		shared_ptr<list<bool>> global_hist = make_shared<list<bool>>();
		for (int i = 0; i < historySize; i++) {
			global_hist->push_front(0);
		}
		shared_ptr<FSM_Table> global_fsm_table = make_shared<FSM_Table>(pow(2,historySize), fsmState);
		for (int i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, 0, historySize, global_hist, global_fsm_table, false));
		}
	}
	else if (!isGlobalHist && isGlobalTable) {
		shared_ptr<FSM_Table> global_fsm_table = make_shared<FSM_Table>(pow(2,historySize), fsmState);
		for (int i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, 0, historySize, nullptr, global_fsm_table, false));
		}
	}
	else {
		shared_ptr<list<bool>> global_hist = make_shared<list<bool>>();
		for (int i = 0; i < historySize; i++) {
			global_hist->push_front(0);
		}
		for (int i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, 0, historySize, global_hist, nullptr, false));
		}
	}
}

unsigned BTB::calcShared(uint32_t pc, int Shared) {
	if (Shared == 1) {
		return (pc >> 2) & ((unsigned)pow(2, historySize) - 1);
	}
	else if (Shared == 2) {
		return (pc >> 16) & ((unsigned)pow(2, historySize) - 1);
	}
	return 0;
}

unsigned BTB::histToInt(shared_ptr<list<bool>> hist) {
	unsigned histIdx = 0;
	int i;
	list<bool>::iterator histIt;
	for (i = 0, histIt = hist->begin(); histIt != hist->end(); ++histIt, ++i) {
		histIdx += (*histIt) * pow(2,i);
	}
	return histIdx;
}

void BTB::update(uint32_t pc, uint32_t targetPc, bool taken) {
	unsigned *btbIdx = new unsigned;
	unsigned *tagIdx = new unsigned;
	parsePc(pc, btbSize, tagSize, tagIdx, btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[*btbIdx];
	unsigned int fsmEntry = histToInt(targetBranch->hist);

	delete btbIdx;
	delete tagIdx;

	targetBranch->fsmTable->update(fsmEntry ^ calcShared(pc, Shared), taken);
	targetBranch->updateHist(taken);
	targetBranch->targetPc = targetPc;
	targetBranch->pc = pc;
}

bool BTB::predict(uint32_t pc, uint32_t *dst) {
	unsigned *btbIdx = new unsigned;
	unsigned *tagIdx = new unsigned;
	parsePc(pc, btbSize, tagSize, tagIdx, btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[*btbIdx];

	delete btbIdx;
	delete tagIdx;

	unsigned int fsmEntry = histToInt(targetBranch->hist);
	
	if (targetBranch->fsmTable->predict(fsmEntry ^ calcShared(pc, Shared))) {
		*dst = targetBranch->targetPc;
		return true;
	}
	*dst = pc + 4;
	return false;
}

void BTB::insertBranch(uint32_t pc, uint32_t targetPc, bool taken) {
	unsigned *btbIdx = new unsigned;
	unsigned *tagIdx = new unsigned;
	parsePc(pc, btbSize, tagSize, tagIdx, btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[*btbIdx];
	unsigned int fsmEntry = isGlobalHist ? histToInt(targetBranch->hist) : 0;

	if (isGlobalHist && isGlobalTable) {
		targetBranch->used = true;
		targetBranch->tag = *tagIdx;
		targetBranch->targetPc = targetPc;
		targetBranch->pc = pc;
		targetBranch->fsmTable->update(fsmEntry ^ calcShared(pc, Shared), taken);
		targetBranch->updateHist(taken);
	}
	else if (!isGlobalHist && isGlobalTable) {
		shared_ptr<list<bool>> local_hist = make_shared<list<bool>>();
		for (int i = 0; i < historySize; i++) {
			local_hist->push_front(0);
		}
		targetBranch->used = true;
		targetBranch->tag = *tagIdx;
		targetBranch->hist = local_hist;
		targetBranch->targetPc = targetPc;
		targetBranch->pc = pc;
		targetBranch->fsmTable->update(fsmEntry ^ calcShared(pc, Shared), taken);
		targetBranch->updateHist(taken);
	}
	else if (isGlobalHist && !isGlobalTable) {
		targetBranch->used = true;
		targetBranch->tag = *tagIdx;
		targetBranch->targetPc = targetPc;
		targetBranch->pc = pc;
		targetBranch->fsmTable = make_shared<FSM_Table>(pow(2,historySize), fsmState);
		targetBranch->fsmTable->update(fsmEntry ^ calcShared(pc, Shared), taken);
		targetBranch->updateHist(taken);
	}
	else {
		shared_ptr<list<bool>> local_hist = make_shared<list<bool>>();
		for (int i = 0; i < historySize; i++) {
			local_hist->push_front(0);
		}
		targetBranch->used = true;
		targetBranch->tag = *tagIdx;
		targetBranch->hist = local_hist;
		targetBranch->targetPc = targetPc;
		targetBranch->pc = pc;
		targetBranch->fsmTable = make_shared<FSM_Table>(pow(2,historySize), fsmState);
		targetBranch->fsmTable->update(histToInt(targetBranch->hist), taken);
		targetBranch->updateHist(taken);
	}

	delete btbIdx;
	delete tagIdx;
}

bool BTB::exists(uint32_t pc) {
	unsigned *btbIdx = new unsigned;
	unsigned *tagIdx = new unsigned;
	parsePc(pc, btbSize, tagSize, tagIdx, btbIdx);
	shared_ptr<Branch> targetBranch = btbTable[*btbIdx];

	bool exists = targetBranch->used && *tagIdx == targetBranch->tag;

	delete btbIdx;
	delete tagIdx;

	return exists;
}

void BTB::parsePc(uint32_t pc, unsigned btbSize, unsigned tagSize, unsigned *tagIdx, unsigned *btbIdx) {
	*tagIdx = (pc >> (2 + (unsigned)log2(btbSize))) & ((unsigned)pow(2, tagSize) - 1);
	*btbIdx = (pc >> 2) & (btbSize - 1);
}


/*****************************************************Class BP*******************************************************/
class BP {
	shared_ptr<BTB> btb;
	SIM_stats stats;
	unsigned historySize;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;

public:
	BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared);
	bool predict(uint32_t pc, uint32_t *dst);
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	void getStats(SIM_stats *curStats);
};

BP::BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared):
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
	stats.flush_num += taken ? pred_dst != targetPc : pred_dst != pc + 4;
	++stats.br_num;
	if (btb->exists(pc)) {
		btb->update(pc, targetPc, taken);
	}
	else {
		btb->insertBranch(pc, targetPc, taken);
	}

	return;
}

void BP::getStats(SIM_stats *curStats) {
	*curStats = stats;
}

/********************************************************************************************************************/

shared_ptr<BP> bp = nullptr;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared) {
	bp = make_shared<BP>(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	if (bp == nullptr) {
		return -1;
	}
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst) {
	return bp->predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
	bp->update(pc, targetPc, taken, pred_dst);
	return;
}

void BP_GetStats(SIM_stats *curStats) {
	bp->getStats(curStats);
	return;
}

