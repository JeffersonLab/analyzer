#ifndef ROOT_THaVDCUVTrack
#define ROOT_THaVDCUVTrack

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVTrack                                                             //
//                                                                           //
// Class for UV Tracks                                                       //
///////////////////////////////////////////////////////////////////////////////
#include <TObject.h>
#include <TClonesArray.h>

class THaVDCCluster;
class THaVDCUVPlane;
class THaTrack;

class THaVDCUVTrack : public TObject {

private:

protected:
  THaVDCCluster * fUClust;       // Cluster in the U plane
  THaVDCCluster * fVClust;       // Cluster in the V plane
  THaVDCUVPlane * fUVPlane;      // UV plane that own's this track
  THaTrack * fTrack;             // Track this UV Track is associated with
  THaVDCUVTrack * fPartner;      // UV track associated with this one in 
                                 // the other UV plane
  // Track Info (Detector cs - X,Y in m; theta, phi in tan(angle) )
  Double_t fX;     // X position of track in U wire-plane 
  Double_t fY;     // Y position of track in U wire-plane 
  Double_t fTheta; // Angle between z-axis and projection of track into xz plane
  Double_t fPhi;   // Angle between z-axis and projection of track into yz plane
  
  THaVDCUVTrack( const THaVDCUVTrack& ) {}
  THaVDCUVTrack& operator=( const THaVDCUVTrack& ) { return *this; }

  
public:
  //Constructors
  THaVDCUVTrack( );

  //Destructor
  virtual ~THaVDCUVTrack();

  virtual THaVDCUVTrack * FindPartner (TClonesArray& trackList, Int_t length);

  // Get and Set Functions  
  THaVDCCluster * GetUCluster() {return fUClust;}
  THaVDCCluster * GetVCluster() {return fVClust;}
  THaVDCUVPlane * GetUVPlane()  {return fUVPlane;}
  THaVDCUVTrack * GetPartner()  {return fPartner;}
  Double_t GetX() {return fX;}
  Double_t GetY() {return fY;}
  Double_t GetTheta() {return fTheta;}
  Double_t GetPhi()   {return fPhi;} 

  void SetUCluster(THaVDCCluster * clust) {fUClust = clust;}
  void SetVCluster(THaVDCCluster * clust) {fVClust = clust;}
  void SetUVPlane(THaVDCUVPlane * plane)  {fUVPlane = plane;}
  void SetTrack(THaTrack * track);
  void SetPartner(THaVDCUVTrack * partner){fPartner = partner;}
  void SetX(Double_t x) { fX = x;}
  void SetY(Double_t y) { fY = y;}
  void SetTheta(Double_t theta) { fTheta = theta;}
  void SetPhi(Double_t phi)     { fPhi = phi;} 

  ClassDef(THaVDCUVTrack,0)             // VDCUVTrack class
};

////////////////////////////////////////////////////////////////////////////////

#endif
