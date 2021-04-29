///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTriggerTime                                                            //
//                                                                           //
//  Simple processing: report the global offset of the common-start/stop     //
//   for a given spectrometer, for use as a common offset to fix-up          //
//   places where the absolute time (ie VDC) is used.                        //
//                                                                           //
//  Lookup in the database what the offset is for each trigger type, and     //
//  report it for each appropriate event.                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTriggerTime.h"
#include "THaDetMap.h"
#include "THaEvData.h"
#include "TMath.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaAnalyzer.h"
#include <cstdio>
#include <vector>
#include <cassert>

using namespace std;
using Podd::TimeCorrectionModule;

//____________________________________________________________________________
THaTriggerTime::THaTriggerTime( const char* name, const char* desc )
  : TimeCorrectionModule(name, desc, THaAnalyzer::kDecode),
    fDetMap(new THaDetMap), fTDCRes(0.0), fCommonStop(0), fEvtType(-1)
{
  // basic do-nothing-else constructor
}

//____________________________________________________________________________
THaTriggerTime::~THaTriggerTime()
{
  // Normal destructor
  RemoveVariables();

  delete fDetMap;
}

//____________________________________________________________________________
void THaTriggerTime::Clear(Option_t* opt)
{
  // Reset event-by-event variables
  TimeCorrectionModule::Clear(opt);
  fEvtType = -1;
  fTrgTimes.assign( fTrgTypes.size(), kBig );
}

//____________________________________________________________________________
Int_t THaTriggerTime::Process( const THaEvData& evdata)
{
  // Look through the channels to determine the trigger type.
  // Be certain to decode only once per event.
  if (fEvtType>0) return kOK;

  // Each trigger is connected to a single TDC channel.
  // All TDCs are started or stopped with the same signal.
  // Whichever trigger occurs absolute first based on these TDCs is
  // considered the event's "trigger type".
  // Each TDC channel may have multiple hits, in which case the
  // hit occurring first in time will be used and compared to the
  // TDC values for the other triggers.
  // Each trigger type is assigned a fixed time offset fToffsets,
  // read from the database.
  // The "event time" is reported as fToffsets[earliest] + fGlOffset.
  // The actual TDC values of all the triggers are reported in fTrgTimes,
  // primarily for debugging purposes.
  UInt_t earliest = kMaxUInt;
  Double_t reftime = kBig;
  for( UInt_t imod = 0; imod < fDetMap->GetSize(); imod++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(imod);

    // Loop over our short-list of relevant channels.
    // Continuing with the assumption that each channel has its own entry in
    // fDetMap, so that fDetMap, fTrgTimes, fTrgTypes, and fToffsets are
    // parallel.

    // look through each channel and take the earliest hit
    // In common stop mode, the earliest hit corresponds to the largest TDC data.
    // This is taken care of by making fTDCRes and hence all fTrgTimes negative.
    for( UInt_t ihit = 0; ihit < evdata.GetNumHits(d->crate, d->slot, d->lo); ihit++ ) {
      Double_t t = fTDCRes * evdata.GetData(d->crate, d->slot, d->lo, ihit);
      if( t < fTrgTimes[imod] )
        fTrgTimes[imod] = t;
    }
    if( earliest == kMaxUInt || fTrgTimes[imod] < reftime ) {
      earliest = imod;
      reftime = fTrgTimes[imod];
    }
  }

  fEvtTime = fGlOffset;
  if( earliest != kMaxUInt ) {
    assert( earliest < fToffsets.size() );
    assert( earliest < fTrgTypes.size() );
    fEvtTime += fToffsets[earliest];
    fEvtType = fTrgTypes[earliest];
  }
  return kOK;
}

//____________________________________________________________________________
Int_t THaTriggerTime::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";

  Int_t ret = Podd::TimeCorrectionModule::ReadDatabase(date);
  if( ret != kOK )
    return ret;

  FILE* file = OpenFile( date );
  if( !file )
    return kFileError;

  fCommonStop = 0;
  fTrgTypes.clear();
  fToffsets.clear();
  fDetMap->Clear();

  // Read configuration parameters
  vector<Double_t> trigdef;
  DBRequest config_request[] = {
    { "tdc_res",  &fTDCRes },
    { "trigdef",  &trigdef,   kDoubleV },
    { "glob_off", &fGlOffset, kDouble, 0, true },
    { "common_stop", &fCommonStop, kInt, 0, true },
    { nullptr }
  };
  Int_t err = LoadDB( file, date, config_request, fPrefix );
  fclose(file);
  if( err )
    return err;

  // Sanity checks
  if( trigdef.size() % 5 != 0 ) {
    Error( Here(here), "Incorrect number of elements in \"trigdef\" "
                       "parameter = %u. Must be a multiple of 5. Fix database.",
           static_cast<unsigned int>(trigdef.size()) );
    return kInitError;
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

  UInt_t ndef = trigdef.size() / 5;
  for( UInt_t i = 0; i<ndef; ++i ) {
    UInt_t base   = 5*i;
    Int_t itrg    = TMath::Nint( trigdef[base] );
    Double_t toff = trigdef[base+1];
    Int_t crate   = TMath::Nint( trigdef[base+2] );
    Int_t slot    = TMath::Nint( trigdef[base+3] );
    Int_t chan    = TMath::Nint( trigdef[base+4] );
    fTrgTypes.push_back( itrg );
    fToffsets.push_back( toff );
    fDetMap->AddModule( crate, slot, chan, chan, itrg );
  }
  assert( ndef == fDetMap->GetSize() );
  assert( ndef == fTrgTypes.size() );
  assert( ndef == fToffsets.size() );

  // Use the TDC mode flag to set the sign of fTDCRes (see comments in Process)
  fTDCRes = (fCommonStop ? 1.0 : -1.0) * TMath::Abs(fTDCRes);

  // now construct the appropriate arrays
  fTrgTimes.resize( fTrgTypes.size() );

  return kOK;
}

//____________________________________________________________________________
Int_t THaTriggerTime::DefineVariables( EMode mode )
{
  // Define/delete event-by-event global variables

  RVarDef vars[] = {
    { "evtype",  "Earliest trg-bit for the event", "fEvtType" },
    { "trgtimes","Times for each trg-type",        "fTrgTimes" },
    { nullptr }
  };

  return DefineVarsFromList( vars, mode );
}

//____________________________________________________________________________
ClassImp(THaTriggerTime)
