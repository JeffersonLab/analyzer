#ifndef FastbusModule_
#define FastbusModule_

/////////////////////////////////////////////////////////////////////
//
//   FastbusModule
//   fastbus module class.  
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <cstdio>
#include <map>
#include <vector>
#include "Rtypes.h"
#include "Decoder.h"
#include "TNamed.h"
#include "Module.h"


#include "THaEvData.h"

using namespace std;

class Decoder::FastbusModule : public Module {

public:

   FastbusModule() {};
   FastbusModule(Int_t crate, Int_t slot);
   virtual ~FastbusModule(); 

   Int_t Slot(UInt_t rdata) { return (rdata>>fSlotShift); };
   Int_t Chan(UInt_t rdata) { return (rdata&fChanMask)>>fChanShift; };
   Int_t Data(UInt_t rdata) { return (rdata&fDataMask); };
         
   Int_t Decode(const Int_t *evbuffer);
   Bool_t IsSlot(UInt_t rdata) { return (Slot(rdata)==fSlot); };
   virtual Int_t LoadSlot(THaSlotData *sldat, const Int_t* evbuffer, const Int_t *pstop);
   void DoPrint();
  
protected:

   Bool_t fHasHeader;
   Int_t fHeader;
   Int_t fWdcntMask;
   Int_t fSlotMask, fSlotShift;
   Int_t fChanMask, fChanShift;
   Int_t fDataMask;
   Int_t fOptMask, fOptShift;
   virtual void Init();  


private:

   static TypeIter_t fgThisType;
   FastbusModule(const FastbusModule &fh);
   FastbusModule& operator=(const FastbusModule &fh);
   void Create(const FastbusModule& rhs);

   ClassDef(FastbusModule,0)  // Fastbus module

};

#endif
