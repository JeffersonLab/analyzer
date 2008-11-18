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
  fGlOffset(0), fNTrgType(0), fTrgTimes(NULL)
{
  // basic do-nothing-else contructor
}

//____________________________________________________________________________
Int_t THaTriggerTime::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  const int LEN = 200;
  char buf[LEN];

  // first is the list of channels to watch to determine the event type
  // This could just come from THaDecData, but for robustness we need
  // another copy.

  // Read data from database 
  FILE* fi = OpenFile( date );
  // however, this is not unexpected since most of the time it is un-necessary
  if( !fi ) return kOK;
  
  while ( ReadComment( fi, buf, LEN ) ) {}
  
  // Read in the time offsets, in the format below, to be subtracted from
  // the times measured in other detectors.
  //
  // TrgType 0 is special, in that it is a global offset that is applied
  //  to all triggers. This gives us a simple single value for adjustments.
  //
  // Trigger types NOT listed are assumed to have a zero offset.
  // 
  // <TrgType>   <time offset in seconds>
  // eg:
  //   0              10   -0.5e-9  # global-offset shared by all triggers and s/TDC
  //   1               0       crate slot chan
  //   2              10.e-9
  int trg;
  float toff;
  float ch2t=-0.5e-9;
  int fnd=0;
  int crate,slot,chan;
  fTrgTypes.clear();
  fToffsets.clear();
  fTDCRes = -0.5e-9; // assume 1872 TDC's.
  
  while ( fgets(buf,LEN,fi) ) {
    if ( (fnd=sscanf(buf,"%d %f %f",&trg,&toff,&ch2t)) < 2) continue;
    if (trg==0) {
      fGlOffset=toff;
      fTDCRes=ch2t;
    }
    else {
      if ( (fnd=sscanf(buf,"%d %f %d %d %d",&trg,&toff,&crate,&slot,&chan)!=5) ) {
	cerr << "Cannot parse line: " << buf << endl;
	continue;
      }
      fTrgTypes.push_back(trg);
      fToffsets.push_back(toff);
      fDetMap->AddModule(crate,slot,chan,chan,trg);
    }
  }    
  fclose(fi);

  // now construct the appropriate arrays
  delete [] fTrgTimes;
  fNTrgType = fTrgTypes.size();
  fTrgTimes = new Double_t[fNTrgType];
//   for (unsigned int i=0; i<fTrgTypes.size(); i++) {
//     if (fTrgTypes[i]==0) continue;
//     fTrg_gl.push_back(gHaVars->Find(Form("%s.bit%d",fDecDataNm.Data(),
// 					 fTrgTypes[i])));
//   }
  return kOK;
}

//____________________________________________________________________________
void THaTriggerTime::Clear(Option_t* )
{
  // Reset all variables to their default/unknown state
  fEvtType = -1;
  fEvtTime = fGlOffset;
  for (Int_t i=0; i<fNTrgType; i++) fTrgTimes[i]=kBig;
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
