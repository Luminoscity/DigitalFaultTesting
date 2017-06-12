//
//  TestGenerator.cpp
//  ECC283 Project
//
//  Created by Timothy Ambrose on 10 May 2017.
//  Copyright (c) 2017 Tim Ambrose. All rights reserved.
//

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <bitset>
#include <vector>
#include <set>
#include "CircuitTypes.h"

using std::set;


void CheckArgs(int argc, char *argv[]);
string binary(uint64_t x, int width);
void DeleteDominants(vector<vector<uint64_t> > &sets, vector<string>
                     &testStrings);

int main(int argc, char *argv[]) {
   CheckArgs(argc, argv);
   Circuit circuit;
   circuit.Build(argv[1]);
   bool printHex = argc > 3 && tolower(argv[3][0]) == 'h';

   auto wireNames = circuit.getWireNames();
   vector<vector<uint64_t> > tests;
   int ins = circuit.numInputs();

   cout << std::internal << std::setfill('0');

   printf("Wire Name    Stuck-At Tests");
   char nameBuffer[25];
   vector<string> testStrings;
   float timeLimit = (float)atof(argv[2]);

   auto t1 = clock();
   for (auto& x : wireNames) {
      if (timeLimit == 0.0)
         tests.push_back(circuit.FindTestsForFault(x, false));
      else
         tests.push_back(circuit.FindTestsForFault(x, false, timeLimit));

      snprintf(nameBuffer, 25, "\n%-12s 0        ", x.c_str());
      printf(nameBuffer);
      testStrings.push_back(nameBuffer);
      if (tests.back().size() > 0)
         for (auto& y : tests.back())
            if (printHex)
               cout << std::hex << std::uppercase
                    << std::setw((ins - 1) / 4 + 1) << y << std::dec << " ";
            else
               printf("%s ", binary(y, ins).c_str());
      else
         printf("redundant");

      if (timeLimit == 0.0)
         tests.push_back(circuit.FindTestsForFault(x, true));
      else
         tests.push_back(circuit.FindTestsForFault(x, true, timeLimit));

      snprintf(nameBuffer, 25, "\n%-12s 1        ", x.c_str());
      printf(nameBuffer);
      testStrings.push_back(nameBuffer);
      if (tests.back().size() > 0)
         for (auto& y : tests.back())
            if (printHex)
               cout << std::hex << std::uppercase
                    << std::setw((ins - 1) / 4 + 1) << y << std::dec << " ";
            else
               printf("%s ", binary(y, ins).c_str());
      else
         printf("redundant"); 
   }

   DeleteDominants(tests, testStrings);
   

   printf("\n\nNon-Dominant Fault Tests:");
   printf("\nWire Name    Stuck-At Tests");
   for (size_t i = 0; i < tests.size(); ++i) {
      cout << testStrings[i];
      if (tests[i].size() > 0)
         for (auto& y : tests[i])
            if (printHex)
               cout << std::hex << std::uppercase
                    << std::setw((ins - 1) / 4 + 1) << y << std::dec << " ";
            else
               printf("%s ", binary(y, ins).c_str());
      else
         printf("redundant");
   }
   
   auto t2 = clock();
   float duration = ((float)t2 - (float)t1) / CLOCKS_PER_SEC;
   cout << "\n\nReduced set generated in " << duration << " seconds.\n";
   cout << "Inputs: " << circuit.numInputs()
        << "\nOutputs: " << circuit.numOutputs()
        << "\nWires: " << circuit.numWires()
        << "\nGates: " << circuit.numGates() << "\n";

   set<uint64_t> reducedSet;
   for (auto& x: tests)
      for (auto& y: x)
         reducedSet.insert(y);
   cout << "Reduced Test Set (" << reducedSet.size() << " tests): ";
   for (auto& x: reducedSet) {
      if (printHex)
         cout << std::hex << std::uppercase
              << std::setw((ins - 1) / 4 + 1) << x << " ";
      else
         printf("%s ", binary(x, ins).c_str());
   }

   printf("\n\nTest%*s Faults Detected\n", (ins > 3 ? ins - 4 : 2), "");
   for (auto& x : reducedSet) {
      if (printHex) {
         cout << std::hex << std::uppercase
              << std::setw((ins - 1) / 4 + 1) << x << std::dec;
         printf(" %*s", (ins > 23 ? 0 : 5 - (ins - 1) / 4), "");
      }
      else
         printf("%s %*s", binary(x, ins).c_str(), (ins > 5 ? 0 : 6 - ins), "");

      auto faults = circuit.FindFaultsDetected(x);
      for (auto& y : faults) {
         cout << y << "   ";
      }
      printf("\n");
   }

   return 0;
}

void CheckArgs(int argc, char *argv[]) {
   if (argc < 3 || argc > 4) {
      fprintf(stderr, "Usage: %s CircuitFile per_fault_time_limit [hex]\n"
                      "0.0 seconds indicates no time limit.\n", argv[0]);
      exit(-1);
   }
   if (atof(argv[2]) < 0.0) {
      fprintf(stderr, "per_fault_time_limit must be at least 0.0 (0 "
                      "indicates no limit)\n");
      exit(-1);
   }
   ifstream file(argv[1]);
   if (!file.good()) {
      fprintf(stderr, "Could not open file: %s\n", argv[1]);
      exit(-1);
   }
}

string binary(uint64_t x, int width) {
   int max = sizeof(unsigned) * 8;
   string toReturn = "";
   for (int i = 0; i < width && i < max; ++i)
      toReturn = ((x >> i & 1) ? '1' : '0') + toReturn;

   return toReturn;
}

void DeleteDominants(vector<vector<uint64_t> > &sets, vector<string>
                     &testStrings) {
   for (unsigned i = 0; i < sets.size(); ++i) {
      if (sets[i].size() > 0) {
         for (unsigned j = 0; j < sets.size(); ++j)
            if (sets[j].size() >= sets[i].size() && i != j) {
               bool matches = true;
               for (auto sIt = sets[i].begin(); sIt != sets[i].end() &&
                    matches; ++sIt)
                  if (std::find(sets[j].begin(), sets[j].end(), *sIt) ==
                     sets[j].end())
                     matches = false;
               if (matches) {
                  sets.erase(sets.begin() + j);
                  testStrings.erase(testStrings.begin() + j);
                  i = (unsigned)-1;
                  j = sets.size();
               }
            }
      }
   }
}
