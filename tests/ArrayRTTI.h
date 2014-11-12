#ifndef Podd_ArrayRTTI
#define Podd_ArrayRTTI

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ArrayRTTI unit test                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"

class ArrayRTTI : public THaAnalysisObject {

public:
  ArrayRTTI( const char* name = "array_rtti",
	     const char* description = "Array RTTI unit test" );
  virtual ~ArrayRTTI();

  Int_t     Test() const;

protected:

  static const Int_t fgD1  = 5;
  static const Int_t fgD21 = 5;
  static const Int_t fgD22 = 7;
  static const Int_t fgDV  = 15;

  // Test data
  Float_t    fArray[fgD1];      // One-dimensional fixed
  Float_t    f2D[fgD21][fgD22]; // Two-dimensional fixed
  Int_t      fN;                // Number of elements in fVarArr
  Float_t*   fVarArr;           // [fN] variable-size

  virtual Int_t  DefineVariables( EMode mode );
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual void   MakePrefix() { THaAnalysisObject::MakePrefix(0); }

  ClassDef(ArrayRTTI,0)   // Example detector
};

////////////////////////////////////////////////////////////////////////////////

#endif
