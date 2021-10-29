/////////////////////////////////////////////////////////////////////
//
//   THaVDCSimDecoder
//   Hall A VDC Event Data from a predefined ROOT file
//
//   Authors:  Ken Rossato (rossato@jlab.org)
//             Jens-Ole Hansen (ole@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaVDCSimDecoder.h"
#include "THaVDCSim.h"
#include "THaBenchmark.h"
#include "VarDef.h"

using namespace std;

#define DEBUG 0
#define MC_PREFIX "MC."

//-----------------------------------------------------------------------------
THaVDCSimDecoder::THaVDCSimDecoder() : fIsSetup{false}
{
  // Constructor

  DefineVariables();
}

//-----------------------------------------------------------------------------
THaVDCSimDecoder::~THaVDCSimDecoder() {

  DefineVariables( THaAnalysisObject::kDelete );
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimDecoder::DefineVariables( THaAnalysisObject::EMode mode )
{
  const char* const here = "THaVDCSimDecoder::DefineVariables";

  if( mode == THaAnalysisObject::kDefine && fIsSetup )
    return THaAnalysisObject::kOK;
  fIsSetup = ( mode == THaAnalysisObject::kDefine );

  RVarDef vars[] = {
    { "tr.n",    "Number of tracks",             "GetNTracks()" },
    { "tr.x",    "Track x coordinate (m)",       "fTracks.TX()" },
    { "tr.y",    "Track x coordinate (m)",       "fTracks.TY()" },
    { "tr.th",   "Tangent of track theta angle", "fTracks.TTheta()" },
    { "tr.ph",   "Tangent of track phi angle",   "fTracks.TPhi()" },
    { "tr.p",    "Track momentum (GeV)",         "fTracks.P()" },
    { "tr.type", "Track type",                   "fTracks.type" },
    { "tr.layer","Layer of track origin",        "fTracks.layer" },
    { "tr.no",   "Track number",                 "fTracks.track_num" },
    { "tr.t0",   "Track time offset",            "fTracks.timeOffset" },
    { "vdc.u1.pos",  "U1 crossing point (m)",    "fTracks.U1Pos()" },
    { "vdc.v1.pos",  "V1 crossing point (m)",    "fTracks.V1Pos()" },
    { "vdc.u2.pos",  "U2 crossing point (m)",    "fTracks.U2Pos()" },
    { "vdc.v2.pos",  "V2 crossing point (m)",    "fTracks.V2Pos()" },
    { "vdc.u1.slope","U1 track slope",           "fTracks.U1Slope()" },
    { "vdc.v1.slope","V1 track slope",           "fTracks.V1Slope()" },
    { "vdc.u2.slope","U2 track slope",           "fTracks.U2Slope()" },
    { "vdc.v2.slope","V2 track slope",           "fTracks.V2Slope()" },
    { nullptr }
  };

  Int_t ret =
    THaAnalysisObject::DefineVarsFromList( vars,
					   THaAnalysisObject::kRVarDef,
					   mode, "", this, MC_PREFIX, here );

  return ret;
}

//-----------------------------------------------------------------------------
void THaVDCSimDecoder::Clear( Option_t* opt )
{
  // Clear track and plane data

  THaEvData::Clear();

  fTracks.clear();
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimDecoder::GetNTracks() const
{
  return fTracks.size();
}

//-----------------------------------------------------------------------------
int THaVDCSimDecoder::LoadEvent(const UInt_t* evbuffer )
{
  // Decode event data in evbuffer

  Clear();

  // Local copy of evbuffer pointer - any good use for it?
  buffer = evbuffer;

  // Cast the evbuffer pointer back to exactly the event type that is present
  // in the input file (in THaVDCSimRun). The pointer-to-integer is there
  // just for compatibility with the standard decoder.
  // Note: simEvent can't be constant - ROOT does not like to iterate
  // over const TList.
  const auto* simEvent = (const THaVDCSimEvent*)(evbuffer);

  if(DEBUG) PrintOut();
  if (first_decode) {
    init_cmap();
    if (init_slotdata() == HED_ERR) return HED_ERR;
    first_decode = false;
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  Clear();
  for( auto i : fSlotClear )
    crateslot[i]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");

  evscaler = 0;

  // FIXME -KCR, put it off, fill it once we know what the answer is
  // If we don't fix the other section below, we can leave this as is
  event_length = 0;

  event_type = 1;
  event_num = simEvent->event_num;

  if( fDoBench ) fBench->Begin("physics_decode");


  // Decode the digitized data.  Populate crateslot array.
  for (int i = 0; i < 4; i++) { // for each plane
    for( const auto& hit : simEvent->wirehits[i] ) {
    // iterate over hits

      // FIXME: HardCode crate/slot/chan nums for now...
      Int_t chan = hit.wirenum % 96;
      Int_t raw = hit.time;
      Int_t roc = 3;
      //  Int_t slot = hit.wirenum / 96 + 6 + i*4;

      Int_t slot = hit.wirenum / 96 + 3 + i*4;
      if(slot > 11)
	slot += 4;
      //   cout << slot << endl;

      if (crateslot[idx(roc,slot)]->loadData("tdc",chan,raw,raw)
	  == SD_ERR) return HED_ERR;
    }
  }

  // Extract MC track info, so we can access it via global variables
  // The list of tracks is already part of the event - no need to generate
  // our own tracks here.
  // FIXME: However, we have to copy the list because the global variable system
  // cannot handle variable pointers.

  fTracks.assign( simEvent->tracks.begin(), simEvent->tracks.end() );

  if( fDoBench ) fBench->Stop("physics_decode");

  // DEBUG:
  //  cout << "SimDecoder: nTracks = " << GetNTracks() << endl;
  //fTracks.Print();

  return HED_OK;
}

//-----------------------------------------------------------------------------
ClassImp(THaVDCSimDecoder)
