#include "Optimizer.h"
#include <iostream>
#include <signal.h>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>
using namespace std;

void HandleInterrupt(int sig){
    cout << "c" << endl;
    cout << "c caught signal... exiting" << endl;

    exit(-1);
}

void SetupSignalHandler(){
    signal(SIGTERM, HandleInterrupt);
    signal(SIGINT, HandleInterrupt);
    signal(SIGQUIT, HandleInterrupt);
    signal(SIGKILL, HandleInterrupt);
}

bool ParseArgument(int argc, char **argv, Argument &argu) {
    argu.seed = 1;
    argu.candidate_set_size = 100;
    argu.use_cnf_reduction = true;
    argu.flag_input_cnf_path = false;
    argu.flag_init_PCA_file_path = false;
    argu.flag_output_testcase_path = false;
    
    argu.use_cache = 1;
    argu.use_invalid_expand = 1;
    argu.use_context_aware = false;
    argu.use_pbo_solver = false;
    argu.uniform_sampling_greedy = false;
    argu.uniform_sampling = false;

    argu.stop_length = 500;

    argu.use_RALS = true;
    argu.opt_method = false;

    for (int i = 1; i < argc; i ++) {
        if (strcmp(argv[i], "-input_cnf_path") == 0) {
			i ++;
			if(i >= argc) return false;
			argu.input_cnf_path = argv[i];
            argu.flag_input_cnf_path = true;
			continue ;
		}
        else if (strcmp(argv[i], "-model_path") == 0) {
			i ++;
			if(i >= argc) return false;
			argu.model_path = argv[i];
			continue ;
		}
        else if (strcmp(argv[i], "-constr_path") == 0) {
			i ++;
			if(i >= argc) return false;
			argu.constr_path = argv[i];
			continue ;
		}
        else if (strcmp(argv[i], "-init_2wiseCA_file_path") == 0) {
			i ++;
			if(i >= argc) return false;
			argu.init_PCA_file_path = argv[i];
            argu.flag_init_PCA_file_path = true;
			continue ;
		}
        else if (strcmp(argv[i], "-output_testcase_path") == 0) {
			i ++;
			if(i >= argc) return false;
			argu.output_testcase_path = argv[i];
            argu.flag_output_testcase_path = true;
			continue ;
		}
        else if (strcmp(argv[i], "-seed") == 0) {
			i ++;
			if(i >= argc) return false;
			sscanf(argv[i], "%d", &argu.seed);
			continue ;
		}
        else if(strcmp(argv[i], "-delta") == 0) {
			i ++;
			if(i >= argc) return false;
			sscanf(argv[i], "%d", &argu.candidate_set_size);
			continue ;
		}
        else if(strcmp(argv[i], "-cache") == 0) {
            i ++;
            if(i >= argc) return false;
            sscanf(argv[i], "%d", &argu.use_cache);
            continue ;
        }

        else if(strcmp(argv[i], "-L") == 0) {
            i ++;
            if(i >= argc) return false;
            sscanf(argv[i], "%d", &argu.stop_length);
            continue ;
        }

        else if(strcmp(argv[i], "-use_fid") == 0) {
            i ++;
            if(i >= argc) return false;
            sscanf(argv[i], "%d", &argu.use_invalid_expand);
            continue ;
        }

        else if(strcmp(argv[i], "-use_RALS") == 0) {
            i ++;
            if(i >= argc) return false;
            sscanf(argv[i], "%d", &argu.use_RALS);
            continue ;
        }

        else if(strcmp(argv[i], "-use_context_aware") == 0) {
            argu.use_context_aware = true;
            argu.use_pbo_solver = true;
            continue ;
        }

        else if(strcmp(argv[i], "-use_pbo_solver") == 0) {
            argu.use_pbo_solver = true;
            continue ;
        }

        else if(strcmp(argv[i], "-uniform_sampling_greedy") == 0) {
            argu.uniform_sampling_greedy = true ;
            continue ;
        }

        else if(strcmp(argv[i], "-uniform_sampling") == 0) {
            argu.uniform_sampling = true;
            continue ;
        }

        else if(strcmp(argv[i], "-opt_method") == 0) {
            argu.opt_method = true;
            continue ;
        }
    }

    if(! argu.flag_input_cnf_path || ! argu.flag_init_PCA_file_path)
        return false;

    if(! argu.flag_output_testcase_path) {
        int pos = argu.input_cnf_path.find_last_of( '/' );
        string cnf_file_name = argu.input_cnf_path.substr(pos + 1);
	    cnf_file_name.replace(cnf_file_name.find(".cnf"), 4, "");

        argu.output_testcase_path = cnf_file_name + "_testcase.out";
    }
    
    return true;
}

