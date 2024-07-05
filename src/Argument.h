#include <cstring>
#include <iostream>
#include <string>
using namespace std;
using std::string;

struct Argument {
    string input_cnf_path;
	string reduced_cnf_file_path;
	string init_PCA_file_path;
    string output_testcase_path;
    string model_path, constr_path;

    int seed;
    int candidate_set_size;
    int use_cnf_reduction;

    bool flag_input_cnf_path;
    bool flag_init_PCA_file_path;
    bool flag_output_testcase_path;

    int use_cache;

    int stop_length;

    int use_invalid_expand;

    bool use_context_aware;
    bool use_pbo_solver;
    bool uniform_sampling;
    bool uniform_sampling_greedy;
    bool opt_method;
    bool use_RALS;

    void print() {
        cout << "input_cnf_path = " + input_cnf_path << endl;
        cout << "init_PCA_file_path = " + init_PCA_file_path << endl;
        cout << "output_testcase_path = " + output_testcase_path << endl;
        cout << seed << " " << candidate_set_size << " " << use_cnf_reduction << endl;
        return ;
    }
};