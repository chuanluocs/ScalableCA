#!/usr/bin/env python3

import os
import sys
import time
import random
import math

from sparkle_csv_help import Sparkle_CSV

def get_time_pid_random_string():
    my_time_str = time.strftime('%Y-%m-%d-%H:%M:%S', time.localtime(time.time()))
    my_pid = os.getpid()
    my_pid_str = str(my_pid)
    my_random = random.randint(1, sys.maxsize)
    my_random_str = str(my_random)
    my_time_pid_random_str = my_time_str + '_' + my_pid_str + '_' + my_random_str
    return my_time_pid_random_str

def calc_virtual_best_performance_of_portfolio(performance_data_csv):
    list_instances = performance_data_csv.list_rows()
    
    sum_solution_quality = 0.0
    
    for instance_name in list_instances:
        list_performance_vector_on_instance = performance_data_csv.list_get_specific_row(instance_name)
        best_solution_quality_on_instance = min(list_performance_vector_on_instance)
        sum_solution_quality += best_solution_quality_on_instance
    
    avg_solution_quality = float(sum_solution_quality) / float(len(list_instances))
    return avg_solution_quality 

def calc_marginal_contribution(performance_data_csv_path):
    performance_data_csv = Sparkle_CSV(performance_data_csv_path)
    best_performance_all = calc_virtual_best_performance_of_portfolio(performance_data_csv)
    
    list_solvers = performance_data_csv.list_columns()
    map_amc = {}
    total_amc = 0.0

    for solver_name in list_solvers:
        tmp_performance_data_csv = Sparkle_CSV(performance_data_csv_path)
        tmp_performance_data_csv.delete_column(solver_name)
        tmp_best_performance = calc_virtual_best_performance_of_portfolio(tmp_performance_data_csv)
        
        tmp_amc = 0
        if tmp_best_performance > best_performance_all:
            tmp_amc = math.log(tmp_best_performance/best_performance_all)
        map_amc[solver_name] = tmp_amc
        total_amc += tmp_amc
    
    
    map_rmc = {}
    for solver_name in list_solvers:
        map_rmc[solver_name] = float(map_amc[solver_name])/total_amc
    
    return map_rmc

if __name__ == '__main__':
    
    avg_performance_data_csv_path = sys.argv[1]
    best_performance_data_csv_path = sys.argv[2]
    
    map_rmc_avg = calc_marginal_contribution(avg_performance_data_csv_path)
    map_rmc_best = calc_marginal_contribution(best_performance_data_csv_path)
    
    avg_performance_data_csv = Sparkle_CSV(avg_performance_data_csv_path)
    
    map_score = {}
    
    total_score = 0.0
    list_solvers = avg_performance_data_csv.list_columns()
    for solver_name in list_solvers:
        tmp_rmc_avg = map_rmc_avg[solver_name]
        tmp_rmc_best = map_rmc_best[solver_name]
        tmp_score = (1.0 + tmp_rmc_avg) * (1.0 + tmp_rmc_best) - 1.0
        map_score[solver_name] = tmp_score
        total_score += tmp_score
    
    list_solver_ratio = []
    for solver_name in list_solvers:
        tmp_score = map_score[solver_name]
        tmp_ratio = tmp_score/total_score
        if tmp_ratio > 0.001:
            list_solver_ratio.append([solver_name, tmp_ratio])
    
    list_solver_ratio = sorted(list_solver_ratio, key = lambda item: item[1], reverse = True)
    print(list_solver_ratio)
    
