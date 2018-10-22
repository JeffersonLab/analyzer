#ifndef Podd_THaGlobals_h_
#define Podd_THaGlobals_h_

// Pick up definition of R__EXTERN
#ifndef Podd_DllImport_h_
#include "DllImport.h"
#endif

// Global Analyzer variables. Defined in THaInterface implementation file.

R__EXTERN class THaVarList*  gHaVars;      //List of global symbolic variables
R__EXTERN class THaCutList*  gHaCuts;      //List of defined cuts
R__EXTERN class TList*       gHaApps;      //List of apparatuses
R__EXTERN class TList*       gHaPhysics;   //List of physics modules
R__EXTERN class TList*       gHaEvtHandlers;   //List of event handlers
R__EXTERN class THaRunBase*  gHaRun;       //The currently active run
R__EXTERN class TClass*      gHaDecoder;   //Class(!) of decoder to use
R__EXTERN class THaDB*       gHaDB;        //Database system to use
R__EXTERN class THaTextvars* gHaTextvars;  //List of text variable definitions

#endif
