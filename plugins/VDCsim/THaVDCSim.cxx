#include "THaVDCSim.h"
#include "TSystem.h"
#include "TMath.h"
#include "VarDef.h"
#include "THaAnalysisObject.h"
#include "TDatime.h"

ClassImp(THaVDCSimWireHit)
ClassImp(THaVDCSimEvent)
ClassImp(THaVDCSimConditions)
ClassImp(THaVDCSimTrack)

#include <iostream>
#include <cstdio>
using namespace std;

void THaVDCSimWireHit::Print( Option_t* ) const
{
  cout << "Hit: wire: " << wirenum << " TDC: " << time
       << " time: " << rawTime << " pos: " << pos
       << " dist: " << distance
       << endl;
}

void THaVDCSimTrack::Clear( Option_t* opt ) {
  origin.Clear();
  momentum.Clear();
  for( auto& r : ray )
    r = 0.0;
  type = 0;
  layer = 0;
  track_num = 0;
  timeOffset = 0;
  for( int i = 0; i<4; i++ )
    slope[i] = xover[i] = 0.0;
  for( auto& planeHits : hits )
    planeHits.clear();
}

void THaVDCSimTrack::Print( Option_t* ) const
{
  //TObject::Print(opt);
  cout << "Track number = " << track_num << ", type = " << type
       << ", layer = " << layer << ", obj = " << hex << this << dec << endl;
  cout << "  Origin    = (" << origin.X() << ", " << origin.Y() << ", "
       << origin.Z() << ")" << endl;
  cout << "  Momentum  = (" << momentum.X() << ", " << momentum.Y() << ", "
       << momentum.Z() << ")" << endl;
  cout << "  TRANSPORT = (" << ray[0] << ", " << ray[1] << ", " << ray[2] << ", "
       << ray[3] << ", " << ray[4] << ")" << endl;
  cout << "  #hits = " << hits[0].size() << ", " << hits[1].size() << ", "
       << hits[2].size() << ", " << hits[3].size() << endl;
  cout << "  t0 = " << timeOffset << endl;
  cout << "  intercepts =";
  for(auto i : xover)
    cout << " " << i;
  cout << endl;
  cout << "  slopes     =";
  for(auto i : slope)
    cout << " " << i;
  cout << endl;
}

THaVDCSimEvent::THaVDCSimEvent() : event_num(0) {}

THaVDCSimEvent::~THaVDCSimEvent() = default;

void THaVDCSimEvent::Clear( Option_t* opt ) {
  for( auto& planehits : wirehits )
    planehits.clear();

  tracks.clear();
}

THaVDCSimConditions::THaVDCSimConditions()
  //set defaults conditions
  : filename("vdctracks.root"), numTrials(10000),
    wireHeight{}, driftVelocities{}, Prefixes{},
    numWires(368), noiseSigma(4.5), noiseMean(0.0), wireEff(1.0), tdcTimeLimit(900.0),
    emissionRate(0.000002), probWireNoise(0.0), x1(-1.8), x2(-0.1),ymean(0.0),
    ysigma(0.01), z0(-1.0),
    pthetamean(TMath::Pi()/4.0), pthetasigma(atan(1.1268) - TMath::Pi()/4.0),
    pphimean(0.0), pphisigma(atan(0.01846)), pmag0(1.0), tdcConvertFactor(2.0),
    cellWidth(0.0042426), cellHeight(0.026)
{
  // Constructor - determine the name of the database file

  const char* dbDir       = gSystem->Getenv("DB_DIR");
  const char* analyzerDir = gSystem->Getenv("ANALYZER");
  if( dbDir ) {
    databaseFile = dbDir;
    databaseFile += "/";
  }
  else if( analyzerDir ) {
    databaseFile = analyzerDir;
    databaseFile += "/DB/";
  }
  databaseFile += "db_L.vdc.dat";
}

THaVDCSimConditions::~THaVDCSimConditions() = default;

void THaVDCSimConditions::set(Double_t *something,
			      Double_t a, Double_t b, Double_t c, Double_t d) {
  something[0] = a;
  something[1] = b;
  something[2] = c;
  something[3] = d;
}

Int_t THaVDCSimConditions::ReadDatabase(Double_t* timeOffsets)
{
  //open the file as read only
  FILE* file = fopen(databaseFile, "r");
  if( !file ) return 1;

  TDatime now;
  Int_t err = THaAnalysisObject::kOK;
  vector<Float_t> tdc_offsets;
  tdc_offsets.reserve(numWires);
  for( int j = 0; j<4; j++ ) {
    Double_t driftVel = 0.0;
    tdc_offsets.clear();
    DBRequest request[] = {
      { "driftvel",    &driftVel,    kDouble, 0, false, -1 },
      { "tdc.offsets", &tdc_offsets, kFloatV },
      { nullptr }
    };
    try {
      TString prefix = Prefixes[j];
      prefix.Append(".");
      err = THaAnalysisObject::LoadDB(file, now, request, prefix.Data());
      if( err )
        break;
    }
    catch( const exception& e ) {
      err = THaAnalysisObject::kFileError;
      break;
    }

    // Move database parameters to destination variables
    driftVelocities[j] = driftVel/1e9;   //convert to m/ns

    if( tdc_offsets.size() != (size_t)numWires ) {
      cerr << "Database error: "
           << "Number of TDC offset values (" << tdc_offsets.size() << ") "
           << "disagrees with number of wires (" << numWires << ")" << endl;
    }
    int start = j*numWires;
    for (int i = 0; i < numWires; i++)
      timeOffsets[start+i] = tdc_offsets[i];
  }

  fclose(file);
  return err;
}

