#ifndef ROOT_THaGlobals
#define ROOT_THaGlobals

// Global Analyzer variables. Defined in THaInterface implementation file.

R__EXTERN class THaVarList*  gHaVars;      //List of global symbolic variables
R__EXTERN class THaCutList*  gHaCuts;      //List of defined cuts
R__EXTERN class TList*       gHaApps;      //List of apparatuses
R__EXTERN class TList*       gHaScalers;   //List of scaler groups
R__EXTERN class TList*       gHaPhysics;   //List of physics modules
R__EXTERN class THaRun*      gHaRun;       //The currently active run

#endif
