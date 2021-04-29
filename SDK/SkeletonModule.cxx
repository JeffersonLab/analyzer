/////////////////////////////////////////////////////////////////////
//
//   SkeletonModule
//   This is an example of a module which a User may add.
//   See the header for more advice.
//
/////////////////////////////////////////////////////////////////////

#define LIKEV792 1

#include "SkeletonModule.h"
#include "THaSlotData.h"

using namespace Decoder;
using namespace std;

Module::TypeIter_t SkeletonModule::fgThisType =
  DoRegister( ModuleType( "Decoder::SkeletonModule" , 4444 ));

SkeletonModule::SkeletonModule(UInt_t crate, UInt_t slot)
  : VmeModule(crate, slot), fNumHits(0)
{
  fDebugFile = nullptr;
  SkeletonModule::Init();
}

SkeletonModule::~SkeletonModule() = default;

void SkeletonModule::Init()
{
  Module::Init();
  fNumChan = 32;
  fData.resize(fNumChan);
  fDebugFile = nullptr;
  Clear();
  IsInit = true;
  fName = "Skeleton Module (example)";
}

#ifdef LIKEV792
UInt_t SkeletonModule::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                 const UInt_t* pstop )
{
  // This is a simple, default method for loading a slot
  // pstop points to last word of data
  const UInt_t* p = evbuffer;
  fWordsSeen = 0;
//  cout << "version like V792"<<endl;
  ++p;
  UInt_t nword=*p-2;
  ++p;
  for( UInt_t i = 0; i < nword && p < pstop; i++ ) {
    ++p;
    UInt_t chan = ((*p) & 0x00ff0000) >> 16;
    UInt_t raw = ((*p) & 0x00000fff);
    Int_t status = sldat->loadData("adc", chan, raw, raw);
    fWordsSeen++;
    if( chan < fData.size() ) fData[chan] = raw;
//       cout << "word   "<<i<<"   "<<chan<<"   "<<raw<<endl;
    if( status != SD_OK )
      return -1;
  }
  return fWordsSeen;
}
#endif

UInt_t SkeletonModule::GetData( UInt_t chan ) const
{
  if( chan > fNumChan ) return 0;
  return fData[chan];
}

void SkeletonModule::Clear( const Option_t* opt )
{
  VmeModule::Clear(opt);
  fNumHits = 0;
  fData.assign(fNumChan,0);
}

ClassImp(SkeletonModule)
