#include <vector>
#include <numeric>
#include <random>
#include <utility>
#include <algorithm>
#include <set>
#include <map>
#include <iostream>
#include <cstring>

using std::vector;
using std::pair;
using std::set;
using std::map;
using std::string;

//typedef unsigned long long uint64_t;

class MyBitSet {
public:
    MyBitSet(): siz(0), cap(0) {
        bitmap = (std::nullptr_t) nullptr;
    }
    MyBitSet(long long _siz): siz(_siz) {
        cap = (_siz + 63) >> 6;
        bitmap = new uint64_t[cap]{0};
    }
    MyBitSet(const MyBitSet& bs): siz(bs.siz), cap(bs.cap) {
        bitmap = new uint64_t[bs.cap]{0};
        memcpy(bitmap, bs.bitmap, sizeof(uint64_t) * cap);
    }
    ~MyBitSet(){  }

    void Free() {
        delete [] bitmap;
    }

    MyBitSet& operator = (const MyBitSet &bs) {
        if (this == &bs) return *this;
        delete [] bitmap;
        siz = bs.siz;
        cap = bs.cap;
        bitmap = new uint64_t[cap]{0};
        memcpy(bitmap, bs.bitmap, sizeof(uint64_t) * cap);
        return *this;
    }

    MyBitSet& operator |= (const MyBitSet &bs) {
        int len = cap < bs.cap ? cap : bs.cap;
        for(int i = 0; i < len; i ++)
            bitmap[i] |= bs.bitmap[i];
        return *this;
    }

    MyBitSet& operator %= (const MyBitSet &bs) { // |= ~bs
        int len = cap < bs.cap ? cap : bs.cap;
        for(int i = 0; i < len; i ++)
            bitmap[i] |= ~bs.bitmap[i];
        return *this;
    }

    bool set(int idx) {
        uint64_t& t1 = bitmap[idx >> 6];
        uint64_t cg = (1ull << (idx & 63));
        if ((t1 & cg) == 0ull){
            t1 |= cg;
            return true;
        }
        return false;
    }

    bool set2(int idx) {
        uint64_t& t1 = bitmap[idx >> 6];
        uint64_t cg = (1ull << (idx & 63));
        if ((t1 & cg) == 0ull){
            t1 |= cg;
            return true;
        }
        return false;
    }

    bool unset(int idx) {
        uint64_t& t1 = bitmap[idx >> 6];
        uint64_t cg = (1ull << (idx & 63));
        if ((t1 & cg) != 0ull){
            t1 ^= cg;
            return true;
        }
        return false;
    }

    bool get(int idx) const {
        return (bitmap[idx >> 6] >> (idx & 63)) & 1;
    }

    int getone() const {
        int cnt = 0;
        for(int i = 0; i < cap; i ++)
            for(uint64_t x = bitmap[i]; x; x >>= 1)
                cnt += (x & 1);
        return cnt;
    }

    void print() {
        //std::cout << cap << " " << siz << std::endl;
        for(long long i = 0; i < siz; i ++)
            std::cout << get(i) << " ";
        std::cout << "\n";
    }

private:
    uint64_t* bitmap;
    int cap;
    long long siz;
};

class TuplesSet {
public:
    TuplesSet() {
        covered_2tuples_bitmap = covered_3tuples_bitmap = (std::nullptr_t) nullptr;
    }
    TuplesSet(const string init_PCA_path, const int nvar,
        vector<vector<int> >& testcase) {
        FILE *in_ca = fopen(init_PCA_path.c_str(), "r");
        
        MyBitSet tmp(nvar);
        vector<int> tep;
        tep.resize(nvar, 0);
        for (int c, p = 0; (c = fgetc(in_ca)) != EOF; ) {
            if (c == '\n') {
                CA.emplace_back(tmp);
                testcase.emplace_back(tep);
                p = 0;
            }
            else if (isdigit(c)){
                if(c == '1')
                    tmp.set(p), tep[p] = 1;
                else tmp.unset(p), tep[p] = 0;
                p ++;
            }
        }

        //CA[5].print();
        /*for(auto testcase : CA)
            testcase.print();
        std::cout<< "-------------------" << "\n";*/
        //std::cout << CA.size() << "\n";
        
        covered_2tuples_bitmap = new MyBitSet[(nvar << 2)];
        for(int i = 0; i < (nvar << 2); i ++)
            covered_2tuples_bitmap[i] = MyBitSet(nvar);
        
        long long M = 2ll * (nvar << 1) * (nvar << 1) + 5;
        covered_3tuples_bitmap = new MyBitSet[M];
        for(long long i = 0; i < M; i ++)
            covered_3tuples_bitmap[i] = MyBitSet(nvar);

        //covered_2tuples_bitmap[15].print();
        for(auto testcase : CA) {
            for(int i = 0; i < nvar; i ++) {
                if(testcase.get(i)) {
                    covered_2tuples_bitmap[i << 2 | 3] |= testcase;
                    covered_2tuples_bitmap[i << 2 | 2] %= testcase; // 取反|
                }
                else{
                    covered_2tuples_bitmap[i << 2 | 1] |= testcase;
                    covered_2tuples_bitmap[i << 2] %= testcase;
                }
            }

            for(int i = 0; i < nvar - 1; i ++) {
                int pi = i << 1 | testcase.get(i);
                long long offset = pi * (nvar << 1);
                for(int j = i + 1; j < nvar; j ++) {
                    int pj = j << 1 | testcase.get(j);
                    long long idx = offset + pj;
                    covered_3tuples_bitmap[idx << 1 | 1] |= testcase;
                    covered_3tuples_bitmap[idx << 1] %= testcase;
                }
            }
        }
        
    }
    ~TuplesSet() {
        //delete [] covered_2tuples_bitmap;
        //delete [] covered_3tuples_bitmap;
    }

    void dump(MyBitSet** covered2, MyBitSet** covered3) {
        *covered2 = covered_2tuples_bitmap;
        *covered3 = covered_3tuples_bitmap;
    }

private:
    vector<MyBitSet> CA;
    MyBitSet *covered_2tuples_bitmap;
    MyBitSet *covered_3tuples_bitmap;
};