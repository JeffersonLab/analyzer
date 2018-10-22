#include "THaVDCSim.h"
#include "TSystem.h"
#include "TMath.h"

ClassImp(THaVDCSimWireHit);
ClassImp(THaVDCSimEvent);
ClassImp(THaVDCSimConditions);
ClassImp(THaVDCSimTrack);

#include <iostream>
#include <cstdio>
using namespace std;

void THaVDCSimWireHit::Print( const Option_t* opt ) const
{
  cout << "Hit: wire: " << wirenum << " TDC: " << time
       << " time: " << rawTime << " pos: " << pos
       << " dist: " << distance
       << endl;
}

void THaVDCSimTrack::Clear( const Option_t* opt ) {
  for (int i = 0; i < 4; i++)
    hits[i].Clear(opt);
}

void THaVDCSimTrack::Print( const Option_t* opt ) const
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
  cout << "  #hits = " << hits[0].GetSize() << ", " << hits[1].GetSize() << ", "
       << hits[2].GetSize() << ", " << hits[3].GetSize() << endl;
  cout << "  t0 = " << timeOffset << endl;
  cout << "  intercepts =";
  for( int i=0; i<4; i++ )
    cout << " " << xover[i];
  cout << endl;
  cout << "  slopes     =";
  for( int i=0; i<4; i++ )
    cout << " " << slope[i];
  cout << endl;
}

THaVDCSimEvent::THaVDCSimEvent() {
}

THaVDCSimEvent::~THaVDCSimEvent() {
  Clear();
}

void THaVDCSimEvent::Clear( const Option_t* opt ) {
  for (Int_t i = 0; i < 4; i++)
    wirehits[i].Delete(opt);

  // Debug
#ifdef DEBUG
  cout << tracks.GetSize() << endl;
  for( int i=0; i<tracks.GetSize(); i++ ) {
    THaVDCSimTrack* track = (THaVDCSimTrack*)tracks.At(i);
    track->Print();
  }
#endif

  tracks.Delete(opt);
}

THaVDCSimConditions::THaVDCSimConditions()
 //set defaults conditions
    : filename("vdctracks.root"), numTrials(10000), 
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
  else
    databaseFile = "/u/home/orsborn/vdcsim/DB/";
  databaseFile += "20030415/db_L.vdc.dat";
}

THaVDCSimConditions::~THaVDCSimConditions()
{
  // Destructor
}

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


  // Use default values until ready to read from database

  const int LEN = 100;
  char buff[LEN];
  
  for( int j = 0; j<4; j++ ) {
    rewind(file);
    // Build the search tag and find it in the file. Search tags
    // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
    TString tag(Prefixes[j]); tag.Prepend("["); tag.Append("]"); 
    TString line, tag2(tag);
    tag.ToLower();
    // cout << "ReadDatabase:  tag = " << tag.Data() << endl;
    bool found = false;
    while (!found && fgets (buff, LEN, file) != NULL) {
      char* buf = ::Compress(buff);  //strip blanks
      line = buf;
      delete [] buf;
      if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
      line.ToLower();
      if ( tag == line ) 
	found = true;
    }
    if( !found ) {
      fclose(file);
      return 1;
    }
    
    for(int i = 0; i<5; i++)
      fgets(buff, LEN, file);  //skip 5 lines to get to drift velocity

    //read in drift velocity
    Double_t driftVel = 0.0;
    fscanf(file, "%lf", &driftVel);
    driftVelocities[j] = driftVel/1e9;   //convert to m/ns

    fgets(buff, LEN, file); // Read to end of line
    fgets(buff, LEN, file); // Skip line
  
    //Found the offset entries for this plane, so read time offsets and wire numbers

    for (int i = j*numWires; i < (j+1)*numWires; i++) {
      int wnum = 0;
      Double_t offset = 0.0;
      fscanf(file, " %d %lf", &wnum, &offset);
      timeOffsets[i] = offset;
    }
  }

  fclose(file);
  return 0;
}

