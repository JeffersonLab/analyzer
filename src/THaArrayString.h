#ifndef ROOT_THaArrayString
#define ROOT_THaArrayString

//////////////////////////////////////////////////////////////////////////
//
// THaArrayString
//
//////////////////////////////////////////////////////////////////////////

#include "TString.h"

class THaArrayString {
  
public:
  enum EStatus { kOK, kBadsyntax, kIllegalchars, kToolarge,  kToolong, 
		 kNotinit };

  THaArrayString() : fNdim(0), fDim(NULL), fLen(-1), fStatus(kNotinit) {}
  THaArrayString( const char* string ) 
    : fName(string), fDim(NULL), fLen(-1)   { Parse(); }
  THaArrayString( const THaArrayString& );
  THaArrayString& operator=( const THaArrayString& );
  THaArrayString& operator=( const char* rhs )
  { Parse( rhs ); return *this; }
  virtual ~THaArrayString()           { delete [] fDim; }

  operator const char*() const { return fName.Data(); }

  const Int_t*    GetDim()  const { return fDim; }
  Int_t           GetLen()  const { return fLen; }
  const char*     GetName() const { return fName.Data(); }
  Byte_t          GetNdim() const { return fNdim; }
  ULong_t         Hash()    const { return fName.Hash(); }
  Bool_t          IsArray() const { return (fNdim > 0); }
  Bool_t          IsError() const { return (fStatus != kOK); }
  virtual Int_t   Parse( const char* string="" );
  virtual void    Print( Option_t* opt="" );
  EStatus         Status()  const { return fStatus; }

protected:

  TString  fName;            //Variable name
  Byte_t   fNdim;            //Number of array dimensions (0=scalar)
  Int_t*   fDim;             //Dimensions, if any (NULL for scalar)
  Int_t    fLen;             //Length of array (product of all dimensions)
  EStatus  fStatus;          //Status of Parse()

  ClassDef(THaArrayString,0) //Parser for variable names with support for arrays
};

#endif

