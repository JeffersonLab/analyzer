
#ifndef ROOT_THaVarList
#define ROOT_THaVarList

//////////////////////////////////////////////////////////////////////////
//
// THaVarList
//
//////////////////////////////////////////////////////////////////////////

#include "TList.h"
#include "THaVar.h"
#include "VarDef.h"

class THaVarList : public TList {
  
public:
  THaVarList() : TList() {}
  virtual ~THaVarList() { Clear(); }

  virtual THaVar*  Define( const char* name, const char* descript, 
			   const Double_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const Float_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const Long_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const ULong_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const Int_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const UInt_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const Short_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const UShort_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const Char_t* var, const Int_t* count=NULL );
  virtual THaVar*  Define( const char* name, const char* descript, 
			   const Byte_t* var, const Int_t* count=NULL );

  THaVar*  Define( const char* name, const Double_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Float_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Long_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const ULong_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Int_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UInt_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Short_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UShort_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  //Avoid ambiguity - cannot specify variable length char array like this
  THaVar*  Define( const char* name, const Char_t* var )
    { return Define( name, name, var ); }
  THaVar*  Define( const char* name, const Byte_t* var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }

  THaVar*  Define( const char* name, const char* descript, 
		   const Double_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Float_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Long_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const ULong_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Int_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const UInt_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Short_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const UShort_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Char_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const char* descript, 
		   const Byte_t& var, const Int_t* count=NULL )
    { return Define( name, descript, &var, count ); }
  THaVar*  Define( const char* name, const Double_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Float_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Long_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const ULong_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Int_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UInt_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Short_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const UShort_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Char_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }
  THaVar*  Define( const char* name, const Byte_t& var,
		   const Int_t* count=NULL )
    { return Define( name, name, var, count ); }

  virtual void     Clear( Option_t* opt="" );
  virtual Int_t    DefineVariables( const VarDef* list, 
				    const char* prefix="",
				    const char* caller="" );
  virtual THaVar*  Find( const char* name ) const;
  virtual void     PrintFull(Option_t *opt="") const;
  virtual Int_t    RemoveName( const char* name );
  virtual Int_t    RemoveRegexp( const char* expr, Bool_t wildcard = kTRUE );

protected:

  void             ExistsWarning( const char* errloc, const char* name );

  static const char* const kHere;

  ClassDef(THaVarList,1)   //List of analyzer global variables
};

#endif

