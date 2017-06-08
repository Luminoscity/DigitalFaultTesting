//
//  CircuitTypes.cpp
//  ECC283 Project
//
//  Created by Timothy Ambrose on 10 May 2017.
//  Copyright (c) 2017 Tim Ambrose. All rights reserved.
//

#include "CircuitTypes.h"

//--------------------------Wire-----------------------------------------------
Wire::Wire(string wireName) {
   in = NULL;
   out = NULL;
   splitIn = NULL;
   splitOut = NULL;
   isFaulty = false;
   faultyVal = false;
   _wireName = wireName;
}

bool Wire::isSplit()
{
   return splitOut != NULL;
}

WireSplit * Wire::getSplit()
{
   return splitOut;
}

void Wire::LinkIn(Single_Output_Gate *inGate) {
   in = inGate;
}

void Wire::LinkIn(WireSplit *inSplit) {
   splitIn = inSplit;
}

void Wire::LinkOut(Single_Output_Gate *outGate) {
   out = outGate;
}

void Wire::LinkOut(WireSplit *outSplit) {
   splitOut = outSplit;
}

void Wire::MakeFaulty(bool faultyValue) {
   isFaulty = true;
   faultyVal = faultyValue;
}

void Wire::MakeNotFaulty() {
   isFaulty = false;
}

string Wire::name()
{
   return _wireName;
}

bool Wire::val()
{
   bool value = false;
   if (isFaulty)
      value = faultyVal;
   else if (in != NULL) {
      value = in->operate();
   }
   else if (splitIn != NULL) {
      value = splitIn->val();
   }
   else {
      fprintf(stderr, "Wire.val(): Floating Wire: No Inputs.\n");
      exit(-1);
   }
   return value;
}

//--------------------------WireSplit-------------------------------------------
bool WireSplit::val()
{
   if (in == NULL) {
      fprintf(stderr, "WireSplit.val(): Input disconnected.\n");
      exit(-1);
   }
   return in->val();
}

void WireSplit::LinkIn(Wire * inWire)
{
   in = inWire;
}

void WireSplit::AddOut(Wire *outWire)
{
   outs.push_back(outWire);
}

vector<Wire*> WireSplit::getOuts()
{
   return outs;
}

//--------------------------Single_Output_Gate---------------------------------
Single_Output_Gate::Single_Output_Gate(string gateName) {
   _gateName = gateName;
}

string Single_Output_Gate::name()
{
   return _gateName;
}

//--------------------------Input_Gate-----------------------------------------
void Input_Gate::LinkOut(Wire * outWire) {
   output = outWire;
}

void Input_Gate::setVal(bool inputValue) {
   val = inputValue;
}

bool Input_Gate::operate() {
   return val;
}

bool Input_Gate::isInput()
{
   return true;
}

bool Input_Gate::isOutput()
{
   return false;
}

//--------------------------Output_Gate----------------------------------------
void Output_Gate::LinkIn(Wire * inWire) {
   if (inputs.size() < 1)
      inputs.push_back(inWire);
   else
      inputs[0] = inWire;
}

bool Output_Gate::operate() {
   return inputs[0]->val();
}

bool Output_Gate::isInput()
{
   return false;
}

bool Output_Gate::isOutput()
{
   return true;
}

//--------------------------Circuit--------------------------------------------
Circuit::Circuit() {

}

void Circuit::Build(const char* input_circuit_filename) {
   const char sec1[] = "begin";
   string toReplace[] = { "(", ")", ",", ":", ";" };
   string replaceWith[] = { " ( ", " ) ", " , ", " : ", " ; " };
   int replaceArrayLen = sizeof(toReplace) / sizeof(toReplace[0]);
   ifstream file(input_circuit_filename);
   string InputsOutputsSignalsSection = "",
          ArchSection = "";
   stringstream ss;

   ss << file.rdbuf();
   string whole = ss.str();

   for (int i = 0; i < replaceArrayLen; ++i) {
      size_t sPos = 0;
      while ((sPos = whole.find(toReplace[i], sPos)) != string::npos) {
         whole.replace(sPos, toReplace[i].length(), replaceWith[i]);
         sPos += replaceWith[i].length();
      }
   }

   size_t pos = whole.find(sec1);
   InputsOutputsSignalsSection = whole.substr(0, pos);
   ArchSection = whole.substr(pos, string::npos);

   GetSignalsInputsOutputs(InputsOutputsSignalsSection);
   GetArchitecture(ArchSection);
   if (numInputs() < 1) {
      fprintf(stderr, "Circuit.Build(): Circuit Has No Inputs");
      exit(-1);
   }
}