int main(int argc, char **argv) {
    SetupSignalHandler();
    
    Argument argu;

	if (! ParseArgument(argc, argv, argu)) {
		cout << "c Argument Error!" << endl;
		return -1;
	}
    if (argu.opt_method)
        argu.stop_length = 100;

    int stop_length = argu.stop_length;

    Expandor expandor(argu);
    expandor.GenerateCoveringArray();

    int nvar, nclauses;
    vector<vector<int> > clauses;
    vector<vector<int> > _2PCA_testcase;
    vector<vector<int> > init_testcase;
    vector<_3tuples> tuples_U;
    vector<vector<int> > pos_in_cls;
    vector<vector<int> > neg_in_cls;

    expandor.dump(nvar, nclauses, clauses, _2PCA_testcase, init_testcase, 
        tuples_U, pos_in_cls, neg_in_cls);
    
    expandor.Free();

    if(! argu.use_RALS) {
        cout << "End" << endl;
        return 0;
    }

    if ((! argu.opt_method) || (! (nvar == 210 && nclauses == 94))) {
        Optimizer optimizer(argu.seed, nvar, nclauses, clauses, 
            _2PCA_testcase, init_testcase, tuples_U,
            argu.output_testcase_path, pos_in_cls, neg_in_cls,
            stop_length, ! argu.opt_method);

        vector<vector<int> >().swap(clauses);
        vector<vector<int> >().swap(_2PCA_testcase);
        vector<vector<int> >().swap(init_testcase);
        vector<_3tuples>().swap(tuples_U);
        vector<vector<int> >().swap(pos_in_cls);
        vector<vector<int> >().swap(neg_in_cls);

        optimizer.search();
    }
    
    if (argu.opt_method) {

        // cout << argu.output_testcase_path << "\n";
        std::string cmd = "python3 src/opt/opt/turnCA.py ";
        cmd += argu.input_cnf_path;
        cmd += " " + argu.model_path + " ";
        cmd += argu.output_testcase_path;
        cmd += " src/tmp/" + argu.output_testcase_path;
        // cout << cmd << "\n";
        system(cmd.c_str());

        // cout << nvar << " " << nclauses << "\n";
        if (nvar == 210 && nclauses == 94) {
            cout << "remove unnecessary testceses\n";

            cmd = "nohup java -Ddoi=3 -Doutput=numeric -jar src/opt/opt/cmd_2.92.jar cmd ";
            
            std::string ipath = "";
            int num = argu.input_cnf_path.length();
            for (int i = 0; i < num - 4; i ++)
                ipath += argu.input_cnf_path[i];
            ipath += ".txt";

            cmd += ipath;
            cmd += " src/tmp/_" + argu.output_testcase_path;
            
            // cmd += " 2> /dev/null";
            // cout << cmd << "\n";
            system(cmd.c_str());
            // cout << "QAQ";
            
            std::string path = "src/tmp/__" + argu.output_testcase_path;
            vector<vector<int> > new_tc;

            std::ifstream fin_model(argu.model_path.c_str());
            int n; fin_model >> n; fin_model >> n;
            // cout << "n = " << n << "\n";
            fin_model.close();

            /* */
            std::string p =  "src/tmp/_" + argu.output_testcase_path;
            std::ifstream fin(p.c_str());
            string tmp;
            while (fin >> tmp)
                if (tmp[0] == '-')
                    break ;
            int x;        
            while (fin >> x) {
                vector<int> tc(n);
                tc[0] = x;
                for (int i = 1; i < n; i ++)
                    fin >> tc[i];
                new_tc.emplace_back(tc);
            }
            fin.close();
            // cout << new_tc.size() << "\n";

            vector<vector<int> > myCA;
            p = "src/tmp/" + argu.output_testcase_path;
            std::ifstream fin_my(p.c_str());
            while (fin_my >> x) {
                vector<int> tc(n);
                tc[0] = x;
                for (int i = 1; i < n; i ++)
                    fin_my >> tc[i];
                myCA.emplace_back(tc);
            }
            fin_my.close();

            std::mt19937_64 gen;
            gen.seed(argu.seed + 2);

            int tc_num = myCA.size();
            for (int i = 0; i < 60; i ++) {
                int idx = gen() % tc_num;
                new_tc.emplace_back(myCA[idx]);
            }

            tc_num = new_tc.size();
            vector<int> sidx(tc_num, 0);
            std::iota(sidx.begin(), sidx.end(), 0);
            std::shuffle(sidx.begin(), sidx.end(), gen);

            std::string result_path = "src/tmp/" + argu.output_testcase_path;
            ofstream res_file(result_path);
            for (int i = 0; i < tc_num; i ++) {
                // cout << sidx[i] << "\n";
                const vector<int>& testcase = new_tc[sidx[i]];
                for (int v = 0; v < n; v++)
                    res_file << testcase[v] << " ";
                    res_file << "\n";
            }
            res_file.close();
        }

        cmd = "cd src/opt/opt; python3 run_autoopt.py ";
        cmd += "../../../" + argu.model_path + " " + "../../../" + argu.constr_path;
        cmd += " " + std::to_string(argu.seed) + " 1000 ";
        cmd += "../../tmp/" + argu.output_testcase_path;
        cmd += " ../../../" + argu.output_testcase_path;
        // cout << cmd << "\n";
        system(cmd.c_str());

        std::cout << "c Testcase set saved in " << argu.output_testcase_path << std::endl;
    }
    cout << "End" << endl;
    return 0;
}