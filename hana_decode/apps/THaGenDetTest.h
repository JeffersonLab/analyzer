#ifndef Podd_THaGenDetTest_h_
#define Podd_THaGenDetTest_h_

/////////////////////////////////////////////////////////////////////
//
//   THaGenDetTest
//   General Detector Test of Decoder
//
//   THaGenDetTest is an illustrative example of how to use
//   the Hall A C++ data I/O and decoder classes for
//   a detector class one might develop.
//   This is also used to measure the baseline speed
//   of I/O and decoder.
//   This is a "throw away" test code.
//
//   author  Robert Michaels  (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "TString.h"

class THaEvData;

class THaGenDetTest
{

 public:

	 THaGenDetTest();
// Called at initialization phase of code
	 void init();
// Processing of event data
	 void process_event(THaEvData *evdata);

 private:
         static const unsigned MAX = 100;
	 unsigned mycrates[MAX];
	 unsigned myslots[MAX];
	 unsigned chanlo[MAX],chanhi[MAX];
	 TString mydevice[MAX];

};

#endif
