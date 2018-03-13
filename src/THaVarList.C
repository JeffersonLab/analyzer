//*-- Author :    Ole Hansen  27/04/00

//////////////////////////////////////////////////////////////////////////
//
//  THaVarList
//
//  A specialized THashList containing global variables of the
//  Hall A analyzer.
//
//  Variables can be added to the list as follows:
//  (assumes THaVarList* gHaVars = new THaVarList;)
//
//     Double_t x;
//     gHaVars->Define("x","x variable",x);
//
//  or for short
//      gHaVars->Define("x",x);
//
//  Likewise for arrays:
//
//      Double_t a[10];
//      gHaVars->Define("a[10]","array of doubles",a);
//
//  or
//      gHaVars->Define("a[10]",a);
//
//  (Note: Array definitions register only the name before the bracket
//  as the variable name; "a" in the example above.  Hence
//
//      gHaVars->Define("a",x)
//  and
//      gHaVars->Define("a[10]",a)
//
//  result in a name conflict, i.e. the second definition will fail.
//
//  Supported data types are:
//
//     Double_t, Float_t, Long_t, ULong_t, Int_t, UInt_t,
//     Short_t, UShort_t, Char_t, Byte_t.
//
//  The type is determined automatically through function overloading.
//  Do not specify the type explicitly using casts. Instead, define variables
//  with the type you need.
//
//  To get maximum efficiency and accuracy, variables should be Double_t.
//  For calculations in THaFormula/THaCut, all data will be promoted to
//  double precision anyhow.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVarList.h"
#include "THaVar.h"
#include "THaRTTI.h"
#include "THaArrayString.h"
#include "TRegexp.h"
#include "TString.h"
#include "TClass.h"
#include "TMethodCall.h"
#include "TFunction.h"
#include "TROOT.h"
#include "TMath.h"

#include <cstring>
#include <string>  // for TFunction::GetReturnTypeNormalizedName
#include <cassert>

ClassImp(THaVarList)

using namespace std;

static const Int_t kInitVarListCapacity = 100;
static const Int_t kVarListRehashLevel  = 3;

//_____________________________________________________________________________
THaVarList::THaVarList() : THashList(kInitVarListCapacity, kVarListRehashLevel)
{
  // Default constructor

  // THaVarList "owns" the variables put into it, i.e. if the list is deleted
  // so are the global variables in it.
  SetOwner(kTRUE);
}

//_____________________________________________________________________________
THaVarList::~THaVarList()
{
  // Destructor
}

//_____________________________________________________________________________
THaVar* THaVarList::DefineByType( const char* name, const char* descript,
				  const void* var, VarType type,
				  const Int_t* count, const char* errloc )
{
  // Define a global variable with given type.
  // Duplicate names are not allowed; if a name already exists, return 0

  THaVar* ptr = Find( name );
  if( ptr ) {
    Warning( errloc, "Variable %s already exists. Not redefined.",
	     ptr->GetName() );
    return 0;
  }

  ptr = new THaVar( name, descript, var, type, -1, 0, count );
  if( ptr && !ptr->IsZombie() )
    AddLast( ptr );
  else {
    Warning( errloc, "Error creating variable %s", name );
    delete ptr; ptr = 0;
  }

  return ptr;
}

