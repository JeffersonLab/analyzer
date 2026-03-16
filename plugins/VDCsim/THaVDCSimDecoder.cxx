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
#include "Helper.h"
#include <vector>

using namespace std;
using namespace Podd;

const char* const MC_PREFIX = "MC.";

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

  const vector<RVarDef> vars = {
    { .name = "tr.n",         .desc = "Number of tracks",             .def = "GetNTracks()"       },
    { .name = "tr.x",         .desc = "Track x coordinate (m)",       .def = "fTracks.TX()"       },
    { .name = "tr.y",         .desc = "Track x coordinate (m)",       .def = "fTracks.TY()"       },
    { .name = "tr.th",        .desc = "Tangent of track theta angle", .def = "fTracks.TTheta()"   },
    { .name = "tr.ph",        .desc = "Tangent of track phi angle",   .def = "fTracks.TPhi()"     },
    { .name = "tr.p",         .desc = "Track momentum (GeV)",         .def = "fTracks.P()"        },
    { .name = "tr.type",      .desc = "Track type",                   .def = "fTracks.type"       },
    { .name = "tr.layer",     .desc = "Layer of track origin",        .def = "fTracks.layer"      },
    { .name = "tr.no",        .desc = "Track number",                 .def = "fTracks.track_num"  },
    { .name = "tr.t0",        .desc = "Track time offset",            .def = "fTracks.timeOffset" },
    { .name = "vdc.u1.pos",   .desc = "U1 crossing point (m)",        .def = "fTracks.U1Pos()"    },
    { .name = "vdc.v1.pos",   .desc = "V1 crossing point (m)",        .def = "fTracks.V1Pos()"    },
    { .name = "vdc.u2.pos",   .desc = "U2 crossing point (m)",        .def = "fTracks.U2Pos()"    },
    { .name = "vdc.v2.pos",   .desc = "V2 crossing point (m)",        .def = "fTracks.V2Pos()"    },
    { .name = "vdc.u1.slope", .desc = "U1 track slope",               .def = "fTracks.U1Slope()"  },
    { .name = "vdc.v1.slope", .desc = "V1 track slope",               .def = "fTracks.V1Slope()"  },
    { .name = "vdc.u2.slope", .desc = "U2 track slope",               .def = "fTracks.U2Slope()"  },
    { .name = "vdc.v2.slope", .desc = "V2 track slope",               .def = "fTracks.V2Slope()"  },
  };

  return THaAnalysisObject::DefineGlobalVariables(vars, mode, this,
      {.prefix = MC_PREFIX, .caller = here});
}

//-----------------------------------------------------------------------------
void THaVDCSimDecoder::Clear( Option_t* )
{
  // Clear track and plane data

  THaEvData::Clear();

  fTracks.clear();
}

//-----------------------------------------------------------------------------
Int_t THaVDCSimDecoder::GetNTracks() const
{
  return ToInt(fTracks.size());
}

//-----------------------------------------------------------------------------
int THaVDCSimDecoder::LoadEvent(const UInt_t* evbuffer )
{
  // Decode event data in evbuffer

  Clear();

  // Local copy of evbuffer pointer - any good use for it?
  buffer = evbuffer;
  ++raw_event_num;

  // Cast the evbuffer pointer back to exactly the event type that is present
  // in the input file (in THaVDCSimRun). The pointer-to-integer is there
  // just for compatibility with the standard decoder.
  // Note: simEvent can't be constant - ROOT does not like to iterate
  // over const TList.
  const auto* simEvent = (const THaVDCSimEvent*)(evbuffer);

  if(fDebug) PrintOut();
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
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaVDCSimDecoder)
#endif
