#include "Optimizer.h"

#include <iostream>
#include <unistd.h>
#include <chrono>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <numeric>
#include <iterator>

using std::vector;
using std::string;
using std::pair;
using std::mt19937_64;

Optimizer:: Optimizer( int _seed,
    int _nvar, int _nclauses,
    vector<vector<int> > _clauses,
    vector<vector<int> > __2PCA_testcase,
    vector<vector<int> > _init_testcase,
    vector<_3tuples> _tuples_U,
    string _output_testcase_path,
    vector<vector<int> > _pos_in_cls,
    vector<vector<int> > _neg_in_cls,
    int _stop_length, bool _print ) {
    
    print = _print;
    gen.seed(seed = _seed);

    stop_length = _stop_length;

    output_testcase_path = _output_testcase_path;

    nvar = _nvar;
    nclauses = _nclauses;
    clauses = _clauses;
    _2PCA_testcase = __2PCA_testcase;
    testcases = _init_testcase;
    testcase_size = testcases.size();
    testcase_idx = testcase_size - 1;

    tuples_U = _tuples_U;
    tuples_nums = tuples_U.size();
    covered_tuples_nums = tuples_nums;
    for(int i = 0; i < tuples_nums; i ++) {
        covered_tuples.emplace_back(i);
        tuples_idx_to_pos.emplace_back(i);
    }

    pos_in_cls = _pos_in_cls;
    neg_in_cls = _neg_in_cls;

    testcase_pos_to_idx = vector<int>(testcase_size, 0);
    testcase_idx_to_pos = vector<int>(testcase_size, 0);
    for(int i = 0; i < testcase_size; i ++) {
        testcase_pos_to_idx[i] = i;
        testcase_idx_to_pos[i] = i;
    }

    for(int i = 0; i < testcase_size; i ++) {
        MyList tmp;
        unique_covered_tuples.emplace_back(tmp);
        tc_covered_tuples.emplace_back(tmp);
    }
    
    covered_times = vector<int>(tuples_nums, 0);
    for(int p = 0; p < tuples_nums; p ++) {
        _3tuples t = tuples_U[p];
        int i = abs(t.v1) - 1, vi = t.v1 > 0;
        int j = abs(t.v2) - 1, vj = t.v2 > 0;
        int k = abs(t.v3) - 1, vk = t.v3 > 0;

        MyList tmp;
        covered_testcases.emplace_back(tmp);
        for(int pos = 0; pos < testcase_size; pos ++) {
            const vector<int>& testcase = testcases[pos];
            if(testcase[i] == vi && testcase[j] == vj && testcase[k] == vk) {
                covered_times[p] += 1;
                covered_testcases[p].insert(testcase_pos_to_idx[pos]);
                tc_covered_tuples[pos].insert(p);
            }
        }
    }

    for(int p = 0; p < tuples_nums; p ++)
        if(covered_times[p] == 1) {
            int idx = covered_testcases[p].get_head_val();
            int pos = testcase_idx_to_pos[idx];
            unique_covered_tuples[pos].insert(p);
        }
    
    clauses_cov = vector<vector<int> >(testcase_size, vector<int>(nclauses, 0));
    for(int i = 0; i < testcase_size; i ++) {
        const vector<int>& testcase = testcases[i];
        vector<int>& cur_clauses_cov = clauses_cov[i];
        for(int j = 0; j < nvar; j ++) {
            const vector<int>& vec = (testcase[j] ? pos_in_cls[j + 1]: neg_in_cls[j + 1]);
            for (int x: vec) ++ cur_clauses_cov[x];
        }
    }

    int sum = 0;
    for(int num : covered_times)
        sum += num;

    greedy_limit = 0;
    last_greedy_time = vector<int>(testcase_size, -testcase_taboo - 1);

    cdcl_solver = new CDCLSolver::Solver;
    cdcl_solver->read_clauses(nvar, clauses);

    cdcl_sampler = new ExtMinisat::SamplingSolver(nvar, clauses, 1, true, 0);

    std::cout << "Optimizer init success" << "\n";
}

