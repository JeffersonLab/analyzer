//*-- Author :    Robert Michaels    Aug 2000

//////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// Helicity of the beam.  If the helicity flag arrives in-time, the flag
// is simply an ADC value (high value = plus, pedestal value = minus).  
// If the helicity flag is in delayed mode, it is more complicated.
// Then we must calibrate time and use the time stamp info to deduce the 
// helicity.  The code for the 'delayed' mode is not written yet.
// Author Robert Michaels, August 2000
//
//
//////////////////////////////////////////////////////////////////////////

#include "THaHelicity.h"
#include "THaEvData.h"
#include <string.h>

ClassImp(THaHelicity)

//_____________________________________________________________________________
THaHelicity::THaHelicity( ) 
{
  Init();
}

//_____________________________________________________________________________
THaHelicity::~THaHelicity( ) 
{
#ifndef STANDALONE
  delete fDetMap;
#endif
}

//_____________________________________________________________________________
int THaHelicity::GetHelicity() const 
{
// Get the helicity data for this event (1-DAQ mode).
// By convention the return codes:   -1 = minus, +1 = plus, 0 = unknown
     return GetHelicity("R");
}

//_____________________________________________________________________________
int THaHelicity::GetHelicity(const TString& spec) const 
{
// Get the helicity data for this event. 
// spec must be "L"(left) or "R"(right) corresponding to 
// which spectrometer read the helicity info.
// By convention the return codes:   -1 = minus, +1 = plus, 0 = unknown

      if ((spec == "left") || (spec == "L")) return Lhel;
      if ((spec == "right") || (spec == "R")) return Rhel;
      return Hel;
}

//_____________________________________________________________________________
void THaHelicity::Init()
{
  // Set up the detector map for this detector
  // 
  // These data might come from a database in the future.

  static const int verbose = 0;

#ifndef STANDALONE
  fDetMap = new THaDetMap;
  fDetMap->AddModule(1,25,60,61);    // There are several redundant channels
  fDetMap->AddModule(2,25,14,15);    
  fDetMap->AddModule(2,25,46,47);    
  fDetMap->AddModule(14,6,0,1);
  indices[0][0] = 60;
  indices[0][1] = 61;
  indices[1][0] = 14;
  indices[1][1] = 15;
#else
  if(verbose) cout << "Warning:  Cannot run THaHelicity::Init() in STANDALONE mode"<<endl;
#endif
  
}

//_____________________________________________________________________________
Int_t THaHelicity::Decode( const THaEvData& evdata ) 
{
// Decode Helicity  data.

  ClearEvent();  
  UShort_t k;
  Int_t indx;

#ifndef STANDALONE
  for( UShort_t i = 0; i < fDetMap->GetSize(); i++ ) {

    THaDetMap::Module* d = fDetMap->GetModule( i );   

    for( UShort_t j = 0; j < evdata.GetNumChan( d->crate, d->slot); j++) {
      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      Double_t data = evdata.GetData (d->crate, d->slot, chan, 0);
      if( chan > d->hi || chan < d->lo ) continue;   
       if (d->crate == 1) {
         indx = NCHAN+1;
         for ( k = 0; k < 2; k++) {	
           if (chan == indices[0][k]) indx = k;
         }
         if(indx >=0 && indx < NCHAN) Ladc_helicity[indx] = data;
       }
       if (d->crate == 2) {
         indx = NCHAN+1;
         for (k = 0; k < 2; k++) {
           if (chan == indices[1][k]) indx = k;
	 }
         if(indx >=0 && indx < NCHAN) Radc_helicity[indx] = data;
       }
       if (d->crate == 14) {
          if(chan == 0) timestamp = data;
       }
    }
  }
  // Use 2 flags to make sure of helicity, for both L and R spectrom.
  // helicity[0] is should be opposite of helicity[1].
  Lhel = Minus;
  if (Ladc_helicity[0] > HCUT) {
      Lhel = Plus;
      if (Ladc_helicity[1] > HCUT) Lhel = Unknown;
  } else {
      if (Ladc_helicity[1] < HCUT) Lhel = Unknown;
  } 
  Rhel = Minus;
  if (Radc_helicity[0] > HCUT) {
      Rhel = Plus;
      if (Radc_helicity[1] > HCUT) Rhel = Unknown;
  } else {
      if (Radc_helicity[1] < HCUT) Rhel = Unknown;
  }
  Hel = Rhel;   // R spectrometer flag is used if running 1-spectrom DAQ.
  if (DEBUG) cout << "Helicity L,R "<<Lhel<<" "<<Rhel<<" time "<<timestamp<<endl;
  return OK;
#else
  return -1;
#endif
}
 
//_____________________________________________________________________________
void THaHelicity::ClearEvent()
{
  // Reset all local data to prepare for next event.
  memset( Ladc_helicity, 0, NCHAN*sizeof(Double_t) );
  memset( Radc_helicity, 0, NCHAN*sizeof(Double_t) );
}




















