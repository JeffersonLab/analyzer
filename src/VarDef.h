#ifndef ROOT_VarDef
#define ROOT_VarDef

//////////////////////////////////////////////////////////////////////////
//
// VarDef
//
// Structure for calls to THaVarList::DefineVariables() and derived
// functions.
//
//////////////////////////////////////////////////////////////////////////

struct VarDef {
  const char*      name;     // Variable name
  const char*      desc;     // Variable description
  Int_t            type;     // Variable data type (see VarType.h)
  Int_t            size;     // Size of array (0/1 = scalar)
  const void*      loc;      // Location of data
  const Int_t*     count;    // Optional: Actual size of variable size array
};

#endif
