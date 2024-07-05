import os, sys

if __name__ == "__main__":

    if len(sys.argv) != 4 and len(sys.argv) != 5:
        print("Error")
        exit(0)
    
    if len(sys.argv) == 4:
        seed = int(sys.argv[1])
        cnfFile = sys.argv[2]
        outputFile = sys.argv[3]
        # command = f"sh simple_run.sh {seed} {cnfFile} {outputFile}"
        # os.system(command)
        print("c now call SamplingCA to generate an initial 2-wise CA")
        command = f"./SamplingCA/SamplingCA -seed {seed} -input_cnf_path {cnfFile} -output_testcase_path ./SamplingCA/{outputFile}"
        os.system(command)
        print("c now call ScalableCA to generate a 3-wise CA")
        command = f"./ScalableCA -seed {seed} -input_cnf_path {cnfFile} -init_2wiseCA_file_path ./SamplingCA/{outputFile} -output_testcase_path {outputFile}"
        os.system(command)

    else:
        seed = int(sys.argv[1])
        modelFile = sys.argv[2]
        constrFile = sys.argv[3]
        outputFile = sys.argv[4]

        cnfFile = modelFile.replace('/', '.').split('.')[-1] + ".cnf"
        command = f"sh format_converter.sh {modelFile} {constrFile}"
        os.system(command)

        cnfFile = f"src/format_converter/tmp/{modelFile.replace('/', '.').split('.')[-2]}_tmp.cnf"
        print(cnfFile)
        # command = f"sh run.sh {seed} {cnfFile} {outputFile}"
        # os.system(command)
        print("c now call SamplingCA to generate an initial 2-wise CA")
        command = f"./SamplingCA/SamplingCA -seed {seed} -input_cnf_path {cnfFile} -output_testcase_path ./SamplingCA/{outputFile}"
        print(command)
        os.system(command)
        print("c now call ScalableCA to generate a 3-wise CA")
        command = f"./ScalableCA -seed {seed} -input_cnf_path {cnfFile} -init_2wiseCA_file_path ./SamplingCA/{outputFile} -output_testcase_path {outputFile} -model_path {modelFile} -constr_path {constrFile} -opt_method"
        os.system(command)
