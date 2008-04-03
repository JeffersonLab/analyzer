//*-- Author :    Robert Michaels    Sept 2002
// Updated by     Vincent Sulkosky   Jan  2006
// More updates by Richard Holmes,   Feb 2006
// Changed into an implementation of THaHelicityDet, Ole Hansen, Aug 2006
////////////////////////////////////////////////////////////////////////
//
// THaADCHelicity
//
// Helicity of the beam - from a single ADC.
//    +1 = plus,  -1 = minus,  0 = unknown
// 
////////////////////////////////////////////////////////////////////////

#include "THaADCHelicity.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include <iostream>
#include <vector>

using namespace std;

static const Double_t kDefaultThreshold = 5000.0;

//____________________________________________________________________
THaADCHelicity::THaADCHelicity( const char* name, const char* description,
				THaApparatus* app ) : 
  THaHelicityDet( name, description, app ),
  fADC_hdata(kBig), fADC_Gate(kBig), fADC_Hel(kUnknown), 
  fThreshold(kDefaultThreshold), fIgnoreGate(kFALSE)
{
  // Constructor
}

//____________________________________________________________________
THaADCHelicity::~THaADCHelicity() 
{
  DefineVariables( kDelete );
}

//_____________________________________________________________________________
Int_t THaADCHelicity::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  THaHelicityDet::DefineVariables( mode );

  const RVarDef var[] = {
    { "adc",       "Helicity ADC raw data",  "fADC_hdata" },
    { "gate_adc",  "Gate ADC raw data",      "fADC_Gate" },
    { "adc_hel",   "Beam helicity from ADC", "fADC_Hel" },
    { 0 }
  };
  return DefineVarsFromList( var, mode );
}


//____________________________________________________________________
Int_t THaADCHelicity::ReadDatabase( const TDatime& date )
{

  static const char* const here = "ReadDatabase";

  Int_t err = THaHelicityDet::ReadDatabase( date );
  if( err )
    return err;

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  vector<Int_t> detmap;
  fThreshold  = kDefaultThreshold;
  fIgnoreGate = kFALSE;
  const  DBRequest request[] = {
    { "detmap",       &detmap,       kIntV,   0, 0, -2 },
    { "threshold",    &fThreshold,   kDouble, 0, 1, -2 },
    { "ignore_gate",  &fIgnoreGate,  kInt,    0, 1, -2 },
    { 0 }
  };
  // Database keys are prefixed with this detector's name, not apparatus.name
  err = LoadDB( file, date, request, fPrefix );
  fclose(file);
  if( err )
    return kInitError;

  // Parse detector map. Warn/err if something looks suspect
  err = FillDetMap( detmap, 0, here );
  if( err <= 0 )
    //Error already printed
    return kInitError;

  Int_t nchan = fDetMap->GetTotNumChan();
  if( nchan < 2 ) {
    if( !fIgnoreGate ) {
      Error( Here(here), "Need excatly 2 detector map channels, found %d. "
	     "Fix database.", nchan );
      return kInitError;
    } else if( nchan == 0 ) {
      Error( Here(here), "Need either 1 or 2 detector map channels if "
	     "ignoring gate, found %d. Fix database.", nchan );
      return kInitError;
    }
  } else if ( nchan > 2 ) {
    Warning( Here(here), "Extra channels in detector map. Should be at most "
	     "2, found %d. Check database.", nchan );
  }

  fIsInit = true;
  return kOK;
}

//____________________________________________________________________
void THaADCHelicity::Clear( Option_t* opt ) 
{
  // Clear the event data

  THaHelicityDet::Clear(opt);
  fADC_hdata = kBig;
  fADC_Gate  = kBig;
  fADC_Hel   = kUnknown;
}

//____________________________________________________________________
Int_t THaADCHelicity::Decode( const THaEvData& evdata )
{
  // Decode Helicity data.
  // Return 1 if helicity was assigned, 0 if not, -1 if error.
  
  Clear();  

  // Only the first two channels defined in the detector map are used
  // here, regardless of how they are defined (consecutive channels
  // in same module or otherwise). ReadDatabase guarantees that two channels
  // are present and warns about extra channels.

  if( !fIsInit )
    return -1;

  // Collect the addresses of the two input channels (helicity bit, gate).
  // This is a bit tedious because the channels could be specified either
  // as adjacent channels in a single module or as two separate modules.
  struct ChanDef_t { UShort_t roc, slot, chan; } addr[2];
  THaDetMap::Module* d = fDetMap->GetModule(0);
  Int_t k = 0;
  // If ignoring gate and only one channel given, decode only one
  Int_t imax = ( fIgnoreGate && fDetMap->GetTotNumChan() == 1 ) ? 1 : 2;
  for( Int_t i = 0; i < imax; i++, k++ ) {
    if( !d || d->GetNchan() == 0 )
      return -1;
    addr[i].roc  = d->crate;
    addr[i].slot = d->slot;
    if( d->reverse )
      addr[i].chan = d->hi-k;
    else
      addr[i].chan = d->lo+k;
    // If the first module contains only one channel, get the second
    // channel from the next module
    if( d->GetNchan() == 1 && (i+1 < imax) ) {
      d = fDetMap->GetModule(i+1);
      k = -1;
    }
  }

  Int_t ret = 0;
  bool gate_high = false;

  for( Int_t i = 0; i < imax; ++i ) {
    Int_t roc = addr[i].roc, slot = addr[i].slot, chan = addr[i].chan;
    if ( !evdata.GetNumHits( roc, slot, chan ))
      continue;

    Double_t data = 
      static_cast<Double_t>(evdata.GetData( roc, slot, chan, 0 ));

    if (fDebug >= 3) {
      cout << "crate "<<roc<<"   slot "<<slot<<"   chan "
	   <<chan<<"   data "<<data<<"   ";
      if (data > fThreshold) 
	cout << "  above cut !";
      cout << endl;
    }

    // Assign gate and helicity bit data. The first data channel is
    // the helicity bit, the second, the gate.
    switch(i) {
    case 0:
      fADC_hdata = data; 
      break;
    case 1:
      fADC_Gate = data;
      gate_high = ( data > fThreshold );
      break;
    }

    // Logic: if gate==0 helicity remains unknown. If gate==1 
    // (or we are ingoring the gate) then helicity is determined by 
    // the helicity bit.
    if( gate_high || fIgnoreGate ) {
      fADC_Hel = ( fADC_hdata > fThreshold ) ? kPlus : kMinus;
      if( fSign<0 )
	fADC_Hel = ( fADC_Hel == kPlus ) ? kMinus : kPlus;
      ret = 1;
    }

    if (fDebug >= 3) {
      cout << "ADC helicity info "<<endl;
      cout << "Gate "<<fADC_Gate<<"  helic. bit "<<fADC_hdata;
      cout << "    resulting helicity "<<fADC_Hel<<endl;
    }

  }
  // fHelicity may be reassigned by derived classes, so we must keep the ADC
  // result separately. But within this class, the two are the same.
  fHelicity = fADC_Hel;

  return ret;
}
 

ClassImp(THaADCHelicity)