void Circuit::GetSignalsInputsOutputs(string section) {
   vector<string> tokLine;
   string line;
   stringstream ss;
   ss.str(section);

   while (getline(ss, line)) {
      tokLine = split(line);

      if (tokLine.size() > 0) {
         if ((tokLine.size() != INPUTS_LINE_TOK_MIN &&
              tokLine.size() < INPUTS_LINE_TOK_BRANCH_MIN) ||
             tokLine.size() % 2 == 0 ||
             tokLine[tokLine.size() - 4] != ":" ||
             tokLine[tokLine.size() - 2] != "std_logic" ||
             tokLine[tokLine.size() - 1] != ";")
            SyntaxError("Circuit.Build()", line);

         string name = tokLine[0];
         if (tokLine[tokLine.size() - 3] == "in")
            inputs.push_back(new Input_Gate(name));
         else if (tokLine[tokLine.size() - 3] == "out")
            outputs.push_back(new Output_Gate(name));
         else if (tokLine[tokLine.size() - 3] == "signal") {
            wires.push_back(new Wire(name));
            if (tokLine[1] == ",") {  //wire split
               int idx = 1;
               wireSplits.push_back(new WireSplit());
               wires.back()->LinkOut(wireSplits.back());
               wireSplits.back()->LinkIn(wires.back());
               do {
                  wires.push_back(new Wire(tokLine[++idx]));
                  wires.back()->LinkIn(wireSplits.back());
                  wireSplits.back()->AddOut(wires.back());
               } while (tokLine[++idx] == ",");
            }
         }
         else
            SyntaxError("Circuit.Build()", line);
      }
   }
}

void Circuit::GetArchitecture(string section) {
   vector<string> tokLine;
   string line;
   stringstream ss;
   ss.str(section);

   while (getline(ss, line)) {
      tokLine = split(line);

      if (tokLine.size() > 0 && tokLine[0] != "begin" && tokLine[0] != "end") {
         if (tokLine.size() < ARCH_LINE_TOK_MIN ||
            tokLine.size() % 2 == 0 || tokLine[1] != ":" ||
            tokLine[3] != "(" || tokLine[tokLine.size() - 2] != ")" ||
            tokLine[tokLine.size() - 1] != ";")
            SyntaxError("Circuit.Build()", line);

         if (tokLine[2] == "AndGate2") {
            gates.push_back(new AND_Gate2(tokLine[0]));
            Build2InputComponent(tokLine);
         }
         else if (tokLine[2] == "NandGate2") {
            gates.push_back(new NAND_Gate2(tokLine[0]));
            Build2InputComponent(tokLine);
         }
         else if (tokLine[2] == "OrGate2") {
            gates.push_back(new OR_Gate2(tokLine[0]));
            Build2InputComponent(tokLine);
         }
         else if (tokLine[2] == "NorGate2") {
            gates.push_back(new NOR_Gate2(tokLine[0]));
            Build2InputComponent(tokLine);
         }
         else if (tokLine[2] == "NotGate1") {
            gates.push_back(new NOT_Gate(tokLine[0]));
            Build1InputComponent(tokLine);
         }
         else if (tokLine[2] == "BufferGate1") {
            gates.push_back(new BUFFER_GATE(tokLine[0]));
            Build1InputComponent(tokLine);
         }
         else if (tokLine[2] == "XorGate2") {
            gates.push_back(new XOR_Gate2(tokLine[0]));
            Build2InputComponent(tokLine);
         }
         else if (tokLine[2] == "XnorGate2") {
            gates.push_back(new XNOR_Gate2(tokLine[0]));
            Build2InputComponent(tokLine);
         }
         else if (tokLine[2] == "InBranch")
            BuildInBranch(tokLine);
         else if (tokLine[2] == "BranchOut")
            BuildBranchOut(tokLine);
         else
            SyntaxError("Circuit.Build(): Invalid Component", line);
      }
   }
}

