#ifndef Podd_THaVar_h_
#define Podd_THaVar_h_

//////////////////////////////////////////////////////////////////////////
//
// THaVar
//
// Facade interface to access arbitrary data in memory via name
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "VarType.h"
#include "Variable.h"
#include <vector>

class THaArrayString;
class TMethodCall;

class THaVar : public TNamed {

public:
  static const Long64_t kInvalidInt;
  static const Double_t kInvalid;

  // Explicit instantiations for supported types are provided in implementation
  template <typename T>
  THaVar( const char* name, const char* descript, T& var, const Int_t* count=0 );

  THaVar( const char* name, const char* descript, const void* obj,
	  VarType type, Int_t offset, TMethodCall* method=0, const Int_t* count=0 );

  THaVar( const char* name, const char* descript, const void* obj,
	  VarType type, Int_t elem_size, Int_t offset, TMethodCall* method=0 );

  //TODO: copy, assignment
  virtual ~THaVar();

  Int_t        GetLen()                         const { return fImpl->GetLen(); }
  Int_t        GetNdim()                        const { return fImpl->GetNdim(); }
  const Int_t* GetDim()                         const { return fImpl->GetDim(); }

  VarType      GetType()                        const { return fImpl->GetType(); }
  size_t       GetTypeSize()                    const { return GetTypeSize(GetType()); }
  const char*  GetTypeName()                    const { return GetTypeName(GetType()); }

  std::vector<Double_t>     GetValues()         const { return fImpl->GetValues(); }
  Double_t     GetValue( Int_t i = 0 )          const { return fImpl->GetValue(i); }
  Long64_t     GetValueInt( Int_t i = 0 )       const { return fImpl->GetValueInt(i); }

  const void*  GetValuePointer()                const { return fImpl->GetValuePointer(); }
  const void*  GetDataPointer( Int_t i = 0 )    const { return fImpl->GetDataPointer(i); }
  size_t       GetData( void* buf )             const { return fImpl->GetData(buf); }
  size_t       GetData( void* buf, Int_t i )    const { return fImpl->GetData(buf,i); }

  Bool_t       HasSizeVar()                     const { return fImpl->HasSizeVar(); }

  Bool_t       HasSameSize( const THaVar& rhs ) const;
  Bool_t       HasSameSize( const THaVar* rhs ) const;
  Int_t        Index( const char* subscripts )  const;
  Int_t        Index( const THaArrayString& s ) const { return fImpl->Index(s); }
  Bool_t       IsArray()                        const { return fImpl->IsArray(); }
  Bool_t       IsBasic()                        const { return fImpl->IsBasic(); }
  Bool_t       IsContiguous()                   const { return fImpl->IsContiguous(); }
  Bool_t       IsFloat()                        const { return fImpl->IsFloat(); }
  Bool_t       IsPointerArray()                 const { return fImpl->IsPointerArray(); }
  Bool_t       IsStreamable()                   const { return fImpl->IsStreamable(); }
  Bool_t       IsTObject()                      const { return fImpl->IsTObject(); }
  Bool_t       IsVarArray()                     const { return fImpl->IsVarArray(); }
  Bool_t       IsVector()                       const { return fImpl->IsVector(); }

  // Overrides of TNamed methods
  virtual void         Print( Option_t* opt="FULL" )
						const { return fImpl->Print(opt); }
  virtual void         SetName( const char* name )    { fImpl->SetName(name); }
  virtual void         SetNameTitle( const char* name, const char* descript )
						      { fImpl->SetNameTitle(name,descript); }

  // Access to detailed information about types defined in VarType.h
  static void          ClearCache();
  static const char*   GetEnumName( VarType type );
  static const char*   GetTypeName( VarType type );
  static size_t        GetTypeSize( VarType type );

protected:
  Podd::Variable* fImpl;   //Pointer to implementation

  ClassDef(THaVar,0)   //Global symbolic variable
};

//_____________________________________________________________________________
inline
Bool_t THaVar::HasSameSize( const THaVar& rhs ) const
{
  return fImpl->HasSameSize( *rhs.fImpl );
}

//_____________________________________________________________________________
inline
Bool_t THaVar::HasSameSize( const THaVar* rhs ) const
{
  if( !rhs )
    return kFALSE;
  return fImpl->HasSameSize( *rhs->fImpl );
}

#endif