Optimizer:: ~Optimizer() {
    delete cdcl_solver;
    delete cdcl_sampler;
}

void Optimizer:: uptate_unique_covered(int tcid) {
    int tcid_p = testcase_idx_to_pos[tcid];
    for(LIST_node *p = unique_covered_tuples[tcid_p].head, *q; p != NULL; p = q) {
        q = p->nxt;
        int tpid = p->val;
        if(covered_times[tpid] != 1)
            unique_covered_tuples[tcid_p].delete_node(p);
    }
}

void Optimizer:: update_covered_testcases(int tpid) {
    for(LIST_node *p = covered_testcases[tpid].head, *q; p != NULL; p = q) {
        q = p->nxt;
        int tcid = p->val;
        if(testcase_idx_to_pos[tcid] < 0)
            covered_testcases[tpid].delete_node(p);
    }
}

int Optimizer:: new_uncovered_tuples_after_remove_testcase(int tcid) {
    uptate_unique_covered(tcid);
    int tcid_p = testcase_idx_to_pos[tcid];
    return unique_covered_tuples[tcid_p].size();
}

int Optimizer:: get_which_remove() {
    int besttc = -1, mini = 0;
    for (int i = 0; i < testcase_size; ++ i) {
        int idx = testcase_pos_to_idx[i];
        int res = new_uncovered_tuples_after_remove_testcase(idx);
        if (besttc == -1 || res < mini) {
            mini = res, besttc = i;
            if(mini == 0) break ;
        }
    }
    return testcase_pos_to_idx[besttc];
}

void Optimizer:: UpdateInfo_remove_testcase(int tcid_idx) {
    int tcid = testcase_idx_to_pos[tcid_idx];
    
    vector<int> break_pos, unique_id;
    for(LIST_node *p = tc_covered_tuples[tcid].head; p != NULL; p = p->nxt) {
        int tpid = p->val;
        covered_times[tpid] --;
        if(covered_times[tpid] == 0) {
            int pos = tuples_idx_to_pos[tpid];
            break_pos.emplace_back(pos);
            uncovered_tuples.emplace_back(tpid);
        }
        else if(covered_times[tpid] == 1)
            unique_id.emplace_back(tpid);
    }

    testcase_idx_to_pos[tcid_idx] = -1;
    for(int tpid : unique_id) {
        update_covered_testcases(tpid);
        int idx = covered_testcases[tpid].get_head_val();
        int pos = testcase_idx_to_pos[idx];
        unique_covered_tuples[pos].insert(tpid);
    }

    sort(break_pos.begin(), break_pos.end());
    int break_num = break_pos.size();
    for(int i = break_num - 1; i >= 0; i --) {
        int pos = break_pos[i];
        if(pos != covered_tuples_nums - 1) {
            int idx_new = covered_tuples[covered_tuples_nums - 1];
            covered_tuples[pos] = idx_new;
            tuples_idx_to_pos[idx_new] = pos;
        }
        covered_tuples.pop_back();
        covered_tuples_nums --;
    }

    if(tcid != testcase_size - 1) {
        tc_covered_tuples[tcid].Free();
        unique_covered_tuples[tcid].Free();

        testcases[tcid] = testcases[testcase_size - 1];
        last_greedy_time[tcid] = last_greedy_time[testcase_size - 1];
        clauses_cov[tcid] = clauses_cov[testcase_size - 1];
        tc_covered_tuples[tcid] = tc_covered_tuples[testcase_size - 1];
        unique_covered_tuples[tcid] = unique_covered_tuples[testcase_size - 1];

        int idx = testcase_pos_to_idx[testcase_size - 1];
        testcase_idx_to_pos[idx] = tcid;
        testcase_pos_to_idx[tcid] = idx;
    }
    
    testcases.pop_back();
    last_greedy_time.pop_back();
    clauses_cov.pop_back();

    tc_covered_tuples.pop_back();
    unique_covered_tuples.pop_back();
    
    testcase_size --;
}