void Circuit::Build2InputComponent(vector<string> &line) {
   unsigned idx = 4;
   Input_Gate *inGate = getInputByName(line[idx]);
   Wire *linkWire;
   if (inGate != NULL) {
      wires.push_back(new Wire(line[idx]));
      inGate->LinkOut(wires.back());
      wires.back()->LinkIn(inGate);
      wires.back()->LinkOut(gates.back());
      ((Two_Input_Gate *)gates.back())->LinkIn1(wires.back());
   }
   else if ((linkWire = getWireByName(line[idx])) != NULL) {
      linkWire->LinkOut(gates.back());
      ((Two_Input_Gate *)gates.back())->LinkIn1(linkWire);
   }
   else {
      fprintf(stderr, "Circuit.Build(): No such source: %s\n",
         line[idx].c_str());
      exit(-1);
   }

   idx += 2;
   inGate = getInputByName(line[idx]);
   if (inGate != NULL) {
      wires.push_back(new Wire(line[idx]));
      inGate->LinkOut(wires.back());
      wires.back()->LinkIn(inGate);
      wires.back()->LinkOut(gates.back());
      ((Two_Input_Gate *)gates.back())->LinkIn2(wires.back());
   }
   else if ((linkWire = getWireByName(line[idx])) != NULL) {
      linkWire->LinkOut(gates.back());
      ((Two_Input_Gate *)gates.back())->LinkIn2(linkWire);
   }
   else {
      fprintf(stderr, "Circuit.Build(): No such source: %s\n",
         line[idx].c_str());
      exit(-1);
   }

   idx += 2;
   Output_Gate *outGate = getOutputByName(line[idx]);
   if (outGate != NULL) {
      wires.push_back(new Wire(line[idx]));
      wires.back()->LinkIn(gates.back());
      ((Two_Input_Gate *)gates.back())->LinkOut(wires.back());
      wires.back()->LinkOut(outGate);
      outGate->LinkIn(wires.back());
   }
   else if ((linkWire = getWireByName(line[idx])) != NULL) {
      linkWire->LinkIn(gates.back());
      ((Two_Input_Gate *)gates.back())->LinkOut(linkWire);
   }
   else {
      fprintf(stderr, "Circuit.Build(): No such wire: %s\n",
         line[idx].c_str());
      exit(-1);
   }
}

void Circuit::Build1InputComponent(vector<string> &line) {
   unsigned idx = 4;
   Input_Gate *inGate = getInputByName(line[idx]);
   Wire *linkWire;
   if (inGate != NULL) {
      wires.push_back(new Wire(line[idx]));
      inGate->LinkOut(wires.back());
      wires.back()->LinkIn(inGate);
      wires.back()->LinkOut(gates.back());
      ((One_Input_Gate *)gates.back())->LinkIn(wires.back());
   }
   else if ((linkWire = getWireByName(line[idx])) != NULL) {
      linkWire->LinkOut(gates.back());
      ((One_Input_Gate *)gates.back())->LinkIn(linkWire);
   }
   else {
      fprintf(stderr, "Circuit.Build(): No such source: %s\n",
             line[idx].c_str());
      exit(-1);
   }

   idx += 2;
   Output_Gate *outGate = getOutputByName(line[idx]);
   if (outGate != NULL) {
      wires.push_back(new Wire(line[idx]));
      wires.back()->LinkIn(gates.back());
      ((One_Input_Gate *)gates.back())->LinkOut(wires.back());
      wires.back()->LinkOut(outGate);
      outGate->LinkIn(wires.back());
   }
   else if ((linkWire = getWireByName(line[idx])) != NULL) {
      linkWire->LinkIn(gates.back());
      ((One_Input_Gate *)gates.back())->LinkOut(linkWire);
   }
   else {
      fprintf(stderr, "Circuit.Build(): No such wire: %s\n",
         line[idx].c_str());
      exit(-1);
   }
}

void Circuit::BuildInBranch(vector<string> &line) {
   unsigned idx = 4;
   Input_Gate *inGate = getInputByName(line[idx]);
   Wire *linkWire = getWireByName(line[0]);
   if (inGate != NULL && linkWire != NULL && linkWire->isSplit()) {
      inGate->LinkOut(linkWire);
      linkWire->LinkIn(inGate);
   }
   else if (inGate == NULL) {
      fprintf(stderr, "Circuit.Build(): No such input: %s\n",
              line[idx].c_str());
      exit(-1);
   }
   else if (linkWire == NULL) {
      fprintf(stderr, "Circuit.Build(): No such wire: %s\n",
              line[0].c_str());
      exit(-1);
   }
   else {
      fprintf(stderr, "Circuit.Build(): Wire %s is not part of a WireSplit.\n",
              line[0].c_str());
      exit(-1);
   }
}

