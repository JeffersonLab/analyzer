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
#include <iostream>

using namespace std;

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

}


double THaEpicsStack::getData(const char* tag, int event) const {
// Get the EPICS data closest to event# 'event'
    int i,j,k,diff,min;
    min = 9999999;
    j = findVar(tag);
    if (j == EPI_ERR) return 0;
    k = -1;
    for (i = 0; i < stack_size; i++) {
       diff = event - epicsData[i].event;
       if (diff < 0) diff = -diff;
       if ((diff < min) && (epicsData[i].filled[j])) {
            min = diff;
            k = i;
       }
    }
    if (k >= 0) return epicsData[k].value[j];
    return 0;    
}

double THaEpicsStack::getLastFilled(int point, int ival) const {
  if (ival < 0 || ival >= MAXEPS) return 0.0;
  if (epicsData[point].filled[ival]) return epicsData[point].value[ival];
  for (int j = 0; j<MAXSTACK-1; j++) {
    if (--point < 0) {
      if(stack_size<MAXSTACK) break;
      point = MAXSTACK-1;
    }
    if (epicsData[point].filled[ival]) return epicsData[point].value[ival];
  }
  return 0.0;
}

void THaEpicsStack::print() {
     cout << "Number of EPICS variables";
     cout << " registered "<<dec<<nepics_strings<<endl;
     for (int i = 0; i < nepics_strings; i++) {
       cout << "Registered variable "<<epics_var_list[i].Data()<<endl;
     }
     cout << "Loaded data stack size "<<stack_size<<endl;
     for (int i = 0; i < stack_size; i++) {
       cout << "Data at stack point "<<i<<endl;
       for (int j = 0; j < nepics_strings; j++) {
         cout << "Event number "<<epicsData[i].event;
         cout << "  Tag "<<epicsData[i].tag[j];
         cout << "  Value "<<getData(i,j)<<endl;
       }
     }
}

ClassImp(THaEpicsStack)
