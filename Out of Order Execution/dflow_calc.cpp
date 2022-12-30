/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <memory>
#include <vector>
using namespace std;

#define ENTRY -1
#define REGS_NUM 32

struct Inst {
    shared_ptr<Inst> parent1;
    shared_ptr<Inst> parent2;
    shared_ptr<InstInfo> info;
    unsigned latency;
    int id;
    unsigned instDepth;

    Inst(shared_ptr<InstInfo> info, unsigned latency, int id, shared_ptr<Inst> parent1, shared_ptr<Inst> parent2);
};

Inst::Inst(shared_ptr<InstInfo> info, unsigned latency, int id, shared_ptr<Inst> parent1, shared_ptr<Inst> parent2):
    info(shared_ptr<InstInfo>(info)), latency(latency), id(id), parent1(parent1), parent2(parent2), instDepth(latency) {}


struct Dflow {
    vector<shared_ptr<Inst>> progInsts;

    Dflow() {}
};



ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    Dflow* dflow = new Dflow();

    if (dflow == NULL) {
        return PROG_CTX_NULL;
    }

    shared_ptr<Inst> entry = make_shared<Inst>(nullptr, 0, ENTRY, nullptr, nullptr);
    shared_ptr<Inst> exit = make_shared<Inst>(nullptr, 0, ENTRY, nullptr, nullptr);

    shared_ptr<Inst> registers[REGS_NUM];
    for (int i = 0; i < REGS_NUM; i++) {
        registers[i] = entry;
    }

    for (int i = 0; i < numOfInsts; i++) {
        shared_ptr<Inst> parent1 = registers[progTrace[i].src1Idx];
        shared_ptr<Inst> parent2 = registers[progTrace[i].src2Idx];

        shared_ptr<Inst> inst = make_shared<Inst>(make_shared<InstInfo>(progTrace[i]), opsLatency[progTrace[i].opcode], i, parent1, parent2);
        if (inst == NULL) {
            return PROG_CTX_NULL;
        }
        inst->instDepth += max(parent1->instDepth, parent2->instDepth);
        registers[progTrace[i].dstIdx] = inst;
        dflow->progInsts.push_back(inst);
    }

    return (void*)(dflow);
}

void freeProgCtx(ProgCtx ctx) {
    delete (Dflow*)ctx;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    Dflow* dflow = (Dflow*)ctx;

    if (dflow == NULL) {
        return -1;
    }

    return dflow->progInsts[theInst]->instDepth - dflow->progInsts[theInst]->latency;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    Dflow* dflow = (Dflow*)ctx;

    if (dflow == NULL) {
        return -1;
    }

    *src1DepInst = dflow->progInsts[theInst]->parent1->id;
    *src2DepInst = dflow->progInsts[theInst]->parent2->id;

    return 0;
}

int getProgDepth(ProgCtx ctx) {
    Dflow* dflow = (Dflow*)ctx;

    if (dflow == NULL) {
        return -1;
    }

    int progDepth = 0;

    for (auto it : dflow->progInsts) {
        progDepth = it->instDepth >= progDepth ? it->instDepth : progDepth;
    }

    return progDepth;
}
