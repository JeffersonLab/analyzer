#ifndef Module_
#define Module_

/////////////////////////////////////////////////////////////////////
//
//   Module
//   Abstract interface for a module that sits in a slot of a crate.
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include "Rtypes.h"
#include "TNamed.h"
#include "Decoder.h"
#include "DecoderGlobals.h"

class Decoder::Module : public TNamed  {

public:

  struct ModuleType {
   public:
     ModuleType ( const char *c1, Int_t i1 ) : fClassName(c1), fMapNum(i1), fTClass(0) {}
     bool operator<( const ModuleType& rhs ) const { return fMapNum < rhs.fMapNum; }
     const char*      fClassName;
     Int_t            fMapNum;
     mutable TClass*  fTClass;
  };

  Module(Int_t crate, Int_t slot);
    
  typedef std::set<ModuleType> TypeSet_t;
  typedef TypeSet_t::iterator TypeIter_t;
  static TypeSet_t& fgModuleTypes();
  
  Module() { };  // for ROOT TClass & I/O

  virtual ~Module();  

  // problems here ... if =0 (abstract) some module types don't get created.
  // better way ?
  virtual Int_t GetNumEvents() { return 0; };  
  virtual Int_t GetMode() { return 0; };

  virtual void SetSlot(Int_t crate, Int_t slot, Int_t header=0, Int_t mask=0) 
  { fCrate=crate; fSlot=slot; fHeader=header; fHeaderMask=mask; };

  virtual void Init() { fModelNum=-1; };

  virtual void Clear(const Option_t *opt) { fWordsSeen = 0; };

  virtual Bool_t IsSlot(UInt_t rdata);

  // inheriting classes need to implement one or more of these
  virtual Int_t GetData(Int_t) { return 0; };
  virtual Int_t GetData(Int_t, Int_t) { return 0; };
  virtual Int_t GetData(Int_t, Int_t, Int_t) { return 0; };

  virtual Int_t Decode(const Int_t *p)=0;  // --> abstract

  virtual Int_t GetCrate() { return fCrate; };
  virtual Int_t GetSlot() { return fSlot; };

  virtual void SetDebugFile(ofstream *file) { fDebugFile = file; };

  virtual void SetHeader(UInt_t header, UInt_t mask) {
    fHeader = header;
    fHeaderMask = mask;
  }

  Module& operator=(const Module &rhs);

  virtual void DoPrint();

// Loads sldat and increments ptr to evbuffer
  virtual Int_t LoadSlot(THaSlotData *sldat,  const Int_t *evbuffer, const Int_t *pstop );  

protected:

  static TypeIter_t DoRegister( const ModuleType& registration_info );

  Int_t fCrate, fSlot;
  UInt_t fHeader, fHeaderMask;
  Int_t fChan, fData, fRawData;   // transient data
  Int_t fWordsExpect, fWordsSeen;
  Int_t fWdcntMask, fWdcntShift;
  Int_t fModelNum;
 
  ofstream *fDebugFile;

  Module(const Module& rhs); 
  void Create(const Module& rhs);
  

private:

  static TypeIter_t fgThisType;

  ClassDef(Module,0)  // A module in a crate and slot

};

#endif
