///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// FadcScintillator (11202017 Hanjie Liu hanjie@jlab.org)                    //
// Generic Scintillator with FADC frontends.                                 //
// Inherits from THaScintillator with some special FADC variables added.     //
//                                                                           //
// Class for a scintillator (hodoscope) consisting of multiple               //
// paddles with phototubes on both ends.                                     //
//									     //          
///////////////////////////////////////////////////////////////////////////////

#include "FadcScintillator.h"
#include "FADCData.h"
#include "THaDetMap.h"
#include "THaEvData.h"
#include <cstdlib>

using namespace std;

namespace HallA {

//_____________________________________________________________________________
FadcScintillator::FadcScintillator( const char* name, const char* description,
                                    THaApparatus* apparatus )
  : THaScintillator(name, description, apparatus),
    fFADCDataL(nullptr), fFADCDataR(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
FadcScintillator::FadcScintillator()
  : THaScintillator(), fFADCDataL(nullptr), fFADCDataR(nullptr)

{
  // Default constructor (for ROOT RTTI)
}

//_____________________________________________________________________________
FadcScintillator::~FadcScintillator()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
Int_t FadcScintillator::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  Int_t err = THaScintillator::ReadDatabase(date);
  if( err )
    return err;

  for( int i = kRight; i <= kLeft; ++i ) {
    auto ret = MakeFADCData(date, this);
    if( ret.second )
      return ret.second; // Database error
    // Keep pointers to the elements around for convenient access
    FADCData*& fdat = (i == kRight) ? fFADCDataR : fFADCDataL;
    fdat = ret.first.get();
    assert(fdat->GetSize() - fNelem == 0);
    fDetectorData.emplace_back(std::move(ret.first));
  }

  return kOK;
}

//_____________________________________________________________________________
Int_t FadcScintillator::DefineVariables( EMode mode )
{
  // Define global analysis variables

  // Add FADC-specific variables for left- and right-side readouts
  class VarDefInfo {
  public:
    FADCData* fadcData;
    const char* key_prefix;
    const char* comment_subst;
    Int_t DefineVariables( EMode mode ) const  // Convenience function
    { return fadcData->DefineVariables(mode, key_prefix, comment_subst); }
  };
  const vector<VarDefInfo> sides = {
    { fFADCDataR, "r", "right side" },
    { fFADCDataL, "l", "left side"  }
  };
  for( const auto& side : sides )
    if( Int_t ret = side.DefineVariables(mode) )
      return ret;

  // Define standard scintillator variables. This must come last so that our
  // FADCData objects will be recognized as already initialized.
  return THaScintillator::DefineVariables(mode);
}

//_____________________________________________________________________________
OptUInt_t FadcScintillator::LoadData( const THaEvData& evdata,
                                      const DigitizerHitInfo_t& hitinfo )
{
  // Callback from Decoder for loading the data for the 'hitinfo' channel.
  // This routine supports FADC modules and returns the pulse amplitude integral.
  // Additional info is retrieved from the FADC modules in StoreHit later.

  if( hitinfo.type == Decoder::ChannelType::kMultiFunctionADC )
    return FADCData::LoadFADCData(hitinfo);

  // Fallback for legacy modules
  return THaScintillator::LoadData(evdata, hitinfo);
}

//_____________________________________________________________________________
Int_t
FadcScintillator::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData. Called from Decode().

  static_assert(kRight == 0 || kLeft == 0, "kRight or kLeft must be 0");
  assert(fNviews == abs(kLeft - kRight) + 1);

  auto side = static_cast<ESide>(GetView(hitinfo));

  // Call StoreHit for the FADC modules first to get updated pedestals
  FADCData* fadcData = (side == kRight) ? fFADCDataR : fFADCDataL;
  fadcData->StoreHit(hitinfo, data);

  // Retrieve pedestal, if available, and update the PMTData calibrations
  const auto& FDAT = fadcData->GetData(hitinfo);
  if( FDAT.fPedq == 0 ) {
    Podd::PMTData*& pmtData = (side == kRight) ? fRightPMTs : fLeftPMTs;
    pmtData->GetCalib(hitinfo).ped = FDAT.fPedestal;
  }

  // Now fill the PMTData in fDetectorData
  return THaScintillator::StoreHit(hitinfo, data);
}

} // namespace HallA

//_____________________________________________________________________________
ClassImp(HallA::FadcScintillator)
