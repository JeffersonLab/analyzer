///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcShower                                                         //
// Generic shower detector with FADC frontends                               //
//									     //
///////////////////////////////////////////////////////////////////////////////

#include "FadcShower.h"
#include "FADCData.h"
#include "THaDetMap.h"
#include "THaEvData.h"

using namespace std;

namespace HallA {

//_____________________________________________________________________________
FadcShower::FadcShower( const char* name, const char* description,
                        THaApparatus* a )
  : THaShower(name, description, a), fFADCData(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
FadcShower::FadcShower() : THaShower(), fFADCData(nullptr)
{
  // Default constructor (for ROOT RTTI)
}

//_____________________________________________________________________________
FadcShower::~FadcShower()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
Int_t FadcShower::DefineVariables( EMode mode )
{
  Int_t ret = THaShower::DefineVariables(mode);
  if( ret )
    return ret;

  return fFADCData->DefineVariables(mode);
}

//_____________________________________________________________________________
Int_t FadcShower::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  Int_t err = THaShower::ReadDatabase(date);
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
OptUInt_t FadcShower::LoadData( const THaEvData& evdata,
                                const DigitizerHitInfo_t& hitinfo )
{
  // Callback from Decoder for loading the data for the 'hitinfo' channel.
  // This routine supports FADC modules and returns the pulse amplitude integral.
  // Additional info is retrieved from the FADC modules in StoreHit later.

  if( hitinfo.type == Decoder::ChannelType::kMultiFunctionADC )
    return FADCData::LoadFADCData(hitinfo);

  // Fallback for legacy modules not recognized above.
  return THaShower::LoadData(evdata, hitinfo);
}

//_____________________________________________________________________________
Int_t FadcShower::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData. Called from Decode().

  // Call StoreHit for the FADC modules first to get updated pedestals
  fFADCData->StoreHit(hitinfo, data);
  const auto& FDAT = fFADCData->GetData(hitinfo);
  if( FDAT.fPedq == 0 )
    fADCData->GetCalib(hitinfo).ped = FDAT.fPedestal;

  // Now fill the PMTData in fDetectorData
  return THaShower::StoreHit(hitinfo, data);
}

} // namespace HallA

//_____________________________________________________________________________
ClassImp(HallA::FadcShower)
