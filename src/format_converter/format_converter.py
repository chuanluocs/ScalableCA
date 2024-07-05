import os, sys

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 format_converter.py modelFile constrFile cnfFile")
        exit(0)

    modelFile = sys.argv[1]
    constrFile = sys.argv[2]
    cnfFile = f"tmp/{modelFile.replace('/', '.').split('.')[-2]}_tmp.cnf"
    actsFile = f"tmp/{modelFile.replace('/', '.').split('.')[-2]}_tmp.txt"
    # print(cnfFile, actsFile)
    command = f"./src/FormatConverter {modelFile} {constrFile} {actsFile}"
    # print(command)
    os.system(command)

    ctwFile = f"tmp/{modelFile.replace('/', '.').split('.')[-2]}_tmp.ctw"

    command = f"python3 turn.py {actsFile} {ctwFile}"
    os.system(command)

    listfile = f"tmp/{modelFile.replace('/', '.').split('.')[-2]}_list_tmp.txt"
    command = f"./ctw_parser {ctwFile} {cnfFile} {listfile} 2> /dev/null"
    os.system(command)