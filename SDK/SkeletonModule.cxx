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
#include <cassert>

using namespace Decoder;
using namespace std;

// Define this module's class name and model number. Be sure to match
// the namespace and class name to your actual module's definition.
// The model number is an arbitrary unsigned int value, but must be unique
// among all modules defined in all loaded libraries.
Module::TypeIter_t SkeletonModule::fgThisType =
  DoRegister( ModuleType( "Decoder::SkeletonModule" , 4444 ));

namespace Decoder {

//_____________________________________________________________________________
SkeletonModule::SkeletonModule( UInt_t crate, UInt_t slot )
  : VmeModule(crate, slot), fMaxBufLen(256), fNumHits(0)
{
  fDebugFile = nullptr;
  SkeletonModule::Init();
}

//_____________________________________________________________________________
SkeletonModule::~SkeletonModule() = default;

//_____________________________________________________________________________
void SkeletonModule::Init()
{
  VmeModule::Init();
  fNumChan = 32;
  fData.resize(fNumChan);
  fDebugFile = nullptr;
  Clear();
  IsInit = true;
  fName = "Skeleton Module (example)";
}

//_____________________________________________________________________________
void SkeletonModule::Init( const char* configstr )
{
  // Example initialization function that sets parameters based on an
  // optional configuration string in db_cratemap.dat.
  // If configuration strings are not needed (the typical case), just do not
  // implement Init(const char*) at all. Do implement Init().
  //
  // Definitions for this module in the cratemap file might look like the
  // examples below, both of which will set the local variable mode=2 and member
  // variable fMaxBufLen=125. The crate, slot and bank numbers are arbitrary.
  // The model number must be one used in the call to DoRegister above.
  //
  // ==== Crate 30 type vme
  // # slot   model   bank   configuration string
  //   10     4444    4000   cfg: buflen=125, mode=2
  //   11     4444    4000   cfg:2 125

  Init(); // set defaults

  // A local variable to be set via the config string. It is initialized here
  // with its default value. Such variables' type MUST be UInt_t if they are
  // to be set with ParseConfigStr below.
  UInt_t mode = 0;

  // Define parameter names and destinations to be searched for in the
  // configuration string. The destination variables MUST have type unsigned int
  // (UInt_t). Parameter names are optional but recommended. If no name is
  // given in the config string, the value is assigned based on its position
  // in the string, using the corresponding position in the 'req' array as the
  // destination. If there are fewer values in the configuration string than
  // in the 'req' array, the missing variables are left unchanged.
  // If the configuration string contains more variables than 'req' or if
  // an unknown parameter name is found, a fatal initialization error occurs.
  // (This is to prevent silent misconfiguration because of typos and similar.)
  vector<ConfigStrReq> req = { {"mode",   mode},
                               {"buflen", fMaxBufLen} };

  // The following call sets the variables referenced in 'req', provided there
  // is data for them in the configuration string.
  // ParseConfigStr is merely a convenience function. If its requirements are
  // too restrictive, you are free to parse the string any other way you like.
  ParseConfigStr(configstr, req);

  // Use the local variable somehow, for example to set an Int_t member
  // variable. (NB: fMode is defined in the Decoder::Module base class.) This
  // is for illustration only. In practice you'll want to do something else.
  fMode = ( mode > kMaxUShort ) ?
          -1 : static_cast<UShort_t>(mode); // truncate to lower 16 bits
}

//_____________________________________________________________________________
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
  UInt_t nword = *p-2;
  ++p;
  for( UInt_t i = 0; i < nword && p < pstop; i++ ) {
    ++p;
    UInt_t chan = ((*p) & 0x00ff0000) >> 16;
    UInt_t raw = ((*p) & 0x00000fff);
    Int_t status = sldat->loadData("adc", chan, raw, raw);
    fWordsSeen++;
    if( chan < fData.size() )
      fData[chan] = raw;
//  else
//    cerr << "Invalid channel" << chan << endl;
//  cout << "word   "<<i<<"   "<<chan<<"   "<<raw<<endl;
    if( status != SD_OK )
      return -1;
  }
  fNumHits = sldat->getNumChan(); // only for illustration, modify as needed
  return fWordsSeen;
}
#endif

//_____________________________________________________________________________
UInt_t SkeletonModule::GetData( UInt_t chan ) const
{
  assert(chan < fNumChan);  // caller expected to ensure that 'chan' is in range
  return fData[chan];
}

//_____________________________________________________________________________
void SkeletonModule::Clear( const Option_t* opt )
{
  VmeModule::Clear(opt);
  fNumHits = 0;
  fData.assign(fNumChan,0);
}

} // namespace Decoder

//_____________________________________________________________________________
ClassImp(Decoder::SkeletonModule)
