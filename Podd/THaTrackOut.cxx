#include "THaTrackOut.h"

#include <TLorentzVector.h>
#include <TTree.h>
#include "THaOutput.h"
#include "THaTrackingModule.h"

using namespace std;

//_____________________________________________________________________________
THaTrackOut::THaTrackOut(const char* name, const char* description,
			 const char* src, Double_t pmass /* GeV */ ) :
  THaPhysicsModule(name,description), fM(pmass), fSrcName(src), fSrc(NULL)
{
  // Prepare four-vectors for storing the output tracks from "src",
  //  assuming is has mass "pmass".
  // it is stored in the TTree as name.p4
  fP4 = new TLorentzVector;
}

//_____________________________________________________________________________
THaTrackOut::~THaTrackOut()
{
  // clean-up.
  delete fP4;
}

//_____________________________________________________________________________
void THaTrackOut::Clear( Option_t* opt )
{  
  // Set vector to default (junk) values
  THaPhysicsModule::Clear(opt);
  fP4->SetXYZT(kBig,kBig,kBig,0); // also gives a large - Mag2()
}

//_____________________________________________________________________________
void THaTrackOut::SetMass ( Double_t m )
{
  // I will permit this to change on an event-by-event basis
  fM = m;
}

//_____________________________________________________________________________
void THaTrackOut::SetSpectrometer( const char* name )
{
  // can only update BEFORE the initialization takes place
  if ( !IsInit())
    fSrcName = name;
  else
    PrintInitError("SetSpectrometer()");
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaTrackOut::Init( const TDatime& run_time )
{
  // Find the pointer to the tracking module providing the track
  // and save the pointer
  fSrc = dynamic_cast<THaTrackingModule*>
    ( FindModule( fSrcName.Data(), "THaTrackingModule"));
  if ( !fSrc )
    return fStatus;

    // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;
  
  if( fM == 0.0 ) {
    Warning( Here("Init"), "Particle mass not defined. Assuming mass of 0.0");
  }
  return fStatus;
}

//_____________________________________________________________________________
Int_t THaTrackOut::InitOutput( THaOutput* output )
{
  // Use the tree to store output
  
  if (fOKOut) return 0; // already initialized.
  if (!output) {
    Error("InitOutput","Cannot get THaOutput object. Output initialization FAILED!");
    return -2;
  }
  TTree* tree = output->GetTree();
  if (!tree) {
    Error("InitOutput","Cannot get Tree! Output initialization FAILED!");
    return -3;
  }

  // create the branches
  if ( tree->Branch(Form("%s.p4.",GetName()),"TLorentzVector",&fP4,4000) ) {
    fOKOut = true;
  }

  if (fOKOut) return 0;
  return -1;
}

//_____________________________________________________________________________
Int_t THaTrackOut::Process( const THaEvData& )
{
  // Calculate the 4-vector for the golden track from fSrc
  
  if ( !IsOK() || !gHaRun ) return -1;
  
  THaTrackInfo *trkifo = fSrc->GetTrackInfo();
  if ( !trkifo || !trkifo->IsOK() ) return 1;

  fP4->SetVectM( trkifo->GetPvect(), fM );
  return 0;
}

ClassImp(THaTrackOut)
