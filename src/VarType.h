#ifndef ROOT_VarType
#define ROOT_VarType

//////////////////////////////////////////////////////////////////////////
//
// VarType
//
// Variable types used by THaVar. Separate include file for convenience.
// kDouble is guaranteed to be 0, all others may vary.
//
// Do not change the order of kDouble ... kByte and kDoubleP ... kByteP.
// Code in THaVarList and THaVar depends on it!
//
//////////////////////////////////////////////////////////////////////////

enum VarType { kDouble = 0, kFloat, kLong, kULong, kInt, kUInt, 
	       kShort, kUShort, kChar, kByte, 
	       kObject,
	       kDoubleP, kFloatP, kLongP, kULongP, kIntP, kUIntP, 
	       kShortP, kUShortP, kCharP, kByteP,
	       kObjectP,
	       kDouble2P, kFloat2P, kLong2P, kULong2P, kInt2P, kUInt2P, 
	       kShort2P, kUShort2P, kChar2P, kByte2P,
	       kObject2P };

#endif
