#ifndef Podd_SkeletonModule_h_
#define Podd_SkeletonModule_h_

/////////////////////////////////////////////////////////////////////
//
//   SkeletonModule
//   This is an example of a module which a User may add.
//   Note, it must inherit from VmeModule.
//   Feel free to copy this and make appropriate changes.
//
//   other steps
//   1. Register (add "DoRegister" call and include the header)
//   note: the number (4444) that is registered must appear in db_cratemap.dat
//   2. Add to namespace Decoder.h
//   3. Add to Makefile
//   4. Add to haDecode_LinkDef.h
//   5. Add line(s) for [crate,slot] in db_cratemap.dat
//
//   if the pre-compiler flag "LIKEV792" is defined, the decoding is
//   sort of like a V792 ... as an example.
//
/////////////////////////////////////////////////////////////////////

#define LIKEV792 1

#include "VmeModule.h"

class SkeletonModule : public Decoder::VmeModule {

public:

  SkeletonModule() : fNumHits(0) {};
  SkeletonModule(Int_t crate, Int_t slot);
  virtual ~SkeletonModule();

  using Decoder::Module::GetData;
  using Decoder::Module::LoadSlot;

  virtual UInt_t GetData(Int_t chan) const;
  virtual void   Init();
  virtual void   Clear(const Option_t* opt="");
  virtual Int_t  Decode(const UInt_t* /* p */ ) { return 0; };

#ifdef LIKEV792
  // Loads slot data.  if you don't define this, the base class's method is used
  virtual Int_t LoadSlot(Decoder::THaSlotData* sldat,
			 const UInt_t* evbuffer, const UInt_t* pstop );
#endif

private:

  Int_t fNumHits;

  static TypeIter_t fgThisType;

  ClassDef(SkeletonModule,0)  //  Skeleton of a module; make your replacements
};

#endif
