//*-- Author :    Ole Hansen   29 March 2001

//////////////////////////////////////////////////////////////////////////
//
// THaTrack
//
// A track in the focal plane
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrack.h"
#include "THaPIDinfo.h"
//#include "THaVertex.h"
#include <iostream>

ClassImp(THaTrack)

//_____________________________________________________________________________
THaTrack::THaTrack( Double_t p, Double_t theta, Double_t phi, 
		    Double_t x, Double_t y, 
		    const THaSpectrometer* s, 
		    const TClonesArray* clusters,
		    THaPIDinfo* pid, 
		    THaVertex* vertex )
  : fP(p), fTheta(theta), fPhi(phi), fX(x), fY(y), fClusters(clusters),
    fPIDinfo(pid), fVertex(vertex), fSpectrometer(s)
{
  // Normal constructor with initialization

  CalcPxyz();
}

//_____________________________________________________________________________
//  THaTrack::THaTrack( const THaTrack& rhs ) : TNamed( rhs )
//  {
//    fNumber     = rhs.fNumber;
//    fFilename   = rhs.fFilename;
//    fFirstEvent = rhs.fFirstEvent;
//    fLastEvent  = rhs.fLastEvent;
//    fDate       = rhs.fDate;
//    fCodaFile   = new THaCodaFile;
//  }

//____________________________________________________________________________
//  THaTrack& THaTrack::operator=(const THaTrack& rhs)
//  {
//    // THaTrack assignment operator.

//    if (this != &rhs) {
//       TNamed::operator=(rhs);
//       fNumber     = rhs.fNumber;
//       fFilename   = rhs.fFilename;
//       fFirstEvent = rhs.fFirstEvent;
//       fLastEvent  = rhs.fLastEvent;
//       fDate       = rhs.fDate;
//       fCodaFile   = new THaCodaFile;
//    }
//    return *this;
//  }

//_____________________________________________________________________________
void THaTrack::Clear( const Option_t* opt )
{
  // Reset track quantities. 

  fP     = 0.0;
  fPx    = 0.0;
  fPy    = 0.0;
  fPz    = 0.0;
  fTheta = 0.0;
  fPhi   = 0.0;
  fX     = 0.0;
  fY     = 0.0;
  if( fPIDinfo ) fPIDinfo->Clear( opt );
  //  if( fVertex )  fVertex->Clear( opt );
}

//_____________________________________________________________________________
//  Int_t THaTrack::Compare( THaTrack* obj )
//  {
//    // Compare two THaTrack objects. Returns 0 when equal, 
//    // -1 when 'this' is smaller and +1 when bigger (like strcmp).

//     if (this == obj) return 0;
//     if      ( fNumber < obj->fNumber ) return -1;
//     else if ( fNumber > obj->fNumber ) return  1;
//     return 0;
//  }

//_____________________________________________________________________________
//  inline
//  void THaTrack::Copy( THaTrack& track )
//  {
//    track = *this;
//  }

//_____________________________________________________________________________
//  void THaTrack::FillBuffer( char*& buffer )
//  {
//    // Encode THaTrack into output buffer.

//    TNamed::FillBuffer( buffer );
//    fFilename.FillBuffer( buffer );
//  }

//_____________________________________________________________________________
void THaTrack::Print( Option_t* opt ) const
{
  // Print track parameters
  TObject::Print( opt );
  cout << "Momentum = " << fP << " GeV/c\n";
  cout << "Theta    = " << fTheta << " rad\n";
  cout << "Phi      = " << fPhi << " rad\n";
  cout << "x_fp     = " << fX   << " mm\n";
  cout << "y_fp     = " << fY   << " mm\n";

  if( fPIDinfo ) fPIDinfo->Print( opt );
  //  if( fVertex )  fVertex->Print( opt );
}



