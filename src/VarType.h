#ifndef Podd_VarType_h_
#define Podd_VarType_h_

//////////////////////////////////////////////////////////////////////////
//
// VarType
//
// Variable types used by THaVar.
// kDouble is guaranteed to be 0, all others may vary.
//
// Do not change the order of kDouble ... kByte, kDoubleP ... kByteP,
// and kDouble2P ... kByte2P.
// Code in THaRTTI, THaVar, and THaAnalysisObject depends on these
// ranges being contiguous and in corresponding order!
// Likewise, the range kIntV ... kObjectPV needs to be contiguous.
//
//////////////////////////////////////////////////////////////////////////

// NB: When updating this list, must also update var_type_info[] in THaVar.C
enum VarType { kDouble = 0, kVarTypeBegin = kDouble,
	       kFloat, kLong, kULong, kInt, kUInt,
	       kShort, kUShort, kChar, kUChar, kByte = kUChar,
	       kObject, kTString, kString,
	       kIntV, kUIntV, kFloatV, kDoubleV, kObjectV, kObjectPV,
	       kIntM, kFloatM, kDoubleM,
	       kDoubleP, kFloatP, kLongP, kULongP, kIntP, kUIntP,
	       kShortP, kUShortP, kCharP, kUCharP, kByteP = kUCharP,
	       kObjectP,
	       kDouble2P, kFloat2P, kLong2P, kULong2P, kInt2P, kUInt2P,
	       kShort2P, kUShort2P, kChar2P, kUChar2P, kByte2P = kUChar2P,
	       kObject2P,
	       kVarTypeEnd };

#endif
