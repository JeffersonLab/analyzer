#ifndef ROOT_THaCoincidenceTime
#define ROOT_THaCoincidenceTime

//////////////////////////////////////////////////////////////////////////
//
// THaCoincidenceTime
//    Calculate coincidence times for (ultimately) an arbitrary number
//    of spectrometers. Everything is calculated relative to the
//    common starts -- timing offsets for different paddles are NOT
//    taken into account here.
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"

class THaSpectrometer;
class THaScintillator;
class THaDetMap;
class THaTrack;

class THaCoincidenceTime : public THaPhysicsModule {
  static const Int_t NSPECT = 2;
public:
  THaCoincidenceTime( const char* name, const char* description,
		      const char* spec1="L", const char* spec2="R",
		      Double_t mass1 = .938272, Double_t mass2 = 0.000511 );
  
  virtual ~THaCoincidenceTime();
  
  virtual void      Clear( Option_t* opt="" );
  
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& );

 protected:

  TString           fSpectroName1;  // Name of spectrometer to consider
  TString           fSpectroName2;  // Name of spectrometer to consider

  Double_t          fpmass1, fpmass2; // masses to use for coinc. time
  
  Int_t             fNSpectro;     // number of spectrometers with tracks
  Double_t*         fVxTime;       //[fNSpectro] time at vertex for golden tracks
  
  Int_t             fNtimes;       // number of time combinations to consider
                                   // should be like Factorial(fNSpectro)
  Double_t*         fdT1_2;        //[fNtimes] timing of delayed trig sp1 after trig sp2
  Double_t*         fTdcRes;       //[fNtimes] s/channel of T2_1
  TString*          fTdcLabels;    //![fNtimes] labels for measurements, to connect
                                   // spectrometers to TDCs cleanly
  
  Double_t*         fDiffT;        //[fNtimes] overall time difference at vtx
  THaTrack**        fGoldTr;       //![fNSpectro] working space for tracks

  virtual Int_t DefineVariables( EMode mode = kDefine );

  virtual Int_t ReadDatabase( const TDatime& date );

  // Information to use for each spectrometer
  class SpecInfo {
  public:
    SpecInfo(THaSpectrometer* s, Double_t m) : spec(s),pmass(m) { }
    THaSpectrometer* spec;
    Double_t pmass;
  };

  typedef std::vector<SpecInfo>::iterator SP_i;
  
  std::vector<SpecInfo> fSpectro;     // Pointers to spectrometer collections

  THaDetMap *fDetMap;

 public:
  
  ClassDef(THaCoincidenceTime,0)   //Single arm kinematics module
};

#endif
