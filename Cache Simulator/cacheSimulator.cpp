#include "cacheSimulator.h"


/*************************************************Class Address*****************************************************/

Address::Address(unsigned long int address, unsigned offsetSize, unsigned setSize, unsigned tagSize) {
    this->address = address;
    offset = address & (offsetSize - 1);
    set = (address >> (uint32_t)log2(offsetSize)) & (setSize - 1);
    tag = (address >> (uint32_t)log2(offsetSize*setSize)) & (tagSize - 1);
}

/*************************************************Class CacheLine****************************************************/

CacheLine::CacheLine(Address address, bool valid, bool dirty): valid(valid), dirty(dirty), address(address) {}

void CacheLine::updateLine(Address address) {
    this->address = address;
    this->valid = true;
    this->dirty = false;
}

void CacheLine::setInvalid() {
    valid = false;
}

/*************************************************Class LRU**********************************************************/

LRU::LRU(unsigned size): size(size) {
    LRU_table = vector<unsigned>();
    for (unsigned i = 0; i < size; i++) {
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

Set::Set(unsigned waysNum): waysNum(waysNum), LRU_table(waysNum) {
    ways = vector<shared_ptr<CacheLine>>();
    for (unsigned i = 0; i < waysNum; i++) {
        ways.push_back(nullptr);
    }
}

void Set::updateLRU(unsigned way) {
    LRU_table.update(way);
}

bool Set::is_full(unsigned* way) {
    for (unsigned i = 0; i < waysNum; i++) {
        if (!(ways[i]->valid)) {
            *way = i;
            return false;
        }
    }
    return true;
}

unsigned Set::getLRUWay() {
    return LRU_table.getLRUWay();
}

/*************************************************Class CacheLevel***************************************************/

void CacheLevel::updateLine(unsigned long int address, unsigned way) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    cacheSetMem[addr.set]->ways[way]->updateLine(addr);
}

bool CacheLevel::is_dirty(unsigned long int address, unsigned way) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    return cacheSetMem[addr.set]->ways[way]->dirty;
}

unsigned long int CacheLevel::getAddr(unsigned long int address, unsigned way) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    return cacheSetMem[addr.set]->ways[way]->address.address;
}

unsigned CacheLevel::getLRUWay(unsigned long int address) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    return cacheSetMem[addr.set]->getLRUWay();
}

bool CacheLevel::is_full(unsigned long int address, unsigned* way) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    return cacheSetMem[addr.set]->is_full(way);
}

void CacheLevel::setInvalid(unsigned long int address, unsigned way) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    cacheSetMem[addr.set]->ways[way]->setInvalid();
}

void CacheLevel::setDirty(unsigned long int address, unsigned way) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    cacheSetMem[addr.set]->ways[way]->dirty = true;
}

void CacheLevel::updateLRU(unsigned long int address, unsigned way) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    cacheSetMem[addr.set]->updateLRU(way);
}

CacheLevel::CacheLevel(unsigned BlockSize, unsigned CacheSize, unsigned AccCyc, unsigned Assoc, unsigned WrAlloc):
    BlockSize(BlockSize), CacheSize(CacheSize), AccCyc(AccCyc), Assoc(Assoc), WrAlloc(WrAlloc), MissCnt(0), AccCnt(0) {
    EntryNum = CacheSize / BlockSize;
    cacheSetMem = vector<shared_ptr<Set>>();
    for (unsigned i = 0; i < (EntryNum/Assoc); i++) {
        cacheSetMem.push_back(make_shared<Set>(Assoc));
    }
    Address addr(0, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));

    for (unsigned i = 0; i < EntryNum; i++) {
        shared_ptr<CacheLine> line = make_shared<CacheLine>(addr, false);
        cacheSetMem[i%(EntryNum/Assoc)]->ways[i/(EntryNum/Assoc)] = line;
    }
}

bool CacheLevel::is_cacheHit(unsigned long int address, unsigned* targetWay) {
    Address addr(address, BlockSize, EntryNum / Assoc, pow(2,(32 - (log2(BlockSize) + log2(EntryNum / Assoc)))));
    shared_ptr<Set> curr_set = cacheSetMem[addr.set];
    for (unsigned i = 0; i < Assoc; i++) {
        if (curr_set->ways[i]->address.tag == addr.tag && curr_set->ways[i]->valid) {
            *targetWay = i;
            return true;
        }
    }
    return false;
}

double CacheLevel::calcMissRate() {
    return AccCnt ? ((double)MissCnt/(double)AccCnt) : 0;
}

/*************************************************Class CacheSim*****************************************************/

