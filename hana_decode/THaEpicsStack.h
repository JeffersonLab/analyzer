#ifndef THaEpicsStack_
#define THaEpicsStack_

/////////////////////////////////////////////////////////////////////
//
//   THaEpicsStack
//   Stack of recent EPICS data.
//
//   EPICS data are stored on a stack for the most recent events.
//   The data are a list of pairs of (tag, value), where tag
//   is character description and value is its numerical value.
//   Also each update to the stack we store the CODA event number
//   nearest in time. 
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#define MAXEPS 40    // Number of variables stored per event
#define MAXSTACK 10  // Max stack size (num of events stored here)
#include "Rtypes.h"
#include "TString.h"
#include <stdlib.h>
#include <iostream>

class THaEpicsStack 
{

public:

   THaEpicsStack();
   virtual ~THaEpicsStack();
   double getData (const char* tag) const;      // get recent value corr. to tag
   double getData (const char* tag, int event) const;    // value nearest 'event'
   double getData (int stack_position, int var_index) const;     // data by index
   int loadData(const char* tag, const char* value, int event);   // load data characterized by pair: (tag,value) 
   int loadData(const char* tag, double value, int event);        // nearest in time to CODA event# 'event'
   void setupDefaultList();                     // setup a default list of popular variables
   int findVar(const char* tag) const;          // return index to variable or EPI_ERR
   int addEpicsTag(const char* tag);            // Add a tag to the list   
   void bumpStack();       // bump the stack pointer (used only by decoder)
   void print();

private:

   static const int EPI_ERR = -1;
   static const int EPI_OK = 1;
   struct epicsDataStack {     // Stack of recent epics data (each is an event)
      int event;               // The nearest CODA event number
      TString tag[MAXEPS];     // EPICS character tags, e.g. "IPM1H04A.XPOS"
      double value[MAXEPS];    // The data value corresponding to the above tag.
      bool filled[MAXEPS];     // Flag to indicate if it was filled by valid data.
   } epicsData[MAXSTACK];
   int stack_point,stack_size;
   int nepics_strings;
   TString epics_var_list[MAXEPS];    // List of desired EPICS variables.
   void init();
   double getLastData(const char* tag) const;  
   double getLastFilled(int point, int ival) const;

   ClassDef(THaEpicsStack,0)  // EPICS data 

};

#endif





