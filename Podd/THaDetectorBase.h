#ifndef Podd_THaDetectorBase_h_
#define Podd_THaDetectorBase_h_

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include "THaDetMap.h"
#include "TVector3.h"
#include "DetectorData.h"
#include <vector>
#include <memory>

class THaDetectorBase : public THaAnalysisObject {
public:
  using VecDetData_t = std::vector<std::unique_ptr<Podd::DetectorData>>;

  virtual ~THaDetectorBase();

  THaDetectorBase(); // only for ROOT I/O

  virtual void     Clear( Option_t* ="" );
  virtual Int_t    Decode( const THaEvData& );
  virtual void     Reset( Option_t* opt="" );

  VecDetData_t&    GetDetectorData() { return fDetectorData; }
  THaDetMap*       GetDetMap() const { return fDetMap; }
  Int_t            GetNelem()  const { return fNelem; }
  Int_t            GetNviews() const { return fNviews; }
  const TVector3&  GetOrigin() const { return fOrigin; }
  const Double_t*  GetSize()   const { return fSize; }
  Double_t         GetXSize()  const { return 2.0*fSize[0]; }
  Double_t         GetYSize()  const { return 2.0*fSize[1]; }
  Double_t         GetZSize()  const { return fSize[2]; }
  const TVector3&  GetXax()    const { return fXax; }
  const TVector3&  GetYax()    const { return fYax; }
  const TVector3&  GetZax()    const { return fZax; }

  virtual Bool_t   IsInActiveArea( Double_t x, Double_t y ) const;
  virtual Bool_t   IsInActiveArea( const TVector3& point ) const;
  TVector3         DetToTrackCoord( const TVector3& point ) const;
  TVector3         DetToTrackCoord( Double_t x, Double_t y ) const;
  TVector3         TrackToDetCoord( const TVector3& point ) const;

  Int_t            FillDetMap( const std::vector<Int_t>& values,
			       UInt_t flags=0,
			       const char* here = "FillDetMap" );
  void             PrintDetMap( Option_t* opt="") const;

  virtual Int_t    GetView( const DigitizerHitInfo_t& hitinfo ) const;

protected:
  // Mapping
  THaDetMap*    fDetMap;    // Hardware channel map for this detector

  // Configuration
  Int_t         fNelem;     // Number of detector elements (paddles, mirrors)
  Int_t         fNviews;    // Number of readouts per element (e.g. L/R side)

  // Geometry
  TVector3      fOrigin;    // Position of detector (m)
  Double_t      fSize[3];   // Detector size in x,y,z (m) - x,y are half-widths

  TVector3      fXax;       // X axis of the detector plane
  TVector3      fYax;       // Y axis of the detector plane
  TVector3      fZax;       // Normal to the detector plane

  // Generic per-event data (optional)
  VecDetData_t  fDetectorData;

  virtual void  DefineAxes( Double_t rotation_angle );

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );
  virtual Int_t ReadGeometry( FILE* file, const TDatime& date,
			      Bool_t required = false );

  // Callbacks from Decode()
  virtual Int_t     StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data );
  virtual OptUInt_t LoadData( const THaEvData& evdata,
                              const DigitizerHitInfo_t& hitinfo );
  virtual void      PrintDecodedData( const THaEvData& evdata ) const;

  void DebugWarning( const char* here, const char* msg, UInt_t evnum );
  void MultipleHitWarning( const DigitizerHitInfo_t& hitinfo, const char* here );
  void DataLoadWarning( const DigitizerHitInfo_t& hitinfo, const char* here );

  // Only derived classes may construct me
  THaDetectorBase( const char* name, const char* description );

  ClassDef(THaDetectorBase,3)   //ABC for a detector or subdetector
};

#endif
