#ifndef ROOT_THaGoldenTrack
#define ROOT_THaGoldenTrack

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
  
  virtual void      Clear( Option_t* opt="" ) { 
    THaPhysicsModule::Clear(opt);
    fTrkIfo.Clear(opt); fIndex = -1; fTrack = NULL;
  }

  THaTrack*         GetTrack()     const { return fTrack; }
  const THaTrackInfo* GetTrackInfo() const { return &fTrkIfo; }
  Int_t             GetIndex()     const { return fIndex; }

  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process( const THaEvData& evdata );
          void      SetSpectrometer( const char* name );

protected:

  THaTrackInfo            fTrkIfo;       // Data of Golden Track
  Int_t                   fIndex;        // Index of the Golden Track
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
