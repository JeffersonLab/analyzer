//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometerDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometerDetector.h"
#include "THaDetMap.h"

#include <iostream>

ClassImp(THaSpectrometerDetector)

//______________________________________________________________________________
THaSpectrometerDetector::THaSpectrometerDetector( const char* name, 
						  const char* description,
						  THaApparatus* apparatus )
  : THaDetector(name,description,apparatus)
{
  // Constructor

}

//______________________________________________________________________________
THaSpectrometerDetector::~THaSpectrometerDetector()
{
  // Destructor

}

