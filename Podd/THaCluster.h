#ifndef Podd_THaCluster_h_
#define Podd_THaCluster_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCluster                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"

class THaCluster : public TObject {

public:
  THaCluster() : fCenter(kBig,kBig,kBig) {}
  virtual ~THaCluster() {}

  TVector3&        GetCenter()       { return fCenter; }
  virtual void     SetCenter( Double_t x, Double_t y, Double_t z )
  { fCenter.SetXYZ(x,y,z); }
  virtual void     SetCenter( const TVector3& vec3 )
  { fCenter = vec3; }
  Double_t         X() const { return fCenter.X(); }
  Double_t         Y() const { return fCenter.Y(); }
  Double_t         Z() const { return fCenter.Z(); }

  // TObject functions redefined
  virtual void     Clear( Option_t* opt="" );
  virtual void     Print( Option_t* opt="" ) const;

  static const Double_t kBig;

protected:

  TVector3         fCenter;          // Center coordinates of cluster

  ClassDef(THaCluster,0)             // Generic wire chamber cluster
};

//////////////////////////////////////////////////////////////////////////////

#endif
