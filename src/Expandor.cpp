#include "Expandor.h"

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
#include <pthread.h>

using std::vector;
using std::string;
using std::pair;
using std::mt19937_64;

const int kMaxPbOCCSATSeed = 10000000;

struct ThreadArgs {
    int id;
    CDCLSolver::Solver* solver;
    int nvar;
    int start;
    int end;

    MyBitSet *bitmap2, *bitmap3;
    vector<_3tuples>* thread_uncovered_tuples;
    int *thread_invalid_nums, *thread_covered_nums, *thread_uncovered_nums;

    vector<_3tuples> *t_3clauses;
    vector<int> *order;
};

void *Expandor::process_3tuples(void *args) {

    ThreadArgs *targs = (ThreadArgs *) args;
    CDCLSolver::Solver *t_cdcl_solver = targs->solver;
    int id = targs->id;

    MyBitSet *tbitmap2 = targs->bitmap2;
    MyBitSet *tbitmap3 = targs->bitmap3;
    vector<_3tuples> *t_uncovered_tuples = targs->thread_uncovered_tuples;
    int *t_uncovered_nums = targs->thread_uncovered_nums;
    int *t_invalid_nums = targs->thread_invalid_nums;
    int *t_covered_nums = targs->thread_covered_nums;

    vector<_3tuples> *t_3clauses = targs->t_3clauses;
    vector<int> *t_order = targs->order;

    std::cout << (targs->start) << " " << (targs->end) << "\n";
    
    int num = 0;
    for (int oi = targs->start; oi < targs->end; oi++) {
        int i = (*t_order)[oi];
        for (int v1 = 0; v1 <= 1; v1 ++) {
            int pi = i << 1 | v1;
            int offset = pi * (targs->nvar << 1);
            for (int j = i + 1; j < targs->nvar - 1; j++) {
                for (int v2 = 0; v2 <= 1; v2++) {
                    int pj = j << 1 | v2;
                    int idx = offset + pj;
                    for (int k = j + 1; k < targs->nvar; k++) {
                        for (int v3 = 0; v3 <= 1; v3++) {
                            if(tbitmap3[idx << 1 | v3].get(k))
                                (*t_covered_nums) ++;
                            
                            else if(! tbitmap2[i << 2 | v1 << 1 | v2].get(j))
                                (*t_invalid_nums) ++;
                            else if(! tbitmap2[i << 2 | v1 << 1 | v3].get(k))
                                (*t_invalid_nums) ++;
                            else if(! tbitmap2[j << 2 | v2 << 1 | v3].get(k))
                                (*t_invalid_nums) ++;

                            else {
                                int sz = t_3clauses->size();
                                bool flag_continue = 0;
                                for(int ti = 0; ti < sz; ti ++) {
                                    _3tuples t = (*t_3clauses)[i];
                                    int pi = v1 ? i + 1 : - i - 1;
                                    int pj = v2 ? j + 1 : - j - 1;
                                    int pk = v3 ? k + 1 : - k - 1;
                                    if(pi == -t.v1 && pj == -t.v2 && pk == -t.v3) {
                                        (*t_invalid_nums) ++;
                                        flag_continue = 1;
                                        break ;
                                    }
                                }
                                if(flag_continue) continue ;

                                num ++;
                                if(num % 100000 == 0)
                                    std::cout << id << ": " << num << "\n";

                                t_cdcl_solver->add_assumption(i, v1);
                                t_cdcl_solver->add_assumption(j, v2);
                                t_cdcl_solver->add_assumption(k, v3);
                                bool res = t_cdcl_solver->solve();
                                t_cdcl_solver->clear_assumptions();
                                if (res) {
                                    _3tuples tep(v1 ? i + 1 : - i - 1, 
                                        v2 ? j + 1 : - j - 1, 
                                        v3 ? k + 1 : - k - 1);
                                    t_uncovered_tuples->emplace_back(tep);
                                    (*t_uncovered_nums) ++;
                                }
                                else (*t_invalid_nums) ++;
                            }
                        }
                    }
                }
            }
        }
    }
    free(targs);
    pthread_exit(NULL);
}

