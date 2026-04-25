/**
   \class TIClockModule
   \ingroup Decoders

   \brief Decoder module to read the TI bank with clock data (like a scaler).

   These data are identified by a bank with the tag 9001.
*/

#include "TIClockModule.h"
#include "Helper.h"
#include "Database.h"

using namespace std;
using namespace Podd;

namespace Decoder {

constexpr UInt_t kNumRawData = 12;  // Number of expected raw data words in bank
constexpr UInt_t kNumChan = 10;     // Number of logical scaler channels

Module::TypeIter_t TIClockModule::fgThisType = // NOLINT(*-throwing-static-initialization)
  DoRegister(ModuleType("Decoder::TIClockModule", 9001));

//_____________________________________________________________________________
TIClockModule::TIClockModule( UInt_t crate, UInt_t slot )
  : GenScaler(crate, slot)
{
  TIClockModule::Init();
}

//_____________________________________________________________________________
void TIClockModule::Init()
{
  GenScaler::Init();
  fNumChan = kNumChan;;
  fWordsExpect = fNumChan;
  // Resize & clear arrays holding scaler data to fWordsExpect
  GenInit();
  // Resize & clear raw data array to kNumRawData
  fData.assign(kNumRawData, 0);
  fModelNum = 9001;
}

//_____________________________________________________________________________
void TIClockModule::Clear(Option_t* opt)
{
  GenScaler::Clear(opt);
  fData.assign(kNumRawData, 0);
}

//_____________________________________________________________________________
UInt_t TIClockModule::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                const UInt_t* pstop )
{
  return LoadSlot(sldat, evbuffer, 0, pstop + 1 - evbuffer);
}

//_____________________________________________________________________________
UInt_t TIClockModule::LoadSlot( THaSlotData*, const UInt_t* evbuffer,
                                UInt_t pos, UInt_t len )
{
  //

  // Header word defined like so (in scaler_server/linuxScalerLib.c):
  //  int index = TISCALER_CODA_OFFSET;
  //  unsigned int header = (nTIScalers & 0xff) + (nTISlot << 8) + (index << 16);
  // where nTISlot = 0 (always?) and nTIScalers = 12 (hardcoded in tiLib, see next).
  //
  // Data words following the header are as follows:
  // (see linuxvme/ti/tiLib.c : tiReadScalers)
  //
  // 0: livetime
  // 1: busytime
  // 2-7: ts_scaler[0..5]
  // 8: input counter (all trigger sources)
  // 9: top 16 bits of event number
  // 10: bottom 32 bits of event number
  // 11: "blank5[0]" (only TS inputs) (trigger bits?)

  const char* const here = "LoadSlot";

  Clear();
  const auto* p = evbuffer + pos;
  UInt_t nwords = bitval(*p, 0, 7);
  if( nwords != kNumRawData ) {
    // Print warning to alert user to potential problems
    Error(Here(here), "Inconsistent number of words, got %u, expected %u "
                      "for crate %u", nwords, kNumRawData, fCrate);
    //firstwarn = false;
    if( nwords > kNumRawData )
      // Data are almost certainly garbage
      return len;
  }
  ++p;
  fData.assign(p, p + nwords);
  assert(fDataArray.size() == fNumChan);
  bool do_ratecalc = false;
  if( fFirstTime ) {
    fFirstTime = false;
  } else {
    do_ratecalc = true;
    fPrevData.swap(fDataArray);
  }
  fDataArray[0] = GetTotalTime();
  memcpy(&fDataArray[1], &fData[0], (fNumChan - 1) * sizeof(UInt_t));
  if( do_ratecalc )
    LoadRates();
  fIsDecoded = true;
  return nwords + 1;
}

//_____________________________________________________________________________
}

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Decoder::TIClockModule)
#endif
