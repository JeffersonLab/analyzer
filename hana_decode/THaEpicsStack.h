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

#include "TString.h"
#include <cstdlib>

class THaEpicsStack {

public:

   THaEpicsStack() { init(); }
   virtual ~THaEpicsStack() {}
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

   static const int EPI_ERR = -1, EPI_OK = 1;
   static const int MAXEPS = 40;   // Number of variables stored per event
   static const int MAXSTACK = 10; // Max stack size (num of events stored here)
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

//=============== inline functions ================================

inline
int THaEpicsStack::findVar(const char* tag) const {
     for (int i=0; i<nepics_strings; i++)
       if (strstr(tag,epics_var_list[i].Data()) != NULL)
         return i;
     return EPI_ERR;
}

inline
int THaEpicsStack::loadData(const char* tag, const char* val, int event) {
  return loadData(tag,std::atof(val),event);     
}

inline
void THaEpicsStack::bumpStack() {
// This is used by only by decoder to increment the stack.
  stack_point++;
  if (stack_point > stack_size) stack_size = stack_point;
  if (stack_point >= MAXSTACK) stack_point=0;
  for (int i=0; i<MAXEPS; i++) epicsData[stack_point].filled[i]=false;
}

inline
double THaEpicsStack::getData(const char* tag) const {
  return getLastData(tag);
}

inline
double THaEpicsStack::getData(int stack, int idx) const {
// Get data from stack position 'stack'
     if (( stack < 0) || (stack >= MAXSTACK)) return -999999;
     if (( idx < 0 ) || (idx >= MAXEPS)) return -999999; 
     return getLastFilled(stack,idx);
};

inline
double THaEpicsStack::getLastData(const char* tag) const {
// Return the value previous to the present stack point.
// This assumes the stack_point is bumped after event was
// loaded by decoder, and it points to the next place to fill.
  int i = findVar(tag);
  if (i == EPI_ERR) return 0;
  if (stack_point == 0)
    return getLastFilled(MAXSTACK-1,i);
  return getLastFilled(stack_point-1,i);
}

inline
int THaEpicsStack::loadData(const char* tag, double val, int event) { 
// Load data into present stack position.
// To be used by decoder only.
     int i = findVar(tag);
     if ( i == EPI_ERR ) return EPI_ERR;
     epicsData[stack_point].event = event;
     epicsData[stack_point].tag[i] = epics_var_list[i];
     epicsData[stack_point].value[i] = val;
     epicsData[stack_point].filled[i] = true;
     return EPI_OK;
}

#endif





