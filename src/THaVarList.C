//*-- Author :    Ole Hansen  27/04/00

//////////////////////////////////////////////////////////////////////////
//
//  THaVarList
//
//  A specialized TList containing global variables of the
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
//  The implementation of this class is somewhat ugly because we cannot use
//  templates and RTTI.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVarList.h"
#include "THaVar.h"
#include "TRegexp.h"
#include "TString.h"
#include <cstring>

ClassImp(THaVarList)

//_____________________________________________________________________________
void THaVarList::Clear( Option_t* opt )
{
   // Remove all variables from the list.

  while( fFirst ) {
    THaVar* obj = (THaVar*)TList::Remove( fFirst );
    delete obj;
  }
}

//_____________________________________________________________________________
THaVar* THaVarList::DefineByType( const char* name, const char* descript, 
				  const void* var, VarType type, 
				  const Int_t* count )
{
  // Define a global variable with given type.
  // Duplicate names are not allowed; if a name already exists, return NULL

  static const char* const here = "DefineByType()";

  THaVar* ptr = Find( name );
  if( ptr ) {
    Warning( here, "Variable %s already exists. Not redefined.", ptr->GetName() );
    return NULL;
  }

  switch( type ) {
  case kDouble:
    ptr = new THaVar( name, descript, 
		      static_cast<const Double_t*>( var ), count );
    break;
  case kFloat:
    ptr = new THaVar( name, descript, 
		      static_cast<const Float_t*>( var ), count );
    break;
  case kLong:
    ptr = new THaVar( name, descript, 
		      static_cast<const Long_t*>( var ), count );
    break;
  case kULong:
    ptr = new THaVar( name, descript, 
		      static_cast<const ULong_t*>( var ), count );
    break;
  case kInt:
    ptr = new THaVar( name, descript, 
		      static_cast<const Int_t*>( var ), count );
    break;
  case kUInt:
    ptr = new THaVar( name, descript, 
		      static_cast<const UInt_t*>( var ), count );
    break;
  case kShort:
    ptr = new THaVar( name, descript, 
		      static_cast<const Short_t*>( var ), count );
    break;
  case kUShort:
    ptr = new THaVar( name, descript, 
		      static_cast<const UShort_t*>( var ), count );
    break;
  case kChar:
    ptr = new THaVar( name, descript, 
		      static_cast<const Char_t*>( var ), count );
    break;
  case kByte:
    ptr = new THaVar( name, descript, 
		      static_cast<const Byte_t*>( var ), count );
    break;
  case kDoubleP:
    ptr = new THaVar( name, descript, 
		      static_cast<const Double_t**>( var ), count );
    break;
  case kFloatP:
    ptr = new THaVar( name, descript, 
		      static_cast<const Float_t**>( var ), count );
    break;
  case kLongP:
    ptr = new THaVar( name, descript, 
		      static_cast<const Long_t**>( var ), count );
    break;
  case kULongP:
    ptr = new THaVar( name, descript, 
		      static_cast<const ULong_t**>( var ), count );
    break;
  case kIntP:
    ptr = new THaVar( name, descript, 
		      static_cast<const Int_t**>( var ), count );
    break;
  case kUIntP:
    ptr = new THaVar( name, descript, 
		      static_cast<const UInt_t**>( var ), count );
    break;
  case kShortP:
    ptr = new THaVar( name, descript, 
		      static_cast<const Short_t**>( var ), count );
    break;
  case kUShortP:
    ptr = new THaVar( name, descript, 
		      static_cast<const UShort_t**>( var ), count );
    break;
  case kCharP:
    ptr = new THaVar( name, descript, 
		      static_cast<const Char_t**>( var ), count );
    break;
  case kByteP:
    ptr = new THaVar( name, descript, 
		      static_cast<const Byte_t**>( var ), count );
    break;
  default:
    Warning( here, "Unknown data type for variable %s."
	     "No variable defined.", name );
    return NULL;
    break;
  }

  if( ptr ) 
    AddLast( ptr );
  else
    Error( here, "Error allocating new variable %s. No variable defined.",
	   name );

  return ptr;
}

