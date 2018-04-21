#ifndef Podd_THaCoincTime_h_
#define Podd_THaCoincTime_h_

//////////////////////////////////////////////////////////////////////////
//
// THaCoincTime
//    Calculate coincidence times for all tracks for a pair of
//    spectrometers. Everything is calculated relative to the
//    common starts -- timing offsets for different paddles are NOT
//    taken into account here.
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"

class THaSpectrometer;
class THaScintillator;
class THaDetMap;
class THaTrack;

class THaCoincTime : public THaPhysicsModule {
  
public:
  THaCoincTime( const char* name, const char* description,
		const char* spec1="L", const char* spec2="R",
		Double_t mass1 = .938272, Double_t mass2 = 0.000511,
		const char* ch_name1=0, const char* ch_name2=0);
  
  virtual ~THaCoincTime();
  
  virtual void      Clear( Option_t* opt="" );
  
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );

 protected:

  TString           fSpectN1, fSpectN2; // Names of spectrometers to use
  Double_t          fpmass1, fpmass2;   // masses to use for coinc. time

  THaSpectrometer  *fSpect1, *fSpect2;  // pointers to spectrometers

  Double_t          fTdcRes[2];    // TDC res. of TDC in spec1 and spec2
  Double_t          fTdcOff[2];    // timing offset (in seconds)

  TString           fTdcLabels[2]; // detector names for reading out TDCs
                                   // (should be obvious like "RbyL" or "LbyBB"

  // per-event data
  Int_t             fSz1, fSz2;         // allocated number of tracks
  Int_t             fNTr1, fNTr2;       // number of tracks in spectrometers
  
  Double_t*         fVxTime1;      //[fNTr1] times at vertex for tracks in spec1
  Double_t*         fVxTime2;      //[fNTr2] times at vertex for tracks in spec2
  
  Double_t          fdTdc[2];      // timing of trig sp2+delay after trig sp1,
                                   // and timiming of trig sp1+delay after trig sp2

  Int_t             fSzNtr;        // allocated number of time combinations
  Int_t             fNtimes;       // = fNTr1*fNTr2
                                   // number of time combinations to consider
  
  Int_t*            fTrInd1;       //[fNtimes] track index from spec1 for fDiff*
  Int_t*            fTrInd2;       //[fNtimes] track index from spec2 for fDiff*
  
  Double_t*         fDiffT2by1;    //[fNtimes] overall time difference at vtx
  Double_t*         fDiffT1by2;    //[fNtimes] overall time difference at vtx
  
  
  virtual Int_t DefineVariables( EMode mode = kDefine );

  virtual Int_t ReadDatabase( const TDatime& date );

  THaDetMap *fDetMap;

 public:
  
  ClassDef(THaCoincTime,0)   //Single arm kinematics module
};

#endif
