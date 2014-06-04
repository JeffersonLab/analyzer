#ifndef ToyModule_
#define ToyModule_

/////////////////////////////////////////////////////////////////////
//
//   ToyModule
//   Abstract interface for a module that sits in a slot of a crate.
//
/////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <vector>
#include <set>
#include "Rtypes.h"
#include "TNamed.h"

class THaSlotData;

class ToyModule : public TNamed  {

public:

  struct ToyModuleType {
   public:
     ToyModuleType ( const char *c1, Int_t i1 ) : fClassName(c1), fMapNum(i1), fTClass(0) {}
     bool operator<( const ToyModuleType& rhs ) const { return fMapNum < rhs.fMapNum; }
     const char*      fClassName;
     Int_t  fMapNum;
     mutable TClass*  fTClass;
  };

  ToyModule(Int_t crate, Int_t slot);
    
  typedef std::set<ToyModuleType> TypeSet_t;
  typedef TypeSet_t::iterator TypeIter_t;
  static TypeSet_t& fgToyModuleTypes();
  
  ToyModule() {};  // for ROOT TClass & I/O

  virtual ~ToyModule();  

  virtual void Init(Int_t crate, Int_t slot, Int_t header, Int_t mask) 
    { fCrate=crate; fSlot=slot; fHeader=header; fHeaderMask=mask; };

  virtual void Clear(const Option_t *opt) { fWordsSeen = 0; };

  virtual Bool_t IsSlot(Int_t rdata);

  virtual Int_t Decode(const Int_t *p)=0;  // --> abstract

  Int_t GetCrate() { return fCrate; };
  Int_t GetSlot() { return fSlot; };

  ToyModule& operator=(const ToyModule &rhs);

  virtual void DoPrint();

// Loads sldat and increments ptr to evbuffer
  Int_t LoadSlot(THaSlotData *sldat,  const Int_t* evbuffer );  

protected:

  static TypeIter_t DoRegister( const ToyModuleType& registration_info );

  Int_t fCrate, fSlot, fHeader, fHeaderMask;
  Int_t fChan, fData, fRawData;   // transient data
  Int_t fWordsExpect, fWordsSeen;
  Int_t fWdcntMask, fWdcntShift;
  ToyModule(const ToyModule& rhs); 
  void Create(const ToyModule& rhs);

private:

  static TypeIter_t fgThisType;

  ClassDef(ToyModule,0)  // A module in a crate and slot

};

#endif