Expandor::Expandor(const Argument &params) {

    input_cnf_path = params.input_cnf_path;
    init_PCA_file_path = params.init_PCA_file_path;
    output_testcase_path = params.output_testcase_path;

    seed = params.seed;
    rng_.seed(seed);
    gen.seed(seed);

    candidate_set_size = params.candidate_set_size;
    use_cnf_reduction = params.use_cnf_reduction;

    use_cache = true;
    if(params.use_cache == 0)
        use_cache = false;
    
    use_invalid_expand = true;
    if(params.use_invalid_expand == 0)
        use_invalid_expand = false;
    if(params.use_context_aware)
        use_context_aware = true;
    if(params.use_pbo_solver)
        use_pbo_solver = true;
    if(params.uniform_sampling) {
        uniform_sampling = true;
        candidate_set_size = 1;
        use_cache = 0;
    }
    if(params.uniform_sampling_greedy)
        uniform_sampling_greedy = true;

    string real_input_cnf_path = input_cnf_path;
    if (use_cnf_reduction){
        mt19937_64 tmp_gen(params.seed);
        string reduced_cnf_path = "/tmp/" + std::to_string(getpid()) + std::to_string(tmp_gen()) + "_reduced.cnf";
        string cmd = "./bin/coprocessor -enabled_cp3 -up -subsimp -no-bve -no-bce -no-dense -dimacs=" + 
            reduced_cnf_path + " " + params.input_cnf_path;
        
        int _ = system(cmd.c_str());
        real_input_cnf_path = reduced_cnf_path;
    }

    CNFInfo cnf(real_input_cnf_path);
    cnf.dump(nvar, nclauses, clauses, pos_in_cls, neg_in_cls);
    
    cdcl_solver = new CDCLSolver::Solver;
    cdcl_solver->read_clauses(nvar, clauses);
    cdcl_solver_list.emplace_back(cdcl_solver);

    cdcl_sampler = new ExtMinisat::SamplingSolver(nvar, clauses, seed, true, 0);
    
    if(use_pbo_solver)
        pbo_solver_ = new PbOCCSATSolver(real_input_cnf_path, rng_.next(kMaxPbOCCSATSeed));

    for(vector<int> c: clauses)
        if(c.size() == 3) {
            _3tuples tep(c[0], c[1], c[2]);
            _3clauses.emplace_back(tep);
        }
    
    TuplesSet tmp(init_PCA_file_path, nvar, old_testcase);
    tmp.dump(&covered_2tuples_bitmap, &covered_3tuples_bitmap);

    long long covered_nums = 0, invalid_nums = 0;
    
    count_each_var_uncovered[0].resize(nvar + 2, 0);
    count_each_var_uncovered[1].resize(nvar + 2, 0);
    uncovered_nums = 0;

    if(use_thread) {
        
        pthread_t threads[n_threads];

        thread_uncovered_tuples.resize(n_threads);
        thread_invalid_nums.resize(n_threads, 0);
        thread_covered_nums.resize(n_threads, 0);
        thread_uncovered_nums.resize(n_threads, 0);

        for(int i = 1; i < n_threads; i ++) {
            CDCLSolver::Solver *cdcl_solver_tep;
            cdcl_solver_tep = new CDCLSolver::Solver;
            cdcl_solver_tep->read_clauses(nvar, clauses);
            cdcl_solver_list.emplace_back(cdcl_solver_tep);
        }

        order.resize(nvar);
        std::iota(order.begin(), order.end(), 0);
        std::mt19937 g(seed);
        std::shuffle(order.begin(), order.end(), g);

        for (int i = 0; i < n_threads; i ++) {
            int *p = (int*)malloc(sizeof(*p));
            *p = i;
            struct ThreadArgs *t_args = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
            t_args->id = (*p);
            t_args->solver = cdcl_solver_list[*p];
            t_args->nvar = nvar;
            t_args->start = (*p) * (nvar - 2) / n_threads;
            t_args->end = ((*p) + 1) * (nvar - 2) / n_threads;

            t_args->bitmap2 = covered_2tuples_bitmap;
            t_args->bitmap3 = covered_3tuples_bitmap;
            t_args->thread_uncovered_tuples = &thread_uncovered_tuples[*p];
            t_args->thread_invalid_nums = &thread_invalid_nums[*p];
            t_args->thread_covered_nums = &thread_covered_nums[*p];
            t_args->thread_uncovered_nums = &thread_uncovered_nums[*p];

            t_args->t_3clauses = &_3clauses;
            t_args->order = &order;
            
            pthread_create(&threads[*p], NULL, process_3tuples, (void *)t_args);
        }

        for (int i = 0; i < n_threads; i ++)
            pthread_join(threads[i], NULL);

        for (int id = 0; id < n_threads; id ++) {
            covered_nums += thread_covered_nums[id];
            invalid_nums += thread_invalid_nums[id];
            uncovered_nums += thread_uncovered_nums[id];
            for(_3tuples t : thread_uncovered_tuples[id]) {
                uncovered_3tuples.emplace_back(t);
                
                int i = abs(t.v1) - 1, v1 = t.v1 > 0;
                int j = abs(t.v2) - 1, v2 = t.v2 > 0;
                int k = abs(t.v3) - 1, v3 = t.v3 > 0;
                count_each_var_uncovered[v1][i] ++;
                count_each_var_uncovered[v2][j] ++;
                count_each_var_uncovered[v3][k] ++;
            }
            thread_uncovered_tuples[id].clear();
        }
    }

    else {
        std::cout << "begin find uncovered 3 tuples" << "\n";
        for(int i = 0; i < nvar - 2; i ++) for(int v1 = 0; v1 <= 1; v1 ++) {
            int pi = i << 1 | v1;
            int offset = pi * (nvar << 1);
            for(int j = i + 1; j < nvar - 1; j ++) for(int v2 = 0; v2 <= 1; v2 ++) {
                int pj = j << 1 | v2;
                int idx = offset + pj;
                for(int k = j + 1; k < nvar; k ++) for(int v3 = 0; v3 <= 1; v3 ++) {
                    if(covered_3tuples_bitmap[idx << 1 | v3].get(k))
                        covered_nums ++;
                    else if(use_invalid_expand && (! check2covered(i, v1, j, v2) 
                        || ! check2covered(i, v1, k, v3)
                        || ! check2covered(j, v2, k, v3)))
                        invalid_nums ++;
                    else if(use_invalid_expand && (! checkCNF(pi, pj, k << 1 | v3)))
                        invalid_nums ++;
                    else {
                        search_nums ++;
                        cdcl_solver->add_assumption(i, v1);
                        cdcl_solver->add_assumption(j, v2);
                        cdcl_solver->add_assumption(k, v3);
                        bool res = cdcl_solver->solve();
                        cdcl_solver->clear_assumptions();
                        if(res) {
                            _3tuples tep(turnCNF(pi), turnCNF(pj), turnCNF(k << 1 | v3));
                            uncovered_3tuples.emplace_back(tep);
                            uncovered_nums ++;
                            count_each_var_uncovered[v1][i] ++;
                            count_each_var_uncovered[v2][j] ++;
                            count_each_var_uncovered[v3][k] ++;
                        }
                        else invalid_nums ++;
                    }
                }
            }
        }
    }
    
    std::cout << "covered valid 3-wise tuple nums: " << covered_nums << "\n";
    std::cout << "uncovered valid 3-wise tuple nums: " << uncovered_3tuples.size() << "\n";
    std::cout << "all valid 3-wise tuple nums: " << covered_nums + uncovered_3tuples.size() << "\n";
    std::cout << "invalid 3-wise tuple nums: " << invalid_nums << "\n";

    uncovered_nums = uncovered_3tuples.size();

    var_positive_appearance_count_ = vector<int>(nvar, 0);
    int old_testcase_size = old_testcase.size();
    for(int i = 0; i < old_testcase_size; i ++) {
        const vector<int>&tc = old_testcase[i];
        for(int j = 0; j < nvar; j ++)
            if(tc[j] == 1)
                var_positive_appearance_count_[j] ++;
    }

    if(use_cache && (! uniform_sampling)) {
        candidate_testcase_set_.resize(2 * candidate_set_size);
        vector<pair<int, int> > prob;
        prob.reserve(nvar);
        for(int i = 0; i < nvar; i ++) {
            int v1, v2;
            if(use_context_aware) {
                v1 = old_testcase_size - var_positive_appearance_count_[i];
                v2 = var_positive_appearance_count_[i];
            }
            else if(uniform_sampling_greedy)
                v1 = v2 = 1;
            else {
                v1 = count_each_var_uncovered[1][i];
                v2 = count_each_var_uncovered[0][i];
            }
            prob.emplace_back(make_pair(v1, v2));
        }

        for(int i = 0; i < candidate_set_size; i ++) {
            if(use_pbo_solver)
                candidate_testcase_set_[i] = getsolution_use_pbo_solver();
            else {
                cdcl_sampler->set_prob(prob);
                cdcl_sampler->get_solution(candidate_testcase_set_[i]);
                search_nums_2 ++;
            }
        }
    }
    else {
        candidate_testcase_set_.resize(candidate_set_size);
    }

    tuples_U = uncovered_3tuples;
}

