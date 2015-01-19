#ifndef SkeletonModule_
#define SkeletonModule_

/////////////////////////////////////////////////////////////////////
//
//   SkeletonModule
//   This is an example of a module which a User may add.
//   Note, it must inherit from VmeModule.
//   Feel free to copy this and make appropriate changes.
//
//   other steps
//   1. Register in Module.C (add "DoRegister" call and include the header)
//   2. Add to namespace Decoder.h
//   3. Add to Makefile
//   4. Add to haDecode_LinkDef.h
//   5. Add line(s) for [crate,slot] in db_cratemap.dat
//  
//
/////////////////////////////////////////////////////////////////////

#define NTDCCHAN   32
#define MAXHIT    100

#include <string>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "Decoder.h"
#include "VmeModule.h"

class Decoder::SkeletonModule : public VmeModule {

public:

   SkeletonModule() {};  
   SkeletonModule(Int_t crate, Int_t slot);  
   virtual ~SkeletonModule();  

   void Init();
   Bool_t IsSlot(UInt_t rdata);

   Int_t GetNumHits() { return fNumHits; };
   
   Int_t GetData(Int_t chan, Int_t hit);

private:
 
// Loads sldat and increments ptr to evbuffer
   Int_t LoadSlot(THaSlotData *sldat,  const Int_t* evbuffer, const Int_t *pstop );  

   Int_t fNumHits;
   Int_t *fTdcData;  // Raw data (either samples or pulse integrals)
   Bool_t IsInit; 
   void Clear(const Option_t *opt);
   static TypeIter_t fgThisType;
   Int_t slotmask, chanmask, datamask;
   ClassDef(SkeletonModule,0)  //  Skeleton of a module; make your replacements

};

#endif
