#ifndef ROOT_THaVDCCluster
#define ROOT_THaVDCCluster

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCCluster                                                             //
//                                                                           //
// Class representing a cluster in the VDC                                   //
///////////////////////////////////////////////////////////////////////////////
#include "TObject.h"


#define MAX_SIZE 20  //Assume no more than 20 hits per cluster

class THaVDCHit;     
class THaVDCPlane;
class THaVDCUVTrack;
class THaTrack;

class THaVDCCluster : public TObject {

public:
  THaVDCCluster( THaVDCPlane* owner = NULL ); 
  virtual ~THaVDCCluster();

  virtual void AddHit(THaVDCHit * hit);
  virtual void EstTrackParameters();
  virtual void ConvertTimeToDist();
  virtual void FitTrack();
  virtual void Clear( const Option_t* opt="" );
    
  //Get and Set Functions
  Int_t GetSize () {return fSize;}
  THaVDCHit** GetHits() {return fHits;} // Get array of pointers
  THaVDCHit * GetHit(Int_t i) {return fHits[i]; }
  THaVDCPlane* GetPlane() {return fPlane;}
  Double_t GetSlope() {return fSlope;}
  Double_t GetSigmaSlope() {return fSigmaSlope;}
  Double_t GetIntercept() {return fInt;}
  Double_t GetSigmaIntercept() {return fSigmaInt;}
  THaVDCHit*  GetPivot() {return fPivot;}
  
  void SetPlane(THaVDCPlane * plane) {fPlane = plane;}
/*    void SetUVTrack(THaVDCUVTrack * uvtrack) {fUVTrack = uvtrack;} */
/*    void SetTrack(THaTrack * track) {fTrack = track;} */
  void SetIntercept(Double_t intercept) {fInt = intercept;}
  void SetSlope (Double_t slope) {fSlope = slope;}
  void SetPivot (THaVDCHit * piv){fPivot = piv; }

protected:
  Int_t fSize;                     //Size of cluster (no. of hits)
  THaVDCHit * fHits[MAX_SIZE];     //Ptr to hits associated with the cluster
  THaVDCPlane * fPlane;            //Plane the cluster belongs to
  //  THaVDCUVTrack * fUVTrack;        //UV Track the cluster belongs to
  //  THaTrack * fTrack;               //Track the cluster belongs to
  //Local Track Parameters 
  Double_t fSlope, fSigmaSlope;    //Slope and error in slope        
  Double_t fInt, fSigmaInt;        //Intercept and error
  THaVDCHit *  fPivot;             //Pivot - hit with smallest drift time 

  
  THaVDCCluster( const THaVDCCluster& ) {}
  THaVDCCluster& operator=( const THaVDCCluster& ) { return *this; }

  ClassDef(THaVDCCluster,0)             // VDCCluster class
};

//////////////////////////////////////////////////////////////////////////////

#endif
