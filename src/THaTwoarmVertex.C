//*-- Author :    Ole Hansen   13-Mar-03

//////////////////////////////////////////////////////////////////////////
//
// THaTwoarmVertex
//
// Calculate the intersection of the Golden Track from two spectrometers.
// The resultsing vertex coordinates are in global variables x,y,z.
//
// This module assumes that the spectrometers only reconstruct the 
// target position transverse to the bend plane (y_tg) with relatively 
// high precision. Each spectrometer therefore reconstructs a 
// "track plane" spanned by the particle's momentum vector and the
// TRANSPORT x-vector, whose origin is the y-target point.
//
// The intersection of the two Golden Tracks is thus actually a line
// with undefined y_lab coordinate. This module always sets y to zero.
// The actual y_lab position can be derived from the beam position and
// is calculated in the THaPhysVertex and THaPhysAvgVertex modules.
//
//////////////////////////////////////////////////////////////////////////

#include "THaTwoarmVertex.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaMatrix.h"
#include "TMath.h"
#include "VarDef.h"

#include <iostream>

ClassImp(THaTwoarmVertex)

//_____________________________________________________________________________
THaTwoarmVertex::THaTwoarmVertex( const char* name, const char* description,
				  const char* spectro1, const char* spectro2 ) :
  THaVertexModule(name,description),
  fName1(spectro1), fSpectro1(NULL), fName2(spectro2), fSpectro2(NULL)
{
  // Normal constructor.

  Clear();
}

//_____________________________________________________________________________
THaTwoarmVertex::~THaTwoarmVertex()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaTwoarmVertex::Clear( Option_t* opt )
{
  // Clear all internal variables.

  fVertex.SetXYZ(0.0,0.0,0.0); 
}

//_____________________________________________________________________________
Int_t THaTwoarmVertex::DefineVariables( EMode mode )
{
  // Define/delete standard variables for a spectrometer (tracks etc.)
  // Can be overridden or extended by derived (actual) apparatuses

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "x",  "two-arm vertex x-position", "fVertex.fX" },
    { "y",  "two-arm vertex y-position", "fVertex.fY" },
    { "z",  "two-arm vertex z-position", "fVertex.fZ" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaTwoarmVertex::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaVertexModule::Init( run_time ) != kOK )
    return fStatus;

  fSpectro1 = static_cast<THaSpectrometer*>
    ( FindModule( fName1.Data(), "THaSpectrometer"));
  if( !fSpectro1 )
    return fStatus;

  fSpectro2 = static_cast<THaSpectrometer*>
    ( FindModule( fName2.Data(), "THaSpectrometer"));

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaTwoarmVertex::Process()
{
  // Calculate the intersection point.

  if( !IsOK() ) return -1;

  THaTrack* t1 = fSpectro1->GetGoldenTrack();
  if( !t1 || !t1->HasTarget()) return 1;
  THaTrack* t2 = fSpectro2->GetGoldenTrack();
  if( !t2 || !t2->HasTarget()) return 2;

  static const TVector3 yax( 0.0, 1.0, 0.0 );

  TVector3 p1( 0.0, t1->GetTY(), 0.0 );
  TVector3 p2( 0.0, t2->GetTY(), 0.0 );
  p1 *= fSpectro1->GetToLabRot();
  p1 += fSpectro1->GetPointingOffset();
  p2 *= fSpectro2->GetToLabRot();
  p2 += fSpectro2->GetPointingOffset();
  THaMatrix denom( t1->GetPvect(), yax, -t2->GetPvect() );
  Double_t det = denom.Determinant();
  // Oops, track planes are parallel?
  if( TMath::Abs(det) < 1e-5 ) 
    return 3;
  THaMatrix nom( t1->GetPvect(), yax, p2-p1 );
  Double_t t = nom.Determinant() / det;
  fVertex = p2 + t*t2->GetPvect();
  if( fDebug<2 )
    fVertex.SetY( 0.0 );

  //DEBUG code
  else {
    cout << "============Two-arm vertex:===========" << endl;
    cout << "--o1\n"; p1.Dump(); 
    cout << "--o2\n"; p2.Dump(); 
    cout << "--p1\n"; (t1->GetPvect()).Dump(); 
    cout << "--p2\n"; (t2->GetPvect()).Dump(); 
    if( t1->HasVertex() ){
      cout << "--v1\n"; (t1->GetVertex()).Dump(); }
    if( t2->HasVertex() ){
      cout << "--v2\n"; (t2->GetVertex()).Dump(); }
    cout << "--twoarm\n";
    fVertex.Dump();

    // consistency check
    THaMatrix d2( t2->GetPvect(), yax, -t1->GetPvect() );
    det = d2.Determinant();
    if( TMath::Abs(det) < 1e-5 ) 
      return 3;
    THaMatrix n2( t2->GetPvect(), yax, p1-p2 );
    t = n2.Determinant() / det;
    TVector3 v2 = p1 + t*t1->GetPvect();
    cout << "--twoarm_alt\n";
    v2.Dump();
  }
  return 0;
}
  
