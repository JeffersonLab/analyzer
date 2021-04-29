///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcCherenkov                                                      //
// Generic Cherenkov with FADC frontends                                     //
//									     //
///////////////////////////////////////////////////////////////////////////////

#include "FadcCherenkov.h"
#include "FADCData.h"
#include "THaDetMap.h"
#include "THaEvData.h"

using namespace std;

namespace HallA {

//_____________________________________________________________________________
FadcCherenkov::FadcCherenkov( const char* name, const char* description,
                              THaApparatus* a )
  : THaCherenkov(name, description, a),fFADCData(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
FadcCherenkov::FadcCherenkov() : THaCherenkov(), fFADCData(nullptr)
{
  // Default constructor (for ROOT RTTI)
}

//_____________________________________________________________________________
FadcCherenkov::~FadcCherenkov()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
Int_t FadcCherenkov::DefineVariables( EMode mode )
{
  Int_t ret = THaCherenkov::DefineVariables(mode);
  if( ret )
    return ret;

  return fFADCData->DefineVariables(mode);
}

//_____________________________________________________________________________
Int_t FadcCherenkov::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  Int_t err = THaCherenkov::ReadDatabase(date);
  if( err )
    return err;

  // Set up a new FADC object, initialized from the database
  auto ret = MakeFADCData(date, this);
  if( ret.second )
    return ret.second;

  // Keep a pointer for convenient access
  fFADCData = ret.first.get();

  // Move data object into the detector data array
  fDetectorData.emplace_back(std::move(ret.first));

  return kOK;
}

//_____________________________________________________________________________
OptUInt_t FadcCherenkov::LoadData( const THaEvData& evdata,
                                   const DigitizerHitInfo_t& hitinfo )
{
  // Callback from Decoder for loading the data for the 'hitinfo' channel.
  // This routine supports FADC modules and returns the pulse amplitude integral.
  // Additional info is retrieved from the FADC modules in StoreHit later.

  if( hitinfo.type == Decoder::ChannelType::kMultiFunctionADC )
    return FADCData::LoadFADCData(hitinfo);

  // Fallback for legacy modules not recognized above.
  return THaCherenkov::LoadData(evdata, hitinfo);
}

//_____________________________________________________________________________
Int_t FadcCherenkov::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData. Called from Decode().

  // Call StoreHit for the FADC modules first to get updated pedestals
  fFADCData->StoreHit(hitinfo, data);

  // Retrieve pedestal, if available, and update the PMTData calibrations
  const auto& FDAT = fFADCData->GetData(hitinfo);
  if( FDAT.fPedq == 0 )
    fPMTData->GetCalib(hitinfo).ped = FDAT.fPedestal;

  // Now fill the PMTData in fDetectorData
  return THaCherenkov::StoreHit(hitinfo, data);
}

} // namespace HallA

//_____________________________________________________________________________
ClassImp(HallA::FadcCherenkov)
