//*-- Author :    Ole Hansen 03-April-2008
////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
////////////////////////////////////////////////////////////////////////

#include "THaHelicity.h"
#include "THaEvData.h"
#include "VarDef.h"
#include <iostream>
#include <cstring>

using namespace std;

//____________________________________________________________________
THaHelicity::THaHelicity( const char* name, const char* description,
			  THaApparatus* app ) : 
  THaADCHelicity(name, description, app), fG0_Hel(kUnknown),
  fGoodHel(false), fGoodHel2(false)
{

}

//____________________________________________________________________
THaHelicity::THaHelicity()
  : THaADCHelicity(), fG0_Hel(kUnknown),
    fGoodHel(false), fGoodHel2(false)
{
  // Default constructor - for ROOT I/O only
}

//____________________________________________________________________
THaHelicity::~THaHelicity() 
{
  // Destructor

  DefineVariables( kDelete );
}

//____________________________________________________________________
Int_t THaHelicity::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;

  // Define standard variables from base class
  THaADCHelicity::DefineVariables( mode );

  // Define variables for the G0 in-time mode
  const RVarDef var[] = {
    { "qrt",       "QRT from ROC",                 "fQrt" },
    { "gate",      "Helicity Gate from ROC",       "fGate" },
    { "pread",     "Present G0 reading",           "fPresentReading" },
    { "timestamp", "Timestamp from ROC",           "fTimestamp" },
    { "validtime", "Timestamp is valid",           "fValidTime" },
    { "g0_hel",    "G0 helicity reading",          "fG0_Hel" },
    { "goodhel",   "ADC and G0 helicities agree",  "fGoodHel" },
    { "goodhel2",  "ADC and G0 helicities agree unless one unknown", 
                                                  "fGoodHel2" },
    { 0 }
  };
  return DefineVarsFromList( var, mode );
}

//____________________________________________________________________
Int_t THaHelicity::ReadDatabase( const TDatime& date )
{
  // Read database

  // Read ADCHelicity parameters
  Int_t st = THaADCHelicity::ReadDatabase( date );
  if( st != kOK )
    return st;

  // Read G0 readout parameters (ROC addresses etc.)
  st = THaG0HelicityReader::ReadDatabase( GetDBFileName(), GetPrefix(),
					  date, fG0Debug );
  if( st != kOK )
    return st;

  // Read parameters for this class
  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  DBRequest req[] = {
    { "verbose",       &fDebug,      kInt, 0, 1, -2 },
    { 0 }
  };
  st = LoadDB( file, date, req );
  fclose(file);
  if( st )
    return kInitError;

  return kOK;
}

//____________________________________________________________________
void THaHelicity::Clear( Option_t* opt ) 
{
  // Clear the event data

  THaADCHelicity::Clear(opt);
  THaG0HelicityReader::Clear(opt);
  fG0_Hel  = kUnknown;
  fGoodHel2 = fGoodHel = false;
}

//____________________________________________________________________
Int_t THaHelicity::Decode( const THaEvData& evdata )
{
  // Decode ADC and G0 helicity data. Check for agreement. 
  // If disagreement, print error if requested.

  Int_t st = THaADCHelicity::Decode( evdata );
  if( st < 0 )
    return st;

  // Read G0 data
  st = ReadData( evdata );
  if( st < 0 )
    return st;
  if( fGate )
    fG0_Hel = ( fPresentReading ) ? kPlus : kMinus;

  // Test helicities for agreement
  fGoodHel  = (fADC_Hel == fG0_Hel);
  fGoodHel2 = (fGoodHel || 
	       (fADC_Hel == kUnknown && fG0_Hel  != kUnknown) ||
	       (fG0_Hel  == kUnknown && fADC_Hel != kUnknown) );

  if( (!fGoodHel && fDebug >= 2) || (!fGoodHel2 && fDebug >= 1) ) {
    Warning( Here("Decode"), "ADC and G0 helicities disagree: %d %d at "
	     "evno = %d", fADC_Hel, fG0_Hel, evdata.GetEvNum() );
  }

  return kOK;
}
 

ClassImp(THaHelicity)