void Circuit::BuildBranchOut(vector<string> &line) {
   unsigned idx = 3;
   Output_Gate *outGate;
   Wire *linkWire = getWireByName(line[0]);;
   if (linkWire == NULL) {
      fprintf(stderr, "Circuit.Build(): No such wire: %s\n",
              line[0].c_str());
      exit(-1);
   }
   if (!linkWire->isSplit()) {
      fprintf(stderr, "Circuit.Build(): Wire %s is not part of a WireSplit.\n",
              line[0].c_str());
      exit(-1);
   }
   vector<Wire *> splitWires = linkWire->getSplit()->getOuts();
   unsigned wiresIdx = 0;

   do {
      outGate = getOutputByName(line[++idx]);
      if (outGate != NULL) {
         outGate->LinkIn(splitWires[wiresIdx]);
         splitWires[wiresIdx++]->LinkOut(outGate);
      }
      else {
         fprintf(stderr, "Circuit.Build(): No such output: %s\n",
                 line[idx].c_str());
         exit(-1);
      }
   } while (line[++idx] == ",");
}

vector<string> Circuit::split(string const &section) {
   std::istringstream buffer(section);
   vector<string> ret((istream_iterator<string>(buffer)),
      istream_iterator<string>());
   return ret;
}

void Circuit::SyntaxError(string context, string line) {
   fprintf(stderr, "%s: Bad Syntax on line: \n%s\n", context.c_str(),
           line.c_str());
   exit(-1);
}

void Circuit::setInputs(vector<bool> inputValues) {
   if (inputValues.size() != inputs.size()) {
      fprintf(stderr, "Circuit::setInputs(): Circuit inputs: %d, given inputs:"
              " %d\n", (int)inputs.size(), (int)inputValues.size());
      exit(-1);
   }
   for (size_t i = 0; i < inputs.size(); ++i)
      inputs[i]->setVal(inputValues[i]);
}

vector<bool> Circuit::evaluateAll()
{
   vector<bool> toReturn;
   for (auto it = outputs.begin(); it != outputs.end(); ++it)
      toReturn.push_back((*it)->operate());

   return toReturn;
}

bool Circuit::evaluate(unsigned inputNum)
{
   if (inputNum >= inputs.size()) {
      fprintf(stderr, "Circuit::evaluate(unsigned): Circuit inputs: %d, given"
              " input: %d\n", (int)inputs.size() - 1, inputNum);
      exit(-1);
   }

   return outputs[inputNum]->operate();
}

bool Circuit::evaluate(string gateName)
{
   bool toReturn = false;
   bool found = false;
   for (auto it = outputs.begin(); it != outputs.end() && !found; ++it)
      if ((*it)->name() == gateName) {
         found = true;
         toReturn = (*it)->operate();
      }

   if (!found) {
      fprintf(stderr, "Circuit::evaluate(string): No such input: %s\n",
              gateName.c_str());
      exit(-1);
   }

   return toReturn;
}

bool Circuit::evaluate(string gateName, vector<bool> inputValues)
{
   bool toReturn = false;
   bool found = false;
   vector<bool> oldInputs;
   for (size_t i = 0; i < inputs.size(); ++i) {
      oldInputs.push_back(inputs[i]->operate());
      inputs[i]->setVal(inputValues[i]);
   }

   for (auto it = outputs.begin(); it != outputs.end() && !found; ++it)
      if ((*it)->name() == gateName) {
         found = true;
         toReturn = (*it)->operate();
      }

   if (!found) {
      fprintf(stderr, "Circuit::evaluate(string): No such input: %s\n",
              gateName.c_str());
      exit(-1);
   }
   
   for (size_t i = 0; i < inputs.size(); ++i)
      inputs[i]->setVal(oldInputs[i]);

   return toReturn;
}

unsigned Circuit::numGates()
{
   return (int)gates.size();
}

unsigned Circuit::numWires()
{
   return (int)wires.size();
}

unsigned Circuit::numInputs()
{
   return (int)inputs.size();
}

unsigned Circuit::numOutputs()
{
   return (int)outputs.size();
}

vector<string> Circuit::getWireNames()
{
   vector<string> wireNames;

   for (auto x : wires)
      wireNames.push_back(x->name());

   return wireNames;
}

void Circuit::makeWireFaultyByName(string name, bool faultyValue) {
   Wire *toFault = getWireByName(name);
   if (toFault != NULL)
      toFault->MakeFaulty(faultyValue);
   else {
      fprintf(stderr, "Circuit::makeWireFaultyByName(): No such wire: %s\n",
              name.c_str());
      exit(-1);
   }
}

