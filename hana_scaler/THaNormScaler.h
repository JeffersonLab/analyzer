#ifndef THaNormScaler_
#define THaNormScaler_

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

#define SCAL_HIST      2
#define SCAL_NUMTRIG  12
#define SCAL_NUMPULS  10

#include "Rtypes.h"
#include "TString.h"
#include "THaScaler.h"
#include "THaScalerBank.h"
#include <stdlib.h>
#include <string>
#include <map>
#include <iostream>

class BscaLoc;

class THaNormScaler : public THaScalerBank
{
public:

   THaNormScaler();
   THaNormScaler(string name);
   virtual ~THaNormScaler();

   Int_t Init(Int_t crate, multimap<string, BscaLoc>& bmap);
   Int_t GetData(string name, Int_t histor);
   Int_t GetData(Int_t chan, Int_t histor);
   Int_t GetTrig(Int_t trig, Int_t histor);
   string GetChanName(int index);

private:

   Int_t crate,helicity;
   char* ctrig;
   map<string, Int_t> name_to_array;
   map<Int_t, string> array_to_name;
   vector<string> chan_name;

#ifndef ROOTPRE3
ClassDef(THaNormScaler,0)  // Normalization scaler, used by THaScaler
#endif

};

#endif





