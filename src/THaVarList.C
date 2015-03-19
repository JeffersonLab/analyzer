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
#include "TRegexp.h"
#include "TString.h"
#include "TClass.h"
#include "TMethodCall.h"
#include "TROOT.h"
#include "TMath.h"

#include <cstring>

ClassImp(THaVarList)

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
  if( ptr ) 
    AddLast( ptr );
  else
    Error( errloc, "Error allocating new variable %s. No variable defined.",
	   name );

  return ptr;
}

//_____________________________________________________________________________
THaVar* THaVarList::DefineByRTTI( const TString& name, const TString& desc, 
				  const TString& def,  const void* const obj,
				  TClass* const cl, const char* errloc )
{
  // Define variable via its ROOT RTTI

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
  // Ssiz_t dot[2];
  Int_t ndot = 0;
  TString s[3];
  while( (pos = def.Index( ".", pos )) != kNPOS && ndot<2 ) {
    // dot[ndot] = pos;
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

  Bool_t        function   =  s[ndot].EndsWith("()");
  TClass*       theClass   =  0;
  ULong_t       loc        =  (ULong_t)obj;
  void**        ploc;

  THaRTTI rtti, objrtti;

  // Access via member function?
  if( !function ) {

   // Process member variables
    switch( ndot ) {
    case 0:
      // Simple member variable or pointer to basic type
      rtti.Find( cl, s[0], obj );
      break;
    case 1:
      // Member variable contained in a member object, or pointer to it
      objrtti.Find( cl, s[0], obj );
      if( !objrtti.IsValid() || !objrtti.IsObject() ) 
	return 0;
      theClass = objrtti.GetClass();
      if( !theClass ) return 0;
      loc += objrtti.GetOffset();
      if( objrtti.IsPointer() ) {
	ploc = (void**)loc;
	loc  = (ULong_t)*ploc;
      }
      rtti.Find( theClass, s[1] );
      break;
    case 2:
      // TSeqCollection member object, or pointer to it
      objrtti.Find( cl, s[0], obj );
      if( !objrtti.IsValid() || !objrtti.IsObject() ) 
	return 0;
      theClass = objrtti.GetClass();
      if( !theClass ) return 0;
      if( !theClass->InheritsFrom( "TSeqCollection" )) {
	Warning( errloc, "Data member %s is not a TSeqCollection. "
		 "Variable %s (%s) not defined.", 
		 s[0].Data(), name.Data(), desc.Data() );
	return 0;
      }
      loc += objrtti.GetOffset();
      if( objrtti.IsPointer() ) {
	ploc = (void**)loc;
	loc  = (ULong_t)*ploc;
      }

      theClass = gROOT->GetClass( s[1] );
      if( !theClass ) {
	Warning( errloc, "Cannot determine class of container "
		 "member object %s. Variable %s (%s) not defined.",
		 s[1].Data(), name.Data(), desc.Data() );
	return 0;
      }
      rtti.Find( theClass, s[2] );
      break;
    default:
      break;
    } 
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

    if( ndot < 2 ) {
      loc += rtti.GetOffset();
      TString theName(name);
      Int_t* count_loc = 0;
      switch( rtti.GetArrayType() ) {
      case THaRTTI::kFixed:
	theName.Append( rtti.GetSubscript() );
	break;
      case THaRTTI::kVariable:
	count_loc = (Int_t*)( (ULong_t)obj + rtti.GetCountOffset() );
	break;
      default:
	break;
      }
      var = DefineByType( theName, desc, (void*)loc, rtti.GetType(), count_loc,
			  errloc );
    } else {
      var = new THaVar( name, desc, (void*)loc, rtti.GetType(),
			rtti.GetOffset() );
      if( var )
	AddLast( var );
      else {
	Error( errloc, "Error allocating new variable %s. No variable defined.",
	       name.Data() );
	return 0;
      }
    }

  } else {
    // Process member functions
    TMethodCall* theMethod = 0;

    TString funcName(s[ndot]);
    Ssiz_t i = funcName.Index( '(' );
    if( i == kNPOS )
      return 0; // Oops
    funcName = funcName(0,i);

    switch( ndot ) {
    case 0:
      theMethod = new TMethodCall( cl, funcName, "" );
      break;
    case 1:
      objrtti.Find( cl, s[0], obj );
      if( !objrtti.IsValid() || !objrtti.IsObject() ) 
	return 0;
      theClass = objrtti.GetClass();
      if( !theClass ) return 0;
      loc += objrtti.GetOffset();
      if( objrtti.IsPointer() ) {
	ploc = (void**)loc;
	loc  = (ULong_t)*ploc;
      }
      theMethod = new TMethodCall( theClass, funcName, "" );
      break;
    case 2:
      objrtti.Find( cl, s[0], obj );
      if( !objrtti.IsValid() || !objrtti.IsObject() ) 
	return 0;
      theClass = objrtti.GetClass();
      if( !theClass ) return 0;
      if( !theClass->InheritsFrom( "TSeqCollection" )) {
	Warning( errloc, "Data member %s is not a TSeqCollection. "
		 "Variable %s (%s) not defined.", 
		 s[0].Data(), name.Data(), desc.Data() );
	return 0;
      }
      loc += objrtti.GetOffset();
      if( objrtti.IsPointer() ) {
	ploc = (void**)loc;
	loc  = (ULong_t)*ploc;
      }

      theClass = gROOT->GetClass( s[1] );
      if( !theClass ) {
	Warning( errloc, "Cannot determine class of container "
		 "member object %s. Variable %s (%s) not defined.",
		 s[1].Data(), name.Data(), desc.Data() );
	return 0;
      }
      theMethod = new TMethodCall( theClass, funcName, "" );
      break;
    default:
      break;
    }

    if( !theMethod ) {
      Warning( errloc, "Error getting function information for variable %s. "
	       "Not defined.", name.Data() );
      return 0;
    }
    if( !theMethod->GetMethod() ) {
      Warning( errloc, "Function %s does not exist. "
	       "Variable %s not defined.", s[ndot].Data(), name.Data() );
      delete theMethod;
      return 0;
    }

    VarType type;
    switch( theMethod->ReturnType()) {
    case TMethodCall::kLong:
      type = kLong;
      break;
    case TMethodCall::kDouble:
      type = kDouble;
      break;
    default:
      Warning( errloc, "Undefined return type for function %s. "
	       "Variable %s not defined.", s[ndot].Data(), name.Data() );
      delete theMethod;
      return 0;
      break;
    }
    var = new THaVar( name, desc, (void*)loc, type, ((ndot==2) ? 0 : -1),
		      theMethod);
    
    if( var )
      AddLast( var );
    else {
      Error( errloc, "Error allocating new variable %s. No variable defined.",
	     name.Data() );
      delete theMethod;
      return 0;
    }

  }
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
    Warning(errloc, "Empty input list. No variables registered.");
    return -1;
  }

  const VarDef* item = list;
  Int_t ndef = 0;
  size_t nlen = 64;
  char* name = new char[nlen];

  while( item->name ) {

    const char* description = item->desc;
    if( !description || !*description )
      description = item->name;

    size_t len = strlen( item->name ), slen = 0, plen = 0;

    // size>1 means 1-d array
    if( item->size>1 ) {
      slen = static_cast<size_t>( TMath::Log10( item->size )) + 3;
    }
    if( prefix ) {
      plen = strlen( prefix );
    }
    // Resize name string buffer if necessay
    if( plen+len+slen+1 > nlen ) {
      delete [] name;
      name = new char[plen+len+slen+1];
      nlen = plen+len+slen+1;
    }
    // Assemble the name string
    if( plen )
      strcpy( name, prefix );
    strcpy( name+plen, item->name );
    if( slen ) {
      sprintf( name+plen+len, "[%d]", item->size );
    }

    // Define this variable, using the indicated type

    THaVar* var = DefineByType( name, description, item->loc, 
				static_cast<VarType>( item->type ), 
				item->count, errloc );
    if( var )
      ndef++;
    item++;
  }
  delete [] name;

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
    Warning(errloc, "No input list. No variables registered.");
    return -1;
  }

  if( !obj ) {
    Warning(errloc, "No base object. No variables registered.");
    return -2;
  }

  // Get the class of the base object
  TClass* cl = obj->IsA();
  if( !cl ) {
    //Oops
    Warning( errloc, "Base object has no class?!? No variables registered.");
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
