#include "Expandor.h"

#include "../minisat_ext/BlackBoxSolver.h"
#include "../minisat_ext/Ext.h"
#include "MyList.h"

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

class Optimizer {

public:
    Optimizer( int seed,
        int _nvar, int _nclauses,
        vector<vector<int> > _clauses,
        vector<vector<int> > __2PCA_testcate,
        vector<vector<int> > _init_testcase,
        vector<_3tuples> _tuples_U,
        string _output_testcase_path,
        vector<vector<int> > _pos_in_cls,
        vector<vector<int> > _neg_in_cls,
        int _stop_length, bool _print );
    ~Optimizer();

    void search();

private:
    int seed = 1;
    int stop_length = 100;
    
    string output_testcase_path;

    int nvar, nclauses;
    vector<vector<int> > clauses;
    vector<vector<int> > _2PCA_testcase;
    vector<vector<int> > testcases;
    int testcase_size;
    
    vector<_3tuples> tuples_U;
    int tuples_nums;
    vector<int> covered_times;

    vector<int> covered_tuples;
    vector<int> uncovered_tuples;
    int covered_tuples_nums;

    vector<vector<int> > pos_in_cls;
    vector<vector<int> > neg_in_cls;
    vector<vector<int> > clauses_cov;

    int greedy_limit;
    int testcase_taboo = 10;
    int __forced_greedy_percent = 10;
    vector<int> last_greedy_time;

    bool use_cdcl_solver = true;
    CDCLSolver:: Solver *cdcl_solver;
    ExtMinisat::SamplingSolver *cdcl_sampler;

    vector<vector<int> > last_tc;

    // vector<vector<int> > unique_covered_tuples;
    // vector<vector<int> > tc_covered_tuples;
    // vector<vector<int> > covered_testcases;

    vector<MyList> unique_covered_tuples;
    vector<MyList> tc_covered_tuples;
    vector<MyList> covered_testcases;

    vector<int> testcase_pos_to_idx;
    vector<int> testcase_idx_to_pos;

    int testcase_idx;
    
    vector<int> tuples_idx_to_pos;

    int cur_step;

    long long search_nums = 0;

    int new_uncovered_tuples_after_remove_testcase(int tcid);
    int get_which_remove();
    // void UpdateInfo_remove_testcase(const vector<int>& tc);
    void UpdateInfo_remove_testcase(int tcid);
    void remove_testcase_greedily();
    pair<bool, pair<int, int> > get_gain_for_forcetuple(int tcid, _3tuples chosen_tp);
    bool random_greedy_step();
    void random_step();
    void forcetuple(int tid, _3tuples tp);
    bool greedy_step_forced(_3tuples tp);
    void change_bit(int v, int ad, const vector<int>& tc, vector<int>& cur_clauses_cov);
    void SaveTestcaseSet(string result_path);
    pair<int, int> get_gain_for_forcetestcase(int tcid, const vector<int>& tc2);
    void forcetestcase(int tcid, const vector<int>& tc2);
    std::mt19937_64 gen;
    void flip_bit(int tid, int vid);
    bool check_for_flip(int tcid, int vid);
    void uptate_unique_covered(int tcid);
    void update_covered_testcases(int tpid);

    bool print;
};