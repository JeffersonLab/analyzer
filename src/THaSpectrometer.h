#ifndef ROOT_THaSpectrometer
#define ROOT_THaSpectrometer

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometer
//
//////////////////////////////////////////////////////////////////////////

#include "TObjArray.h"
#include "THaApparatus.h"
#include "TClonesArray.h"

class THaSpectrometerDetector;
class THaParticleInfo;
class THaPidDetector;
//class THaTrack;

class THaSpectrometer : public THaApparatus {
  
public:
  virtual ~THaSpectrometer();
  
  virtual Int_t            AddDetector( THaDetector* det );
  virtual Int_t            AddPidParticle( const char* shortname, 
					   const char* name,
					   Double_t mass, Int_t charge = 0 );
  virtual Int_t            CalcPID();
          void             Clear( Option_t* opt="" );
  virtual void             DefinePidParticles();
  virtual Int_t            DefineVariables( EMode mode = kDefine );
  virtual Int_t            FindVertices( TClonesArray& tracks ) = 0;
          Int_t            GetNpidParticles() const;
          Int_t            GetNpidDetectors() const;
  const   THaParticleInfo* GetPidParticleInfo( Int_t i ) const;
  const   THaPidDetector*  GetPidDetector( Int_t i ) const;
          Int_t            GetNTracks()  const { return fTracks->GetLast()+1; }
          TClonesArray*    GetTracks()   const { return fTracks; }
          TClonesArray*    GetTrackPID() const { return fTrackPID; }
          TClonesArray*    GetVertices() const { return fVertices; }
          Bool_t           IsPID() const       { return fPID; }
  virtual Int_t            Reconstruct();
          void             SetPID( Bool_t b = kTRUE ) { fPID = b; }
  virtual Int_t            TrackCalc() = 0;

protected:

  static const Int_t kInitTrackMultiplicity = 3;

  TClonesArray*   fTracks;                //Tracks 
  TClonesArray*   fTrackPID;              //PID info for the tracks
  TClonesArray*   fVertices;              //Track vertices
  TList*          fTrackingDetectors;     //Tracking detectors
  TList*          fNonTrackingDetectors;  //Non-tracking detectors
  TObjArray*      fPidDetectors;          //PID detectors
  TObjArray*      fPidParticles;          //Particles for which we want PID

  Bool_t          fPID;                   //PID enabled

  THaSpectrometer() {}     // only derived classes can construct me
  THaSpectrometer( const char* name, const char* description );


private:
  Bool_t          fListInit;      //Detector lists initialized

  void            ListInit();     //Initializes lists of specialized detectors

  ClassDef(THaSpectrometer,0)     //A generic spectrometer
};


//---------------- inlines ----------------------------------------------------

inline Int_t THaSpectrometer::GetNpidParticles() const
{
  return fPidParticles->GetLast()+1;
}

//_____________________________________________________________________________
inline Int_t THaSpectrometer::GetNpidDetectors() const
{
  return fPidDetectors->GetLast()+1;
}

//_____________________________________________________________________________
inline const THaParticleInfo* THaSpectrometer::GetPidParticleInfo( Int_t i ) 
  const
{
  return (const THaParticleInfo*) fPidParticles->At(i);
}

//_____________________________________________________________________________
inline const THaPidDetector* THaSpectrometer::GetPidDetector( Int_t i ) const
{
  return (const THaPidDetector*) fPidDetectors->At(i);
}


#endif