void Optimizer:: remove_testcase_greedily() {
    int idx = get_which_remove();
    UpdateInfo_remove_testcase(idx);
}

void Optimizer:: change_bit(int v, int ad, const vector<int>& tc, vector<int>& cur_clauses_cov) {
    int vid = abs(v) - 1;
    int curbit = tc[vid], tt = v > 0;
    if (curbit != tt) {
        const vector<int>& var_cov_old = (curbit ? pos_in_cls[vid + 1]: neg_in_cls[vid + 1]);
        const vector<int>& var_cov_new = (tt ? pos_in_cls[vid + 1]: neg_in_cls[vid + 1]);
        for (int cid: var_cov_new) cur_clauses_cov[cid] += ad;   
        for (int cid: var_cov_old) cur_clauses_cov[cid] -= ad;
    }
}

pair<bool, pair<int, int> > Optimizer:: get_gain_for_forcetuple(int tcid, _3tuples chosen_tp) {
    const vector<int>& tc = testcases[tcid];
    vector<int> tc2 = tc;
    vector<int>& cur_clauses_cov = clauses_cov[tcid];

    change_bit(chosen_tp.v1, 1, tc, cur_clauses_cov);
    change_bit(chosen_tp.v2, 1, tc, cur_clauses_cov);
    change_bit(chosen_tp.v3, 1, tc, cur_clauses_cov);
    
    bool has0 = false;
    for (int i = 0; i < nclauses; ++i){
        if (cur_clauses_cov[i] == 0){
            has0 = true; break;
        }
    }

    change_bit(chosen_tp.v1, -1, tc, cur_clauses_cov);
    change_bit(chosen_tp.v2, -1, tc, cur_clauses_cov);
    change_bit(chosen_tp.v3, -1, tc, cur_clauses_cov);

    if (has0) return {false, {0, 0}};

    tc2[abs(chosen_tp.v1) - 1] = (chosen_tp.v1 > 0);
    tc2[abs(chosen_tp.v2) - 1] = (chosen_tp.v2 > 0);
    tc2[abs(chosen_tp.v3) - 1] = (chosen_tp.v3 > 0);

    auto res = get_gain_for_forcetestcase(tcid, tc2);
    return {true, res};
}

void Optimizer:: forcetuple(int tid, _3tuples tp) {

    vector<int> tc2 = testcases[tid];
    tc2[abs(tp.v1) - 1] = (tp.v1 > 0);
    tc2[abs(tp.v2) - 1] = (tp.v2 > 0);
    tc2[abs(tp.v3) - 1] = (tp.v3 > 0);

    forcetestcase(tid, tc2);
}

bool Optimizer:: random_greedy_step() {
    int uncovered_cnt = uncovered_tuples.size();
    int picked_tuple = gen() % uncovered_cnt;

    int tpid = uncovered_tuples[picked_tuple];
    _3tuples tp = tuples_U[tpid];
    int besttcid = -1;
    long long maxi = 0;

    for (int i = 0; i < testcase_size; i ++) {
        if(greedy_limit - last_greedy_time[i] <= testcase_taboo)
            continue ;

        auto res = get_gain_for_forcetuple(i, tp);
        if (res.first){
            int net_gain = res.second.second - res.second.first;
            if (net_gain > maxi)
                besttcid = i, maxi = net_gain;
        }
    }

    if (besttcid != -1) {
        forcetuple(besttcid, tp);
        ++ greedy_limit;
        last_greedy_time[besttcid] = greedy_limit;
        return true;
    }

    if ((gen() % 100) < __forced_greedy_percent){
        return greedy_step_forced(tp);
    }

    return false;
}

