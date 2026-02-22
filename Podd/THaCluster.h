#ifndef Podd_THaCluster_h_
#define Podd_THaCluster_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCluster                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"
#include "DataType.h" // for kBig

class THaCluster : public TObject {

public:
  THaCluster() : fCenter(kBig,kBig,kBig) {}

  TVector3&        GetCenter()       { return fCenter; }
  virtual void     SetCenter( Double_t x, Double_t y, Double_t z )
  { fCenter.SetXYZ(x,y,z); }
  virtual void     SetCenter( const TVector3& vec3 )
  { fCenter = vec3; }
  Double_t         X() const { return fCenter.X(); }
  Double_t         Y() const { return fCenter.Y(); }
  Double_t         Z() const { return fCenter.Z(); }

  // TObject functions redefined
  void             Clear( Option_t* opt="" ) override;
  void             Print( Option_t* opt="" ) const override;

protected:

  TVector3         fCenter;          // Center coordinates of cluster

  ClassDefOverride(THaCluster,0)     // Generic wire chamber cluster
};

//////////////////////////////////////////////////////////////////////////////

#endif
