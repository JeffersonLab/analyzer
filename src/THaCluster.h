#ifndef ROOT_THaCluster
#define ROOT_THaCluster

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCluster                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TVector3.h"

class THaCluster : public TObject {

public:
  THaCluster() {}
  THaCluster( const THaCluster& rhs ) : TObject(rhs), fCenter(rhs.fCenter) {}
  THaCluster& operator=( const THaCluster& );
  virtual ~THaCluster() {}

  TVector3&        GetCenter()       { return fCenter; }
  virtual void     SetCenter( Double_t x, Double_t y, Double_t z )
  { fCenter.SetXYZ(x,y,z); }
  virtual void     SetCenter( const TVector3& vec3 )
  { fCenter = vec3; }

  // TObject functions redefined
  virtual void     Clear( Option_t* opt="" );
  virtual void     Print( Option_t* opt="" ) const;

protected:

  TVector3         fCenter;          // Center coordinates of cluster

  ClassDef(THaCluster,0)             // Generic wire chamber cluster
};

//////////////////////////////////////////////////////////////////////////////

#endif
