//*-- Author :    Ole Hansen   26 March 2001

//////////////////////////////////////////////////////////////////////////
//
// THaRICHClusterList
//
//////////////////////////////////////////////////////////////////////////


#include "THaRICHClusterList.h"

#include <fstream>
#include <stdio.h>
#include <string.h>
#include <math.h>

ClassImp(THaRICHHit)
ClassImp(THaRICHCluster)

//_____________________________________________________________________________
Int_t THaRICHHit::Compare( const TObject* theOtherHit ) const
{
  if (fADC < static_cast<const THaRICHHit*>( theOtherHit )->fADC)
    return -1;
  if (fADC > static_cast<const THaRICHHit*>( theOtherHit )->fADC)
    return +1;
  else
    return 0;
}

//_____________________________________________________________________________
void THaRICHHit::Show(FILE * fout) 
{
  fprintf(fout," I, J ");
  fprintf(fout,"%4d,%4d",fI,fJ);
  fprintf(fout," ; X, Y ");
  fprintf(fout,"%4f,%4f",fX,fY);
  fprintf(fout,"; fADC = ");
  fprintf(fout,"%4d",fADC);
  fprintf(fout,"\n");
}

//=============================================================================
void THaRICHCluster::Clear( Option_t* opt )
{
  //Reset the cluster to an empty state.
  //Don't delete the hits, just clear the internal list.

  fHitList->Clear("nodelete");
  fMIPflag = kFALSE;
  fXcenter = 0.0;
  fYcenter = 0.0;
  fCharge  = 0.0;
  fAngle   = 0.0;
  fTrack   = 0;
  fMIP     = 0;
}  

//_____________________________________________________________________________
Float_t THaRICHCluster::Dist( const THaRICHCluster* c ) const
{
  // Calculate distance of this cluster to another cluster

  Float_t dx = GetXcenter() - c->GetXcenter();
  Float_t dy = GetYcenter() - c->GetYcenter();
  return sqrt( dx*dx + dy*dy );
}

//_____________________________________________________________________________
void THaRICHCluster::Insert( THaRICHHit* theHit )
{
  //Add a hit to the cluster

  fHitList->AddLast( theHit );
  fXcenter += theHit->GetADC()*theHit->GetX();
  fYcenter += theHit->GetADC()*theHit->GetY();
  fCharge  += theHit->GetADC();
}

//_____________________________________________________________________________
Int_t THaRICHCluster::Test( THaRICHHit* theHit, 
			     Float_t par1, Float_t par2, Float_t par3 )
{
  // Check if theHit is sufficiently close to any of the hits of the
  // cluster to be considered part of this cluster
  //
  // Parameters: par1
  //             par2
  //             par3
  //

  Float_t dx,dy,dist;
  TIter next( fHitList );

  while( THaRICHHit* pHit = static_cast<THaRICHHit*>( next() )) {
    dx   = theHit->GetX() - pHit->GetX();
    dy   = theHit->GetY() - pHit->GetY();
    dist = sqrt( dx*dx + dy*dy );
    if( dist>par1 && fabs(dx)<par2 && fabs(dy)<par3 )
      return 1;
  }
  return 0;
}

//_____________________________________________________________________________
void THaRICHCluster::Show( FILE* fout )
{
  //Print info about cluster statistics and all the hits in the cluster.

  fHitList->Print();
  return;
}

//_____________________________________________________________________________
void THaRICHCluster::ShowElements(FILE * fout) 
{
  fprintf(fout,"\n");
  fprintf(fout, "Cluster Number Elements ");
  fprintf(fout,"%4d", GetNHits());
  fprintf(fout,"\n");
  fprintf(fout," X, Y ");
  fprintf(fout,"%4f,%4f",fXcenter,fYcenter);
  fprintf(fout,"; Charge  = ");
  fprintf(fout,"%4f",fCharge);
  fprintf(fout,"\n");
  fprintf(fout,"Cluster elements: ");
  fprintf(fout,"\n");
  Show(fout);
}


//_____________________________________________________________________________
