#ifndef ROOT_THaMatrix
#define ROOT_THaMatrix

//////////////////////////////////////////////////////////////////////////
//
// THaMatrix
//
// A TMatrix with support for initializing rows and columns with vectors.
//
//////////////////////////////////////////////////////////////////////////

#include "TMatrix.h"
#include "TVector2.h"
#include "TVector3.h"

class THaMatrix : public TMatrix {
  
public:
  THaMatrix() {}
  THaMatrix( Int_t nrows, Int_t ncols )
    : TMatrix( nrows, ncols ) {}
  THaMatrix( Int_t row_lwb, Int_t row_upb, Int_t col_lwb, Int_t col_upb )
    : TMatrix( row_lwb, row_upb, col_lwb, col_upb ) {}
  THaMatrix( const TMatrix& another )
    : TMatrix( another ) {}
  THaMatrix( TMatrix::EMatrixCreatorsOp1 op, const TMatrix& prototype)
    : TMatrix( op, prototype) {}
  THaMatrix( const TMatrix& a, TMatrix::EMatrixCreatorsOp2 op, 
	     const TMatrix& b)
    : TMatrix( a, op, b) {}
#if ROOT_VERSION_CODE >= 262144 // 4.00/00
  THaMatrix( const TMatrixFLazy& lazy_constructor) 
    : TMatrix( lazy_constructor) {}
#else
  THaMatrix( const TLazyMatrix& lazy_constructor) 
    : TMatrix( lazy_constructor) {}
#endif
  THaMatrix( const TVector2& col0, const TVector2& col1 );
  THaMatrix( const TVector3& col0, const TVector3& col1, 
	     const TVector3& col2 );
  virtual ~THaMatrix() {}
  
  void SetColumn( const TVector& v,  Int_t col );
  void SetColumn( const TVector2& v, Int_t col );
  void SetColumn( const TVector3& v, Int_t col );
  void SetRow( const TVector& v,   Int_t row );
  void SetRow( const TVector2& v,  Int_t row );
  void SetRow( const TVector3& v,  Int_t row );

  ClassDef(THaMatrix,1)   // Matrix class
};



#endif