//_____________________________________________________________________________
THaVar* THaVarList::DefineByRTTI( const TString& name, const TString& desc,
				  const TString& def,  const void* const obj,
				  TClass* const cl, const char* errloc )
{
  // Define variable via its ROOT RTTI

  typedef vector<TSeqCollection*> VecSC_t;

  assert( sizeof(ULong_t) == sizeof(void*) ); // ULong_t must be of pointer size

  if( !obj || !cl ) {
    Warning( errloc, "Invalid class or object. Variable %s not defined.",
	     name.Data() );
    return 0;
  }

  THaVar* var = Find( name );
  if( var ) {
    Warning( errloc, "Variable %s already exists. Not redefined.",
	     var->GetName() );
    return 0;
  }

  // Find up to two dots in the definition and extract the strings between them
  Ssiz_t pos = 0, ppos = 0;
  Int_t ndot = 0;
  TString s[3];
  while( (pos = def.Index( ".", pos )) != kNPOS && ndot<2 ) {
    Int_t len = pos - ppos;
    if( len >= 0 )
      s[ndot] = def(ppos, len);
    ppos = ++pos;
    ++ndot;
  }
  s[ndot] = def(ppos, def.Length()-ppos );

  // More than two dots?
  if( pos != kNPOS ) {
    Warning( errloc, "Too many dots in definition of variable %s (%s). "
	     "Variable not defined.", name.Data(), desc.Data() );
    return 0;
  }
  // Any zero-length strings?
  Int_t i = 0;
  while( i <= ndot )
    if( s[i++].Length() == 0 )
      return 0;

  TClass* theClass   =  0;
  ULong_t loc        =  (ULong_t)obj;

  THaRTTI objrtti;
  Int_t vecidx = -1;

  // Parse RTTI info
  switch( ndot ) {
  case 0:
    // Simple member variable or pointer to basic type
    theClass = cl;
    break;
  case 1:
    // Member variable contained in a member object, or pointer to it (->scalar);
    // or member variable contained in an object inside a vector (->vararray)
    // (like ndot==2, except element type is known)
    objrtti.Find( cl, s[0], obj );
    if( !objrtti.IsValid() ||
	!(objrtti.IsObject() || objrtti.IsObjVector()) ) {
      Warning( errloc, "Unsupported type of data member %s. "
	       "Variable %s (%s) not defined.",
	       s[0].Data(), name.Data(), desc.Data() );
      return 0;
    }
    loc += objrtti.GetOffset();
    if( objrtti.IsPointer() ) {
      void** ploc = (void**)loc;
      loc  = (ULong_t)*ploc;
    }
    theClass = objrtti.GetClass();
    assert( theClass ); // else objrtti.Find should not have succeeded
    break;
  case 2:
    // TSeqCollection member object, or pointer to it, or std::vector<TSeqCollection*>
    objrtti.Find( cl, s[0], obj );
    if( !objrtti.IsValid() || !objrtti.IsObject() ) {
      // Check for vector element
      bool good = false;
      THaArrayString s0(s[0]);
      if( s0 && s0.IsArray() && s0.GetNdim() == 1 ) {
	objrtti.Find( cl, s0, obj );
	if( objrtti.IsValid() && objrtti.IsObjVector() &&
	    objrtti.GetType() == kObjectPV ) {
	  vecidx = s0[0];
	  good = true;
	}
      }
      if( !good ) {
	Warning( errloc, "Unsupported type of data member %s. "
		 "Variable %s (%s) not defined.",
		 s[0].Data(), name.Data(), desc.Data() );
	return 0;
      }
    }
    theClass = objrtti.GetClass();
    assert( theClass );
    if( !theClass->InheritsFrom( "TSeqCollection" )) {
      Warning( errloc, "Data member %s is not a TSeqCollection. "
	       "Variable %s (%s) not defined.",
	       s[0].Data(), name.Data(), desc.Data() );
      return 0;
    }
    loc += objrtti.GetOffset();
    if( objrtti.IsPointer() ) {
      void** ploc = (void**)loc;
      loc  = (ULong_t)*ploc;
    }
    // For std::vector<TSeqCollection*>, get the requested vector element
    if( objrtti.IsObjVector() ) {
      VecSC_t vec = *reinterpret_cast<VecSC_t*>(loc);
      if( vecidx < 0 || vecidx >= (Int_t)vec.size() ) {
	Warning( errloc, "Illegal index %d into std::vector %s. "
		 "Current size = %d. Variable %s (%s) not defined.",
		 vecidx, s[0].Data(), (Int_t)vec.size(), name.Data(),
		 desc.Data() );
	return 0;
      }
      loc = reinterpret_cast<ULong_t>(vec[vecidx]);
      if( !loc ) {
	Warning( errloc, "Null pointer in std::vector %s. "
		 "Variable %s (%s) not defined.",
		 s[0].Data(), name.Data(), desc.Data() );
	return 0;
      }
      objrtti.Reset(); // clear objrtti.IsObjVector()
    }

    theClass = gROOT->GetClass( s[1] );
    if( !theClass ) {
      Warning( errloc, "Cannot determine class of container "
	       "member object %s. Variable %s (%s) not defined.",
	       s[1].Data(), name.Data(), desc.Data() );
      return 0;
    }
    break;
  default:
    assert(false); // not reached
    break;
  }

  // Define the variable based on the parse results
  Bool_t function = s[ndot].EndsWith("()");
  if( !function ) {
    // Member variables

    THaRTTI rtti;
    rtti.Find( theClass, s[ndot], (ndot==0) ? obj : 0 );
    if( !rtti.IsValid() ) {
      Warning( errloc, "No RTTI information for variable %s. "
	       "Not defined.", name.Data() );
      return 0;
    }
    if( rtti.IsObject() ) {
      Warning( errloc, "Variable %s is an object. Must be a basic type. "
	       "Not defined.", name.Data() );
      return 0;
    }

    if( ndot < 2 && !objrtti.IsObjVector() ) {
      // Basic data, arrays/vectors of basic data, or basic data in single objects
      loc += rtti.GetOffset();
      TString theName(name);
      Int_t* count_loc = 0;
      switch( rtti.GetArrayType() ) {
      case THaRTTI::kScalar:
      case THaRTTI::kVector:
	break;
      case THaRTTI::kFixed:
	theName.Append( rtti.GetSubscript() );
	break;
      case THaRTTI::kVariable:
	count_loc = (Int_t*)( (ULong_t)obj + rtti.GetCountOffset() );
	break;
      }
      var = DefineByType( theName, desc, (void*)loc, rtti.GetType(),
			  count_loc, errloc );
    } else {
      // Collections of objects
      if( !objrtti.IsObjVector() )
	var = new THaVar( name, desc, (void*)loc, rtti.GetType(),
			  rtti.GetOffset() );
      else {
	assert( ndot == 1 );
	VarType type = objrtti.GetType();
	assert( type == kObjectV || type == kObjectPV );
	Int_t sz = (type == kObjectV) ? objrtti.GetClass()->Size() : 0;
	var = new THaVar( name, desc, (void*)loc, rtti.GetType(), sz,
			  rtti.GetOffset() );
      }
      if( var && !var->IsZombie() )
	AddLast( var );
      else {
	Warning( errloc, "Error creating variable %s", name.Data() );
	delete var;
	return 0;
      }
    }

  } else { // if( !function )
    // Member functions

    TString funcName(s[ndot]);
    Ssiz_t i = funcName.Index( '(' );
    assert( i != kNPOS );  // else EndsWith("()") lied
    funcName = funcName(0,i);

    TMethodCall* theMethod = new TMethodCall( theClass, funcName, "" );
    if( !theMethod->IsValid() ) {
      Warning( errloc, "Error getting function information for variable %s. "
	       "Not defined.", name.Data() );
      delete theMethod;
      return 0;
    }
    TFunction* func = theMethod->GetMethod();
    if( !func ) {
      Warning( errloc, "Function %s does not exist. Variable %s not defined.",
	       s[ndot].Data(), name.Data() );
      delete theMethod;
      return 0;
    }

    TMethodCall::EReturnType rtype = theMethod->ReturnType();
    if( rtype != TMethodCall::kLong && rtype != TMethodCall::kDouble ) {
      Warning( errloc, "Unsupported return type for function %s. "
	       "Variable %s not defined.", s[ndot].Data(), name.Data() );
      delete theMethod;
      return 0;
    }

#if ROOT_VERSION_CODE <  ROOT_VERSION(5,34,6)
    VarType type = ( rtype == TMethodCall::kLong ) ? kLong : kDouble;
#else
    // Attempt to get the real function return type
    VarType type = kVarTypeEnd;
    string ntype = func->GetReturnTypeNormalizedName();
    if( ntype == "double" )
      type = kDouble;
    else if( (sizeof(int) == sizeof(Int_t) && ntype == "int") ||
	     (sizeof(long) == sizeof(Int_t) && ntype == "long") )
      type = kInt;
    else if( (sizeof(unsigned int) == sizeof(UInt_t) && ntype == "unsigned int") ||
	     (sizeof(unsigned long) == sizeof(UInt_t) && ntype == "unsigned long") )
      type = kUInt;
    else if( ntype == "float" )
      type = kFloat;
    else if( ntype == "bool" ) {
      if( sizeof(bool) == sizeof(Char_t) )
	type = kChar;
      else if( sizeof(bool) == sizeof(Short_t) )
	type = kShort;
      else if( sizeof(bool) == sizeof(Int_t) )
	type = kInt;
    }
    else if( (sizeof(long) == sizeof(Long_t) && ntype == "long") ||
	     (sizeof(long long) == sizeof(Long_t) && ntype == "long long") ||
	     (sizeof(int) == sizeof(Long_t) && ntype == "int") )
      type = kLong;
    else if( (sizeof(unsigned long) == sizeof(ULong_t) && ntype == "unsigned long") ||
	     (sizeof(unsigned long long) == sizeof(ULong_t) && ntype == "unsigned long long") ||
	     (sizeof(unsigned int) == sizeof(ULong_t) && ntype == "unsigned int") )
      type = kULong;
    else if( (sizeof(short) == sizeof(Short_t) && ntype == "short") )
      type = kShort;
    else if( (sizeof(unsigned short) == sizeof(UShort_t) && ntype == "unsigned short") )
      type = kUShort;
    else if( ntype == "char" )
      type = kChar;
    else if( ntype == "unsigned char" )
      type = kUChar;

    if( type == kVarTypeEnd ) {
      Warning( errloc, "Unsupported return type \"%s\" for function %s. "
	       "Variable %s not defined.", ntype.c_str(), s[ndot].Data(), name.Data() );
      delete theMethod;
      return 0;
    }
    if( (rtype == TMethodCall::kDouble && type != kDouble && type != kFloat) ||
	(rtype == TMethodCall::kLong && (type == kDouble || type == kFloat)) ) {
      // Someone lied to us
      Warning( errloc, "Ill-formed TMethodCall returned from ROOT for function %s, "
	       "variable %s. Call expert.", s[ndot].Data(), name.Data() );
      delete theMethod;
      return 0;
    }
#endif
    if( !objrtti.IsObjVector() )
      var = new THaVar( name, desc, (void*)loc, type, ((ndot==2) ? 0 : -1),
			theMethod );
    else {
      assert( ndot == 1 );
      VarType otype = objrtti.GetType();
      assert( otype == kObjectV || otype == kObjectPV );
      Int_t sz = (otype == kObjectV) ? objrtti.GetClass()->Size() : 0;
      var = new THaVar( name, desc, (void*)loc, type, sz, 0, theMethod );
    }
    if( var && !var->IsZombie() )
      AddLast( var );
    else {
      Warning( errloc, "Error creating variable %s", name.Data() );
      delete theMethod;
      delete var;
      return 0;
    }

  } // if( !function ) else

  return var;
}

