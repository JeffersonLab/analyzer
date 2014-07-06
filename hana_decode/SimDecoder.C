/////////////////////////////////////////////////////////////////////
//
//   Podd::SimDecoder
//
//   Generic simulation decoder interface
//
/////////////////////////////////////////////////////////////////////

#include "SimDecoder.h"

using namespace std;

ClassImp(Podd::SimDecoder)
ClassImp(Podd::MCHitInfo)

namespace Podd {

// Prefix of our own global variables (MC truth data)
const char* const MC_PREFIX = "MC.";

//_____________________________________________________________________________
SimDecoder::SimDecoder() : fMCHits(0), fMCTracks(0), fIsSetup(false)
{
  // Constructor. Derived classes must allocate the track and hit
  // TClonesArrays using their respective hit and track classes

  fMCPoints = new TClonesArray( "Podd::MCTrackPoint", 50 );
}

//_____________________________________________________________________________
SimDecoder::~SimDecoder()
{
  // Destructor. Derived classes must either not delete their TClonesArrays or
  // delete them and set the pointers to zero (using SafeDelete, for example),
  // else we may double-delete.

  SafeDelete(fMCPoints);
  SafeDelete(fMCTracks);
  SafeDelete(fMCHits);
}

//_____________________________________________________________________________
void SimDecoder::Clear( Option_t* opt )
{
  // Clear the TClonesArrays, assuming their elements do not allocate any
  // memory. If they do, derived classes must override this function
  // to call either Clear("C") or Delete().

  THaEvData::Clear();

  if( fMCHits )
    fMCHits->Clear();
  if( fMCTracks )
    fMCTracks->Clear();
  fMCPoints->Clear();
}

//_____________________________________________________________________________
MCHitInfo SimDecoder::GetMCHitInfo( Int_t crate, Int_t slot, Int_t chan ) const
{
  // Return MCHitInfo for the given digitized hardware channel.
  // This is a dummy function that derived classes should override if they
  // need this functionality.

  return MCHitInfo();
}

//-----------------------------------------------------------------------------
Int_t SimDecoder::DefineVariables( THaAnalysisObject::EMode mode )
{
  // Define generic global variables. Derived classes may override or extend
  // this function. It is not automatically called.

  const char* const here = "SimDecoder::DefineVariables";

  if( mode == THaAnalysisObject::kDefine && fIsSetup )
    return THaAnalysisObject::kOK;
  fIsSetup = ( mode == THaAnalysisObject::kDefine );

  RVarDef vars[] = {
    // Generated hit and track info. Just report the sizes of the arrays.
    // Anything beyond this requires the type of the actual hit and
    // track classes.
    { "hit.n",     "Number of MC hits",   "GetNMCHits()" },
    { "tr.n",      "Number of MC tracks", "GetNMCTracks()" },
    { 0 }
  };

  return THaAnalysisObject::
    DefineVarsFromList( vars, THaAnalysisObject::kRVarDef,
			mode, "", this, MC_PREFIX, here );
}

//_____________________________________________________________________________
void MCHitInfo::MCPrint() const
{
  // Print MC hit info

  cout << " MCtrack = " << fMCTrack
       << ", MCpos = " << fMCPos
       << ", MCtime = " << fMCTime
       << ", num_bg = " << fContam
       << endl;
}

//_____________________________________________________________________________
void MCTrackPoint::Print( Option_t* ) const
{
  // Print MC hit info

  cout << " MCtrack = " << fMCTrack
       << ", plane = "  << fPlane
       << ", coord = "; fMCPoint.Print();
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd
