///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTriggerTime                                                            //
//                                                                           //
//  Simple processing: report the global offset of the common-start/stop     //
//   for a given spectrometer, for use as a common offset to fix-up          //
//   places where the absolute time (ie VDC) is used.                        //
//                                                                           //
//  Lookup in the database what the offset is for each trigger-type, and     //
//  report it for each appropriate event.                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTriggerTime.h"
#include "THaDetMap.h"
#include "THaEvData.h"
#include "TMath.h"
#include "VarDef.h"
#include "VarType.h"

#include <cstring>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace std;

//____________________________________________________________________________
THaTriggerTime::THaTriggerTime( const char* name, const char* desc,
        THaApparatus* apparatus ) :
  THaNonTrackingDetector(name, desc, apparatus),
  fEvtTime(kBig), fEvtType(-1), fTDCRes(0),
  fGlOffset(0), fCommonStop(0),fNTrgType(0), fTrgTimes(NULL)
{
  // basic do-nothing-else contructor
}

//____________________________________________________________________________
Int_t THaTriggerTime::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file )
    return kFileError;

  fGlOffset = 0.0;
  fCommonStop = 0.0;
  fTrgTypes.clear();
  fToffsets.clear();
  fDetMap->Clear();
  delete [] fTrgTimes; fTrgTimes = 0;
  fNTrgType = 0;

  // Read configuration parameters
  vector<Double_t> trigdef;
  DBRequest config_request[] = {
    { "tdc_res",  &fTDCRes },
    { "trigdef",  &trigdef,   kDoubleV },
    { "glob_off", &fGlOffset, kDouble, 0, 1 },
    { "common_stop", &fCommonStop, kInt, 0, 1 },
    { 0 }
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

  Int_t ndef = trigdef.size() / 5;
  for( Int_t i = 0; i<ndef; ++i ) {
    Int_t base = 5*i;
    Int_t itrg    = TMath::Nint( trigdef[base] );
    Double_t toff = trigdef[base+1];
    Int_t crate   = TMath::Nint( trigdef[base+2] );
    Int_t slot    = TMath::Nint( trigdef[base+3] );
    Int_t chan    = TMath::Nint( trigdef[base+4] );
    fTrgTypes.push_back( itrg );
    fToffsets.push_back( toff );
    fDetMap->AddModule( crate, slot, chan, chan, itrg );
  }

  // now construct the appropriate arrays
  fNTrgType = fTrgTypes.size();
  fTrgTimes = new Double_t[fNTrgType];

  return kOK;
}

//____________________________________________________________________________
void THaTriggerTime::Clear(Option_t* opt)
{
  // Reset all variables to their default/unknown state
  THaNonTrackingDetector::Clear(opt);
  fEvtType = -1;
  fEvtTime = fGlOffset;
  for (Int_t i=0; i<fNTrgType; i++) fTrgTimes[i]=kBig * fCommonStop;
}

//____________________________________________________________________________
Int_t THaTriggerTime::Decode(const THaEvData& evdata)
{
  // Look through the channels to determine the trigger type.
  // Be certain to decode only once per event.
  if (fEvtType>0) return kOK;

  for( Int_t i=0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);

    // Loop over our short-list of relevant channels.
    // Continuing with the assumption that each channel has its own entry in
    // fDetMap, so that fDetMap, fTrgTimes, fTrgTypes, and fToffsets are
    // parallel.

    // look through each channel and take the earliest hit
    if(fCommonStop) fTDCRes *=-1;

    for( Int_t j = 0; j < evdata.GetNumHits(d->crate, d->slot, d->lo); j++ ) {
      Double_t t = fTDCRes*evdata.GetData(d->crate,d->slot,d->lo,j);
      if (t < fTrgTimes[i]) fTrgTimes[i] = t;
    }
  }

  int earliest = -1;
  Double_t reftime=0;
  for (Int_t i=0; i<fNTrgType; i++) {
    if (earliest<0 || fTrgTimes[i]<reftime) {
      earliest = i;
      reftime = fTrgTimes[i];
    }
  }

  Double_t offset = fGlOffset;
  if (earliest>=0) {
    offset += fToffsets[earliest];
    fEvtType = fTrgTypes[earliest];
  }
  fEvtTime = offset;
  return kOK;
}

//____________________________________________________________________________
Int_t THaTriggerTime::DefineVariables( EMode mode )
{
  // Define/delete standard event-by-event time offsets

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "evtime",  "Time-offset for event (trg based)", "fEvtTime" },
    { "evtype",  "Earliest trg-bit for the event",    "fEvtType" },
    { "trgtimes", "Times for each trg-type",          "fTrgTimes" },
    { 0 }
  };

  return DefineVarsFromList( vars, mode );
}

//____________________________________________________________________________
THaTriggerTime::~THaTriggerTime()
{
  // Normal destructor
  if ( fIsSetup )
    RemoveVariables();

  delete [] fTrgTimes;
}

ClassImp(THaTriggerTime)
