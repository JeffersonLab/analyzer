
#ifndef ROOT_THaVarList
#define ROOT_THaVarList

//////////////////////////////////////////////////////////////////////////
//
// THaVarList
//
//////////////////////////////////////////////////////////////////////////

#include "THashList.h"
#include "THaVar.h"
#include "VarDef.h"
#include <vector>

class THaVarList : public THashList {
  
public:
  THaVarList();
  virtual ~THaVarList();

  // Define() with reference to variable
  THaVar*  Define( const char* name, const char* descript, 
		   const Double_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kDouble, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Float_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kFloat, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Long_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kLong, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const ULong_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kULong, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Int_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kInt, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const UInt_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kUInt, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Short_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kShort, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const UShort_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kUShort, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Char_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kChar, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Byte_t& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kByte, count ); }

  THaVar*  Define( const char* name, const char* descript, 
  		   const std::vector<Int_t>& var )
    { return DefineByType( name, descript, &var, kIntV, 0 ); }
  THaVar*  Define( const char* name, const char* descript, 
  		   const std::vector<UInt_t>& var )
    { return DefineByType( name, descript, &var, kUIntV, 0 ); }
  THaVar*  Define( const char* name, const char* descript, 
  		   const std::vector<Float_t>& var )
    { return DefineByType( name, descript, &var, kFloatV, 0 ); }
  THaVar*  Define( const char* name, const char* descript, 
  		   const std::vector<Double_t>& var )
    { return DefineByType( name, descript, &var, kDoubleV, 0 ); }

  THaVar*  Define( const char* name, const Double_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Float_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Long_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const ULong_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Int_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UInt_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Short_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UShort_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Char_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Byte_t& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }

  THaVar*  Define( const char* name, const std::vector<Int_t>& var )
    { return Define( name, name, var ); }
  THaVar*  Define( const char* name, const std::vector<UInt_t>& var )
    { return Define( name, name, var ); }
  THaVar*  Define( const char* name, const std::vector<Float_t>& var )
    { return Define( name, name, var ); }
  THaVar*  Define( const char* name, const std::vector<Double_t>& var )
    { return Define( name, name, var ); }

  // Define() with pointer to variable
  THaVar*  Define( const char* name, const char* descript, 
		   const Double_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kDoubleP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Float_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kFloatP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Long_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kLongP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const ULong_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kULongP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Int_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kIntP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const UInt_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kUIntP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Short_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kShortP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const UShort_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kUShortP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Char_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kCharP, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Byte_t*& var, const Int_t* count=0 )
    { return DefineByType( name, descript, &var, kByteP, count ); }

  THaVar*  Define( const char* name, const Double_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Float_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Long_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const ULong_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Int_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UInt_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Short_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UShort_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }
  //Avoid ambiguity - cannot specify variable length char array like this
  //Must use the form with description
  THaVar*  Define( const char* name, const Char_t*& var )
    { return Define( name, name, var ); }
  THaVar*  Define( const char* name, const Byte_t*& var,
		   const Int_t* count=0 )
    { return Define( name, name, var, count ); }

  virtual THaVar*  DefineByType( const char* name, const char* desc,
				 const void* loc, VarType type,
				 const Int_t* count,
				 const char* errloc = "DefineByType" );
  virtual THaVar*  DefineByRTTI( const TString& name, const TString& desc,
				 const TString& def, const void* const obj,
				 TClass* const cl,
				 const char* errloc = "DefineByRTTI" );
  virtual Int_t    DefineVariables( const VarDef* list, 
				    const char* prefix="",
				    const char* caller="" );
  virtual Int_t    DefineVariables( const RVarDef* list, 
				    const TObject* obj,
				    const char* prefix="",
				    const char* caller="",
				    const char* var_prefix="" );
  virtual THaVar*  Find( const char* name ) const;
  virtual void     PrintFull(Option_t *opt="") const;
  virtual Int_t    RemoveName( const char* name );
  virtual Int_t    RemoveRegexp( const char* expr, Bool_t wildcard = kTRUE );

protected:

  ClassDef(THaVarList,2)   //List of analyzer global variables
};

#endif

