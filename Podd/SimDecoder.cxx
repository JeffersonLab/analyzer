/////////////////////////////////////////////////////////////////////
//
//   Podd::SimDecoder
//
//   Generic simulation decoder interface
//
/////////////////////////////////////////////////////////////////////

#include "SimDecoder.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

namespace Podd {

// Prefix of our own global variables (MC truth data)
const char* const MC_PREFIX = "MC.";

// Default half-size of search window for reconstructed hits (m)
Double_t MCTrackPoint::fgWindowSize = 1e-3;

//_____________________________________________________________________________
SimDecoder::SimDecoder() :
  fWeight{1.0},
  fMCHits{nullptr},
  fMCTracks{nullptr},
  fMCPoints{new TClonesArray("Podd::MCTrackPoint", 50)},
  fIsSetup{false}
{
  // Constructor. Derived classes must allocate the track and hit
  // TClonesArrays using their respective hit and track classes

  const char* const here = "SimDecoder::SimDecoder";

  // Register standard global variables for event header data
  // (It is up to the actual implementation of SimDecoder to fill these)
  if( gHaVars ) {
    const vector<VarDef> vars = {
        { .name = "runnum",  .desc = "Run number",    .type = kInt,   .loc = &run_num      },
        { .name = "runtype", .desc = "CODA run type", .type = kInt,   .loc = &run_type     },
        { .name = "runtime", .desc = "CODA run time", .type = kLong,  .loc = &fRunTime     },
        { .name = "evnum",   .desc = "Event number",  .type = kULong, .loc = &event_num    },
        { .name = "evtyp",   .desc = "Event type",    .type = kInt,   .loc = &event_type   },
        { .name = "evlen",   .desc = "Event Length",  .type = kInt,   .loc = &event_length },
        { .name = "evtime",  .desc = "Event time",    .type = kULong, .loc = &evt_time     },
    };
    TString prefix("g");
    // Prevent global variable clash if there are several instances of us
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".");
    gHaVars->DefineVariables(vars, {.prefix = prefix, .caller = here});
  } else
    Warning(here,"No global variable list found. Variables not registered.");
}

