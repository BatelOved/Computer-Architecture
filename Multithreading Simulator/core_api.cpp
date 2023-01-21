/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>
#include <memory>
#include <list>
using namespace std;


/**************************************************Class Thread***************************************************/

class Thread {
public:
    typedef enum {THREAD_ACTIVE, THREAD_FINISHED, THREAD_WAITING} STATUS;

private:
    int id;
    tcontext regs;
    vector<Instruction> instructions;
    int curr_inst_idx;
    STATUS status;
    int remaining_cycles;
    int total_cycles;
    int load_latency;
    int store_latency;

public:
    Thread(int id);
    int GetInstNum();
    STATUS GetThreadStatus();
    void ExecuteCommand(Instruction* inst = NULL);
    void InsertInst(Instruction inst);
    tcontext GetThreadRegs();
    int GetTotalCycles();
    void UpdateRemainingCyc(int cycles);
    int GetRemainingCyc();
};

Thread::Thread(int id): id(id), curr_inst_idx(0), status(THREAD_ACTIVE), remaining_cycles(0), total_cycles(0) {
    load_latency  = SIM_GetLoadLat();
    store_latency = SIM_GetStoreLat();
    for (int i = 0; i < REGS_COUNT; i++) {
        regs.reg[i] = 0;
    }
}

int Thread::GetRemainingCyc() {
    return remaining_cycles;
}

void Thread::UpdateRemainingCyc(int cycles) {
    remaining_cycles = (remaining_cycles >= cycles ? remaining_cycles-cycles : 0);
    if (remaining_cycles == 0) {
        status = Thread::THREAD_ACTIVE;
        ++curr_inst_idx;
    }
}

int Thread::GetInstNum() {
    return instructions.size();
}

int Thread::GetTotalCycles() {
    return total_cycles;
}

tcontext Thread::GetThreadRegs() {
    return regs;
}

Thread::STATUS Thread::GetThreadStatus() {
    return status;
}

void Thread::InsertInst(Instruction inst) {
    instructions.push_back(inst);
}

void Thread::ExecuteCommand(Instruction* inst) {
    if (inst == NULL) {
        inst = &instructions[curr_inst_idx];
    }
    switch(inst->opcode) {
        case CMD_ADD:
            regs.reg[inst->dst_index] = regs.reg[inst->src1_index] + regs.reg[inst->src2_index_imm];
            status = Thread::THREAD_ACTIVE;
            ++curr_inst_idx;
            break;

        case CMD_SUB:
            regs.reg[inst->dst_index] = regs.reg[inst->src1_index] - regs.reg[inst->src2_index_imm];
            status = Thread::THREAD_ACTIVE;
            ++curr_inst_idx;
            break;

        case CMD_ADDI:
            regs.reg[inst->dst_index] = regs.reg[inst->src1_index] + inst->src2_index_imm;
            status = Thread::THREAD_ACTIVE;
            ++curr_inst_idx;
            break;

        case CMD_SUBI:
            regs.reg[inst->dst_index] = regs.reg[inst->src1_index] - inst->src2_index_imm;
            status = Thread::THREAD_ACTIVE;
            ++curr_inst_idx;
            break;

        case CMD_LOAD:
            SIM_MemDataRead(regs.reg[inst->src1_index] + (inst->isSrc2Imm ? inst->src2_index_imm : regs.reg[inst->src2_index_imm]), &regs.reg[inst->dst_index]);
            status = Thread::THREAD_WAITING;
            remaining_cycles = load_latency;
            break;

        case CMD_STORE:
            SIM_MemDataWrite(regs.reg[inst->dst_index] + (inst->isSrc2Imm ? inst->src2_index_imm : regs.reg[inst->src2_index_imm]), regs.reg[inst->src1_index]);
            status = Thread::THREAD_WAITING;
            remaining_cycles = store_latency;
            break;

        case CMD_NOP:
            break;

        case CMD_HALT:
            status = Thread::THREAD_FINISHED;
            break;

        default:
            status = Thread::THREAD_ACTIVE;
            break;
    }
    ++total_cycles;
}

/**************************************************Class MT_Sim****************************************************/

class MT_Sim {
protected:
    vector<shared_ptr<Thread>> threads;
    int threads_num;
    int switch_cycles;
    int curr_thread;
    int switch_num;

public:
    typedef enum {SUCCESS, FAILURE} STATUS;

    MT_Sim();
    void InitThreads();
    double CPI();
    Thread::STATUS GetThreadStatus(int thread_id = -1);
    int GetThreadsNum();
    MT_Sim::STATUS ContextSwitch();
    tcontext GetThreadRegs(int thread_id);
    void UpdateRemainingCyc(int cycles);
    int GetThreadsDone();
    void InsertNOP();
    void CORE_MT();
    virtual void ExecuteThread(int thread_id = -1) = 0;
};

MT_Sim::MT_Sim(): switch_cycles(0), curr_thread(0), switch_num(0) {
    threads_num = SIM_GetThreadsNum();

    for (int i = 0; i < threads_num; i++) {
        threads.push_back(make_shared<Thread>(i));
    }
}

