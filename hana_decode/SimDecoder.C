/////////////////////////////////////////////////////////////////////
//
//   Podd::SimDecoder
//
//   Generic simulation decoder interface
//
/////////////////////////////////////////////////////////////////////

#include "SimDecoder.h"
#include <iostream>

using namespace std;

namespace Podd {

// Prefix of our own global variables (MC truth data)
const char* const MC_PREFIX = "MC.";

// Default half-size of search window for reconstructed hits (m)
Double_t MCTrackPoint::fgWindowSize = 1e-3;

//_____________________________________________________________________________
SimDecoder::SimDecoder()
  : fWeight(1.0), fMCHits(0), fMCTracks(0), fIsSetup(false)
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
    fMCHits->Clear(opt);
  if( fMCTracks )
    fMCTracks->Clear(opt);
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

//_____________________________________________________________________________
Int_t SimDecoder::DefineVariables( THaAnalysisObject::EMode mode )
{
  // Define generic global variables. Derived classes may override or extend
  // this function. It is not automatically called.

  const char* const here = "SimDecoder::DefineVariables";

  if( mode == THaAnalysisObject::kDefine && fIsSetup )
    return THaAnalysisObject::kOK;
  fIsSetup = ( mode == THaAnalysisObject::kDefine );

  RVarDef vars[] = {
    // Event info
    { "weight",    "Event weight",        "fWeight" },
    // Generated hit and track info. Just report the sizes of the arrays.
    // Anything beyond this requires the type of the actual hit and
    // track classes.
    { "tr.n",      "Number of MC tracks", "GetNMCTracks()" },
    { "hit.n",     "Number of MC hits",   "GetNMCHits()" },
    // MCTrackPoints
    { "pt.n",      "Number of MC track points",
                                                        "GetNMCPoints()" },
    { "pt.plane",  "Plane number",
                                   "fMCPoints.Podd::MCTrackPoint.fPlane" },
    { "pt.type",   "Plane type",
                                    "fMCPoints.Podd::MCTrackPoint.fType" },
    { "pt.status", "Reconstruction status",
                                  "fMCPoints.Podd::MCTrackPoint.fStatus" },
    { "pt.nfound", "# reconstructed hits found near this point",
                                  "fMCPoints.Podd::MCTrackPoint.fNFound" },
    { "pt.clustsz",  "Size of closest reconstructed cluster",
                               "fMCPoints.Podd::MCTrackPoint.fClustSize" },
    { "pt.time",   "Track arrival time [s]",
                                  "fMCPoints.Podd::MCTrackPoint.fMCTime" },
    { "pt.p",      "Track momentum [GeV]",
                                      "fMCPoints.Podd::MCTrackPoint.P() "},
    // MC point positions in Cartesian/TRANSPORT coordinates
    { "pt.x",      "Track pos lab x [m]",
                                      "fMCPoints.Podd::MCTrackPoint.X()" },
    { "pt.y",      "Track pos lab y [m]",
                                      "fMCPoints.Podd::MCTrackPoint.Y()" },
    { "pt.th",     "Track dir tan(theta)",
                                 "fMCPoints.Podd::MCTrackPoint.ThetaT()" },
    { "pt.ph",     "Track dir tan(phi)",
                                   "fMCPoints.Podd::MCTrackPoint.PhiT()" },
    // MC point positions and directions in cylindrical/spherical coordinates
    { "pt.r",      "Track pos lab r_trans [m]",
                                      "fMCPoints.Podd::MCTrackPoint.R()" },
    { "pt.theta",  "Track pos lab theta [rad]",
                                  "fMCPoints.Podd::MCTrackPoint.Theta()" },
    { "pt.phi",    "Track pos lab phi [rad]",
                                    "fMCPoints.Podd::MCTrackPoint.Phi()" },
    { "pt.thdir",  "Track dir theta [rad]",
                               "fMCPoints.Podd::MCTrackPoint.ThetaDir()" },
    { "pt.phdir",  "Track dir phi [rad]",
                                 "fMCPoints.Podd::MCTrackPoint.PhiDir()" },
    // MC point analysis results
    { "pt.deltaE", "Eloss wrt prev plane (GeV)",
                                  "fMCPoints.Podd::MCTrackPoint.fDeltaE" },
    { "pt.deflect","Deflection wrt prev plane (rad)",
                                 "fMCPoints.Podd::MCTrackPoint.fDeflect" },
    { "pt.tof",    "Time-of-flight from prev plane (s)",
                                     "fMCPoints.Podd::MCTrackPoint.fToF" },
    { "pt.hitres", "Hit residual (mm)",
                                "fMCPoints.Podd::MCTrackPoint.fHitResid" },
    { "pt.trkres", "Track residual (mm)",
                              "fMCPoints.Podd::MCTrackPoint.fTrackResid" },
    { 0 }
  };

  return THaAnalysisObject::
    DefineVarsFromList( vars, THaAnalysisObject::kRVarDef,
			mode, "", this, MC_PREFIX, here );
}

//_____________________________________________________________________________
MCTrack::MCTrack( Int_t number, Int_t pid,
		  const TVector3& vertex, const TVector3& momentum )
  : fNumber(number), fPID(pid), fOrigin(vertex),
    fMomentum(momentum), fNHits(0), fHitBits(0), fNHitsFound(0), fFoundBits(0),
    fReconFlags(0), fContamFlags(0), fMatchval(KBIG), fFitRank(-1),
    fTrackRank(-1)
{
  for( Int_t i = 0; i < NFP; ++i ) {
    fMCFitPar[i] = KBIG;
    fRcFitPar[i] = KBIG;
  }
}

//_____________________________________________________________________________
MCTrack::MCTrack()
  : fNumber(0), fPID(0), fNHits(0), fHitBits(0), fNHitsFound(0),
    fFoundBits(0), fReconFlags(0), fContamFlags(0), fMatchval(KBIG),
    fFitRank(-1), fTrackRank(-1)
{
  for( Int_t i = 0; i < NFP; ++i ) {
    fMCFitPar[i] = KBIG;
    fRcFitPar[i] = KBIG;
  }
}

//_____________________________________________________________________________
void MCTrack::Print( const Option_t* /*opt*/ ) const
{
  // Print MCTrack info

  cout << "track: num = " << fNumber
       << ", PID = " << fPID
       << endl;
  cout << "  Origin    = ";  fOrigin.Print();
  cout << "  Momentum  = ";  fMomentum.Print();
}

//_____________________________________________________________________________
void MCHitInfo::MCPrint() const
{
  // Print MC digitized hit info

  cout << " MCtrack = " << fMCTrack
       << ", MCpos = "  << fMCPos
       << ", MCtime = " << fMCTime
       << ", num_bg = " << fContam
       << endl;
};

//_____________________________________________________________________________
Int_t MCTrackPoint::Compare( const TObject* obj ) const
{
  // Sorting function for MCTrackPoints. Orders by fType, then fPlane.
  // Returns -1 if this is smaller than rhs, 0 if equal, +1 if greater.

  assert( dynamic_cast<const MCTrackPoint*>(obj) );
  const MCTrackPoint* rhs = static_cast<const MCTrackPoint*>(obj);

  if( fType  < rhs->fType  ) return -1;
  if( fType  > rhs->fType  ) return  1;
  if( fPlane < rhs->fPlane ) return -1;
  if( fPlane > rhs->fPlane ) return  1;
  return 0;
}

//_____________________________________________________________________________
void MCTrackPoint::Print( Option_t* ) const
{
  // Print MC track point info

  cout << " MCtrack = "     << fMCTrack
       << ", plane = "      << fPlane
       << ", plane_type = " << fType
       << ", status = "     << fStatus << endl
       << " coord = ";  fMCPoint.Print();
  cout << " P = ";      fMCP.Print();
  cout << " time = "        << fMCTime << " s"
       << ", deltaE = "     << fDeltaE << " GeV"
       << ", deflect = "    << 1e3*fDeflect << " mrad"
       << ", tof = "        << 1e9*fToF << " ns"
       << ", hit_resid = "  << 1e3*fHitResid << " mm"
       << ", trk_resid = "  << 1e3*fTrackResid << " mm"
       << endl;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

ClassImp(Podd::MCHitInfo)
ClassImp(Podd::MCTrack)
ClassImp(Podd::MCTrackPoint)
ClassImp(Podd::SimDecoder)
