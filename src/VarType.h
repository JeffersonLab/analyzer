#ifndef ROOT_VarType
#define ROOT_VarType

//////////////////////////////////////////////////////////////////////////
//
// VarType
//
// Variable types used by THaVar. Separate include file for convenience.
// kDouble is guaranteed to be 0, all others may vary.
//
//////////////////////////////////////////////////////////////////////////

enum VarType { kDouble = 0, kFloat, kLong, kULong, kInt, kUInt, 
	       kShort, kUShort, kChar, kByte };

#endif