//-----------------------------------------------------------------------------
Int_t THaVarList::DefineVariables( const VarDef* list, const char* prefix,
				   const char* caller )
{
  // Add all variables specified in 'list' to the list. 'list' is a C-style
  // structure defined in VarDef.h and must be terminated with a NULL name.
  //
  // Allows definition of:
  //    scalers
  //    fixed size arrays
  //    variable size arrays
  //    fixed size arrays of pointers
  //    variable size arrays of pointers
  // Data can be anywhere in memory, typically in member variables.
  // Data type must be specified explicitly. To use the ROOT RTTI features
  // to determine data types (and arrays sizes etc.) automatically, or
  // to access data in member ROOT objects, use the second form of this
  // method.
  //
  // The names of all newly created variables will be prefixed with 'prefix',
  // if given.  Error messages will include 'caller', if given.
  // Output a warning if a name already exists.  Return values >0 indicate the
  // number of variables defined, <0 indicate errors (no variables defined).

  TString errloc("DefineVariables [called from ");
  if( !caller || !*caller ) {
    caller = "(global scope)";
  }
  errloc.Append(caller);
  errloc.Append("]");

  if( !list ) {
    Error(errloc, "Empty input list. No variables registered.");
    return -1;
  }

  if( !prefix )
    prefix = "";

  const VarDef* item = list;
  Int_t ndef = 0;
  TString name;

  while( item->name ) {

    const char* description = item->desc;
    if( !description || !*description )
      description = item->name;

    // Assemble the name string
    name = prefix;
    name.Append(item->name);
    // size>1 means fixed-size 1-d array
    bool fixed_array = (item->size > 1);
    bool var_array = (item->count != 0) ||
      (item->type >= kIntV && item->type <= kDoubleV);
    if( item->size>0 && var_array ) {
      Warning( errloc, "Variable %s: variable-size arrays must have size=0. "
	       "Ignoring size.", name.Data() );
    } else if( fixed_array ) {
      char dimstr[256];
      sprintf( dimstr, "[%d]", item->size );
      name.Append(dimstr);
    }

    // Define this variable, using the indicated type

    THaVar* var = DefineByType( name, description, item->loc,
				static_cast<VarType>( item->type ),
				item->count, errloc );
    if( var )
      ndef++;
    item++;
  }

  return ndef;
}

