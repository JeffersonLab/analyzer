/////////////////////////////////////////////////////////////////////
//
//   THaNormScaler
//
//   A Unit of Normalization Scaler Data.  This class is 
//   for internal use as a PRIVATE member of THaScaler class.  
//   This means the user should look at THaScaler and not 
//   this class.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaNormScaler.h"
#include "THaScalerBank.h"
#include "THaScalerDB.h"

#ifndef ROOTPRE3
ClassImp(THaNormScaler)
#endif

THaNormScaler::THaNormScaler() : THaScalerBank(" ") {
}

THaNormScaler::THaNormScaler(string myname) : THaScalerBank(myname) {
// A Normalization scaler is also a scalerbank, characterized
// additionally by helicity and having specific data like charge
// or trigger counts on individual channels specificed in chan_name.
   ctrig = new char[20];
   int pos1;
   helicity = 0;
   pos1 = myname.find("minus");
   if (pos1 >= 0) helicity = -1;
   pos1 = myname.find("plus");
   if (pos1 >= 0) helicity = 1;
   chan_name.reserve(50);
// The following names must appear in the scaler.map file
   chan_name.push_back("bcm_u1");
   chan_name.push_back("bcm_d1");
   chan_name.push_back("bcm_u3");
   chan_name.push_back("bcm_d3");
   chan_name.push_back("bcm_u10");
   chan_name.push_back("bcm_d10");
   for (int i = 0; i < 12; i++) {
     sprintf(ctrig,"trigger-%d",i);
     chan_name.push_back(ctrig);
   }
   chan_name.push_back("TS-accept");
   chan_name.push_back("clock");
   chan_name.push_back("unser");
   chan_name.push_back("edtpulser");
   chan_name.push_back("edtatrig");
   chan_name.push_back("helicity+");
   chan_name.push_back("helicity-");
   chan_name.push_back("sray");
   chan_name.push_back("strobe");
   chan_name.push_back("q10abusy");
   chan_name.push_back("clkabusy");
   chan_name.push_back("mlut1");
   chan_name.push_back("mlut3");
};


THaNormScaler::~THaNormScaler() {
};

Int_t THaNormScaler::Init(Int_t mycrate, multimap<string, BscaLoc>& bmap) {
// Initialize the map between names and indices of data[] array.
// The scaler described by scalerloc is matched to names in bmap.
  int minc = size; 
  int maxc = 0;
  int lslot = -1;
  //  pair<string, int> ptpair;
  //  pair<int, string> tppair;
  typedef map<string,Int_t>::value_type pt_valType;
  typedef map<Int_t,string>::value_type tp_valType;
  for (vector<string>::iterator p = chan_name.begin(); 
       p != chan_name.end(); p++) {
    string myname = *p;
    multimap<string, BscaLoc>::iterator lb = bmap.lower_bound(myname);
    multimap<string, BscaLoc>::iterator ub = bmap.upper_bound(myname);
    for (multimap<string, BscaLoc>::iterator pm = lb; pm != ub; pm++) {
      BscaLoc sloc = pm->second;
      if (sloc.crate == mycrate && sloc.helicity == helicity) {
         lslot = sloc.slot;
         if (sloc.startchan[0] < minc) minc = sloc.startchan[0];
         if (sloc.startchan[0] > maxc) maxc = sloc.startchan[0];
// This assumes there is only ONE channel for the channel name 'myname'
	 //         ptpair.first = myname;
	 //	 ptpair.second = sloc.startchan[0];  
         name_to_array.insert( pt_valType( myname, sloc.startchan[0] ));
	 //         tppair.first = sloc.startchan[0];
	 //         tppair.second = myname;
         array_to_name.insert( tp_valType( sloc.startchan[0], myname ));
      }
    }
 }
 location.clear();
 if (maxc > minc) {
    location.crate = mycrate;
    location.slot = lslot;
    location.helicity = helicity;
    location.startchan.push_back(minc);
    location.numchan.push_back(maxc-minc+1);
    location.setvalid();
    location.short_desc = name;
    didinit = 1;
 } 
 return 1;
};   

Int_t THaNormScaler::GetTrig(Int_t trig, Int_t histor) {
  sprintf(ctrig,"trigger-%d",trig);
  return GetData(ctrig, histor);
};

Int_t THaNormScaler::GetData(string myname, Int_t histor) {
// Get data corresponding to string 'myname' (e.g. 'u3', 'trigger-1', etc)
// histor is a flag indicating if it is the present event or previous one.
// Previous events stored in upper half of THaScalerBank data array.
  map<string, int>::iterator p = name_to_array.find(myname);
  if (p != name_to_array.end()) {
    int idx = p->second;
    if (histor == 1) idx = idx + size;
    if (idx >= 0 && idx < 2*size) return data[idx];
  }
  return 0;
};

string THaNormScaler::GetChanName(int index) {
// Get the channel name by index
  map<int, string>::iterator p = array_to_name.find(index);
  if (p != array_to_name.end()) {
    return p->second;
  }
  return "NULL";
};

Int_t THaNormScaler::GetData(Int_t chan, Int_t histor) {
    return THaScalerBank::GetData(chan, histor);
};