Expandor::~Expandor() {
    if(use_pbo_solver)
        delete pbo_solver_;
}

vector<int> Expandor:: getsolution_use_pbo_solver() {
    
    int pbo_seed = rng_.next(kMaxPbOCCSATSeed);
    pbo_solver_->set_seed(pbo_seed);

    vector<int> init_solution(nvar, 0);
    vector<double> context_aware_flip_priority_(nvar, 0);
    int testcase_size = old_testcase.size() + new_testcase.size();
    
    for(int i = 0; i < nvar; i ++) {
        int t, v0, v1;
        if(uniform_sampling)
            v0 = v1 = 1;
        else if(use_context_aware) {
            v0 = var_positive_appearance_count_[i];
            v1 = testcase_size - var_positive_appearance_count_[i];
        }
        else {
            v0 = count_each_var_uncovered[0][i];
            v1 = count_each_var_uncovered[1][i];
        }
        if(v0 == 0 && v1 == 0)
            v0 = v1 = 1;
            
        t = gen() % (v0 + v1);
        init_solution[i] = (t < v0) ? 0 : 1;
    }

    for(int i = 0; i < nvar; i ++) {
        double positive_sample_weight = 1.0 * var_positive_appearance_count_[i] / testcase_size;
        if(init_solution[i])
            context_aware_flip_priority_[i] = positive_sample_weight;
        else
            context_aware_flip_priority_[i] = 1.0 - positive_sample_weight;
    }

    pbo_solver_->set_init_solution(init_solution);
    pbo_solver_->set_var_flip_priority_ass_aware(context_aware_flip_priority_);
    
    bool is_sat = pbo_solver_->solve();
    return pbo_solver_->get_sat_solution();
}