void MT_Sim::UpdateRemainingCyc(int cycles) {
    for (int i = 0; i < threads_num; i++) {
        if (threads[i]->GetThreadStatus() == Thread::THREAD_WAITING) {
            threads[i]->UpdateRemainingCyc(cycles);
        }
    }
}

int MT_Sim::GetThreadsDone() {
    int threads_done = 0;
    for (int i = 0; i < threads_num; i++) {
        if (threads[i]->GetThreadStatus() == Thread::THREAD_FINISHED) {
            ++threads_done;
        }
    }
    return threads_done;
}

int MT_Sim::GetThreadsNum() {
    return threads_num;
}

tcontext MT_Sim::GetThreadRegs(int thread_id) {
    return threads[thread_id]->GetThreadRegs();
}

void MT_Sim::InsertNOP() {
    Instruction inst;
    inst.opcode = CMD_NOP;
    UpdateRemainingCyc(1);
    threads[curr_thread]->ExecuteCommand(&inst);
}

void MT_Sim::InitThreads() {
    for (int i = 0; i < threads_num; i++) {
        Instruction inst;
        uint32_t addr = 0;
        while(1) {
            SIM_MemInstRead(addr, &inst, i);
            threads[i]->InsertInst(inst);
            if (inst.opcode == CMD_HALT) {
                break;
            }
            addr += 1;
        }
    }
}

double MT_Sim::CPI() {
    int total_sim_cycles = 0, total_sim_insts = 0;
    for (int i = 0; i < threads_num; i++) {
        total_sim_cycles += threads[i]->GetTotalCycles();
        total_sim_insts  += threads[i]->GetInstNum();
    }
    total_sim_cycles += switch_num * switch_cycles;
    return total_sim_insts > 0 ? (double)total_sim_cycles/(double)total_sim_insts : -1;
}

Thread::STATUS MT_Sim::GetThreadStatus(int thread_id) {
    if (thread_id == -1) {
        thread_id = curr_thread;
    }
    return threads[thread_id]->GetThreadStatus();
}

MT_Sim::STATUS MT_Sim::ContextSwitch() {
    for (int i = 0; i < threads_num-1; i++) {
        if (threads[(curr_thread+1+i)%threads_num]->GetThreadStatus() == Thread::THREAD_ACTIVE) {
            curr_thread = (curr_thread+1+i)%threads_num;
            UpdateRemainingCyc(switch_cycles);
            ++switch_num;
            return SUCCESS;
        }
    }
    if (threads[curr_thread]->GetThreadStatus() == Thread::THREAD_ACTIVE) {
        return SUCCESS;
    }
    return FAILURE;
}

void MT_Sim::CORE_MT() {
    InitThreads();

    while (1) {
        ExecuteThread();
        if (GetThreadsDone() == GetThreadsNum()) {
            break;
        }
        while (ContextSwitch() == MT_Sim::FAILURE) {
            InsertNOP();
            if (GetThreadStatus() == Thread::THREAD_ACTIVE) {
                break;
            }
        }
    }
}

/**********************************************Class BlockedMT_Sim*************************************************/

class BlockedMT_Sim : public MT_Sim {
public:
    BlockedMT_Sim();
    void ExecuteThread(int thread_id = -1);
};

BlockedMT_Sim::BlockedMT_Sim(): MT_Sim() { switch_cycles = SIM_GetSwitchCycles(); }

void BlockedMT_Sim::ExecuteThread(int thread_id) {
    if (thread_id == -1) {
        thread_id = curr_thread;
    }
    while (threads[thread_id]->GetThreadStatus() == Thread::THREAD_ACTIVE) {
        UpdateRemainingCyc(1);
        threads[thread_id]->ExecuteCommand();
    }
}

/*********************************************Class FinegrainedMT_Sim**********************************************/

class FinegrainedMT_Sim : public MT_Sim {
public:
    FinegrainedMT_Sim();
    void ExecuteThread(int thread_id = -1);
};

FinegrainedMT_Sim::FinegrainedMT_Sim(): MT_Sim() { switch_cycles = 0; }

void FinegrainedMT_Sim::ExecuteThread(int thread_id) {
    if (thread_id == -1) {
        thread_id = curr_thread;
    }
    if (threads[thread_id]->GetThreadStatus() == Thread::THREAD_ACTIVE) {
        UpdateRemainingCyc(1);
        threads[thread_id]->ExecuteCommand();
    }
}

/************************************************CORE MT Functions**************************************************/

BlockedMT_Sim     BlockedMT_sim;
FinegrainedMT_Sim FinegrainedMT_sim;

void CORE_BlockedMT() {
    BlockedMT_sim = BlockedMT_Sim();
    BlockedMT_sim.CORE_MT();
}

void CORE_FinegrainedMT() {
    FinegrainedMT_sim = FinegrainedMT_Sim();
    FinegrainedMT_sim.CORE_MT();
}

double CORE_BlockedMT_CPI() {
    return BlockedMT_sim.CPI();
}

double CORE_FinegrainedMT_CPI() {
    return FinegrainedMT_sim.CPI();
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
    context[threadid] = BlockedMT_sim.GetThreadRegs(threadid);
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
    context[threadid] = FinegrainedMT_sim.GetThreadRegs(threadid);
}
