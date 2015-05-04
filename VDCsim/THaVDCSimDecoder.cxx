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
THaVDCSimDecoder::THaVDCSimDecoder()
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
    { "tr.x",    "Track x coordinate (m)",       "fTracks.THaVDCSimTrack.TX()" },
    { "tr.y",    "Track x coordinate (m)",       "fTracks.THaVDCSimTrack.TY()" },
    { "tr.th",   "Tangent of track theta angle", "fTracks.THaVDCSimTrack.TTheta()" },
    { "tr.ph",   "Tangent of track phi angle",   "fTracks.THaVDCSimTrack.TPhi()" },
    { "tr.p",    "Track momentum (GeV)",         "fTracks.THaVDCSimTrack.P()" },
    { "tr.type", "Track type",                   "fTracks.THaVDCSimTrack.type" },
    { "tr.layer","Layer of track origin",        "fTracks.THaVDCSimTrack.layer" },
    { "tr.no",   "Track number",                 "fTracks.THaVDCSimTrack.track_num" },
    { "tr.t0",   "Track time offset",            "fTracks.THaVDCSimTrack.timeOffset" },
    { "vdc.u1.pos",  "U1 crossing point (m)",    "fTracks.THaVDCSimTrack.U1Pos()" },
    { "vdc.v1.pos",  "V1 crossing point (m)",    "fTracks.THaVDCSimTrack.V1Pos()" },
    { "vdc.u2.pos",  "U2 crossing point (m)",    "fTracks.THaVDCSimTrack.U2Pos()" },
    { "vdc.v2.pos",  "V2 crossing point (m)",    "fTracks.THaVDCSimTrack.V2Pos()" },
    { "vdc.u1.slope","U1 track slope",           "fTracks.THaVDCSimTrack.U1Slope()" },
    { "vdc.v1.slope","V1 track slope",           "fTracks.THaVDCSimTrack.V1Slope()" },
    { "vdc.u2.slope","U2 track slope",           "fTracks.THaVDCSimTrack.U2Slope()" },
    { "vdc.v2.slope","V2 track slope",           "fTracks.THaVDCSimTrack.V2Slope()" },
    { 0 }
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

  // Never delete the tracks, only clear the list. The tracks are deleted
  // in THaVDCSimRun::ReadEvent() by the call to event->Clear().
  fTracks.Clear("nodelete");
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimDecoder::GetNTracks() const
{
  return fTracks.GetSize();
}

//-----------------------------------------------------------------------------
int THaVDCSimDecoder::LoadEvent(const int* evbuffer )
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
  THaVDCSimEvent* simEvent = (THaVDCSimEvent*)(evbuffer);

  if(DEBUG) PrintOut();
  if (first_decode) {
    init_cmap();     
    if (init_slotdata(map) == HED_ERR) return HED_ERR;
    first_decode = false;
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  Clear();
  for( int i=0; i<fNSlotClear; i++ )
    crateslot[fSlotClear[i]]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");
  
  evscaler = 0;

  // FIXME -KCR, put it off, fill it once we know what the answer is
  // If we don't fix the other section below, we can leave this as is
  event_length = 0;

  event_type = 1;
  event_num = simEvent->event_num;
  recent_event = event_num;

  if( fDoBench ) fBench->Begin("physics_decode");


  // Decode the digitized data.  Populate crateslot array.
  for (int i = 0; i < 4; i++) { // for each plane
    TIter nextHit( &simEvent->wirehits[i] );
    // iterate over hits
    while( THaVDCSimWireHit *hit = 
	   static_cast<THaVDCSimWireHit*>( nextHit() )) {
	 
      // FIXME: HardCode crate/slot/chan nums for now...
      Int_t chan = hit->wirenum % 96;
      Int_t raw = static_cast<Int_t>(hit->time);
      Int_t roc = 3;
      //  Int_t slot = hit->wirenum / 96 + 6 + i*4;

      Int_t inislot = hit->wirenum / 96 + 3 + i*4;
      Int_t slot;

      if(inislot <= 11){
	slot = inislot;
      }else{
	slot = inislot + 4;
      }
      // cout << inislot << " > " << slot << endl;

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

  TIter nextTrack( &simEvent->tracks );
  while( THaVDCSimTrack* track = (THaVDCSimTrack*)nextTrack() ) {
    fTracks.Add( track );
  }

  if( fDoBench ) fBench->Stop("physics_decode");

  // DEBUG:
  //  cout << "SimDecoder: nTracks = " << GetNTracks() << endl;
  //fTracks.Print();

  return HED_OK;
}

//-----------------------------------------------------------------------------
ClassImp(THaVDCSimDecoder)
