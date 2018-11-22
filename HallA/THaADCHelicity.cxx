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

static const Double_t kDefaultThreshold = 4000.0;

//____________________________________________________________________
THaADCHelicity::THaADCHelicity( const char* name, const char* description,
				THaApparatus* app ) : 
  THaHelicityDet( name, description, app ),
  fADC_hdata(kBig), fADC_Gate(kBig), fADC_Hel(kUnknown), 
  fThreshold(kDefaultThreshold), fIgnoreGate(kFALSE),
  fInvertGate(kFALSE), fNchan(0)
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

  vector<Int_t> heldef, gatedef;
  fThreshold  = kDefaultThreshold;
  Int_t ignore_gate = -1, invert_gate = 0;
  const  DBRequest request[] = {
    { "helchan",      &heldef,       kIntV,   0, 0, -2 },
    { "gatechan",     &gatedef,      kIntV,   0, 1, -2 },
    { "threshold",    &fThreshold,   kDouble, 0, 1, -2 },
    { "ignore_gate",  &ignore_gate,  kInt,    0, 1, -2 },
    { "invert_gate",  &invert_gate,  kInt,    0, 1, -2 },
    { 0 }
  };
  // Database keys are prefixed with this detector's name, not apparatus.name
  err = LoadDB( file, date, request, fPrefix );
  fclose(file);
  if( err )
    return kInitError;

  if( heldef.size() != 3 ) {
    Error( Here(here), "Incorrect definition of helicity data channel. Must be "
	   "exactly 3 numbers (roc,slot,chan), found %u. Fix database.", 
	   static_cast<unsigned int>(heldef.size()) );
    return kInitError;
  }
  // Missing gate channel implies ignoring gate unless explicitly set
  if( gatedef.empty() ) {
    if( ignore_gate < 0 )
      fIgnoreGate = kTRUE;
    else {
      Error( Here(here), "Missing gate data channel definition gatechan. "
	     "Fix database." );
      return kInitError;
    }
  }
  if( !gatedef.empty() && gatedef.size() != 3 ) {
    Error( Here(here), "Incorrect definition of gate data channel. Must be "
	   "exactly 3 numbers (roc,slot,chan), found %u. Fix database.", 
	   static_cast<unsigned int>(gatedef.size()) );
    return kInitError;
  }

  fIgnoreGate = (ignore_gate > 0);
  // If ignoring gate and no gate channel given, decode only one
  fNchan = (fIgnoreGate && gatedef.empty()) ? 1 : 2;

  try {
    fAddr[0] = heldef;
    if( !gatedef.empty() )
      fAddr[1] = gatedef;
  }
  catch( const std::out_of_range& e) {
    Error( Here(here), "%s. Fix database.", e.what() );
    return kInitError;
  }
  fInvertGate = (invert_gate != 0);

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
  
  // Only the first two channels defined in the detector map are used
  // here, regardless of how they are defined (consecutive channels
  // in same module or otherwise). ReadDatabase guarantees that two channels
  // are present and warns about extra channels.

  if( !fIsInit )
    return -1;

  Int_t ret = 0;
  bool gate_high = false;

  for( Int_t i = 0; i < fNchan; ++i ) {
    Int_t roc = fAddr[i].roc, slot = fAddr[i].slot, chan = fAddr[i].chan;
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
      gate_high = fInvertGate ? (data < fThreshold) : (data >= fThreshold);
      break;
    }
  }

  // Logic: if gate==0 helicity remains unknown. If gate==1 
  // (or we are ingoring the gate) then helicity is determined by 
  // the helicity bit.
  if( gate_high || fIgnoreGate ) {
    fADC_Hel = ( fADC_hdata >= fThreshold ) ? kPlus : kMinus;
    ret = 1;
  }

  // fHelicity may be reassigned by derived classes, so we must keep the ADC
  // result separately. But within this class, the two are the same.
  if( fSign >= 0 )
    fHelicity = fADC_Hel;
  else
    fHelicity = ( fADC_Hel == kPlus ) ? kMinus : kPlus;

  if (fDebug >= 3) {
    cout << "ADC helicity info "<<endl
	 << "Gate "<<fADC_Gate<<"  helic. bit "<<fADC_hdata
	 << "    ADC helicity "<<fADC_Hel
	 << "    resulting helicity"<<fHelicity<<endl;
  }

  return ret;
}
 

ClassImp(THaADCHelicity)

