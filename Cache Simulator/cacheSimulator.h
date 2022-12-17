#include <memory>
#include <vector>
#include <math.h>
using namespace std;

/*************************************************Class Address*****************************************************/

struct Address {
    unsigned long int address;
    unsigned offset;
    unsigned set;
    unsigned tag;

    Address(unsigned long int address, unsigned offsetSize, unsigned setSize, unsigned tagSize);
};

Address::Address(unsigned long int address, unsigned offsetSize, unsigned setSize, unsigned tagSize) {
    this->address = address;
    offset = address & (offsetSize - 1);
    set = (address >> (uint32_t)log2(offsetSize)) & (setSize - 1);
    tag = (address >> (uint32_t)log2(offsetSize*setSize)) & (tagSize - 1);
}

/*************************************************Class cacheLine****************************************************/

struct cacheLine {
    bool valid;
    bool dirty;
    Address address;

public:
    cacheLine(Address address, bool valid = true, bool dirty = false);
    void setDirty(bool dirty);
    void update(Address address);
    void setInvalid();
};

cacheLine::cacheLine(Address address, bool valid, bool dirty): valid(valid), dirty(dirty), address(address) {}

void cacheLine::setDirty(bool dirty) {
    this->dirty = dirty;
}

void cacheLine::update(Address address) {
    this->address = address;
    this->valid = true;
    this->dirty = false;
}

void cacheLine::setInvalid() {
    valid = false;
}

/*************************************************Class LRU**********************************************************/

class LRU {
    unsigned size;
    vector<unsigned> LRU_table;

public:
    LRU(unsigned size);
    void update(unsigned way);
    unsigned getLRUWay();
};

LRU::LRU(unsigned size): size(size) {
    LRU_table = vector<unsigned>();
    for (int i = 0; i < size; i++) {
        LRU_table.push_back(0);
    }
}

void LRU::update(unsigned way) { 
    unsigned prev_cnt = LRU_table[way];
    for (unsigned i = 0; i < size; i++) {
        LRU_table[i] = LRU_table[i] > prev_cnt ? (LRU_table[i] - 1) : LRU_table[i];
    }
    LRU_table[way] = size-1;
}

unsigned LRU::getLRUWay() {
    unsigned way = 0;
    for (unsigned i = 0; i < size; i++) {
        way = LRU_table[i] < LRU_table[way] ? i : way;
    }
    return way;
}


/*************************************************Class Set**********************************************************/

struct Set {
    unsigned waysNum;
    shared_ptr<LRU> LRU_table;
    vector<shared_ptr<cacheLine>> ways;

    Set(unsigned waysNum);
    void updateLRU(unsigned way);
    bool is_full(unsigned* way);
    unsigned getLRUWay();
};

Set::Set(unsigned waysNum): waysNum(waysNum) {
    LRU_table = make_shared<LRU>(waysNum);
    ways = vector<shared_ptr<cacheLine>>();
    for (int i = 0; i < waysNum; i++) {
        ways.push_back(nullptr);
    }
}

void Set::updateLRU(unsigned way) {
    LRU_table->update(way);
}

bool Set::is_full(unsigned* way) {
    for (int i = 0; i < waysNum; i++) {
        if (!(ways[i]->valid)) {
            *way = i;
            return false;
        }
    }
    return true;
}

unsigned Set::getLRUWay() {
    return LRU_table->getLRUWay();
}

/*************************************************Class cacheLevel***************************************************/

class cacheLevel {
    unsigned blockSize;
    unsigned cacheSize;
    unsigned cycNum;
    unsigned cacheAssoc;
    unsigned WrAlloc;
    
    unsigned missCnt;
    unsigned accessCnt;
    unsigned entryNum;
    vector<shared_ptr<cacheLine>> cacheMem;
    vector<shared_ptr<Set>> cacheSetMem;

    friend class cacheSim;

public:
    cacheLevel(unsigned blockSize, unsigned cacheSize, unsigned cycNum, unsigned cacheAssoc, unsigned WrAlloc);
    bool is_cacheHit(Address address, unsigned* targetWay);
    double calcMissRate();
};

cacheLevel::cacheLevel(unsigned blockSize, unsigned cacheSize, unsigned cycNum, unsigned cacheAssoc, unsigned WrAlloc):
    blockSize(blockSize), cacheSize(cacheSize), cycNum(cycNum), cacheAssoc(cacheAssoc), WrAlloc(WrAlloc), missCnt(0), accessCnt(0) {
    entryNum = cacheSize / blockSize;
    cacheSetMem = vector<shared_ptr<Set>>();
    for (int i = 0; i < (entryNum/cacheAssoc); i++) {
        cacheSetMem.push_back(make_shared<Set>(cacheAssoc));
    }
    cacheMem = vector<shared_ptr<cacheLine>>();
    Address addr(0, blockSize, entryNum / cacheAssoc, pow(2,(32 - (log2(blockSize) + log2(entryNum / cacheAssoc)))));
    bool fullyAssoc = false;
    bool directMap = false;
    if (cacheAssoc >= entryNum) {
        fullyAssoc = true;
    }
    if (cacheAssoc == 1) {
        directMap = true;
    }
    for (int i = 0; i < entryNum; i++) {
        shared_ptr<cacheLine> line = make_shared<cacheLine>(addr, false);
        cacheMem.push_back(line);

        if (fullyAssoc || directMap) {
            cacheSetMem[i/cacheAssoc]->ways[i%cacheAssoc] = line;
        }
        else {
            cacheSetMem[i%cacheAssoc]->ways[i/cacheAssoc] = line;
        }
    }
}