//_____________________________________________________________________________
SimDecoder::~SimDecoder()
{
  // Destructor. Derived classes must either not delete their TClonesArrays or
  // delete them and set the pointers to zero (using SafeDelete, for example),
  // else we may double-delete.

  SafeDelete(fMCPoints)
  SafeDelete(fMCTracks)
  SafeDelete(fMCHits)

  // Unregister global variables registered in the constructor
  if( gHaVars ) {
    TString prefix("g");
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".*");
    gHaVars->RemoveRegexp( prefix );
  }
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
MCHitInfo SimDecoder::GetMCHitInfo( Int_t /*crate*/, Int_t /*slot*/,
				    Int_t /*chan*/ ) const
{
  // Return MCHitInfo for the given digitized hardware channel.
  // This is a dummy function that derived classes should override if they
  // need this functionality.

  return {};
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

  const vector<RVarDef> vars = {
    // Event info
    { .name = "weight",    .desc = "Event weight",        .def = "fWeight" },
    // Generated hit and track info. Just report the sizes of the arrays.
    // Anything beyond this requires the type of the actual hit and
    // track classes.
    { .name = "tr.n",      .desc = "Number of MC tracks", .def = "GetNMCTracks()" },
    { .name = "hit.n",     .desc = "Number of MC hits",   .def = "GetNMCHits()" },
    // MCTrackPoints
    { .name = "pt.n",      .desc = "Number of MC track points",
                                                        .def = "GetNMCPoints()" },
    { .name = "pt.plane",  .desc = "Plane number",
                                   .def = "fMCPoints.Podd::MCTrackPoint.fPlane" },
    { .name = "pt.type",   .desc = "Plane type",
                                    .def = "fMCPoints.Podd::MCTrackPoint.fType" },
    { .name = "pt.status", .desc = "Reconstruction status",
                                  .def = "fMCPoints.Podd::MCTrackPoint.fStatus" },
    { .name = "pt.nfound", .desc = "# reconstructed hits found near this point",
                                  .def = "fMCPoints.Podd::MCTrackPoint.fNFound" },
    { .name = "pt.clustsz",  .desc = "Size of closest reconstructed cluster",
                               .def = "fMCPoints.Podd::MCTrackPoint.fClustSize" },
    { .name = "pt.time",   .desc = "Track arrival time [s]",
                                  .def = "fMCPoints.Podd::MCTrackPoint.fMCTime" },
    { .name = "pt.p",      .desc = "Track momentum [GeV]",
                                      .def = "fMCPoints.Podd::MCTrackPoint.P() "},
    // MC point positions in Cartesian/TRANSPORT coordinates
    { .name = "pt.x",      .desc = "Track pos lab x [m]",
                                      .def = "fMCPoints.Podd::MCTrackPoint.X()" },
    { .name = "pt.y",      .desc = "Track pos lab y [m]",
                                      .def = "fMCPoints.Podd::MCTrackPoint.Y()" },
    { .name = "pt.th",     .desc = "Track dir tan(theta)",
                                 .def = "fMCPoints.Podd::MCTrackPoint.ThetaT()" },
    { .name = "pt.ph",     .desc = "Track dir tan(phi)",
                                   .def = "fMCPoints.Podd::MCTrackPoint.PhiT()" },
    // MC point positions and directions in cylindrical/spherical coordinates
    { .name = "pt.r",      .desc = "Track pos lab r_trans [m]",
                                      .def = "fMCPoints.Podd::MCTrackPoint.R()" },
    { .name = "pt.theta",  .desc = "Track pos lab theta [rad]",
                                  .def = "fMCPoints.Podd::MCTrackPoint.Theta()" },
    { .name = "pt.phi",    .desc = "Track pos lab phi [rad]",
                                    .def = "fMCPoints.Podd::MCTrackPoint.Phi()" },
    { .name = "pt.thdir",  .desc = "Track dir theta [rad]",
                               .def = "fMCPoints.Podd::MCTrackPoint.ThetaDir()" },
    { .name = "pt.phdir",  .desc = "Track dir phi [rad]",
                                 .def = "fMCPoints.Podd::MCTrackPoint.PhiDir()" },
    // MC point analysis results
    { .name = "pt.deltaE", .desc = "Eloss wrt prev plane (GeV)",
                                  .def = "fMCPoints.Podd::MCTrackPoint.fDeltaE" },
    { .name = "pt.deflect",.desc = "Deflection wrt prev plane (rad)",
                                 .def = "fMCPoints.Podd::MCTrackPoint.fDeflect" },
    { .name = "pt.tof",    .desc = "Time-of-flight from prev plane (s)",
                                     .def = "fMCPoints.Podd::MCTrackPoint.fToF" },
    { .name = "pt.hitres", .desc = "Hit residual (mm)",
                                .def = "fMCPoints.Podd::MCTrackPoint.fHitResid" },
    { .name = "pt.trkres", .desc = "Track residual (mm)",
                              .def = "fMCPoints.Podd::MCTrackPoint.fTrackResid" },
  };

  return THaAnalysisObject::
    DefineGlobalVariables(vars, mode, this,
                          {.prefix = MC_PREFIX, .caller = here});
}

//_____________________________________________________________________________
MCTrack::MCTrack( Int_t number, Int_t pid,
		  const TVector3& vertex, const TVector3& momentum )
  : fNumber(number), fPID(pid), fOrigin(vertex),
    fMomentum(momentum), fNHits(0), fHitBits(0), fNHitsFound(0), fFoundBits(0),
    fReconFlags(0), fContamFlags(0), fMatchval(kBig), fFitRank(-1),
    fTrackRank(-1), fMCFitPar{}, fRcFitPar{}
{
  fill_n( fMCFitPar, NFP, kBig );
  fill_n( fRcFitPar, NFP, kBig );
}

//_____________________________________________________________________________
MCTrack::MCTrack()
  : fNumber(0), fPID(0), fNHits(0), fHitBits(0), fNHitsFound(0),
    fFoundBits(0), fReconFlags(0), fContamFlags(0), fMatchval(kBig),
    fFitRank(-1), fTrackRank(-1), fMCFitPar{}, fRcFitPar{}
{
  fill_n( fMCFitPar, NFP, kBig );
  fill_n( fRcFitPar, NFP, kBig );
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
}

//_____________________________________________________________________________
Int_t MCTrackPoint::Compare( const TObject* obj ) const
{
  // Sorting function for MCTrackPoints. Orders by fType, then fPlane.
  // Returns -1 if this is smaller than rhs, 0 if equal, +1 if greater.

  assert( dynamic_cast<const MCTrackPoint*>(obj) );
  const auto* rhs = static_cast<const MCTrackPoint*>(obj);

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