bool Optimizer:: greedy_step_forced(_3tuples tp) {

    int vid1 = abs(tp.v1) - 1, bit1 = tp.v1 > 0;
    int vid2 = abs(tp.v2) - 1, bit2 = tp.v2 > 0;
    int vid3 = abs(tp.v3) - 1, bit3 = tp.v3 > 0;
    
    if(use_cdcl_solver) {
        cdcl_solver->clear_assumptions();
        cdcl_solver->add_assumption(vid1, bit1);
        cdcl_solver->add_assumption(vid2, bit2);
        cdcl_solver->add_assumption(vid3, bit3);
    }
    
    else {
        cdcl_sampler->clear_assumptions();
        cdcl_sampler->add_assumption(vid1, bit1);
        cdcl_sampler->add_assumption(vid2, bit2);
        cdcl_sampler->add_assumption(vid3, bit3);
    }

    int besttcid = -1;
    long long maxi = 0;
    vector<int> besttc2;

    vector<pair<int, int> > prob = vector<pair<int, int> >(nvar, make_pair(0, 0));

    for (int i = 0; i < testcase_size; i ++) {
        if (greedy_limit - last_greedy_time[i] <= testcase_taboo)
            continue ;

        if(use_cdcl_solver) 
            for(int j = 0; j < nvar; j ++)
                cdcl_solver->setPolarity(j, testcases[i][j] == 0);

        else {
            for(int j = 0; j < nvar; j ++) {
                if(testcases[i][j])
                    prob[j] = make_pair(1, 0);
                else prob[j] = make_pair(0, 1);
            }
            cdcl_sampler->set_prob(prob);
        }

        vector<int> tc2 = vector<int>(nvar, 0);
        if(use_cdcl_solver) {
            bool ret = cdcl_solver->solve();
            search_nums ++;
            if(! ret) {
                std::cout << "c \033[1;31mError: SAT solve failing!\033[0m" << std::endl;
                return false;
            }
            cdcl_solver->get_solution(tc2);
        }
        
        else {
            vector<int> tc2 = vector<int>(nvar, 0);
            cdcl_sampler->get_solution(tc2);
            search_nums ++;
        }
        
        auto res = get_gain_for_forcetestcase(i, tc2);

        int net_gain = res.second - res.first;
        if (besttcid == -1 || net_gain > maxi){
            besttcid = i;
            besttc2 = tc2;
            maxi = net_gain;
        }
    }
    if (besttcid == -1)
        return false;

    forcetestcase(besttcid, besttc2);
    
    ++ greedy_limit;
    last_greedy_time[besttcid] = greedy_limit;
    return true;
}

pair<int, int> Optimizer:: get_gain_for_forcetestcase(int tcid, const vector<int>& tc2) {
    const vector<int>& tc = testcases[tcid];
    vector<int> break_id;

    int tcid_idx = testcase_pos_to_idx[tcid];
    uptate_unique_covered(tcid_idx);
    int break_cnt = unique_covered_tuples[tcid].size();
    int gain_cnt = 0;

    for(LIST_node *p = unique_covered_tuples[tcid].head; p != NULL; p = p->nxt) {
        int tpid = p->val;
        _3tuples t = tuples_U[tpid];
        int vid1 = abs(t.v1) - 1, bit1 = (t.v1 > 0);
        int vid2 = abs(t.v2) - 1, bit2 = (t.v2 > 0);
        int vid3 = abs(t.v3) - 1, bit3 = (t.v3 > 0);
        if(tc2[vid1] == bit1 && tc2[vid2] == bit2 && tc2[vid3] == bit3)
            gain_cnt ++;
    }

    for(int tpid : uncovered_tuples) {
        _3tuples t = tuples_U[tpid];
        int vid1 = abs(t.v1) - 1, bit1 = (t.v1 > 0);
        int vid2 = abs(t.v2) - 1, bit2 = (t.v2 > 0);
        int vid3 = abs(t.v3) - 1, bit3 = (t.v3 > 0);
        if(tc2[vid1] == bit1 && tc2[vid2] == bit2 && tc2[vid3] == bit3)
            gain_cnt ++;
    }

    return {break_cnt, gain_cnt};
}

