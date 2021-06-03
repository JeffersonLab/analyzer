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
//   2. (Optional) Put code in a suitable namespace, e.g. "Decoder" or "User"
//   3. Add to CMakeLists.txt
//   4. Add to <package>_LinkDef.h, where <package> is the library that will
//      include this module
//   5. Add line(s) for [crate,slot] in db_cratemap.dat
//
//   if the pre-compiler flag "LIKEV792" is defined, the decoding is
//   sort of like a V792 ... as an example.
//
/////////////////////////////////////////////////////////////////////

#define LIKEV792 1

#include "VmeModule.h"

namespace Decoder {

class SkeletonModule : public Decoder::VmeModule {

public:

  SkeletonModule() : fMaxBufLen(256), fNumHits(0) {};
  SkeletonModule( UInt_t crate, UInt_t slot );
  virtual ~SkeletonModule();

  using Decoder::Module::GetData;
  using Decoder::Module::LoadSlot;

  virtual UInt_t GetData( UInt_t chan ) const;
  virtual void   Init();
  virtual void   Clear( const Option_t* opt = "" );
  virtual Int_t  Decode( const UInt_t* /* p */ ) { return 0; };

  // optional Init method only needed if using cratemap configuration string
  virtual void   Init( const char* configstr );

#ifdef LIKEV792
  // Loads slot data.  if you don't define this, the base class's method is used
  virtual UInt_t LoadSlot( Decoder::THaSlotData* sldat,
                           const UInt_t* evbuffer, const UInt_t* pstop );
#endif

private:

  UInt_t fMaxBufLen;  // some arbitrary configurable parameter
  UInt_t fNumHits;    // Number of hits in current event

  static TypeIter_t fgThisType;

  ClassDef(SkeletonModule,0)  // Skeleton decoder module; make your replacements
};

} // namespace Decoder

#endif
