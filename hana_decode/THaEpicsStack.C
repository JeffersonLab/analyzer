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

#include "THaEpicsStack.h"
#include <cstdlib>

ClassImp(THaEpicsStack)

THaEpicsStack::THaEpicsStack() {
    init();
};

THaEpicsStack::~THaEpicsStack() {
};

void THaEpicsStack::init() {
   int i,j;
   nepics_strings = 0;
   stack_point = 0;
   stack_size = 0;
   for (i = 0; i<MAXSTACK; i++) {
     for (j = 0; j<MAXEPS; j++) {
        epicsData[i].filled[j] = false;
     }
   }
};


int THaEpicsStack::addEpicsTag(const char* tag) {
  for (int i=0; i<nepics_strings; i++) {
    if (epics_var_list[i] == tag) return EPI_ERR;
  }
  if (nepics_strings < MAXEPS) {
    epics_var_list[nepics_strings++] = tag;
    return EPI_OK;
  } else {
    cout << "THaEpicsStack: addEpicsTag: WARNING: attempt to register "<<endl;
    cout << " too many epics tags "<<endl;
    return EPI_ERR;
  }
};

void THaEpicsStack::setupDefaultList() { 
// Here we setup a list of popular EPICS variables which will be decoded.
// More can be added to the list via public method addEpicsTag.

     addEpicsTag("IPM1H03A.XPOS");  // The 2 target BPM's were at one time
     addEpicsTag("IPM1H03A.YPOS");  // called '3A' and '4A', but this
     addEpicsTag("IPM1H03B.XPOS");  // got renamed 3->4 in early 2000.
     addEpicsTag("IPM1H03B.YPOS");
     addEpicsTag("IPM1H04A.XPOS");   // New names of BPMs near target
     addEpicsTag("IPM1H04A.YPOS");   // in year 2000. 
     addEpicsTag("IPM1H04B.XPOS");
     addEpicsTag("IPM1H04B.YPOS");
     addEpicsTag("hac_bcm_average");
     addEpicsTag("hac_bcm_dvm1_read");
     addEpicsTag("hac_bcm_dvm2_read");
     addEpicsTag("hac_bcm_dvm1_current");
     addEpicsTag("hac_bcm_dvm2_current");
     addEpicsTag("IBC0L02Current");
     addEpicsTag("MBSY3C_energy");
     addEpicsTag("halla_MeV");

};

int THaEpicsStack::findVar(const char* tag) const {
     int i;
     for (i=0; i<nepics_strings; i++) {
       if (strstr(tag,epics_var_list[i].Data()) != NULL) {
         return i;
       }
     }
     return EPI_ERR;
};

int THaEpicsStack::loadData(const char* tag, const char* val, int event) {
     return loadData(tag,std::atof(val),event);     
};

int THaEpicsStack::loadData(const char* tag, double val, int event) { 
// Load data into present stack position.
// To be used by decoder only.
     int i = findVar(tag);
     if ( i == EPI_ERR ) return EPI_ERR;
     if ( (i < 0) || (i >= MAXEPS) ) return EPI_ERR;
     epicsData[stack_point].event = event;
     epicsData[stack_point].tag[i] = epics_var_list[i];
     epicsData[stack_point].value[i] = val;
     epicsData[stack_point].filled[i] = true;
     if (stack_point >= stack_size) stack_size = stack_point+1;
     return EPI_OK;
};

void THaEpicsStack::bumpStack() {
// This is used by only by decoder to increment the stack.
    stack_point++;
    if (stack_point >= MAXSTACK) stack_point=0;
    for (int i=0; i<MAXEPS; i++) epicsData[stack_point].filled[i]=false;
};

double THaEpicsStack::getData(const char* tag) const {
    return getLastData(tag);
};

double THaEpicsStack::getData(int stack, int idx) const {
// Get data from stack position 'stack'
     if (( stack < 0) || (stack >= MAXSTACK)) return -999999;
     if (( idx < 0 ) || (idx >= MAXEPS)) return -999999; 
     return getLastFilled(stack,idx);
};

double THaEpicsStack::getData(const char* tag, int event) const {
// Get the EPICS data closest to event# 'event'
    int i,j,k,diff,min;
    min = 9999999;
    j = findVar(tag);
    if (j == EPI_ERR) return 0;
    k = -1;
    for (i = 0; i < stack_size; i++) {
       diff = event - epicsData[i].event;
       if (diff < 0) diff = -1*diff;
       if ((diff < min) && (epicsData[i].filled[j])) {
            min = diff;
            k = i;
       }
    }
    if (k >= 0) return epicsData[k].value[j];
    return 0;    
};

double THaEpicsStack::getLastData(const char* tag) const {
// Return the value previous to the present stack point.
// This assumes the stack_point is bumped after event was
// loaded by decoder, and it points to the next place to fill.
     int i,point;
     i = findVar(tag);
     if (i == EPI_ERR) return 0;
     if (stack_point == 0) {
         point = MAXSTACK-1;
     } else {
         point = stack_point-1;
     }
     return getLastFilled(point,i);
};

double THaEpicsStack::getLastFilled(int point, int ival) const {
     int j;
     if (ival < 0 || ival >= MAXEPS) return 0;
     if (epicsData[point].filled[ival]) return epicsData[point].value[ival];
     for (j = 0; j<MAXSTACK-1; j++) {
       j = point - 1;
       if (j < 0) j = j + MAXSTACK;
       if (j >= 0 && j < MAXSTACK) {
         if (epicsData[j].filled[ival]) return epicsData[j].value[ival];
       }
     }
     return 0;
};

void THaEpicsStack::print() {
     int i,j;
     cout << "Number of EPICS variables";
     cout << " registered "<<dec<<nepics_strings<<endl;
     for (i = 0; i < nepics_strings; i++) {
       cout << "Registered variable "<<epics_var_list[i].Data()<<endl;
     }
     cout << "Loaded data stack size "<<stack_size<<endl;
     for (i = 0; i < stack_size; i++) {
       cout << "Data at stack point "<<i<<endl;
       for (j = 0; j < nepics_strings; j++) {
         cout << "Event number "<<epicsData[i].event;
         cout << "  Tag "<<epicsData[i].tag[j].Data();
         cout << "  Value "<<getData(i,j)<<endl;
       }
     }
};


