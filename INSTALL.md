## Instructions for Building *ScalableCA*

```
sh build.sh
```

By executing this script file, users can build both *SamplingCA* and *ScalableCA*. Note that both *SamplingCA* and *ScalableCA* should be built on a 64-bit GNU/Linux operating system.

**Tip**: Both *SamplingCA* and *ScalableCA* depend on *MiniSAT*, but *MiniSAT* may not compile successfully when using specific versions of gcc. In this case, users may seek for solutions on the Internet or in the Github page of [MiniSAT](https://github.com/niklasso/minisat).

## Quick Installation Testing

To check whether the compilation is successful or not, the user may run the following command in the root directory:


```
python3 run.py 1 cnf_benchmarks/toybox.cnf toybox_CA.out
```

The command above executes the `run.py` script, the arguments of which are briefly described in the [README.md](./README.md). By running the command above, the user should be able to find a 3-wise CA for the benchmark `cnf_benchmarks/toybox.cnf` in `./toybox_CA.out`.

The console output is expected to be similar with the following text:

```
c now call SamplingCA to generate an initial 2-wise CA
...
...  <-- possibly some huge amount of output by Coprocessor
...
1: 147696
2: 215991
3: 236892
4: 243238
5: 247682
6: 251078
7: 253000
8: 254520
9: 255284
10: 255854
11: 256154
12: 256318
13: 256406
14: 256454
15: 256486
16: 256494

c Clear final: fix-up all remaining tuples ...
c All possible 2-tuple number: 256494
c 2-tuple coverage: 1
c Generate testcase set finished, containing 16 testcases!
c CPU time cost by generating testcase set: 0.759 seconds
c Testcase set saved in ./SamplingCA/toybox_CA.out
c 2-tuple number of generated testcase set: 256494
c now call ScalableCA to generate a 3-wise CA
...
...  <-- possibly some huge amount of output by Coprocessor
...
begin find uncovered 3 tuples
covered valid 3-wise tuple nums: 60029675
uncovered valid 3-wise tuple nums: 533284
all valid 3-wise tuple nums: 60562959
invalid 3-wise tuple nums: 152906993
c current test suite size: 17, current uncovered valid 3-wise tuple nums: 413252 
c current test suite size: 18, current uncovered valid 3-wise tuple nums: 329284 
c current test suite size: 19, current uncovered valid 3-wise tuple nums: 269504 
c current test suite size: 20, current uncovered valid 3-wise tuple nums: 225380 
c current test suite size: 21, current uncovered valid 3-wise tuple nums: 182356 
c current test suite size: 22, current uncovered valid 3-wise tuple nums: 148184 
c current test suite size: 23, current uncovered valid 3-wise tuple nums: 124240 
c current test suite size: 24, current uncovered valid 3-wise tuple nums: 107080 
c current test suite size: 25, current uncovered valid 3-wise tuple nums: 91868 
c current test suite size: 26, current uncovered valid 3-wise tuple nums: 77812 
c current test suite size: 27, current uncovered valid 3-wise tuple nums: 66936 
c current test suite size: 28, current uncovered valid 3-wise tuple nums: 56876 
c current test suite size: 29, current uncovered valid 3-wise tuple nums: 48448 
c current test suite size: 30, current uncovered valid 3-wise tuple nums: 40828 
c current test suite size: 31, current uncovered valid 3-wise tuple nums: 35316 
c current test suite size: 32, current uncovered valid 3-wise tuple nums: 30356 
c current test suite size: 33, current uncovered valid 3-wise tuple nums: 25364 
c current test suite size: 34, current uncovered valid 3-wise tuple nums: 21188 
c current test suite size: 35, current uncovered valid 3-wise tuple nums: 18192 
c current test suite size: 36, current uncovered valid 3-wise tuple nums: 15552 
c current test suite size: 37, current uncovered valid 3-wise tuple nums: 13216 
c current test suite size: 38, current uncovered valid 3-wise tuple nums: 11144 
c current test suite size: 39, current uncovered valid 3-wise tuple nums: 9072 
c current test suite size: 40, current uncovered valid 3-wise tuple nums: 7496 
c current test suite size: 41, current uncovered valid 3-wise tuple nums: 6164 
c current test suite size: 42, current uncovered valid 3-wise tuple nums: 5084 
c current test suite size: 43, current uncovered valid 3-wise tuple nums: 4156 
c current test suite size: 44, current uncovered valid 3-wise tuple nums: 3444 
c current test suite size: 45, current uncovered valid 3-wise tuple nums: 2860 
c current test suite size: 46, current uncovered valid 3-wise tuple nums: 2420 
c current test suite size: 47, current uncovered valid 3-wise tuple nums: 2004 
c current test suite size: 48, current uncovered valid 3-wise tuple nums: 1620 
c current test suite size: 49, current uncovered valid 3-wise tuple nums: 1232 
c current test suite size: 50, current uncovered valid 3-wise tuple nums: 896 
c current test suite size: 51, current uncovered valid 3-wise tuple nums: 640 
c current test suite size: 52, current uncovered valid 3-wise tuple nums: 392 
c current test suite size: 53, current uncovered valid 3-wise tuple nums: 224 
c current test suite size: 54, current uncovered valid 3-wise tuple nums: 120 
c current test suite size: 55, current uncovered valid 3-wise tuple nums: 40 
c current test suite size: 56, current uncovered valid 3-wise tuple nums: 0 
uncovered_nums: 0
add new testcase: 40
uncovered_nums: 0
search_nums: 533284
search_nums_2: 4200
16
40
c Testcase set saved in toybox_CA.out
Optimizer init success
c current 3-wise CA size: 56, step #0 
c current 3-wise CA size: 55, step #44 
c current 3-wise CA size: 54, step #86 
c current 3-wise CA size: 53, step #160 
c current 3-wise CA size: 52, step #251 
c current 3-wise CA size: 51, step #445 
c current 3-wise CA size: 50, step #606 
c current 3-wise CA size: 49, step #904 
c current 3-wise CA size: 48, step #1391 
c current 3-wise CA size: 47, step #1683 
c current 3-wise CA size: 46, step #2140 
final 3-wise CA size is: 46
c Testcase set saved in toybox_CA.out
End
```

Here, the output lines between `c now call SamplingCA to generate an initial 2-wise CA` and `c now call ScalableCA to generate a 3-wise CA` are the output of *SamplingCA*. The user can know from the line `c Generate testcase set finished, containing 16 testcases!` that the 2-wise CA generated by *SamplingCA* contains 16 test cases, and there are 256494 valid 2-wise tuples covered by the 2-wise CA in total.

The output lines after `c now call ScalableCA to generate a 3-wise CA` are the output of *ScalableCA*. The user can know from the line `covered valid 3-wise tuple nums: 60029675` that the 2-wise CA generated by *SamplingCA* has covered 60029675 valid 3-wise tuples and the next line `uncovered valid 3-wise tuple nums: 533284` means that there are 533284 valid 3-wise tuples that remain uncovered. The line `c current test suite size: 56, current uncovered valid 3-wise tuple nums: 0` indicates that the initial 3-wise CA generated by *ScalableCA* without RALS has a size of 56. The line `final 3-wise CA size is: 46` indicates that after applying RALS, the final size of the 3-wise CA generated by *ScalableCA* has been reduced to 46.

We finally note that due to potential differences in the random number generation mechanism over different versions of g++, the console output and the size of the generated 3-wise CA on the user's machine may be slightly different from the one presented here.