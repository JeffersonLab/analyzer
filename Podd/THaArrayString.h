#ifndef Podd_THaArrayString_h_
#define Podd_THaArrayString_h_

//////////////////////////////////////////////////////////////////////////
//
// THaArrayString
//
//////////////////////////////////////////////////////////////////////////

#include "TString.h"
#include <cassert>

class THaArrayString {

public:
  enum EStatus { kOK, kBadsyntax, kIllegalchars, kToolarge,  kToolong,
		 kNotinit };

  THaArrayString() : fNdim(0), fDim(nullptr), fLen(-1), fStatus(kNotinit) {}
  THaArrayString( const char* string )   // intentionally not explicit NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
    : fName(string), fNdim(0), fDim(nullptr), fLen(-1), fStatus(kNotinit)
  { Parse(); }
  THaArrayString( const THaArrayString& );
  THaArrayString& operator=( const THaArrayString& );
  THaArrayString& operator=( const char* rhs ) { Parse( rhs ); return *this; }
  virtual ~THaArrayString() { if( fNdim>kMaxA ) delete [] fDim; }

  operator const char*()    const { return fName.Data(); } // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
  operator const TString&() const { return fName; }        // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
  Int_t operator[](Int_t i) const;
  bool  operator!()         const { return IsError(); }

  const Int_t*    GetDim()  const;
  Int_t           GetLen()  const { return fLen; }
  const char*     GetName() const { return fName.Data(); }
  Int_t           GetNdim() const { return fNdim; }
  ULong_t         Hash()    const { return fName.Hash(); }
  Bool_t          IsArray() const { return (fNdim > 0); }
  Bool_t          IsError() const { return (fStatus != kOK); }
  virtual Int_t   Parse( const char* string="" );
  virtual void    Print( Option_t* opt="" ) const;
  EStatus         Status()  const { return fStatus; }

protected:
  static inline constexpr Int_t kMaxA = sizeof(void*)/sizeof(Int_t);

  TString  fName;            //Variable name
  Int_t    fNdim;            //Number of array dimensions (0=scalar)
  union {
    Int_t* fDim;             //Dimensions if fNdim > kMaxA
    Int_t  fDimA[kMaxA];     //Dimensions if fNdim <= kMaxA
  };
  Int_t    fLen;             //Length of array (product of all dimensions)
  EStatus  fStatus;          //Status of Parse()

  ClassDef(THaArrayString,0) //Parser for variable names with support for arrays
};

//__________________ inlines __________________________________________________
inline
Int_t THaArrayString::operator[](Int_t i) const
{
  // Return i-th array dimension
  assert( i >= 0 && i < fNdim );
  if( fNdim>kMaxA ) {
    assert( fDim );
    return fDim[i];
  } else
    return fDimA[i];
}

//_____________________________________________________________________________
inline
const Int_t* THaArrayString::GetDim() const
{
  if( fNdim == 0 )
    return 0;
  else if( fNdim>kMaxA )
    return fDim;
  else
    return fDimA;
}

#endif