void Expandor::dump(int& _nvar, int& _nclauses, 
    vector<vector<int> >& _clauses,
    vector<vector<int> >& _2PCA_testcase,
    vector<vector<int> >& _init_testcase, 
    vector<_3tuples>& _tuples_U, 
    vector<vector<int> >& _pos_in_cls,
    vector<vector<int> >& _neg_in_cls ) {
    
    _nvar = nvar;
    _nclauses = nclauses;
    _clauses = clauses;
    _2PCA_testcase = old_testcase;
    _init_testcase = new_testcase;
    _tuples_U = tuples_U;
    _pos_in_cls = pos_in_cls;
    _neg_in_cls = neg_in_cls;
}

void Expandor::GenerateCandidateTestcaseSet() {

    vector<pair<int, int> > prob;
    prob.reserve(nvar);

    int testcase_size = old_testcase.size() + new_testcase.size();
    for(int i = 0; i < nvar; i ++) {
        int v1, v2;
        if(use_context_aware) {
            v1 = testcase_size - var_positive_appearance_count_[i];
            v2 = var_positive_appearance_count_[i];
        }
        else if(uniform_sampling_greedy || uniform_sampling)
            v1 = v2 = 1;
        else {
            v1 = count_each_var_uncovered[1][i];
            v2 = count_each_var_uncovered[0][i];
        }
        prob.emplace_back(make_pair(v1, v2));
    }

    if(use_cache && (!uniform_sampling)) {
        for(int i = candidate_set_size; i < 2 * candidate_set_size; i ++) {
            if(use_pbo_solver)
                candidate_testcase_set_[i] = getsolution_use_pbo_solver();
            else {
                cdcl_sampler->set_prob(prob);
                cdcl_sampler->get_solution(candidate_testcase_set_[i]);
                search_nums_2 ++;
            }
        }
    }
    else {
        for(int i = 0; i < candidate_set_size; i ++) {
            if(use_pbo_solver)
                candidate_testcase_set_[i] = getsolution_use_pbo_solver();
            else {
                cdcl_sampler->set_prob(prob);
                cdcl_sampler->get_solution(candidate_testcase_set_[i]);
                search_nums_2 ++;
            }
        }
    }
}

