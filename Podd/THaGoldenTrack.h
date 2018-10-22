#ifndef Podd_THaGoldenTrack_h_
#define Podd_THaGoldenTrack_h_

//////////////////////////////////////////////////////////////////////////
//
// THaGoldenTrack
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaTrackInfo.h"
#include "TString.h"

class THaSpectrometer;
class THaTrack;

class THaGoldenTrack : public THaPhysicsModule {
  
public:
  THaGoldenTrack( const char* name, const char* description,
		  const char* spectro="" );
  virtual ~THaGoldenTrack();
  
  virtual void      Clear( Option_t* opt="" );
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& evdata );

  THaTrack*         GetTrack()     const { return fTrack; }
  const THaTrackInfo* GetTrackInfo() const { return &fTrkIfo; }
  Int_t             GetIndex()     const { return fIndex; }
  Double_t          GetGoldBeta()  const { return fGoldBeta; }

  void              SetSpectrometer( const char* name );

protected:

  THaTrackInfo            fTrkIfo;       // Data of Golden Track
  Int_t                   fIndex;        // Index of the Golden Track
  Double_t                fGoldBeta;     // Beta of the Golden Track
  THaTrack*               fTrack;        // Pointer to Golden Track

  TString                 fSpectroName;  // Name of spectrometer
  THaSpectrometer*        fSpectro;      // Pointer to spectrometer object

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(THaGoldenTrack,1)   //Golden track module
};

//_________ inlines __________________________________________________________
inline 
void THaGoldenTrack::SetSpectrometer( const char* name ) {
  fSpectroName = name; 
}

#endif