//-----------------------------------------------------------------------------
Int_t THaVarList::DefineVariables( const RVarDef* list, const TObject* obj,
				   const char* prefix,  const char* caller,
				   const char* var_prefix )
{
  // Add all variables specified in 'list' to the list. 'list' is a C-style
  // structure defined in VarDef.h and must be terminated with a NULL name.
  //
  // Allows definition of:
  //    scalers
  //    fixed size arrays
  //    variable size arrays
  //
  // Data must be persistent ROOT data members or accessible via
  // member functions. They can be member variables, member variables
  // of member ROOT objects, or member variables of ROOT objects
  // contained in member ROOT containers derived from TSeqCollection.
  //
  // The names of all newly created variables will be prefixed with 'prefix',
  // if given.  Error messages will include 'caller', if given.
  // Output a warning if a name already exists.  Return values >0 indicate the
  // number of variables defined, <0 indicate errors (no variables defined).

  TString errloc("DefineVariables [called from ");
  if( !caller || !*caller) {
    caller = "(global scope)";
  }
  errloc.Append(caller);
  errloc.Append("]");

  if( !list ) {
    Error(errloc, "No input list. No variables registered.");
    return -1;
  }

  if( !obj ) {
    Error(errloc, "No base object. No variables registered.");
    return -2;
  }

  // Get the class of the base object
  TClass* cl = obj->IsA();
  if( !cl ) {
    //Oops
    Error( errloc, "Base object has no class?!? No variables registered.");
    return -3;
  }

  const RVarDef* item;
  Int_t ndef = 0;
  while( (item = list++) && item->name ) {

    // Assemble the name and description strings
    TString name( item->name );
    if( name.IsNull() )
      continue;
    if( prefix )
      name.Prepend(prefix);

    TString desc( (item->desc && *item->desc ) ? item->desc : name.Data() );

    // Process the variable definition
    char* c = ::Compress( item->def );
    TString def( c );
    delete [] c;
    if( def.IsNull() ) {
      Warning( errloc, "Invalid definition for variable %s (%s). "
	       "Variable not defined.", name.Data(), desc.Data() );
      continue;
    }
    if( var_prefix && *var_prefix )
      def.Prepend( var_prefix );

    THaVar* var = DefineByRTTI( name, desc, def, obj, cl, errloc );

    if( var )
      ndef++;
  }

  return ndef;
}