int Expandor::get_gain(vector<int> testcase) {
    int score = 0;
    for(_3tuples t : uncovered_3tuples) {
        int i = abs(t.v1) - 1, vi = t.v1 > 0;
        int j = abs(t.v2) - 1, vj = t.v2 > 0;
        int k = abs(t.v3) - 1, vk = t.v3 > 0;
        if(testcase[i] == vi && testcase[j] == vj && testcase[k] == vk)
            score ++;
    }
    return score;
}

int Expandor::SelectTestcaseFromCandidateSetByTupleNum() {
    if(use_cache) {
        vector<pair<int, int> > gain;
        gain.resize(2 * candidate_set_size);

        for(int i = 0; i < 2 * candidate_set_size; i ++) {
            gain[i].first = get_gain(candidate_testcase_set_[i]);
            gain[i].second = i;
        }
        sort(gain.begin(), gain.end());

        if(gain[2 * candidate_set_size - 1].first <= 0)
            return -1;

        vector<vector<int> > tmp;
        tmp.resize(candidate_set_size);
        for(int i = candidate_set_size; i < 2 * candidate_set_size; i ++)
            tmp[i - candidate_set_size] = candidate_testcase_set_[gain[i].second];
        for(int i = 0; i < candidate_set_size; i ++)
            candidate_testcase_set_[i] = tmp[i];
        
        return candidate_set_size - 1;
    }
    
    else {
        int mx = get_gain(candidate_testcase_set_[0]), mxi = 0;
        for (int i = 1; i < candidate_set_size; i ++) {
            int score = get_gain(candidate_testcase_set_[i]);
            if(score > mx)
                mx = score, mxi = i;
        }
        return mx > 0 ? mxi : -1;
    }
}

int Expandor::GenerateTestcase() {
    GenerateCandidateTestcaseSet();
    return SelectTestcaseFromCandidateSetByTupleNum();
}

void Expandor::Update3TupleInfo(vector<int> testcase, bool setmap) {

    for(int i = 0; i < nvar; i ++)
        if(testcase[i] == 1)
            var_positive_appearance_count_[i] ++;

    vector<_3tuples> tep;
    for(_3tuples t : uncovered_3tuples) {
        int i = abs(t.v1) - 1, vi = t.v1 > 0;
        int pi = i << 1 | vi;
        int offset = pi * (nvar << 1);

        int j = abs(t.v2) - 1, vj = t.v2 > 0;
        int pj = j << 1 | vj;
        int idx = offset + pj;

        int k = abs(t.v3) - 1, vk = t.v3 > 0;

        if(testcase[i] == vi && testcase[j] == vj && testcase[k] == vk) {
            uncovered_nums --;
            if(setmap)
                covered_3tuples_bitmap[idx << 1 | vk].set(k);
            
            count_each_var_uncovered[vi][i] --;
            count_each_var_uncovered[vj][j] --;
            count_each_var_uncovered[vk][k] --;
        }
        else tep.emplace_back(t);
    }
    uncovered_3tuples = tep;
}

void Expandor::GenerateCoveringArray() {
    int _2wise_size = old_testcase.size();
    for (int num_generated_testcase_ = 1; ; num_generated_testcase_ ++) {
        int idx = GenerateTestcase();
        if(idx == -1)
            break ;
        
        new_testcase.emplace_back(candidate_testcase_set_[idx]);
        Update3TupleInfo(candidate_testcase_set_[idx], true);

        std::cout << "\033[;32mc current test suite size: " 
            << _2wise_size + num_generated_testcase_ 
            << ", current uncovered valid 3-wise tuple nums: " << uncovered_nums
            << " \033[0m" << std::endl;
    }

    ReplenishTestCase();
}

