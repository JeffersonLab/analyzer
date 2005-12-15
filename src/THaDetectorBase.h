#ifndef ROOT_THaDetectorBase
#define ROOT_THaDetectorBase

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include "TVector3.h"

class THaDetMap;

class THaDetectorBase : public THaAnalysisObject {
  
public:
  virtual ~THaDetectorBase();

  THaDetectorBase(); // only for ROOT I/O

  virtual Int_t            Decode( const THaEvData& ) = 0;
          Int_t            GetNelem()  const    { return fNelem; }
          const TVector3&  GetOrigin() const    { return fOrigin; }
          const Float_t*   GetSize()   const    { return fSize; }
          void             PrintDetMap( Option_t* opt="") const;

protected:

  // Mapping
  THaDetMap*      fDetMap;    // Hardware channel map for this detector

  // Configuration
  Int_t           fNelem;     // Number of detector elements (paddles, mirrors)

  // Geometry 
  TVector3        fOrigin;    // Origin of detector plane in detector coordinates (m)
  Float_t         fSize[3];   // Detector size in x,y,z (m) - x,y are half-widths
  
  THaDetectorBase( const char* name, const char* description );

  ClassDef(THaDetectorBase,1)   //ABC for a detector or subdetector
};

#endif
