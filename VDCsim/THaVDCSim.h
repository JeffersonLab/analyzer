// THaVDCSim.h
//
// These are all data classes intended for storing results
// from randomly generated and digitized "simulated" data.
//
// author Ken Rossato (rossato@jlab.org)
//

#ifndef Podd_THaVDCSim_h_
#define Podd_THaVDCSim_h_

#include "TList.h"
#include "TVector3.h"
#include "TString.h"

class THaVDCSimConditions : public TObject {
 public:
  THaVDCSimConditions();
  virtual ~THaVDCSimConditions();

  TString filename;           //name of output file
  TString textFile;           //name of output text file (holds track information)
  int numTrials;              //number of events generated in monte carlo
  Double_t wireHeight[4];      // Height of each wire in Z coord
  Double_t driftVelocities[4]; // Drift velocity in Z coord
  TString Prefixes[4];        //array of prefixes to use in reading database
  TString databaseFile;       //name of database file to read
  int numWires;               //number of wires in each chamber
  double noiseSigma;          //sigma value of noise distribution
  double noiseMean;           //mean of noise distribution 
  double wireEff;             //wire efficiency (decimal form)
  double tdcTimeLimit;        //window of time the tdc reads (in ns)
  double emissionRate;        //rate particles are emitted from target (in particles/ns)
  double probWireNoise;     //probability of random chamber wires firing
  const Double_t x1;      
  const Double_t x2;           //x coordinate limits for origin of tracks
  const Double_t ymean;
  const Double_t ysigma;       //mean and sigma value for origin  y coord. distribution
  const Double_t z0;           //z coordinate of track origin
  const Double_t pthetamean;
  const Double_t pthetasigma;  //mean and sigma for momentum theta distribution 
  const Double_t pphimean;
  const Double_t pphisigma;    //mean and sigma of momentum phi distribution
  const Double_t pmag0;        //magnitude of momentum
  Double_t tdcConvertFactor;   //width of single time bin (in ns), used to convert from time to tdc values
  const Double_t cellWidth;    //width of cell (i.e. horizontal spacing between sense wires)
  const Double_t cellHeight;   //height of cell (i.e. vertical spacing between u and v wire planes)

  void set(Double_t *something,
	   Double_t a, Double_t b, Double_t c, Double_t d);

  Int_t ReadDatabase(Double_t* timeOffsets);
 
 ClassDef (THaVDCSimConditions, 5) // Simulation Conditions
};

class THaVDCSimWireHit : public TObject {
public:
  THaVDCSimWireHit()
    : wirenum(0), type(0), rawTime(0), rawTDCtime(0), time(0), distance(0), wireFail(false) {}
  Int_t wirenum;      // Wire number
  Int_t type;         // flag for tyep of hit. actual = 0, noise = 1
  Double_t rawTime;    // Time of wire hit in nanoseconds
  Int_t rawTDCtime; //tdc time w/out noise
  Int_t time;       //tdctime w/additional noise
  Double_t distance;   //drift distance 
  bool wireFail;      //set to true when the wire fails
  Double_t pos;       //wire position (m) along u(v) coordinate

  virtual void Print( const Option_t* opt="" ) const;

  ClassDef (THaVDCSimWireHit, 4) // Simple Wirehit class
};

class THaVDCSimEvent : public TObject {
public:
  THaVDCSimEvent();
  virtual ~THaVDCSimEvent();

  // Administrative data
  Int_t event_num;

  // "Simulated" data
  TList wirehits[4]; //list of hits for each set of wires (u1,v1,u2,v2)
  TList tracks;      //list of tracks for each plane

  virtual void Clear( const Option_t* opt="" );

  ClassDef (THaVDCSimEvent, 3) // Simple simulated track class
};

class THaVDCSimTrack : public TObject {
 public: 
  THaVDCSimTrack(Int_t type = 0, Int_t num = 0)
    : type(type), layer(0), track_num(num) {}

  TVector3 origin; // Origin of track
  TVector3 momentum; // Momentum of track
  Double_t X() {return origin.X();}
  Double_t Y() {return origin.Y();}
  Double_t ray[5];  //position on U1 plane in transport coordinates
  // Functions needed for global variables - FIXME: remove when g.v. system improved
  Double_t TX()     { return ray[0]; }
  Double_t TY()     { return ray[1]; }
  Double_t TTheta() { return ray[2]; }
  Double_t TPhi()   { return ray[3]; }
  Double_t P()      { return momentum.Mag(); }
  Double_t U1Slope(){ return slope[0]; }
  Double_t V1Slope(){ return slope[1]; }
  Double_t U2Slope(){ return slope[2]; }
  Double_t V2Slope(){ return slope[3]; }
  Double_t U1Pos()  { return xover[0]; }
  Double_t V1Pos()  { return xover[1]; }
  Double_t U2Pos()  { return xover[2]; }
  Double_t V2Pos()  { return xover[3]; }

  Int_t type;   //type of track. 0 = trigger, 1 = coincident, 2 = delta ray, 3 = cosmic ray
  Int_t layer;  //layer of material track originated from. 0 = target, 1 = vacuum window, 
                // 2 = vdc frame etc.
  Int_t track_num;  //track index
  Double_t timeOffset; //time offset of the track (0 if trigger, random otherwise)
  Double_t slope[4];  //angle between z and projection into (wire axis)-z plane
  Double_t xover[4]; // cross-over coordinate (intercept) at each wire plane

  TList hits[4]; // Hits of this track only in each wire plane

  virtual void Clear( const Option_t* opt="" );
  virtual void Print( const Option_t* opt="" ) const;
  
  ClassDef (THaVDCSimTrack, 4) // Simluated VDC track
};

#endif
