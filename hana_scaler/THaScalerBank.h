#ifndef THaScalerBank_
#define THaScalerBank_


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

#include "Rtypes.h"
#include "THaScalerDB.h"
#include <stdlib.h>
#include <string>
#include <map>
#include <iostream>
#define SCAL_NUMCHAN     32

class THaScalerBank 
{
public:

   THaScalerBank(std::string myname);
   virtual ~THaScalerBank();

   std::string name;
   Int_t didinit;
   Int_t size,header;
   Int_t ndata;
   Int_t *data;
   BscaLoc location;
   virtual Int_t Init(Int_t crate, std::multimap<std::string, BscaLoc>& bmap);
   virtual Int_t Load(int *data);
   virtual void Clear();
   Int_t GetData(Int_t index, Int_t histor);
   virtual void Print();
   virtual std::string GetChanName(Int_t index);
   virtual void SetChanName(Int_t index, char* name);
   THaScalerBank& operator=(const THaScalerBank &bk);

private:

   THaScalerBank(const THaScalerBank &bk);   // users cannot copy
   virtual void LoadPrevious();
   std::vector<std::string> chan_name;


#ifndef ROOTPRE3
ClassDef(THaScalerBank,0)  // A bank, or group, of scaler data.
#endif


};

#endif





