//
//  CircuitTypes.h
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
#include <iterator>
#include <fstream>
#include <sstream>
#include <vector>

#define INPUTS_LINE_TOK_MIN 5
#define INPUTS_LINE_TOK_BRANCH_MIN 9
#define ARCH_LINE_TOK_MIN 7

using std::ifstream;
using std::cout;
using std::vector;
using std::string;
using std::stringstream;
using std::istream_iterator;

class WireSplit;
class Single_Output_Gate;

class Wire {
private:
   Single_Output_Gate *in;
   Single_Output_Gate *out;
   WireSplit *splitIn;
   WireSplit *splitOut;
   bool faultyVal;
   bool isFaulty;
   string _wireName;
public:
   Wire(string wireName);
   bool isSplit();
   WireSplit *getSplit();
   void LinkIn(Single_Output_Gate *inGate);
   void LinkIn(WireSplit *inSplit);
   void LinkOut(Single_Output_Gate *outGate);
   void LinkOut(WireSplit *outSplit);
   void MakeFaulty(bool faultyValue);
   void MakeNotFaulty();
   string name();
   bool val();
};

class WireSplit {
private:
   Wire *in;
   vector<Wire *> outs;
public:
   void LinkIn(Wire *inWire);
   void AddOut(Wire *outWire);
   vector<Wire *> getOuts();
   bool val();
};

class Single_Output_Gate {
protected:
   vector<Wire *> inputs;  /* For every input, a link back to the
                                            gate that drives that input.
                                            NULL if the input is an input to
                                            the circuit instead of a gate */
   Wire *output;           /* A link to the gate that this
                                            single output drives.
                                            NULL if the output is an output to
                                            the circuit instead of a gate */
   string _gateName;
public:
   Single_Output_Gate(string gateName);
   string name();
   virtual bool operate() = 0;     //return value of the output
   virtual bool isInput() = 0;     //is this an Input_Gate
   virtual bool isOutput() = 0;    //is this an Output_Gate
};

class Input_Gate : public Single_Output_Gate {
private:
   bool val;
public:
   Input_Gate(string gateName) : Single_Output_Gate(gateName) {};
   void LinkOut(Wire *outWire);
   void setVal(bool inputValue);   //value of the input
   bool operate();
   bool isInput();
   bool isOutput();
};

class Output_Gate : public Single_Output_Gate {
public:
   Output_Gate(string gateName) : Single_Output_Gate(gateName) {};
   void LinkIn(Wire *inWire);
   bool operate();
   bool isInput();
   bool isOutput();
};

class Circuit {
private:
   vector<Input_Gate *> inputs;
   vector<Output_Gate *> outputs;
   vector<Single_Output_Gate *> gates;
   vector<Wire *> wires;
   vector<WireSplit *> wireSplits;
   void GetSignalsInputsOutputs(string section);
   void GetArchitecture(string section);
   void Build2InputComponent(vector<string> &line);
   void Build1InputComponent(vector<string> &line);
   void BuildInBranch(vector<string> &line);
   void BuildBranchOut(vector<string> &line);
   vector<string> split(string const &section);
   void SyntaxError(string context, string line);
public:
   Circuit();
   void Build(const char* input_circuit_filename);
   void setInputs(vector<bool> inputValues);
   vector<bool> evaluateAll();
   bool evaluate(unsigned inputNum);
   bool evaluate(string gateName);
   bool evaluate(string gateName, vector<bool> inputValues);
   unsigned numGates();
   unsigned numWires();
   unsigned numInputs();
   unsigned numOutputs();
   vector<string> getWireNames();
   void makeWireFaultyByName(string name, bool faultyValue);
   void makeWireNotFaultyByName(string name);
   Wire *getWireByName(string name);
   Input_Gate *getInputByName(string name);
   Output_Gate *getOutputByName(string name);
   Single_Output_Gate *getOtherGateByName(string name);
   vector<uint64_t> FindTestsForFault(string faultyWire, bool faultyValue);
   vector<uint64_t> FindTestsForFault(string faultyWire, bool faultyValue,
                                      float timeLimit);
   vector<string> FindFaultsDetected(uint64_t test);
};

class One_Input_Gate : public Single_Output_Gate {
public:
   One_Input_Gate(string gateName) : Single_Output_Gate(gateName) {};
   virtual bool operate();
   virtual void LinkIn(Wire *inWire);
   virtual void LinkOut(Wire *outWire);

   //evaluate the logical value of the output from the given input
   virtual bool operate(bool inputA) = 0;
   bool isInput();
   bool isOutput();
};

class Two_Input_Gate : public Single_Output_Gate {
public:
   Two_Input_Gate(string gateName) : Single_Output_Gate(gateName) {};
   virtual bool operate();
   virtual void LinkIn1(Wire *inWire);
   virtual void LinkIn2(Wire *inWire);
   virtual void LinkOut(Wire *outWire);

   //evaluate the logical value of the output from the given input
   virtual bool operate(bool inputA, bool inputB) = 0;
   bool isInput();
   bool isOutput();
};

class AND_Gate2 : public Two_Input_Gate {
public:
   AND_Gate2(string gateName) : Two_Input_Gate(gateName) {};
   bool operate(bool inputA, bool inputB);
};

class NAND_Gate2 : public Two_Input_Gate {
public:
   NAND_Gate2(string gateName) : Two_Input_Gate(gateName) {};
   bool operate(bool inputA, bool inputB);
};

class OR_Gate2 : public Two_Input_Gate {
public:
   OR_Gate2(string gateName) : Two_Input_Gate(gateName) {};
   bool operate(bool inputA, bool inputB);
};

class NOR_Gate2 : public Two_Input_Gate {
public:
   NOR_Gate2(string gateName) : Two_Input_Gate(gateName) {};
   bool operate(bool inputA, bool inputB);
};

class NOT_Gate : public One_Input_Gate {
public:
   NOT_Gate(string gateName) : One_Input_Gate(gateName) {};
   bool operate(bool inputA);
};

class BUFFER_GATE : public One_Input_Gate {
public:
   BUFFER_GATE(string gateName) : One_Input_Gate(gateName) {};
   bool operate(bool inputA);
};

class XOR_Gate2 : public Two_Input_Gate {
public:
   XOR_Gate2(string gateName) : Two_Input_Gate(gateName) {};
   bool operate(bool inputA, bool inputB);
};

class XNOR_Gate2 : public Two_Input_Gate {
public:
   XNOR_Gate2(string gateName) : Two_Input_Gate(gateName) {};
   bool operate(bool inputA, bool inputB);
};