bool cacheLevel::is_cacheHit(Address address, unsigned* targetWay) {
    shared_ptr<Set> curr_set = cacheSetMem[address.set];
    for (int i = 0; i < cacheAssoc; i++) {
        if (curr_set->ways[i]->address.tag == address.tag && curr_set->ways[i]->valid) {
            *targetWay = i;
            return true;
        }
    }
    return false;
}

double cacheLevel::calcMissRate() {
    return accessCnt ? ((double)missCnt/(double)accessCnt) : 0;
}

/*************************************************Class cacheSim*****************************************************/

class cacheSim {
    typedef enum {Level1, Level2} cacheLevels; 

    shared_ptr<cacheLevel> L1;
    shared_ptr<cacheLevel> L2;
    unsigned MemCyc;

public:
    cacheSim(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Cyc,
    unsigned L2Cyc, unsigned L1Assoc, unsigned L2Assoc, unsigned WrAlloc);
    void cacheSim_update(char operation, unsigned long int address);
    void cacheSim_GetStats(double* L1MissRate, double* L2MissRate, double* avgAccTime);
    void eviction(cacheLevels level, unsigned* targetWayL1, unsigned* targetWayL2, Address addr1, Address addr2);
};

cacheSim::cacheSim(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Cyc,
    unsigned L2Cyc, unsigned L1Assoc, unsigned L2Assoc, unsigned WrAlloc): MemCyc(MemCyc),
        L1(make_shared<cacheLevel>(pow(2,BSize), pow(2,L1Size), L1Cyc, pow(2,L1Assoc), WrAlloc))
        ,L2(make_shared<cacheLevel>(pow(2,BSize), pow(2,L2Size), L2Cyc, pow(2,L2Assoc), WrAlloc)) {}

void cacheSim::eviction(cacheLevels level, unsigned* targetWayL1, unsigned* targetWayL2, Address addr1, Address addr2) {
    if (level == Level1) {
        if (L1->cacheSetMem[addr1.set]->ways[*targetWayL1]->dirty) {
            unsigned long int targetAddr = L1->cacheSetMem[addr1.set]->ways[*targetWayL1]->address.address;
            Address dirtyAddr2(targetAddr, L2->blockSize, L2->entryNum / L2->cacheAssoc, pow(2,(32 - (log2(L2->blockSize) + log2(L2->entryNum / L2->cacheAssoc)))));
            unsigned dirtyTargetWayL2 = 0;
            L2->is_cacheHit(dirtyAddr2, &dirtyTargetWayL2);
            L2->cacheSetMem[addr2.set]->ways[dirtyTargetWayL2]->setDirty(true);
            L2->cacheSetMem[addr2.set]->updateLRU(dirtyTargetWayL2);
        }
        L1->cacheSetMem[addr1.set]->ways[*targetWayL1]->setInvalid();
    }
    
    else if (level == Level2) {
        L1->cacheSetMem[addr1.set]->is_full(targetWayL1);
        eviction(Level1, targetWayL1, targetWayL2, addr1, addr2);
        unsigned long int targetAddr = L1->cacheSetMem[addr1.set]->ways[*targetWayL1]->address.address;
        Address dirtyAddr2(targetAddr, L2->blockSize, L2->entryNum / L2->cacheAssoc, pow(2,(32 - (log2(L2->blockSize) + log2(L2->entryNum / L2->cacheAssoc)))));
        unsigned dirtyTargetWayL2 = 0;
        L2->is_cacheHit(dirtyAddr2, &dirtyTargetWayL2);
        if (L2->cacheSetMem[addr2.set]->ways[dirtyTargetWayL2]->dirty) {
            // cyc time to main memory
        }
        L2->cacheSetMem[addr2.set]->ways[dirtyTargetWayL2]->setInvalid();
    }
}