void Circuit::makeWireNotFaultyByName(string name) {
   Wire *toFault = getWireByName(name);
   if (toFault != NULL)
      toFault->MakeNotFaulty();
   else {
      fprintf(stderr, "Circuit::makeWireNotFaultyByName(): No such wire: %s\n",
              name.c_str());
      exit(-1);
   }
}

Wire *Circuit::getWireByName(string name)
{
   bool found = false;
   Wire *toReturn;
   for (auto it = wires.begin(); it != wires.end() && !found; ++it)
      if ((*it)->name() == name) {
         toReturn = *it;
         found = true;
      }

   return found ? toReturn : NULL;
}

Input_Gate *Circuit::getInputByName(string name)
{
   bool found = false;
   Input_Gate *toReturn;
   for (auto it = inputs.begin(); it != inputs.end() && !found; ++it)
      if ((*it)->name() == name) {
         toReturn = *it;
         found = true;
      }

   return found ? toReturn : NULL;
}

Output_Gate *Circuit::getOutputByName(string name)
{
   bool found = false;
   Output_Gate *toReturn;
   for (auto it = outputs.begin(); it != outputs.end() && !found; ++it)
      if ((*it)->name() == name) {
         toReturn = *it;
         found = true;
      }

   return found ? toReturn : NULL;
}

Single_Output_Gate *Circuit::getOtherGateByName(string name)
{
   bool found = false;
   Single_Output_Gate *toReturn;
   for (auto it = gates.begin(); it != gates.end() && !found; ++it)
      if ((*it)->name() == name) {
         toReturn = *it;
         found = true;
      }

   return found ? toReturn : NULL;
}

vector<unsigned> Circuit::FindTestsForFault(string faultyWire,
                                           bool faultyValue, float timeLimit) {
   clock_t t1 = clock();
   unsigned nInputs = numInputs();
   unsigned nOutputs = numOutputs();
   vector<bool> inputs(nInputs, false);
   unsigned limit = (unsigned)pow(2.0, nInputs);
   vector<unsigned> results;
   bool inTime = true;

   for (unsigned input = 0; input < limit && inTime; ++input) {
      for (unsigned i = 0; i < nInputs; ++i)
         inputs[nInputs - i - 1] = (input >> i) & 0x1;

      setInputs(inputs);

      clock_t t2 = clock();
      inTime = (((float)t2 - (float)t1) / CLOCKS_PER_SEC) < timeLimit;
      if (inTime) {
         vector<bool> out = evaluateAll();
         t2 = clock();
         inTime = (((float)t2 - (float)t1) / CLOCKS_PER_SEC) < timeLimit;
         if (inTime) {
            int result1 = 0;
            for (unsigned i = 0; i < nOutputs; ++i)
               result1 += (out[i] ? (unsigned)pow(10, nOutputs - i - 1) : 0);

            makeWireFaultyByName(faultyWire, faultyValue);
            t2 = clock();
            inTime = (((float)t2 - (float)t1) / CLOCKS_PER_SEC) < timeLimit;
            if (inTime) {
               out = evaluateAll();
               t2 = clock();
               inTime = (((float)t2 - (float)t1) / CLOCKS_PER_SEC) < timeLimit;
               if (inTime) {
                  int result2 = 0;
                  for (unsigned i = 0; i < nOutputs; ++i)
                     result2 += (out[i] ?
                     (unsigned)pow(10, nOutputs - i - 1) : 0);

                  if (result1 != result2)
                     results.push_back(input);
               }
            }
            makeWireNotFaultyByName(faultyWire);
         }
      }
   }

   return results;
}

vector<unsigned> Circuit::FindTestsForFault(string faultyWire,
                                            bool faultyValue) {
   unsigned nInputs = numInputs();
   unsigned nOutputs = numOutputs();
   vector<bool> inputs(nInputs, false);
   unsigned limit = (unsigned)pow(2.0, nInputs);
   vector<unsigned> results;

   for (unsigned input = 0; input < limit; ++input) {
      for (unsigned i = 0; i < nInputs; ++i)
         inputs[nInputs - i - 1] = (input >> i) & 0x1;

      setInputs(inputs);

      vector<bool> out = evaluateAll();
      int result1 = 0;
      for (unsigned i = 0; i < nOutputs; ++i)
         result1 += (out[i] ? (unsigned)pow(10.0, nOutputs - i - 1) : 0);

      makeWireFaultyByName(faultyWire, faultyValue);
      out = evaluateAll();
      int result2 = 0;
      for (unsigned i = 0; i < nOutputs; ++i)
         result2 += (out[i] ? (unsigned)pow(10.0, nOutputs - i - 1) : 0);
      makeWireNotFaultyByName(faultyWire);

      if (result1 != result2)
         results.push_back(input);

   }

   return results;
}