void Optimizer:: forcetestcase(int tcid, const vector<int>& tc2) {
    
    int tcid_idx = testcase_pos_to_idx[tcid];

    testcase_idx_to_pos[tcid_idx] = -1;

    testcase_idx ++;
    testcase_pos_to_idx[tcid] = testcase_idx;
    testcase_idx_to_pos.emplace_back(tcid);

    vector<int>& tc = testcases[tcid];
    vector<int> break_pos, unique_id;

    for(LIST_node *p = tc_covered_tuples[tcid].head; p != NULL; p = p->nxt) {
        int tpid = p->val;
        _3tuples t = tuples_U[tpid];
        covered_times[tpid] --;
        if(covered_times[tpid] == 0) {
            break_pos.emplace_back(tuples_idx_to_pos[tpid]);
            uncovered_tuples.emplace_back(tpid);
        }
        if(covered_times[tpid] == 1)
            unique_id.emplace_back(tpid);
    }
    
    sort(break_pos.begin(), break_pos.end());
    int break_num = break_pos.size();
    for(int i = break_num - 1; i >= 0; i --) {
        int p = break_pos[i];
        if(p != covered_tuples_nums) {
            int idx_new = covered_tuples[covered_tuples_nums - 1];
            covered_tuples[p] = idx_new;
            tuples_idx_to_pos[idx_new] = p;
        }
        covered_tuples.pop_back();
        covered_tuples_nums --;
    }

    for(int tpid : unique_id) {
        update_covered_testcases(tpid);
        int idx = covered_testcases[tpid].get_head_val();
        int pos = testcase_idx_to_pos[idx];
        unique_covered_tuples[pos].insert(tpid);
    }

    tc_covered_tuples[tcid].Free();
    unique_covered_tuples[tcid].Free();
    testcases[tcid] = tc2;

    for(int tpid : covered_tuples) {
        _3tuples t = tuples_U[tpid];

        int i = abs(t.v1) - 1, vi = (t.v1 > 0);
        int j = abs(t.v2) - 1, vj = (t.v2 > 0);
        int k = abs(t.v3) - 1, vk = (t.v3 > 0);

        if(tc2[i] == vi && tc2[j] == vj && tc2[k] == vk) {
            covered_times[tpid] ++;
            tc_covered_tuples[tcid].insert(tpid);
            covered_testcases[tpid].insert(testcase_idx);
        }
    }

    break_pos.clear();
    int u_p = 0;
    for(int tpid : uncovered_tuples) {
        _3tuples t = tuples_U[tpid];

        int i = abs(t.v1) - 1, vi = (t.v1 > 0);
        int j = abs(t.v2) - 1, vj = (t.v2 > 0);
        int k = abs(t.v3) - 1, vk = (t.v3 > 0);

        if(tc2[i] == vi && tc2[j] == vj && tc2[k] == vk) {

            covered_tuples.emplace_back(tpid);
            tuples_idx_to_pos[tpid] = covered_tuples_nums;
            covered_times[tpid] = 1;
            covered_tuples_nums ++;

            tc_covered_tuples[tcid].insert(tpid);
            unique_covered_tuples[tcid].insert(tpid);
            covered_testcases[tpid].insert(testcase_idx);

            break_pos.emplace_back(u_p);
        }
        u_p ++;
    }

    sort(break_pos.begin(), break_pos.end());
    break_num = break_pos.size();
    int uncovered_tuples_nums = uncovered_tuples.size();
    for(int i = break_num - 1; i >= 0; i --) {
        int p = break_pos[i];
        if(p != uncovered_tuples_nums)
            uncovered_tuples[p] = uncovered_tuples[uncovered_tuples_nums - 1];
        uncovered_tuples.pop_back();
        uncovered_tuples_nums --;
    }

    vector<int>& cur_clauses_cov = clauses_cov[tcid];
    cur_clauses_cov = vector<int>(nclauses, 0);
    for(int i = 0; i < nvar; i ++) {
        const vector<int>& var = (tc2[i] ? pos_in_cls[i + 1]: neg_in_cls[i + 1]);
        for (int cid: var) cur_clauses_cov[cid] ++;
    }
}

