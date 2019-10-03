//*-- Author :    Ole Hansen   23-Sep-19

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::DynamicTriggerTime                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "DynamicTriggerTime.h"
#include "THaDetMap.h"
#include "THaEvData.h"
#include "TMath.h"
#include "VarDef.h"
#include "VarType.h"
#include "TError.h"
#include <cstdio>
#include <iostream>

using namespace std;

namespace Podd {

//____________________________________________________________________________
DynamicTriggerTime::DynamicTriggerTime( const char* name, const char* desc,
                                        THaApparatus* apparatus ) :
        THaTriggerTime( name, desc, apparatus )
{
  // Basic constructor
}

//____________________________________________________________________________
DynamicTriggerTime::DynamicTriggerTime( const char* name, const char* desc,
                                        const char* expr, const char* cond,
                                        THaApparatus *apparatus ) :
        THaTriggerTime( name, desc, apparatus )
{
  // Construct with formula 'expr' for calculating the time correction and
  // optional condition 'cond' that must be true for applying the correction.
  // This formula is evaluated /after/ any other definitions read from the
  // database (like a default value).

  if( !expr )
    return;     // behave like the basic constructor
  if( !cond )
    cond = "";

  fDefs.insert( TimeCorrDef(expr, cond, kMaxUInt) );
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::ReadDatabase( const TDatime &date )
{
  // Read this detector's parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char *const here = "ReadDatabase";

  FILE *file = OpenFile( date );
  if( !file )
    return kFileError;

  fGlOffset = 0.0;
  // fDefs.clear(); //FIXME: this would clear the definition from the constructor too

  // Read configuration parameters
  TString defstr;
  DBRequest config_request[] = {
          { "t_corr",   &defstr,    kTString, 0, 1 },
          { "glob_off", &fGlOffset, kDouble, 0, 1 },
          { 0 }
  };
  Int_t err = LoadDB( file, date, config_request, fPrefix );
  fclose( file );
  if( err )
    return err;

  if( !defstr.IsNull() ) {
    // Parse the definition string. The format is
    //
    // <condition>   <formula for calculating time correction>
    //
    // No spaces are allowed in the condition. Conditions are evaluated in
    // the order listed. Comments (starting with '#') are ignored.
    // Definitions may have trailing comments.
    // Both the condition and formula may contain any global variable and/or
    // cut name that has been calculated at the corresponding stage.
    //
    // Examples:
    //
    // L.trg.eval_at = Decode   # Evaluate expressions at Decode stage
    // L.trg.t_corr =
    //   DL.evtypebits&0x20!=0
    //      L.s2.rt_c[L.s2.t_pads[0]]-R.s2.rt_c[R.s2.t_pads[0]]  # s2 L-TDC time diff
    //   DL.evtypebits&0x10!=0
    //      L.s2.lt_c[L.s2.t_pads[0]]-R.s2.lt_c[R.s2.t_pads[0]]  # s2 R-TDC time diff
    //

    cout << defstr.Data() << endl;
  }

  // Read in the time offsets, in the format below, to be subtracted from
  // the times measured in other detectors.
  //
  // Trigger types NOT listed are assumed to have a zero offset.
  //
  // <TrgType>   <time offset in seconds>
  // eg:
  //   1               0       crate slot chan
  //   2              10.e-9   crate slot chan

//  Int_t ndef = trigdef.size() / 5;
//  for( Int_t i = 0; i < ndef; ++i ) {
//    Int_t base = 5 * i;
//    Int_t itrg = TMath::Nint( trigdef[base] );
//    Double_t toff = trigdef[base + 1];
//    Int_t crate = TMath::Nint( trigdef[base + 2] );
//    Int_t slot = TMath::Nint( trigdef[base + 3] );
//    Int_t chan = TMath::Nint( trigdef[base + 4] );
//    fTrgTypes.push_back( itrg );
//    fToffsets.push_back( toff );
//    fDetMap->AddModule( crate, slot, chan, chan, itrg );
//  }
//
//  // now construct the appropriate arrays
//  fNTrgType = fTrgTypes.size();
//  fTrgTimes = new Double_t[fNTrgType];

  return kOK;
}

//____________________________________________________________________________
void DynamicTriggerTime::Clear( Option_t *opt )
{
  // Reset all variables to their default/unknown state
  THaNonTrackingDetector::Clear( opt );
  fEvtType = -1;
  fEvtTime = fGlOffset;
  for( Int_t i = 0; i < fNTrgType; i++ ) fTrgTimes[i] = kBig * fCommonStop;
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::Decode( const THaEvData &evdata )
{
  // Look through the channels to determine the trigger type.
  // Be certain to decode only once per event.
  if( fEvtType > 0 ) return kOK;

  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module *d = fDetMap->GetModule( i );

    // Loop over our short-list of relevant channels.
    // Continuing with the assumption that each channel has its own entry in
    // fDetMap, so that fDetMap, fTrgTimes, fTrgTypes, and fToffsets are
    // parallel.

    // look through each channel and take the earliest hit
    if( fCommonStop ) fTDCRes *= -1;

    for( Int_t j = 0; j < evdata.GetNumHits( d->crate, d->slot, d->lo ); j++ ) {
      Double_t t = fTDCRes * evdata.GetData( d->crate, d->slot, d->lo, j );
      if( t < fTrgTimes[i] ) fTrgTimes[i] = t;
    }
  }

  int earliest = -1;
  Double_t reftime = 0;
  for( Int_t i = 0; i < fNTrgType; i++ ) {
    if( earliest < 0 || fTrgTimes[i] < reftime ) {
      earliest = i;
      reftime = fTrgTimes[i];
    }
  }

  Double_t offset = fGlOffset;
  if( earliest >= 0 ) {
    offset += fToffsets[earliest];
    fEvtType = fTrgTypes[earliest];
  }
  fEvtTime = offset;
  return kOK;
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::DefineVariables( EMode mode )
{
  // Define/delete standard event-by-event time offsets

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = (mode == kDefine);

  RVarDef vars[] = {
//          { "evtime",   "Time-offset for event (trg based)", "fEvtTime" },
//          { "evtype",   "Earliest trg-bit for the event",    "fEvtType" },
//          { "trgtimes", "Times for each trg-type",           "fTrgTimes" },
          { 0 }
  };

  return DefineVarsFromList( vars, mode );
}

} // namespace Podd

ClassImp( Podd::DynamicTriggerTime )
