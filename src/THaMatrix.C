//*-- Author :    Ole Hansen   22-Jun-2001

//////////////////////////////////////////////////////////////////////////
//
// THaMatrix
//
// A TMatrix with support for initializing rows and columns with vectors.
//
//////////////////////////////////////////////////////////////////////////

#include "THaMatrix.h"

ClassImp(THaMatrix)

//_____________________________________________________________________________
THaMatrix::THaMatrix( const TVector2& col0, const TVector2& col1 )
{
  // Construct a 2x2 matrix from two TVector2s, each vector defining
  // a column of the matrix.

  Allocate(2,2);
  *(fElements)   = col0.X();
  *(fElements+1) = col0.Y();
  *(fElements+2) = col1.X();
  *(fElements+3) = col1.Y();
}

//_____________________________________________________________________________
THaMatrix::THaMatrix( const TVector3& col0, const TVector3& col1,
		      const TVector3& col2 )
{
  // Construct a 3x3 matrix from three TVector3s, each vector defining
  // a column of the matrix.

  Allocate(3,3);
  *(fElements)   = col0.X();
  *(fElements+1) = col0.Y();
  *(fElements+2) = col0.Z();
  *(fElements+3) = col1.X();
  *(fElements+4) = col1.Y();
  *(fElements+5) = col1.Z();
  *(fElements+6) = col2.X();
  *(fElements+7) = col2.Y();
  *(fElements+8) = col2.Z();
}

//_____________________________________________________________________________
void THaMatrix::SetColumn( const TVector& v, Int_t col )
{
  // Set column of matrix to contents of vector

  if( v.GetNrows() != fNrows ) {
    Error( "SetColumn", "vector is not compatible with matrix");
    return;
  }
  if( col < 0 || col >= fNcols ) {
    Error( "SetColumn", "column number out of range");
    return;
  }
  Real_t* cp = fIndex[col];
  for( Int_t i = 0; i < fNrows; i++, cp++ )
    *cp = v(i);
}

//_____________________________________________________________________________
void THaMatrix::SetColumn( const TVector2& v, Int_t col )
{
  // Set column of matrix to contents of vector

  if( fNrows != 2 ) {
    Error( "SetColumn", "vector is not compatible with matrix");
    return;
  }
  if( col < 0 || col >= fNcols ) {
    Error( "SetColumn", "column number out of range");
    return;
  }
  Real_t* cp = fIndex[col];
  *(cp)   = v.X();
  *(cp+1) = v.Y();
}
  
//_____________________________________________________________________________
void THaMatrix::SetColumn( const TVector3& v, Int_t col )
{
  // Set column of matrix to contents of vector

  if( fNrows != 3 ) {
    Error( "SetColumn", "vector is not compatible with matrix");
    return;
  }
  if( col < 0 || col >= fNcols ) {
    Error( "SetColumn", "column number out of range");
    return;
  }
  Real_t* cp = fIndex[col];
  *(cp)   = v.X();
  *(cp+1) = v.Y();
  *(cp+2) = v.Z();
}

//_____________________________________________________________________________
void THaMatrix::SetRow( const TVector& v, Int_t row )
{
  // Set column of matrix to contents of vector

  if( v.GetNrows() != fNcols ) {
    Error( "SetRow", "vector is not compatible with matrix");
    return;
  }
  if( row < 0 || row >= fNrows ) {
    Error( "SetRow", "row number out of range");
    return;
  }
  Real_t* cp = fElements+row; 
  for( Int_t i = 0; i < fNcols; i++, cp += fNrows )
    *cp = v(i);
}

//_____________________________________________________________________________
void THaMatrix::SetRow( const TVector2& v, Int_t row )
{
  // Set column of matrix to contents of vector

  if( fNcols != 2 ) {
    Error( "SetRow", "vector is not compatible with matrix");
    return;
  }
  if( row < 0 || row >= fNrows ) {
    Error( "SetRow", "row number out of range");
    return;
  }
  Real_t* cp = fElements+row;
  *(cp)        = v.X();
  *(cp+fNrows) = v.Y();
}
  
//_____________________________________________________________________________
void THaMatrix::SetRow( const TVector3& v, Int_t row )
{
  // Set column of matrix to contents of vector

  if( fNcols != 3 ) {
    Error( "SetRow", "vector is not compatible with matrix");
    return;
  }
  if( row < 0 || row >= fNrows ) {
    Error( "SetRow", "row number out of range");
    return;
  }
  Real_t* cp = fElements+row;
  *(cp)          = v.X();
  *(cp+fNrows)   = v.Y();
  *(cp+2*fNrows) = v.Z();
}

