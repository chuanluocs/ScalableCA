#ifndef LOCALSEARCH_OPTIMIZER_INCLUDE_H
#define LOCALSEARCH_OPTIMIZER_INCLUDE_H
#endif
#include "./core/Solver.h"
#include "pboccsatsolver.h"

#include "../minisat_ext/BlackBoxSolver.h"
#include "../minisat_ext/Ext.h"
#include "Argument.h"
#include "TuplesSet.h"
#include "cnfinfo.h"

#include <vector>
#include <numeric>
#include <random>
#include <utility>
#include <algorithm>
#include <set>
#include <map>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <chrono>

using std::vector;
using std::pair;
using std::set;
using std::map;
using std::string;

struct _3tuples {
    int v1, v2, v3;
    _3tuples() { }
    _3tuples(int i, int j, int k) {
        v1 = i, v2 = j, v3 = k;
    }
};

class Expandor {
public:
    Expandor(const Argument &params);
    ~Expandor();
    void GenerateCoveringArray();
    void dump(int& _nvar, int& _nclauses, 
        vector<vector<int> >& _clauses,
        vector<vector<int> >& _2PCA_testcase,
        vector<vector<int> >& _init_testcase, 
        vector<_3tuples>& _tuples_U,
        vector<vector<int> >& _pos_in_cls,
        vector<vector<int> >& _neg_in_cls );
    static void *process_3tuples(void *arg);

    void Free();

    vector<vector<int> > old_testcase;
    vector<vector<int> > new_testcase;

private:
    bool check2covered(int i, int v1, int j, int v2) {
        int p = (i << 2) | (v1 << 1) | v2;
        return covered_2tuples_bitmap[p].get(j);
    }
    bool checkCNF(int i, int j, int k) {        
        for(_3tuples t : _3clauses)
            if(turnCNF(i) == -t.v1 && 
                turnCNF(j) == -t.v2 && turnCNF(k) == -t.v3)
                return 0;
        return 1;
    }
    int turnCNF(int x) {
        int v = (x >> 1) + 1;
        return (x & 1) ? v : -v;
    }
    int turnIdx(int x, bool v) {
        return x << 1 | v;
    }

    string input_cnf_path;
	string reduced_cnf_path;
	string init_PCA_file_path;
    string output_testcase_path;

    int seed;
    int candidate_set_size;
    int use_cnf_reduction;

    bool use_invalid_expand = true;
    bool use_context_aware = false;
    bool use_pbo_solver = false;
    bool uniform_sampling = false;
    bool uniform_sampling_greedy = false;

    int nvar, nclauses;
    vector<vector<int> > clauses;
    vector<vector<int> > pos_in_cls;
    vector<vector<int> > neg_in_cls;

    vector<_3tuples> uncovered_3tuples, tuples_U;

    MyBitSet *covered_2tuples_bitmap;
    MyBitSet *covered_3tuples_bitmap;
    
    vector<_3tuples> _3clauses;

    CDCLSolver::Solver *cdcl_solver;
    vector<CDCLSolver::Solver *> cdcl_solver_list;
    ExtMinisat::SamplingSolver *cdcl_sampler;
    PbOCCSATSolver *pbo_solver_;

    vector<int> count_each_var_uncovered[2];

    vector<vector<int> > candidate_testcase_set_;

    int uncovered_nums;

    vector<int> var_positive_appearance_count_;

    void GenerateCandidateTestcaseSet();
    int get_gain(vector<int> testcase);
    int SelectTestcaseFromCandidateSetByTupleNum();
    int GenerateTestcase();
    void Update3TupleInfo(vector<int> testcase, bool setmap);
    void ReplenishTestCase();
    void SaveTestcaseSet(string result_path);
    void Update_3TupleInfo(int st, vector<int> testcase, vector<int> idx);

    bool use_thread = false;
    int n_threads = 16;

    long long search_nums = 0, search_nums_2 = 0;

    vector<vector<_3tuples> > thread_uncovered_tuples;
    vector<int> thread_invalid_nums;
    vector<int> thread_covered_nums;
    vector<int> thread_uncovered_nums;

    bool use_cache;
    vector<int> order;

    Mersenne rng_;
    std::mt19937_64 gen;

    vector<int> getsolution_use_pbo_solver();
};
