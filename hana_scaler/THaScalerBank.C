/////////////////////////////////////////////////////////////////////
//
//   THaScalerBank
//
//   A Bank of Scaler Data.  The bank is a collection of scaler
//   channels identified by 'name', crate, slot, helicity (defined
//   in data member 'location').  Typically a bank may be one slot
//   in VME, but not necessarily the entire slot.
//   This class is for internal use as a PRIVATE member 
//   of THaScaler class.  This means the user should look at 
//   THaScaler and not this class.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaScaler.h"
#include "THaScalerBank.h"

using namespace std;

THaScalerBank::THaScalerBank(string myname) {
   didinit = 0;
   name = myname; 
   size = SCAL_NUMCHAN;  // a default size
// present data in lower 1/2, previous event in upper 1/2 of array
   data = new Int_t[2*size];  
   header = 0;
   location.clear();
   chan_name.reserve(SCAL_NUMCHAN);
   for (int i = 0; i < SCAL_NUMCHAN; i++) chan_name.push_back("NULL");
};

THaScalerBank::~THaScalerBank() {
   delete [] data;
};

THaScalerBank& THaScalerBank::operator=(const THaScalerBank &that)
{
  if (this != &that) {
    name = that.name;
    didinit = that.didinit;
    size = that.size;
    header = that.header;
    ndata = that.ndata;
    data = new Int_t[2*size];
    memset(data, 0, 2*size*sizeof(Int_t));
    for (int i = 0; i < ndata; i++) data[i] = that.data[i];
    location = that.location;
  }
  return *this;
}

Int_t THaScalerBank::Init(Int_t crate, multimap<string, BscaLoc>& bmap) {  
// Initialize the scaler bank, verifying first that it belongs to crate.
  multimap<string, BscaLoc>::iterator lb = bmap.lower_bound(name);
  multimap<string, BscaLoc>::iterator ub = bmap.upper_bound(name);
  for (multimap<string, BscaLoc>::iterator p = lb; p != ub; p++) {
    location = p->second;
    if (location.crate == crate) {
      Clear();
      didinit = 1;
      return 1;
    }
  }
  location.clear();
  return SCAL_ERROR;
};

void THaScalerBank::Clear() {
   memset(data, 0, 2*size*sizeof(Int_t));
};

void THaScalerBank::LoadPrevious() {
// previous reading kept in "upper" part of array
  for (int i = 0; i < size; i++) data[i+size] = data[i];  
};

Int_t THaScalerBank::Load(int *rdata) {  
// Load the raw data 'rdata' into the data of this bank.
  ndata = 0;
  if ( !didinit ) return 0;  // undefined bank, not necessarily error.
  if ( !location.valid() ) return SCAL_ERROR;
  LoadPrevious();
  for (int i = 0; i < (long)location.startchan.size(); i++) {
    int start = location.startchan[i] + location.slot*SCAL_NUMCHAN; 
    if (location.numchan[i] > 0 && start >= 0 && 
       start+location.numchan[i] < SCAL_NUMCHAN*SCAL_NUMBANK) {
       for (int k = start; k < start+location.numchan[i]; k++) {
	  if (ndata < size) {
            data[ndata++] = rdata[k];
	  } else {
	    if (SCAL_VERBOSE) cout <<"Warning: THaScalerBank: Truncation of data. Need to init with bigger size ?"<<endl;
	  }
      }
    }
  }
  return 0;
};

Int_t THaScalerBank::GetData(Int_t index, Int_t histor) {
// histor = 0 get present event, = 1 get previous event.
  Int_t idx = index;
  if (histor == 1) idx = idx + size;
  if (idx >= 0 && idx < 2*size) return data[idx];
  return -1;  // something wrong
};

void THaScalerBank::SetChanName(Int_t index, char* newname) {
  if (index >= 0 && index < SCAL_NUMCHAN) chan_name[index] = newname;
  return;
};   

string THaScalerBank::GetChanName(Int_t index) {
  if (index >= 0 && index < SCAL_NUMCHAN) return chan_name[index];
  return " ";
};


void THaScalerBank::Print() {
  cout << "Bank name = "<<name<<"  ndata = "<<ndata<<endl;
   for (int k = 0; k < ndata; k++) {  	  
       cout << "Data ["<<dec<<k<<"] = 0x"<<hex<<data[k];
       cout << "   = (dec)  "<<dec<<data[k]<<endl;
   }
};

#ifndef ROOTPRE3
ClassImp(THaScalerBank)
#endif

