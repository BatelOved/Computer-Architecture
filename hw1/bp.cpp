/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
using namespace std;



/*************************************************Class FSM_Table***************************************************/
class FSM_Table {
	class FSM {
		unsigned fsmState;
		enum {SNT = 0, WNT = 1, WT = 2, ST = 3};

	public:
		FSM(unsigned fsmState = WT): fsmState(fsmState) {}
		bool predict() { return (fsmState == WT || fsmState == ST); }
	};

	vector<shared_ptr<FSM>> fsmTable;
	unsigned fsmTableSize;

public:
	FSM_Table(unsigned fsmTableSize, unsigned fsmState): fsmTableSize(fsmTableSize) {
		for (int i = 0; i < fsmTableSize; i++) {
			fsmTable.push_back(make_shared<FSM>(fsmState));
		}
	}

	~FSM_Table() {
		for (int i = 0; i < fsmTableSize; i++) {
			fsmTable.pop_back();
		}
	}

	bool predict(int histEntry) {
		return fsmTable[histEntry]->predict();
	}
};


/***************************************************Class BTB******************************************************/
class BTB {
public:
	struct Branch {
		uint32_t pc;
		uint32_t targetPc;
		unsigned historySize;
		shared_ptr<vector<bool>> hist;
		shared_ptr<FSM_Table> fsmTable;

	public:
		Branch(uint32_t pc, uint32_t targetPc, unsigned historySize, shared_ptr<vector<bool>> hist, shared_ptr<FSM_Table> fsmTable): 
					pc(pc), targetPc(targetPc), historySize(historySize), hist(hist), fsmTable(fsmTable) {}
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

public:
	BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared);
	bool exists(uint32_t pc,  shared_ptr<Branch> targetBranch);
	void parsePc(uint32_t pc, unsigned btbSize, unsigned tagSize, unsigned *tagIdx, unsigned *btbIdx);
};

BTB::BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared):
	btbSize(btbSize), historySize(historySize), tagSize(tagSize), fsmState(fsmState), isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), Shared(Shared) {
	if (!isGlobalHist && !isGlobalTable) {
		for (int i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, historySize, make_shared<vector<bool>>(), make_shared<FSM_Table>(historySize, fsmState)));
		}
	}
	else if (isGlobalHist && isGlobalTable) {
		shared_ptr<vector<bool>> global_hist = make_shared<vector<bool>>();
		shared_ptr<FSM_Table> global_fsm_table = make_shared<FSM_Table>(historySize, fsmState);
		for (int i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, historySize, global_hist, global_fsm_table));
		}
	}
	else if (!isGlobalHist && isGlobalTable) {
		shared_ptr<FSM_Table> global_fsm_table = make_shared<FSM_Table>(historySize, fsmState);
		for (int i = 0; i < btbSize; i++) {
			btbTable.push_back(make_shared<Branch>(0, 0, historySize, make_shared<vector<bool>>(), global_fsm_table));
		}
	}
	else {
		printf("ERROR!!!");
	}
}

bool BTB::exists(uint32_t pc, shared_ptr<BTB::Branch> targetBranch) {
	for (auto brPtr : btbTable) {
		if (pc == brPtr->pc) {
			targetBranch = brPtr;

			return true;
		}
	}
	return false;
}

void BTB::parsePc(uint32_t pc, unsigned btbSize, unsigned tagSize, unsigned *tagIdx, unsigned *btbIdx) {
	*btbIdx = (pc >> 2) & (btbSize - 1);
	*tagIdx = (pc >> (2 + (unsigned)log2(btbSize))) & (tagSize - 1);
}


/*****************************************************Class BP*******************************************************/
class BP {
	shared_ptr<BTB> btb;

	unsigned historySize;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;

	int histToInt(shared_ptr<vector<bool>> hist);

public:
	BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared);
	bool predict(uint32_t pc, uint32_t *dst);
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
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
}

int BP::histToInt(shared_ptr<vector<bool>> hist) {
	int fsmEntry = 0;
	for (int i = 0; i < historySize; i++) {
		fsmEntry += (*hist)[i] * pow(2,i);
	}
	return fsmEntry;
}

bool BP::predict(uint32_t pc, uint32_t *dst) {
	shared_ptr<BTB::Branch> targetBranch;
	if (btb->exists(pc, targetBranch)) {
		if (targetBranch->fsmTable->predict(histToInt(targetBranch->hist))) {
			*dst = targetBranch->targetPc;
			return true;
		}
	}
	*dst = pc + 4;

	return false;
}

void BP::update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
	shared_ptr<BTB::Branch> targetBranch;
	if (btb->exists(pc, targetBranch)) {
		// update hist
		// update fsm
	}

	return;
}

/********************************************************************************************************************/

shared_ptr<BP> bp = nullptr;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	bp = make_shared<BP>(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	if (bp == nullptr) {
		return -1;
	}
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return bp->predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	bp->update(pc, targetPc, taken, pred_dst);
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}
