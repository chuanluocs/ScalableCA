import os, sys

if __name__ == "__main__":

    if len(sys.argv) != 4:
        print("Error")
        exit(0)

    modelFile = sys.argv[1]
    CAFile = sys.argv[2]
    bugFile = sys.argv[3]

    print(bugFile)
    with open(bugFile, "r") as file:
        lines = file.readlines()
    
    Faults = []
    for line in lines:
        arr = line.split()
        if len(arr) == 0:
            continue
        if arr[0] == "Fault":
            Faults.append([])
        else:
            Faults[-1].append(arr)
    # print(Faults)

    with open(modelFile, "r") as file2:
        lines = file2.readlines()
    values_num = list(map(int, lines[2].split()))
    # print(values_num)
    nvar = len(values_num)
    encode, cnt = {}, 0
    for i in range(nvar):
        for j in range(values_num[i]):
            encode[cnt] = (i, j)
            cnt += 1
    # print(encode)
    
    CA = []
    with open(CAFile, "r") as file3:
        lines = file3.readlines()
    for line in lines:
        tc = list(map(int, line.split()))
        CA.append(tc)
    # print(CA)
    
    def check(mod, tc):
        for x in mod:
            i, j = encode[int(x)]
            # print(i, j)
            if tc[i] != j:
                return False
        return True

    detection, fault_idx = [], 0
    for fault in Faults:
        flag = False
        for mod in fault:
            for tc in CA:
                if check(mod, tc):
                    flag = True
                    break
            if flag:
                break
        if flag:
            detection.append(f"Fault {fault_idx}")
        fault_idx += 1
        # print()
    print(detection)