//_____________________________________________________________________________
THaVar* THaVarList::Find( const char* name ) const
{
  // Find a variable in the list.  If 'name' has array syntax ("var[3]"),
  // the search is performed for the array basename ("var").

  TObject* ptr;
  const char* p = strchr( name, '[' );
  if( !p )
    ptr = FindObject( name );
  else {
    size_t n = p-name;
    char* basename = new char[ n+1 ];
    strncpy( basename, name, n );
    basename[n] = '\0';
    ptr = FindObject( basename );
    delete [] basename;
  }
  return static_cast<THaVar*>(ptr);
}

//_____________________________________________________________________________
void THaVarList::PrintFull( Option_t* option ) const
{
  // Print variable names, descriptions, and data.
  // Identical to Print() except that variable data is also printed.
  // Supports selection of subsets of variables via 'option'.
  // E.g.: option="var*" prints only variables whose names start with "var".

  if( !option )  option = "";
  TRegexp re(option,kTRUE);
  TIter next(this);

  while( TObject* obj = next() ) {
    if( *option ) {
      TString s = obj->GetName();
      if( s.Index(re) == kNPOS )
	continue;
    }
    obj->Print("FULL");
  }
}

//_____________________________________________________________________________
Int_t THaVarList::RemoveName( const char* name )
{
  // Delete the variable with the given name from the list.
  // Returns 1 if variable was deleted, 0 is it wasn't in the list.
  //
  // Note: This differs from TList::Remove(), which doesn't delete the
  // element itself.

  TObject* ptr = Find( name );
  if( !ptr )
    return 0;
  ptr = Remove( ptr );
  delete ptr;
  return 1;
}

//_____________________________________________________________________________
Int_t THaVarList::RemoveRegexp( const char* expr, Bool_t wildcard )
{
  // Delete all variables matching regular expression 'expr'. If 'wildcard'
  // is true, the more user-friendly wildcard format is used (see TRegexp).
  // Returns number of variables removed, or <0 if error.

  TRegexp re( expr, wildcard );
  if( re.Status() ) return -1;

  Int_t ndel = 0;
  TIter next( this );
  while( TObject* ptr = next() ) {
    TString name = ptr->GetName();
    if( name.Index( re ) != kNPOS ) {
      ptr = Remove( ptr );
      delete ptr;
      ndel++;
    }
  }
  return ndel;
}
