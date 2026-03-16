//*-- Author :    Bodo Reitz

//////////////////////////////////////////////////////////////////////////
//
// THaBeamDet
//
// Abstract detector class that provides position and direction of
// a particle beam, usually event by event.
// 
//////////////////////////////////////////////////////////////////////////

#include "THaBeamDet.h"

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaBeamDet)
#endif

//_____________________________________________________________________________
