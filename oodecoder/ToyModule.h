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

class THaEvData;

class ToyModule : public TNamed  {

public:

  struct ToyModuleType {
   public:
     ToyModuleType ( const char *c1 ) : fClassName(c1), fTClass(0) {}
    // The class names have to be unique, so use them for sorting
     bool operator<( const ToyModuleType& rhs ) const { return fClassName < rhs.fClassName; }
     const char*      fClassName;
     mutable TClass*  fTClass;
  };

  ToyModule(Int_t crate, Int_t slot);
    
  typedef std::set<ToyModuleType> TypeSet_t;
  typedef TypeSet_t::iterator TypeIter_t;
  static TypeSet_t& fgToyModuleTypes();
  
  ToyModule() {};  // for ROOT TClass & I/O

  virtual ~ToyModule();  

  virtual void Init(TString name, Int_t crate, Int_t slot) { fName = name; fCrate=crate; fSlot=slot; };
  virtual Bool_t IsSlot(Int_t rdata)=0;
  virtual Int_t GetNumWords() { return fNumWords; };
  virtual Int_t Decode(THaEvData *evdata, Int_t start=0)=0;
  Int_t GetCrate() { return fCrate; };
  Int_t GetSlot() { return fSlot; };
  ToyModule& operator=(const ToyModule &rhs);
  virtual void DoPrint();

protected:

  static TypeIter_t DoRegister( const ToyModuleType& registration_info );

  Int_t fCrate, fSlot;
  Int_t fNumWords;
  ToyModule(const ToyModule& rhs); 
  void Create(const ToyModule& rhs);

private:

  static TypeIter_t fgThisType;

  ClassDef(ToyModule,0)  // A module in a crate and slot

};

#endif
