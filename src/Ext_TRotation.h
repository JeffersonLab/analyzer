// The following is a workaround for older versions of ROOT which
// do not provide the function TRotation::SetZAxis

#include "TRotation.h"

#if ROOT_VERSION_CODE < ROOT_VERSION(3,5,4) && !defined(Ext_TRotation)
#define ROOT_Ext_TRotation

class Ext_TRotation : public TRotation
{
public:
  Ext_TRotation& SetZAxis(const TVector3& axis, const TVector3& zxPlane);
  void MakeBasis(TVector3& xAxis, TVector3& yAxis, TVector3& zAxis) const;
};

#define TOLERANCE (1.0E-6)

void Ext_TRotation::MakeBasis(TVector3& xAxis,
			      TVector3& yAxis,
			      TVector3& zAxis) const
{
  // Make the zAxis into a unit variable. 
  Double_t zmag = zAxis.Mag();
  if (zmag<TOLERANCE) {
      Warning("MakeBasis(X,Y,Z)","non-zero Z Axis is required");
  }
  zAxis *= (1.0/zmag);

  Double_t xmag = xAxis.Mag();
  if (xmag<TOLERANCE*zmag) {
      xAxis = zAxis.Orthogonal();
      xmag = 1.0;
  }

  // Find the yAxis
  yAxis = zAxis.Cross(xAxis)*(1.0/xmag);
  Double_t ymag = yAxis.Mag();
  if (ymag<TOLERANCE*zmag) {
      yAxis = zAxis.Orthogonal();
  }

  else {
      yAxis *= (1.0/ymag);
  }

  xAxis = yAxis.Cross(zAxis);
}

Ext_TRotation& Ext_TRotation::SetZAxis(const TVector3& axis, 
				       const TVector3& zxPlane)
{
  TVector3 xAxis(zxPlane);
  TVector3 yAxis;
  TVector3 zAxis(axis);
  MakeBasis(xAxis,yAxis,zAxis);
  fxx = xAxis.X();  fyx = xAxis.Y();  fzx = xAxis.Z();
  fxy = yAxis.X();  fyy = yAxis.Y();  fzy = yAxis.Z();
  fxz = zAxis.X();  fyz = zAxis.Y();  fzz = zAxis.Z();
  return *this;
}

#endif   
