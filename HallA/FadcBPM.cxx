///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcBPM                                                            //
//									     //
// BPM with FADC frontends                                                   //
//									     //
///////////////////////////////////////////////////////////////////////////////

#include "FadcBPM.h"
#include "FADCData.h"

using namespace std;

namespace HallA {

//_____________________________________________________________________________
FadcBPM::FadcBPM( const char* name, const char* description,
                  THaApparatus* apparatus ) :
  THaBPM(name, description, apparatus)
{
  // Constructor
}

//_____________________________________________________________________________
OptInt_t FadcBPM::LoadData( const THaEvData& /*evdata*/,
                            const DigitizerHitInfo_t& hitinfo )
{
  // Retrieve pulse integral data from FADC for the channel in 'hitinfo'

  return FADCData::LoadFADCData(hitinfo);
}

//_____________________________________________________________________________

} // namespace HallA

//_____________________________________________________________________________
ClassImp(HallA::FadcBPM)
