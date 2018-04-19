//*-- Author :    Ole Hansen   13-Mar-03

//////////////////////////////////////////////////////////////////////////
//
// THaAvgVertex
//
// Calculate the average of the reaction points of the Golden Track from
// two seperate spectrometers.  Requires that the standard track-beam
// vertex information of each of the two Golden Tracks has been calculated, 
// for example using the THaReactionPoint module.
//
//////////////////////////////////////////////////////////////////////////

#include "THaAvgVertex.h"
#include "THaTrackingModule.h"
#include "THaTrackInfo.h"
#include "TMath.h"
#include "VarDef.h"

#include <iostream>

ClassImp(THaAvgVertex)

//_____________________________________________________________________________
THaAvgVertex::THaAvgVertex( const char* name, const char* description,
			    const char* spectro1, const char* spectro2 ) :
  THaPhysicsModule(name,description),
  fName1(spectro1), fName2(spectro2), fSpectro1(NULL), fSpectro2(NULL)
{
  // Normal constructor.

  Clear();
}

//_____________________________________________________________________________
THaAvgVertex::~THaAvgVertex()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaAvgVertex::Clear( Option_t* opt )
{
  // Clear all internal variables.

  THaPhysicsModule::Clear(opt);
  VertexClear();
  //  fZerror = 0.0;
}

//_____________________________________________________________________________
Int_t THaAvgVertex::DefineVariables( EMode mode )
{
  // Define/delete standard variables for a spectrometer (tracks etc.)
  // Can be overridden or extended by derived (actual) apparatuses

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  return DefineVarsFromList( THaVertexModule::GetRVarDef(), mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaAvgVertex::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  fSpectro1 = dynamic_cast<THaVertexModule*>
    ( FindModule( fName1.Data(), "THaVertexModule"));
  if( !fSpectro1 )
    return fStatus;

  fSpectro2 = dynamic_cast<THaVertexModule*>
    ( FindModule( fName2.Data(), "THaVertexModule"));

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaAvgVertex::Process( const THaEvData& )
{
  // Calculate the average vertex coordinates and uncertainties.

  static const int N = 2;  // consider two tracks

  THaVertexModule* t[N] = { fSpectro1, fSpectro2 }; 

  if( !IsOK() ) return -1;
  if( !t[0]->HasVertex() ) return 1;
  if( !t[1]->HasVertex() ) return 2;

  // Compute the weighted average of the vertex positions.
  // Use the uncertainty in each vertex z as the weight for each vertex.
  fVertex.SetXYZ( 0.0, 0.0, 0.0 ); 
  Double_t sigsum = 0.0;
  for( Int_t it=0; it<N; it++ ) {
    Double_t sigma = t[it]->GetVertexError()(2);
    Double_t sig2  = sigma*sigma;
    if( sig2 > 0.0 ) {
      sigsum += 1.0/sig2;
      for( Int_t i=0; i<3; i++ ) {
	Double_t x  = t[it]->GetVertex()(i);
	fVertex(i) += x/sig2;
      }
    }
  }
  if( sigsum > 0.0 ) {
    fVertex *= 1.0/sigsum;
    // requires beam info to get x/y uncertainties right
    //    fZerror  = 1.0/TMath::Sqrt(sigsum);
  }
  fVertexOK = kTRUE;

  return 0;
}
  
