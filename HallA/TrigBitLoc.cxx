//////////////////////////////////////////////////////////////////////////
//
// TrigBitLoc
//
// Plugin for DecData generic raw data decoder to process Hall A
// trigger bits
//
//////////////////////////////////////////////////////////////////////////

#include "TrigBitLoc.h"
#include "THaEvData.h"
#include "TObjArray.h"

using namespace std;

BdataLoc::TypeIter_t TrigBitLoc::fgThisType =
  DoRegister( BdataLocType( "TrigBitLoc",    "bit",    6 ));

//_____________________________________________________________________________
Int_t TrigBitLoc::Configure( const TObjArray* params, Int_t start )
{
  // Initialize CrateLoc from given parameters

  // Load name, crate, slot, channel
  Int_t ret = CrateLocMulti::Configure( params, start );
  if( ret )
    return ret;

  // Additional parameters: lower and upper TDC cuts
  cutlo = GetString( params, start+4 ).Atoi();
  cuthi = GetString( params, start+5 ).Atoi();

  // The bit number is determined from any trailing digits of the name,
  // which must be in the range 0-31
  assert( fName.Length() > 0 ); // else bug in base class Configure()
  Int_t pos = fName.Length()-1, end = pos;
  TString e;
  do {
    e = fName(pos);
  } while( e.IsDigit() && pos-- > 0 );
  if( pos == end ) { // No digits at the end of the name
    Error( "TrigBitLoc", "Name of trigger bit variable %s must end with bit "
	   "number (0-31). Example: \"bit12\". Fix database.", GetName() );
    return 50;
  }
  e = fName(pos+1,end-pos);
  Int_t val = e.Atoi();
  if( val < 0 || val > 31 ) {
    Error( "TrigBitLoc", "Illegal bit number %d in trigger bit name %s. "
	   "Must be 0-31. Fix database.", val, GetName() );
    return 50;
  }
  bitnum = val;

  return 0;
}

//_____________________________________________________________________________
Int_t TrigBitLoc::DefineVariables( EMode mode )
{
  // Define the global variable for trigger bit test result. This result is
  // stored in the "data" member of the base class, not in the rdata array, so
  // here we just do what the base class does.

  return BdataLoc::DefineVariables(mode);
}

//_____________________________________________________________________________
void TrigBitLoc::Load( const THaEvData& evdata )
{
  // Test hit(s) in our TDC channel for a valid trigger bit and set results

  // Read hit(s) from defined multihit TDC channel
  CrateLocMulti::Load( evdata );

  // Figure out which triggers got a hit.  These are multihit TDCs, so we
  // need to sort out which hit we want to take by applying cuts.
  for( UInt_t ihit = 0; ihit < NumHits(); ++ihit ) {
    if( Get(ihit) > cutlo && Get(ihit) < cuthi ) {
      data = 1;
      if( bitloc )
	*bitloc |= BIT(bitnum);
      break;
    }
  }
}

//_____________________________________________________________________________
Int_t TrigBitLoc::OptionPtr( void* ptr )
{
  // TrigBitLoc uses the optional pointer to set the 'bitloc' address

  bitloc = static_cast<UInt_t*>(ptr);
  return 0;
}

//_____________________________________________________________________________
void TrigBitLoc::Print( Option_t* opt ) const
{
  // Print name and all data values

  PrintCrateLocHeader(opt);
  TString option(opt);
  if( option.Contains("FULL") ) {
    cout << "\t cutlo/cuthi = " << cutlo << "/" << cuthi;
  }
  PrintMultiData(opt);
  cout << endl;
}

//_____________________________________________________________________________
ClassImp(TrigBitLoc)