bool Optimizer:: check_for_flip(int tcid, int vid) {

    const vector<int>& tc = testcases[tcid];
    int curbit = tc[vid];

    const vector<int>& var_cov_old = (curbit ? pos_in_cls[vid + 1]: neg_in_cls[vid + 1]);
    const vector<int>& var_cov_new = (curbit ? neg_in_cls[vid + 1]: pos_in_cls[vid + 1]);
    vector<int>& cur_clauses_cov = clauses_cov[tcid];

    bool has0 = true;
    for (int cid: var_cov_new)
        cur_clauses_cov[cid] ++;
    for (int cid: var_cov_old){
        cur_clauses_cov[cid] --;
        if (cur_clauses_cov[cid] == 0)
            has0 = false;
    }

    for (int cid: var_cov_new) -- cur_clauses_cov[cid];
    for (int cid: var_cov_old) ++ cur_clauses_cov[cid];

    return has0;
}

void Optimizer:: flip_bit(int tid, int vid) {
    vector<int> tc2 = testcases[tid];
    tc2[vid] ^= 1;
    forcetestcase(tid, tc2);
}

void Optimizer:: random_step() {

    long long all_nums = testcase_size * nvar;
    vector<int> flip_order;
    flip_order = vector<int>(all_nums, 0);
    std::iota(flip_order.begin(), flip_order.end(), 0);
    std::mt19937 g(1);
    std::shuffle(flip_order.begin(), flip_order.end(), g);

    for(int idx : flip_order) {
        int tid = idx / nvar, vid = idx % nvar;
        if(check_for_flip(tid, vid)) {
            flip_bit(tid, vid); break ;
        }
    }
}

void Optimizer:: search() {

    cur_step = 0;
    int last_success_step = 0;
    int _2PCAsiz = _2PCA_testcase.size();
    for(; last_success_step < stop_length; ) {
        
        if(uncovered_tuples.empty()) {
            last_tc = testcases;
            std::cout << "\033[;32mc current 3-wise CA size: " << _2PCAsiz + testcase_size 
                << ", step #" << cur_step << " \033[0m" << std::endl;
            remove_testcase_greedily();
            last_success_step = 0;
            continue ;
        }
        cur_step ++;
        last_success_step ++;

        int cyc = gen() % 100;
        if (cyc < 1 || ! random_greedy_step()) {
            random_step();
        }
    }

    if(! uncovered_tuples.empty()) {
        testcases = last_tc;
        testcase_size = testcases.size();
    }

    SaveTestcaseSet(output_testcase_path);
}

void Optimizer:: SaveTestcaseSet(string result_path) {
    ofstream res_file(result_path);

    if (print) {
        std:: cout << "final 3-wise CA size is: ";
        std:: cout << _2PCA_testcase.size() + testcases.size() << "\n";
    }
    
    for (const vector<int>& testcase: _2PCA_testcase) {
        for (int v = 0; v < nvar; v++)
            res_file << testcase[v] << " ";
        res_file << "\n";
    }

    for (const vector<int>& testcase: testcases) {
        for (int v = 0; v < nvar; v++)
            res_file << testcase[v] << " ";
        res_file << "\n";
    }
    res_file.close();
    if (print)
        cout << "c Testcase set saved in " << result_path << endl;
}