void Expandor::Update_3TupleInfo(int st, vector<int> testcase, vector<int> sidx) {
    int sz = uncovered_3tuples.size();
    for(int p = st; p < sz; p ++) {
        _3tuples t = uncovered_3tuples[sidx[p]];
        int i = abs(t.v1) - 1, vi = t.v1 > 0;
        int pi = i << 1 | vi;
        int offset = pi * (nvar << 1);

        int j = abs(t.v2) - 1, vj = t.v2 > 0;
        int pj = j << 1 | vj;
        int idx = offset + pj;
        
        int k = abs(t.v3) - 1, vk = t.v3 > 0;

        if(covered_3tuples_bitmap[idx << 1 | vk].get(k))
            continue ;

        if(testcase[i] == vi && testcase[j] == vj && testcase[k] == vk) {
            uncovered_nums --;
            covered_3tuples_bitmap[idx << 1 | vk].set(k);
            count_each_var_uncovered[vi][i] --;
            count_each_var_uncovered[vj][j] --;
            count_each_var_uncovered[vk][k] --;
        }
    }
}

void Expandor::ReplenishTestCase() {
    std::cout << "uncovered_nums: " << uncovered_3tuples.size() << "\n";
    std::cout << "add new testcase: " << new_testcase.size() << "\n";

    vector<int> sidx(uncovered_nums, 0);
    std::iota(sidx.begin(), sidx.end(), 0);
    std::mt19937 g(seed);
    std::shuffle(sidx.begin(), sidx.end(), g);

    int sz = uncovered_3tuples.size();
    vector<pair<int, int> > prob;
    prob.resize(nvar, make_pair(0, 0));

    int old_sz = old_testcase.size();
    for(int p = 0; p < sz; p ++) {
        _3tuples t = uncovered_3tuples[sidx[p]];

        if(uncovered_nums <= 0)
            break ;

        int i = abs(t.v1) - 1, vi = t.v1 > 0;
        int pi = i << 1 | vi, offset = pi * (nvar << 1);

        int j = abs(t.v2) - 1, vj = t.v2 > 0;
        int pj = j << 1 | vj, idx = offset + pj;
        
        int k = abs(t.v3) - 1, vk = t.v3 > 0;
        
        if(covered_3tuples_bitmap[idx << 1 | vk].get(k))
            continue ;

        cdcl_sampler->add_assumption(i, vi);
        cdcl_sampler->add_assumption(j, vj);
        cdcl_sampler->add_assumption(k, vk);
        
        int testcase_size = old_testcase.size() + new_testcase.size();
        for(int i = 0; i < nvar; i ++) {
            int v1, v2;
            if(use_context_aware) {
                v1 = testcase_size - var_positive_appearance_count_[i];
                v2 = var_positive_appearance_count_[i];
            }
            else if(uniform_sampling_greedy || uniform_sampling)
                v1 = v2 = 1;
            else {
                v1 = count_each_var_uncovered[1][i];
                v2 = count_each_var_uncovered[0][i];
            }
            prob[i] = make_pair(v1, v2);
        }
        cdcl_sampler->set_prob(prob);
        vector<int> tep(nvar, 0);
        cdcl_sampler->get_solution(tep);
        search_nums_2 ++;
        new_testcase.emplace_back(tep);

        Update_3TupleInfo(p, tep, sidx);
        
        cdcl_sampler->clear_assumptions();

        std::cout << "\033[;32mc current test suite size: " 
            <<  old_sz + new_testcase.size()
            << ", current uncovered valid 3-wise tuple nums: " << uncovered_nums
            << " \033[0m" << std::endl;
    }

    std::cout << "uncovered_nums: " << uncovered_nums << "\n";

    std::cout << "search_nums: " << search_nums << "\n";
    std::cout << "search_nums_2: " << search_nums_2 << "\n";
    SaveTestcaseSet(output_testcase_path);
}

void Expandor::SaveTestcaseSet(string result_path) {
    ofstream res_file(result_path);

    std::cout << old_testcase.size() << "\n";
    for (const vector<int>& testcase: old_testcase) {
        for (int v = 0; v < nvar; v++)
            res_file << testcase[v] << " ";
        res_file << "\n";
    }

    std::cout << new_testcase.size() << "\n";
    for (const vector<int>& testcase: new_testcase) {
        for (int v = 0; v < nvar; v++)
            res_file << testcase[v] << " ";
        res_file << "\n";
    }
    res_file.close();
    cout << "c Testcase set saved in " << result_path << endl;
}

void Expandor:: Free() {
    delete [] covered_2tuples_bitmap;
    delete [] covered_3tuples_bitmap;
    delete cdcl_solver;
    delete cdcl_sampler;

    if(use_thread) {
        for(int i = 1; i < n_threads; i ++)
            delete cdcl_solver_list[i];
    }
}