void cacheSim::cacheSim_update(char operation, unsigned long int address) {
    Address addr1(address, L1->blockSize, L1->entryNum / L1->cacheAssoc, pow(2,(32 - (log2(L1->blockSize) + log2(L1->entryNum / L1->cacheAssoc)))));
    Address addr2(address, L2->blockSize, L2->entryNum / L2->cacheAssoc, pow(2,(32 - (log2(L2->blockSize) + log2(L2->entryNum / L2->cacheAssoc)))));
    unsigned targetWayL1 = L1->cacheSetMem[addr1.set]->getLRUWay();
    unsigned targetWayL2 = L2->cacheSetMem[addr2.set]->getLRUWay();
    // L1 hit
    if (L1->is_cacheHit(addr1, &targetWayL1)) {
        cout << " L1 hit " << endl;
        L1->cacheSetMem[addr1.set]->updateLRU(targetWayL1);
        // Read or Write
        if (operation == 'w') {
            L1->cacheSetMem[addr1.set]->ways[targetWayL1]->setDirty(true);
        }
    }
    
    // L1 miss
    else {
        L1->missCnt++;

        // L2 hit
        if (L2->is_cacheHit(addr2, &targetWayL2)) {
            cout << " L1 miss & L2 hit " << endl;
            L2->cacheSetMem[addr2.set]->updateLRU(targetWayL2);
            // Write
            if (operation == 'w') {
                if (L1->WrAlloc) {
                    if (L1->cacheSetMem[addr1.set]->is_full(&targetWayL1)) {
                        eviction(Level1, &targetWayL1, &targetWayL2, addr1, addr2);
                    }
                    L1->cacheSetMem[addr1.set]->ways[targetWayL1]->update(addr1);
                    L1->cacheSetMem[addr1.set]->ways[targetWayL1]->setDirty(true);
                    L1->cacheSetMem[addr1.set]->updateLRU(targetWayL1);
                }
                else {
                    L2->cacheSetMem[addr2.set]->ways[targetWayL2]->setDirty(true);
                }
            }
            // Read
            else {
                if (L1->cacheSetMem[addr1.set]->is_full(&targetWayL1)) {
                    eviction(Level1, &targetWayL1, &targetWayL2, addr1, addr2);
                }
                L1->cacheSetMem[addr1.set]->ways[targetWayL1]->update(addr1);
                L1->cacheSetMem[addr1.set]->updateLRU(targetWayL1);
            }
        }

        // L2 miss
        else {
            cout << " L1 miss & L2 miss " << endl;
            L2->missCnt++;
            // Write
            if (operation == 'w') {
                if (L2->WrAlloc) {
                    if (L1->WrAlloc) {
                        if (L2->cacheSetMem[addr2.set]->is_full(&targetWayL2)) {
                            eviction(Level2, &targetWayL1, &targetWayL2, addr1, addr2);
                        }
                        else if (L1->cacheSetMem[addr1.set]->is_full(&targetWayL1)) {
                            eviction(Level1, &targetWayL1, &targetWayL2, addr1, addr2);
                        }
                        L2->cacheSetMem[addr2.set]->ways[targetWayL2]->update(addr2);
                        L2->cacheSetMem[addr2.set]->updateLRU(targetWayL2);
                        L1->cacheSetMem[addr1.set]->ways[targetWayL1]->update(addr1);
                        L1->cacheSetMem[addr1.set]->updateLRU(targetWayL1);
                        L1->cacheSetMem[addr1.set]->ways[targetWayL1]->setDirty(true);
                    }
                    else {
                        if (L2->cacheSetMem[addr2.set]->is_full(&targetWayL2)) {
                            eviction(Level2, &targetWayL1, &targetWayL2, addr1, addr2);
                        }
                        L2->cacheSetMem[addr2.set]->ways[targetWayL2]->update(addr2);
                        L2->cacheSetMem[addr2.set]->ways[targetWayL2]->setDirty(true);
                        L2->cacheSetMem[addr2.set]->updateLRU(targetWayL2);
                    }
                }
                
                else {
                    // cyc time to memory
                }
            }
            // Read
            else {
                if (L2->cacheSetMem[addr2.set]->is_full(&targetWayL2)) {
                    eviction(Level2, &targetWayL1, &targetWayL2, addr1, addr2);
                }
                else if (L1->cacheSetMem[addr1.set]->is_full(&targetWayL1)) {
                    eviction(Level1, &targetWayL1, &targetWayL2, addr1, addr2);
                }
                L2->cacheSetMem[addr2.set]->ways[targetWayL2]->update(addr2);
                L2->cacheSetMem[addr2.set]->updateLRU(targetWayL2);
                L1->cacheSetMem[addr1.set]->ways[targetWayL1]->update(addr1);
                L1->cacheSetMem[addr1.set]->updateLRU(targetWayL1);
            }
        }
        
        L2->accessCnt++;
    }
    
    L1->accessCnt++;
}

void cacheSim::cacheSim_GetStats(double* L1MissRate, double* L2MissRate, double* avgAccTime) {
    *L1MissRate = L1->calcMissRate();
    *L2MissRate = L2->calcMissRate();
    // avgAccTime = ;
}