//-----------------------------------------------------------------------------
Int_t THaVarList::DefineVariables( const VarDef* list, const char* prefix, 
				   const char* caller )
{
  // Define variables specified in 'list' to the list. 'list' must be 
  // terminated with a NULL name. 
  // The names of all newly created variables will be prefixed with 'prefix',
  // if given.
  // Error messages will include 'caller', if given.
  // Output warnings if names already exist.  Return values >0 indicate the
  // number of variables defined, <0 indicate errors (no variables defined).
  
  TString errloc("DefineVariables()");
  if( caller) {
    errloc.Append(" ");
    errloc.Append(caller);
  }

  if( !list ) {
    Warning(errloc.Data(), "Empty input list. No variables registered.");
    return -1;
  }

  const VarDef* item = list;
  Int_t ndef = 0;
  size_t nlen = 64;
  char* name = new char[nlen];

  while( item->name ) {

    const char* description = item->desc;
    if( !description || strlen(description) == 0 )
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
				item->count );
    if( var )
      ndef++;
    //error already printed by Define()
    //    else
    //  ExistsWarning( errloc.Data(), name );   

    // Next item on the list
    item++;
  }
  delete [] name;

  return ndef;
}

//_____________________________________________________________________________
void THaVarList::ExistsWarning( const char* errloc, const char* name )
{
  // Print warning that variable already exists. Internal function.

  Warning(errloc, "Variable %s already exists. Not redefined.", name);
}
      
//_____________________________________________________________________________
THaVar* THaVarList::Find( const char* name ) const
{
  // Find a variable in the list.  If 'name' has array syntax ("var[3]"),
  // the search is performed for the array basename ("var").

  THaVar* ptr;
  const char* p = strchr( name, '[' );
  if( !p ) 
    return static_cast<THaVar*>(FindObject( name ));
  else {
    size_t n = p-name;
    char* basename = new char[ n+1 ];
    strncpy( basename, name, n );
    basename[n] = '\0';
    ptr = static_cast<THaVar*>(FindObject( basename ));
    delete [] basename;
  }
  return ptr;
}

//_____________________________________________________________________________
void THaVarList::PrintFull( Option_t* option ) const
{
  // Print variable names, descriptions, and data.
  // Identical to Print() except that variable data is also printed.
  // Supports selectrion of subsets of variables via the option.
  // E.g. option="xxx*" prints only variables xxx*.

  TRegexp re(option,kTRUE);
  TIter next(this);
  TObject* object;
  Int_t nch = strlen(option);

  while ((object = next())) {
    TString s = object->GetName();
    if (nch && strcmp(option,object->GetName()) && s.Index(re) == kNPOS) 
      continue;
    object->Print("FULL");
  }
}

//_____________________________________________________________________________
Int_t THaVarList::RemoveName( const char* name )
{
  // Remove a variable from the list
  // Since each name is unique, we only have to find it once
  // Returns 1 if variable was deleted, 0 is it wasn't in the list

  THaVar* ptr = Find( name );
  if( ptr ) {
    THaVar* p = static_cast< THaVar* >(TList::Remove( ptr ));
    delete p;
    return 1;
  } else
    return 0;
}

//_____________________________________________________________________________
Int_t THaVarList::RemoveRegexp( const char* expr, Bool_t wildcard )
{
  // Remove all variables matching regular expression 'expr'. If 'wildcard'
  // is true, the more user-friendly wildcard format is used (see TRegexp).
  // Returns number of variables removed, or <0 if error.

  TRegexp re( expr, wildcard );
  if( re.Status() ) return -1;

  Int_t ndel = 0;
  TString name;
  TIter next( this );
  while( THaVar* ptr = static_cast< THaVar* >( next() )) {
    name = ptr->GetName();
    if( name.Index( re ) != kNPOS ) {
      THaVar* p = static_cast< THaVar* >(TList::Remove( ptr ));
      delete p;
      ndel++;
    } 
  }
  return ndel;
}