vector<string> Circuit::FindFaultsDetected(unsigned test) {
   unsigned nInputs = numInputs();
   unsigned nOutputs = numOutputs();
   vector<bool> inputs(nInputs, false);
   unsigned limit = (unsigned)pow(2.0, nInputs);
   vector<string> results;

   for (unsigned i = 0; i < nInputs; ++i)
      inputs[nInputs - i - 1] = (test >> i) & 0x1;
   setInputs(inputs);

   for (auto& wire: wires) {
      vector<bool> out = evaluateAll();
      int result1 = 0;
      for (unsigned i = 0; i < nOutputs; ++i)
         result1 += (out[i] ? (unsigned)pow(10.0, nOutputs - i - 1) : 0);

      makeWireFaultyByName(wire->name(), false);
      out = evaluateAll();
      int result2 = 0;
      for (unsigned i = 0; i < nOutputs; ++i)
         result2 += (out[i] ? (unsigned)pow(10.0, nOutputs - i - 1) : 0);

      if (result1 != result2)
         results.push_back(wire->name() + " / 0");

      makeWireFaultyByName(wire->name(), true);
      out = evaluateAll();
      result2 = 0;
      for (unsigned i = 0; i < nOutputs; ++i)
         result2 += (out[i] ? (unsigned)pow(10.0, nOutputs - i - 1) : 0);
      makeWireNotFaultyByName(wire->name());

      if (result1 != result2)
         results.push_back(wire->name() + " / 1");
   }

   return results;
}

//--------------------------One_Input_Gate-------------------------------------
void One_Input_Gate::LinkIn(Wire *inWire) {
   if (inputs.size() == 0)
      inputs.push_back(inWire);
   else
      inputs[0] = inWire;
}

void One_Input_Gate::LinkOut(Wire *outWire) {
   output = outWire;
}

bool One_Input_Gate::operate() {
   if (inputs.size() < 1) {
      fprintf(stderr, "One_Input_Gate.operate(): no inputs\n");
      exit(-1);
   }
   return operate(inputs[0]->val());
}

bool One_Input_Gate::isInput()
{
   return false;
}

bool One_Input_Gate::isOutput()
{
   return false;
}

//--------------------------Two_Input_Gate-------------------------------------
void Two_Input_Gate::LinkIn1(Wire *inWire) {
   if (inputs.size() == 0)
      inputs.push_back(inWire);
   else
      inputs[0] = inWire;
}

void Two_Input_Gate::LinkIn2(Wire *inWire) {
   if (inputs.size() == 1)
      inputs.push_back(inWire);
   else if (inputs.size() == 2)
      inputs[1] = inWire;
   else {
      fprintf(stderr, "Two_Input_Gate.LinkIn2(): LinkIn1 must be called before"
              " LinkIn2\n");
      exit(-1);
   }
}

void Two_Input_Gate::LinkOut(Wire *outWire) {
   output = outWire;
}

bool Two_Input_Gate::operate() {
   if (inputs.size() < 2) {
      fprintf(stderr, "Two_Input_Gate.operate(): Less than 2 inputs\n");
      exit(-1);
   }
   return operate(inputs[0]->val(), inputs[1]->val());
}

bool Two_Input_Gate::isInput()
{
   return false;
}

bool Two_Input_Gate::isOutput()
{
   return false;
}

//--------------------------Logic Gates----------------------------------------
bool AND_Gate2::operate(bool inputA, bool inputB) {
   return inputA && inputB;
}

bool NAND_Gate2::operate(bool inputA, bool inputB) {
   return !(inputA && inputB);
}

bool OR_Gate2::operate(bool inputA, bool inputB) {
   return inputA || inputB;
}

bool NOR_Gate2::operate(bool inputA, bool inputB) {
   return !(inputA || inputB);
}

bool NOT_Gate::operate(bool inputA) {
   return !inputA;
}

bool BUFFER_GATE::operate(bool inputA) {
   return inputA;
}

bool XOR_Gate2::operate(bool inputA, bool inputB) {
   return (inputA && !inputB) || (!inputA && inputB);
}

bool XNOR_Gate2::operate(bool inputA, bool inputB) {
   return (inputA && inputB) || (!inputA && !inputB);
}
