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
#include "THaTrackInfo.h"
#include "THaTrack.h"
#include "TMath.h"
#include "VarDef.h"

#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaTwoarmVertex::THaTwoarmVertex( const char* name, const char* description,
				  const char* spectro1, const char* spectro2 ) :
  THaPhysicsModule(name,description),
  fName1(spectro1), fName2(spectro2), fSpectro1(NULL), fSpectro2(NULL)
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

  THaPhysicsModule::Clear(opt);
  VertexClear();
}

//_____________________________________________________________________________
Int_t THaTwoarmVertex::DefineVariables( EMode mode )
{
  // Define/delete standard variables for a spectrometer (tracks etc.)
  // Can be overridden or extended by derived (actual) apparatuses

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  return DefineVarsFromList( THaVertexModule::GetRVarDef(), mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaTwoarmVertex::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  fSpectro1 = dynamic_cast<THaTrackingModule*>
    ( FindModule( fName1.Data(), "THaTrackingModule"));
  if( !fSpectro1 )
    return fStatus;

  fSpectro2 = dynamic_cast<THaTrackingModule*>
    ( FindModule( fName2.Data(), "THaTrackingModule"));

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaTwoarmVertex::Process( const THaEvData& )
{
  // Calculate the intersection point.

  if( !IsOK() ) return -1;

  THaTrackInfo* t1 = fSpectro1->GetTrackInfo();
  if( !t1 || !t1->IsOK()) return 1;
  THaTrackInfo* t2 = fSpectro2->GetTrackInfo();
  if( !t2 || !t2->IsOK()) return 2;
  THaSpectrometer* s1 = t1->GetSpectrometer();
  THaSpectrometer* s2 = t2->GetSpectrometer();
  if( !s1 || !s2 ) return 4;

  static const TVector3 yax( 0.0, 1.0, 0.0 );

  TVector3 p1( 0.0, t1->GetY(), 0.0 );
  TVector3 p2( 0.0, t2->GetY(), 0.0 );
  p1 *= s1->GetToLabRot();
  p1 += s1->GetPointingOffset();
  p2 *= s2->GetToLabRot();
  p2 += s2->GetPointingOffset();
  Double_t t;
  if( !IntersectPlaneWithRay( t1->GetPvect(), yax, p1, 
			      p2, t2->GetPvect(), t, fVertex ))
    return 3; // Oops, track planes are parallel?
  fVertexOK = kTRUE;

#ifdef WITH_DEBUG
  if( fDebug<2 )
#endif
    fVertex.SetY( 0.0 );

#ifdef WITH_DEBUG
  //DEBUG code
  else {
    cout << "============Two-arm vertex:===========" << endl;
    cout << "--o1\n"; p1.Dump(); 
    cout << "--o2\n"; p2.Dump(); 
    cout << "--p1\n"; (t1->GetPvect()).Dump(); 
    cout << "--p2\n"; (t2->GetPvect()).Dump(); 
    THaTrack* tr1 = fSpectro1->GetTrack();
    THaTrack* tr2 = fSpectro2->GetTrack();
    if( tr1->HasVertex() ){
      cout << "--v1\n"; (tr1->GetVertex()).Dump(); }
    if( tr2->HasVertex() ){
      cout << "--v2\n"; (tr2->GetVertex()).Dump(); }
    cout << "--twoarm\n";
    fVertex.Dump();

    // consistency check
    TVector3 v2;
    if(!IntersectPlaneWithRay( t2->GetPvect(), yax, p2, 
			       p1, t1->GetPvect(), t, v2 ))
      return 3;
    cout << "--twoarm_alt\n";
    v2.Dump();
  }
#endif
  fDataValid = true;
  return 0;
}
  
///////////////////////////////////////////////////////////////////////////////
ClassImp(THaTwoarmVertex)
