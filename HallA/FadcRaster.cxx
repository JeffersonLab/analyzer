///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcRaster                                                         //
//									     //
///////////////////////////////////////////////////////////////////////////////

#include "FadcRaster.h"
#include "FADCData.h"

using namespace std;

namespace HallA {

//_____________________________________________________________________________
FadcRaster::FadcRaster( const char* name, const char* description,
                        THaApparatus* a )
  : THaRaster(name, description, a)
{
  // Constructor
}

//_____________________________________________________________________________
OptUInt_t FadcRaster::LoadData( const THaEvData& /*evdata*/,
                                const DigitizerHitInfo_t& hitinfo )
{
  // Retrieve pulse integral data from FADC for the channel in 'hitinfo'

  if( !CheckHitInfo(hitinfo) )
    return nullopt;
  return FADCData::LoadFADCData(hitinfo);
}

//_____________________________________________________________________________

} // namespace HallA

///////////////////////////////////////////////////////////////////////////////
ClassImp(HallA::FadcRaster)