CacheSim::CacheSim(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Cyc,
    unsigned L2Cyc, unsigned L1Assoc, unsigned L2Assoc, unsigned WrAlloc): MemCyc(MemCyc),
        L1(make_shared<CacheLevel>(pow(2,BSize), pow(2,L1Size), L1Cyc, pow(2,L1Assoc), WrAlloc))
        ,L2(make_shared<CacheLevel>(pow(2,BSize), pow(2,L2Size), L2Cyc, pow(2,L2Assoc), WrAlloc)) {}

void CacheSim::eviction(CacheLevels level, unsigned targetWayL1, unsigned targetWayL2, unsigned long int address, bool snooping) {
    unsigned long int targetAddr;
    unsigned targetWay = 0;

    if (level == Level1) {
        if (L1->is_dirty(address, targetWayL1)) {
            targetAddr = L1->getAddr(address, targetWayL1);
            L2->is_cacheHit(targetAddr, &targetWay);
            L2->setDirty(targetAddr, targetWay);
            if (snooping == false) {
                L2->updateLRU(targetAddr, targetWay);
            }
        }
        L1->setInvalid(address, targetWayL1);
    }
    
    else if (level == Level2) {
        targetAddr = L2->getAddr(address, targetWayL2);
        if (L1->is_cacheHit(targetAddr, &targetWay)) { // snooping
            eviction(Level1, targetWay, targetWayL2, targetAddr, true);
        }
        L2->setInvalid(address, targetWayL2);
    }
}

void CacheSim::updateLine(char operation, unsigned long int address) {
    unsigned targetWayL1 = L1->getLRUWay(address);
    unsigned targetWayL2 = L2->getLRUWay(address);

    // L1 hit
    if (L1->is_cacheHit(address, &targetWayL1)) {
        L1->updateLRU(address, targetWayL1);
        // Write
        if (operation == 'w') {
            L1->setDirty(address, targetWayL1);
        }
    }
    
    // L1 miss
    else {
        L1->MissCnt++;

        // L2 hit
        if (L2->is_cacheHit(address, &targetWayL2)) {
            L2->updateLRU(address, targetWayL2);
            // Write
            if (operation == 'w') {
                if (L1->WrAlloc) {
                    if (L1->is_full(address, &targetWayL1)) {
                        eviction(Level1, targetWayL1, targetWayL2, address);
                    }
                    L1->updateLine(address, targetWayL1);
                    L1->setDirty(address, targetWayL1);
                    L1->updateLRU(address, targetWayL1);
                }
                else {
                    L2->setDirty(address, targetWayL2);
                }
            }
            // Read
            else {
                if (L1->is_full(address, &targetWayL1)) {
                    eviction(Level1, targetWayL1, targetWayL2, address);
                }
                L1->updateLine(address, targetWayL1);
                L1->updateLRU(address, targetWayL1);
            }
        }

        // L2 miss
        else {
            L2->MissCnt++;
            // Write
            if (operation == 'w') {
                if (L2->WrAlloc) {
                    if (L1->WrAlloc) {
                        if (L2->is_full(address, &targetWayL2)) {
                            eviction(Level2, targetWayL1, targetWayL2, address);
                        }
                        L2->updateLine(address, targetWayL2);
                        L2->updateLRU(address, targetWayL2);
                        if (L1->is_full(address, &targetWayL1)) {
                            eviction(Level1, targetWayL1, targetWayL2, address);
                        }
                        L1->updateLine(address, targetWayL1);
                        L1->setDirty(address, targetWayL1);
                        L1->updateLRU(address, targetWayL1);
                    }
                    else {
                        if (L2->is_full(address, &targetWayL2)) {
                            eviction(Level2, targetWayL1, targetWayL2, address);
                        }
                        L2->updateLine(address, targetWayL2);
                        L2->setDirty(address, targetWayL2);
                        L2->updateLRU(address, targetWayL2);
                    }
                }
            }
            // Read
            else {
                if (L2->is_full(address, &targetWayL2)) {
                    eviction(Level2, targetWayL1, targetWayL2, address);
                }
                L2->updateLine(address, targetWayL2);
                L2->updateLRU(address, targetWayL2);
                if (L1->is_full(address, &targetWayL1)) {
                    eviction(Level1, targetWayL1, targetWayL2, address);
                }
                L1->updateLine(address, targetWayL1);
                L1->updateLRU(address, targetWayL1);
            }
        }

        L2->AccCnt++;
    }
    
    L1->AccCnt++;
}

void CacheSim::getStats(double* L1MissRate, double* L2MissRate, double* avgAccTime) {
    *L1MissRate = L1->calcMissRate();
    *L2MissRate = L2->calcMissRate();
    *avgAccTime = (1 - (*L1MissRate))*L1->AccCyc + (*L1MissRate)*(L1->AccCyc + (1 - (*L2MissRate))*L2->AccCyc + (*L2MissRate)*(L2->AccCyc + MemCyc));
}
