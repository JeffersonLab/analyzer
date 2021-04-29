//////////////////////////////////////////////////////////////////////////
//
// HallA::FADCData
//
//////////////////////////////////////////////////////////////////////////

#include "FADCData.h"
#include "THaDetectorBase.h"
#include "Fadc250Module.h"
#include "Decoder.h"
#include <stdexcept>

using namespace std;
using namespace Decoder;

namespace HallA {

//_____________________________________________________________________________
// Convenience macro for readability
#if __cplusplus >= 201402L
# define MKFADCDATA(name,title,nelem) make_unique<FADCData>((name),(title),(nelem))
#else
# define MKFADCDATA(name,title,nelem) unique_ptr<FADCData>(new FADCData((name),(title),(nelem)))
#endif

//_____________________________________________________________________________
pair<unique_ptr<FADCData>,Int_t> MakeFADCData( const TDatime& date,
                                               THaDetectorBase* det )
{
  // Set up FADC data object and read configuration from the database.
  // Return value:
  //  ret.first:   unique_ptr to FADCData object
  //  ret.second:  return value from ReadConfig (a ReadDatabase EStatus)

  pair<unique_ptr<FADCData>, Int_t> ret(nullptr, THaAnalysisObject::kFileError);
  if( !det )
    return ret;

  // Create new FADC data object
  auto detdata = MKFADCDATA(det->GetPrefixName(), det->GetTitle(),
                            det->GetNelem());

  // Initialize FADC configuration parameters, using the detector's database
  FILE* file = det->OpenFile(date);
  if( !file )
    return ret;
  Int_t err = detdata->ReadConfig(file, date, det->GetPrefix());
  fclose(file);
  if( err ) {
    ret.second = err;
    return ret;
  }

  ret.first = std::move(detdata);
  ret.second = THaAnalysisObject::kOK;
  return ret;
}

//_____________________________________________________________________________
FADCData::FADCData( const char* name, const char* desc, Int_t nelem )
  : DetectorData(name, desc), fFADCData(nelem)
{
  // Constructor
}

//_____________________________________________________________________________
void FADCData::Clear( Option_t* opt )
{
  // Clear event-by-event data
  DetectorData::Clear(opt);

  for( auto& fdat : fFADCData ) {
    fdat.clear();
  }
//  fNHits.clear();
}

//_____________________________________________________________________________
void FADCData::Reset( Option_t* opt )
{
  // Clear event-by-event data and reset calibration values to defaults.
  Clear(opt);

  fConfig.reset();
}

//_____________________________________________________________________________
Int_t FADCData::ReadConfig( FILE* file, const TDatime& date, const char* prefix )
{
  // Load FADC configuration parameters (required) from database

  VarType kDataType = std::is_same<Data_t, Float_t>::value ? kFloat : kDouble;

  fConfig.reset();  // Sets default TDC scale
  DBRequest calib_request[] = {
    {"NPED",     &fConfig.nped,     kInt},
    {"NSA",      &fConfig.nsa,      kInt},
    {"NSB",      &fConfig.nsb,      kInt},
    {"Win",      &fConfig.win,      kInt},
    {"TFlag",    &fConfig.tflag,    kInt},
    {"TDCscale", &fConfig.tdcscale, kDataType, 0, true},
    {nullptr}
  };
  return THaAnalysisObject::LoadDB(file, date, calib_request, prefix);
}

//_____________________________________________________________________________
static inline
OptUInt_t GetFADCValue( EModuleType type, const DigitizerHitInfo_t& hitinfo,
                        Fadc250Module* fadc ) {
  // Get item 'm.type' from FADC module pointed to by fFADC

  assert(fadc);
  OptUInt_t val;
  if( fadc->HasCapability(type) &&
      hitinfo.hit < fadc->GetNumEvents(type, hitinfo.chan) ) {
    val = fadc->GetData(type, hitinfo.chan, hitinfo.hit);
    if( val == kMaxUInt ) // error return code
      val = nullopt;
  }
  return val;
}

//_____________________________________________________________________________
OptUInt_t FADCData::LoadFADCData( const DigitizerHitInfo_t& hitinfo )
{
  // Retrieve "pulse integral" value from FADC channel given in 'hitinfo'

  auto* fadc = dynamic_cast<Fadc250Module*>(hitinfo.module);
  if( !fadc )
    throw logic_error("Bad module type (expected Fadc250Module). "
                      "Should never happen. Call expert.");

  return GetFADCValue( kPulseIntegral, hitinfo, fadc );
}

//_____________________________________________________________________________
Int_t FADCData::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Retrieve full set of FADC data and store it in our data members.
  // Must call LoadData first. Pass LoadData's return value (= pulse integral
  // value) into this routine as 'data'.

