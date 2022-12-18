#ifndef CACHE_SIMULATOR_H_
#define CACHE_SIMULATOR_H_

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

/*************************************************Class CacheLine****************************************************/

struct CacheLine {
    bool valid;
    bool dirty;
    Address address;

    CacheLine(Address address, bool valid = true, bool dirty = false);
    void updateLine(Address address);
    void setInvalid();
};

/*************************************************Class LRU**********************************************************/

class LRU {
    unsigned size;
    vector<unsigned> LRU_table;

public:
    LRU(unsigned size);
    void update(unsigned way);
    unsigned getLRUWay();
};

/*************************************************Class Set**********************************************************/

struct Set {
    unsigned waysNum;
    shared_ptr<LRU> LRU_table;
    vector<shared_ptr<CacheLine>> ways;

    Set(unsigned waysNum);
    void updateLRU(unsigned way);
    bool is_full(unsigned* way);
    unsigned getLRUWay();
};

/*************************************************Class CacheLevel***************************************************/

class CacheLevel {
    unsigned BlockSize;
    unsigned CacheSize;
    unsigned AccCyc;
    unsigned Assoc;
    unsigned WrAlloc;
    unsigned MissCnt;
    unsigned AccCnt;
    unsigned EntryNum;
    vector<shared_ptr<CacheLine>> cacheMem;
    vector<shared_ptr<Set>> cacheSetMem;

    friend class CacheSim;

public:
    CacheLevel(unsigned BlockSize, unsigned CacheSize, unsigned AccCyc, unsigned Assoc, unsigned WrAlloc);
    bool is_cacheHit(unsigned long int address, unsigned* targetWay);
    double calcMissRate();
    bool is_dirty(unsigned long int address, unsigned way);
    void setDirty(unsigned long int address, unsigned way);
    void updateLRU(unsigned long int address, unsigned way);
    void setInvalid(unsigned long int address, unsigned way);
    bool is_full(unsigned long int address, unsigned* way);
    unsigned getLRUWay(unsigned long int address);
    void updateLine(unsigned long int address, unsigned way);
    unsigned long int getAddr(unsigned long int address, unsigned way);
    unsigned calcAccTime();
};

/*************************************************Class CacheSim*****************************************************/

class CacheSim {
    typedef enum {Level1, Level2} CacheLevels; 

    shared_ptr<CacheLevel> L1;
    shared_ptr<CacheLevel> L2;
    unsigned MemCyc;
    unsigned MemAccCnt;

public:
    CacheSim(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Cyc,
    unsigned L2Cyc, unsigned L1Assoc, unsigned L2Assoc, unsigned WrAlloc);
    void updateLine(char operation, unsigned long int address);
    void getStats(double* L1MissRate, double* L2MissRate, double* avgAccTime);
    void eviction(CacheLevels level, unsigned* targetWayL1, unsigned* targetWayL2, unsigned long int address);
};


#endif /* CACHE_SIMULATOR_H_ */