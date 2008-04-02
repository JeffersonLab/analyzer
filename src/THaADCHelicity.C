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
    Error( Here(here), "Need exactly 2 detector map channels, found %d. "
	   "Fix database.", nchan );
    return kInitError;
  } else if ( nchan > 2 ) {
    Warning( Here(here), "Extra channels in detector map. Should be exactly "
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

  // Collect the input channel addresses
  struct ChanDef_t { UShort_t roc, slot, chan; } addr[2];
  THaDetMap::Module* d = fDetMap->GetModule(0);
  if( !d )
    return -1;
  memcpy( addr, d, 3*sizeof(UShort_t) );
  if( d->GetNchan() > 1 ) {
    memcpy( addr+1, d, 3*sizeof(UShort_t) );
    ++addr[1].chan;
  } else {
    d = fDetMap->GetModule(1);
    if( !d )
      return -1;
    memcpy( addr+1, d, 3*sizeof(UShort_t) );
  }

  Int_t ret = 0;
  bool gate_high = false;

  for( Int_t i = 0; i < 2; ++i ) {
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
    // the gate, the second, the helicity bit.
    // NB: As of 9/04, there is no gate on L-arm, only helicity bit.
    // For this arm, set fIgnoreGate true.
    switch(i) {
    case 0:
      fADC_Gate = data;
      gate_high = ( data > fThreshold );
      break;
    case 1:
      fADC_hdata = data; 
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