  if( hitinfo.type != ChannelType::kMultiFunctionADC )
    return 0;

  size_t k = GetLogicalChannel(hitinfo);
  if( k >= fFADCData.size() )
    throw
      std::invalid_argument(msg(hitinfo, "Logical channel number out of range"));

  auto* fadc = dynamic_cast<Fadc250Module*>(hitinfo.module);
  assert(fadc);  // should have been caught in LoadData

  auto& FDAT = fFADCData[k];
  FDAT.fIntegral  = data;
  FDAT.fOverflow  = fadc->GetOverflowBit(hitinfo.chan, hitinfo.hit);
  FDAT.fUnderflow = fadc->GetUnderflowBit(hitinfo.chan, hitinfo.hit);
  FDAT.fPedq      = fadc->GetPedestalQuality(hitinfo.chan, hitinfo.hit);

  class TypeItem { public: EModuleType type; const string name; };
  static const vector<TypeItem> items = {
    { kPulsePeak,     "kPulsePeak" },
    { kPulseTime,     "kPulseTime" },
    { kPulsePedestal, "kPulsePedestal" }
  };

  for( const auto& item : items ) {
    OptUInt_t val = GetFADCValue(item.type, hitinfo, fadc);
    if( !val ) {
      string s("Error retrieving FADC item type ");
      s += item.name; s += ". Decoder bug. Call expert.";
      throw logic_error(msg(hitinfo,s.c_str())); // FADC's GetNumHits lied to us
    }
    switch( item.type ) {
      case kPulsePeak:
        FDAT.fPeak = val.value();
        break;
      case kPulseTime:
        FDAT.fT   = val.value();
        FDAT.fT_c = FDAT.fT * fConfig.tdcscale;
        break;
      case kPulsePedestal:
        // Retrieve pedestal, if available
        if( FDAT.fPedq == 0 ) {
          Data_t p = val.value();
          if( fConfig.tflag ) {
            p *= static_cast<Data_t>(fConfig.nsa + fConfig.nsb) / fConfig.nped;
          } else {
            p *= static_cast<Data_t>(fConfig.win) / fConfig.nped;
          }
          FDAT.fPedestal = p;
        }
        break;
      default:
        assert(false); // Not reached
    }
  }
  fHitDone = true;
  return 0;
}

//_____________________________________________________________________________
Int_t FADCData::DefineVariablesImpl( THaAnalysisObject::EMode mode,
                                     const char* key_prefix,
                                     const char* comment_subst )
{
  // Export FADCData results as global variables

  const char* const here = "FADCData::DefineVariables";

  // Define variables of the base class, if any.
  Int_t ret = DetectorData::DefineVariablesImpl(mode, key_prefix, comment_subst);
  if( ret )
    return ret;

  const RVarDef vars[] = {
    { "peak",      "FADC ADC peak values %s",               "fFADCData.fPeak" },
    { "t_fadc",    "FADC TDC values %s",                    "fFADCData.fT" },
    { "tc_fadc",   "FADC Corrected times %s",               "fFADCData.fT_c" },
    { "overflow",  "Overflow bit of FADC pulse %s",         "fFADCData.fOverflow" },
    { "underflow", "Underflow bit of FADC pulse %s",        "fFADCData.fUnderflow" },
    { "badped",    "Pedestal quality bit of FADC pulse %s", "fFADCData.fPedq" },
    { nullptr }
  };
  return StdDefineVariables(vars, mode, key_prefix, here, comment_subst);
}

//_____________________________________________________________________________

} // namespace HallA

ClassImp(HallA::FADCData)
