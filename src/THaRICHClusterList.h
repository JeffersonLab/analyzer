#ifndef ROOT_THaRICHClusterList
#define ROOT_THaRICHClusterList

//****************************************************************
// The header file for the "ClusterRICH.C"
//****************************************************************

#include "TObject.h"
#include "TList.h"
class THaTrack;

#include <iostream>
#include <stdio.h>

// Data class contains the data of a single pad (of a cluster).
// (i,j) coordinate of a pad; (x,y) coordinated of a pad; ADC value.
// Above all it contains a module to compare adc values. This allows 
// the ordering according the adc values itself of the components of a cluster.

class THaRICHHit : public TObject {

public:
  THaRICHHit() {}
  THaRICHHit( Int_t number, Int_t ADC, Int_t i, Int_t j, 
	       Float_t x, Float_t y )
    : fNumber(number), fADC(ADC), fI(i), fJ(j), fX(x), fY(y), fFlag(0) {}
  virtual ~THaRICHHit() {}

  void       Show(FILE * fout);

  Int_t      GetNumber()   const {return fNumber;}
  Float_t    GetX()        const {return fX;}
  Float_t    GetY()        const {return fY;}
  Int_t      GetI()        const {return fI;}
  Int_t      GetJ()        const {return fJ;}
  Int_t      GetADC()      const {return fADC;}
  Int_t      GetFlag()     const {return fFlag;} 
  void       SetNumber( Int_t number ) {fNumber = number;}
  void       SetX( Float_t x )         {fX = x;}
  void       SetY( Float_t y )         {fY = y;}
  void       SetI( Int_t i )           {fI = i;}
  void       SetJ( Int_t j )           {fJ = j;}
  void       SetADC( Int_t ADC )       {fADC = ADC;}
  void       SetFlag( Int_t Flag )     {fFlag = Flag;}

  virtual Int_t   Compare( const TObject* ) const;
  virtual Bool_t  IsSortable() const { return kTRUE; }

private:
  Int_t     fNumber;
  Int_t     fADC;
  Int_t     fI;
  Int_t     fJ;
  Float_t   fX;
  Float_t   fY;
  Int_t     fFlag;

  ClassDef(THaRICHHit,0)   //A hit in the RICH
};


// --------------------------------------------------------------

// ClusterElement: class of elements that make up a cluster. They are orderd
// according to the ADC value read in the corresponding Pad (from the highest
// value to the smallest).
// ClusterCompon is the general (ADT) class from which all the other classes
// are derived.
// HeadClusterElement and TailClusterElement are just null elements, 
// that just contain the pointer to the first and the last ClusterElement 
// respectively.
// InternalClusterElement contain a pointer to the next element in the list 
// of the cluster components. It contains also a pointer to the Class Hit
// that contains the information (ADC (X,Y) etc. relative).
// The definition of the classes above allows:
// 1) to avoid creation of arrays of predefined sizes of Hit. 
//         Only the elements  needed will be created to make up the cluster.
// 2) To just accomplish further algorithm  of ordering cluster element 
//  just changing module Hit::Compare above.


// A cluster of hits in the RICH.
// Calculates charge-weighted center position of all hits.
// Hits belonging to the cluster are kept in a TList.
// Hits must be TObjects.
// If you want a sorted list, just replace TList with TSortedList

class THaRICHCluster : public TObject {

public:
  THaRICHCluster() : 
    fMIPflag(kFALSE), fXcenter(0.0), fYcenter(0.0), fCharge(0.0), 
    fAngle(0.0), fMIP(0), fTrack(0)
    { fHitList = new TList; }
  ~THaRICHCluster() { delete fHitList; }

  void       Clear( Option_t* opt="" );
  Float_t    Dist( const THaRICHCluster* c ) const;
  void       Insert( THaRICHHit* theHit );
  Bool_t     IsMIP()      const              { return fMIPflag; }
  Int_t      GetNHits()   const              { return fHitList->GetSize(); }
  Float_t    GetXcenter() const              { return fXcenter/fCharge; }
  Float_t    GetYcenter() const              { return fYcenter/fCharge; }
  Float_t    GetCharge()  const              { return fCharge; }
  Float_t    GetAngle()   const              { return fAngle; }
  THaTrack*  GetTrack()   const              { return fTrack; }
  void       MakeMIP( Bool_t flag = kTRUE );
  void       SetMIP( THaRICHCluster* mip )  { fMIP = mip; }
  void       SetAngle( Float_t angle )       { fAngle = angle; }
  void       SetTrack( THaTrack* track )     { fTrack = track; }
  void       Show( FILE* fout );
  void       ShowElements( FILE* fout );
  Int_t      Test( THaRICHHit* theHit, 
		   Float_t par1, 
		   Float_t par2, 
		   Float_t par3 );

private:
  TList*     fHitList;   //List of hits belonging to this cluster
  Bool_t     fMIPflag;   //True if this cluster is a MIP
  Float_t    fXcenter;   //Sum of x*adc of all hits in the list
  Float_t    fYcenter;   //Sum of y*adc of all hits in the list
  Float_t    fCharge;    //Sum of adc values of all hits
  Float_t    fAngle;     //Calculated angle wrt particle ray (not used yet)
  THaRICHCluster* fMIP; //Pointer to MIP cluster belonging to this cluster
  THaTrack*  fTrack;     //Track associated with this cluster (only for MIPs)

  ClassDef(THaRICHCluster,0)  //A cluster of hits in the RICH
};

// ---------------- inlines -------------------------------------

inline
void THaRICHCluster::MakeMIP( Bool_t flag )
{
  fMIPflag = flag;
  if( flag )
    fMIP = this;
  else
    fMIP = NULL;
}

#endif

