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
#include <vector>
#include <memory>

class THaSpectrometer;
class THaScintillator;
class THaDetMap;
class THaTrack;

class THaCoincTime : public THaPhysicsModule {
  
public:
  THaCoincTime( const char* name, const char* description,
		const char* spec1="L", const char* spec2="R",
		Double_t mass1 = .938272, Double_t mass2 = 0.000511,
		const char* ch_name1=nullptr, const char* ch_name2= nullptr);
  
  virtual ~THaCoincTime();
  
  virtual void      Clear( Option_t* opt="" );
  
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );

  Int_t   GetNTr1()   const { return fVxTime1.size(); }
  Int_t   GetNTr2()   const { return fVxTime2.size(); }
  Int_t   GetNTimes() const { return fTimeCombos.size(); }

 protected:

  // Configuration
  TString           fSpectN1, fSpectN2; // Names of spectrometers to use
  Double_t          fpmass1, fpmass2;   // masses to use for coinc. time

  THaSpectrometer  *fSpect1, *fSpect2;  // pointers to spectrometers

  TString           fTdcLabels[2]; // detector names for reading out TDCs
                                   // (should be obvious like "RbyL" or "LbyBB"

  std::unique_ptr<THaDetMap> fDetMap;

  // Calibrations
  Double_t          fTdcRes[2];    // TDC res. of TDC in spec1 and spec2
  Double_t          fTdcOff[2];    // timing offset (in seconds)

  // per-event data
  Double_t          fTdcData[2];   // timing of trig sp2+delay after trig sp1,
                                   // and timing of trig sp1+delay after trig sp2

  std::vector<Double_t> fVxTime1;  // times at vertex for tracks in spec1
  std::vector<Double_t> fVxTime2;  // times at vertex for tracks in spec2
  

  class TimeCombo {
  public:
    TimeCombo(Int_t i, Int_t j, Double_t t21, Double_t t12 )
      : fTrInd1(i), fTrInd2(j), fDiffT2by1(t21), fDiffT1by2(t12) {}
    Int_t    fTrInd1;              // track index from spec1 for fDiff*
    Int_t    fTrInd2;              // track index from spec2 for fDiff*
    Double_t fDiffT2by1;           // overall time difference at vtx
    Double_t fDiffT1by2;           // overall time difference at vtx
  };
  std::vector<TimeCombo> fTimeCombos;  // time combinations to consider

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );


 public:
  
  ClassDef(THaCoincTime,0)   //Single arm kinematics module
};

#endif
