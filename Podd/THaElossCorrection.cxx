//*-- Author :    Ole Hansen   23-Apr-04

//////////////////////////////////////////////////////////////////////////
//
// THaElossCorrection
//
//////////////////////////////////////////////////////////////////////////

#include "THaElossCorrection.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaTrackInfo.h"
#include "THaVertexModule.h"
#include "TMath.h"
#include "TVector3.h"
#include "VarDef.h"
#include "VarType.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaElossCorrection::THaElossCorrection( const char* name, 
					const char* description,
					const char* input_tracks,
					Double_t particle_mass,
					Int_t hadron_charge ) :
  THaPhysicsModule(name,description), fZ(hadron_charge),
  fZmed(0.0), fAmed(0.0), fDensity(0.0), fPathlength(0.0), 
  fZref(0.0), fScale(0.0),
  fTestMode(kFALSE), fExtPathMode(kFALSE), fInputName(input_tracks),
  fVertexModule(NULL)
{
  // Normal constructor.

  SetMass(particle_mass);
  Clear();
}

//_____________________________________________________________________________
THaElossCorrection::~THaElossCorrection()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaElossCorrection::Clear( Option_t* opt )
{
  // Clear all event-by-event variables.
  
  THaPhysicsModule::Clear(opt);
  if( !fTestMode )
    fEloss = kBig;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaElossCorrection::Init( const TDatime& run_time )
{
  // Initialize the module.

  if( fExtPathMode ) {
    fVertexModule = dynamic_cast<THaVertexModule*>
      ( FindModule( fVertexName.Data(), "THaVertexModule" ));
    if( !fVertexModule )
      return fStatus;
  }

  // Continue with standard initialization
  THaPhysicsModule::Init( run_time );

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaElossCorrection::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Locally computed data
  const RVarDef var[] = {
    { "eloss", "Calculated energy loss correction (GeV)", "fEloss" },
    { "pathl", "Pathlength thru medium for this event",   "fPathlength" },
    { 0 }
  };
  DefineVarsFromList( var, mode );

  return 0;
}

//_____________________________________________________________________________
Int_t THaElossCorrection::ReadRunDatabase( const TDatime& date )
{
  // Query the run database for the module parameters.

  Int_t err = THaPhysicsModule::ReadRunDatabase( date );
  if( err ) return err;

  FILE* f = OpenRunDBFile( date );
  if( !f ) return kFileError;

  DBRequest req[] = {
    { "M",          &fM,          kDouble, 0, 0, 0, "M (particle mass [GeV/c^2])" },
    { "Z",          &fZ,          kInt,    0, 1, 0, "Z (particle Z)" },
    { "Z_med",      &fZmed,       kDouble, 0, 0, 0, "Z_med (Z of medium)" },
    { "A_med",      &fAmed,       kDouble, 0, 0, 0, "A_med (A of medium)" },
    { "density",    &fDensity,    kDouble, 0, 0, 0, "density of medium [g/cm^3]" },
    { "pathlength", &fPathlength, kDouble, 0, 0, 0, "pathlength through medium [m]" },
    { 0 }
  };
  // Allow pathlength key to be absent in variable pathlength mode
  if( fExtPathMode ) {
    int i = 0;
    while( req[i].name ) {
      if( TString(req[i].name) == "pathlength" ) {
	req[i].optional = kTRUE;
	break;
      }
      ++i;
    }
  }

  // Ignore database entries if parameter already set
  DBRequest* item = req;
  while( item->name ) {
    if( *((double*)item->var) != 0.0 )
      item->var = 0;
    item++;
  }

  // Try to read any unset parameters from the database
  err = LoadDB( f, date, req );
  fclose(f);
  if( err )
    return kInitError;

  return kOK;
}
  
//_____________________________________________________________________________
void THaElossCorrection::SetInputModule( const char* name ) 
{
  // Set the name of the module that provides the input data
  if( !IsInit())
    fInputName = name; 
  else
    PrintInitError("SetInputModule");
}

//_____________________________________________________________________________
void THaElossCorrection::SetMass( Double_t m ) 
{
  // Set mass of track particle
  if( !IsInit() ) {
    fM = m; 
    fElectronMode = (TMath::Abs(fM) < 1e-3);
  } else
    PrintInitError("SetMass");
}

//_____________________________________________________________________________
void THaElossCorrection::SetTestMode( Bool_t enable, Double_t eloss_value ) 
{
  // Enable test mode. If enabled, apply fixed energy correction for
  // every event. Negative eloss values are ignored.

  if( !IsInit() ) {
    fTestMode = enable;
    if( enable )
      fEloss = TMath::Max(eloss_value,0.0);
  } else
    PrintInitError("SetTestMode");
}

//_____________________________________________________________________________
void THaElossCorrection::SetMedium( Double_t Z, Double_t A,
				    Double_t density ) 
{
  // Set parameters of the medium in which the energy loss occurs
  if( !IsInit() ) {
    fZmed = Z;
    fAmed = A;
    fDensity = density;
  } else
    PrintInitError("SetMedium");
}

//_____________________________________________________________________________
void THaElossCorrection::SetPathlength( Double_t pathlength ) 
{
  // Use fixed pathlength for energy loss calculations

  if( !IsInit() ) {
    fPathlength = pathlength;
    fExtPathMode = kFALSE;
  } else
    PrintInitError("SetPathlength");
}

//_____________________________________________________________________________
void THaElossCorrection::SetPathlength( const char* vertex_module,
					Double_t z_ref, Double_t scale ) 
{
  // Use variable pathlength for eloss calculations. 
  // The pathlength is calculated for every event as
  //   path = abs(z_vertex - z_ref) * scale

  if( !IsInit() ) {
    fZref       = z_ref;
    fScale      = scale;
    fVertexName = vertex_module;
    fExtPathMode = kTRUE;
  } else
    PrintInitError("SetPathlength");
}

//-----------------------------------------------------------------------
// The following four routines have been taken from ESPACE 
// (file kinematics/eloss.f) and translated from FORTRAN to C++.
// 
// Original comments follow:
//-----------------------------------------------------------------------
//    Copyright (c) 2001 Institut des Sciences Nucleaires de Grenoble
//
//    Authors: J. Mougey, E. Voutier                 (Name@isn.in2p3.fr) 
//-----------------------------------------------------------------------
//
//    WARNING !!
//
//    These routines calculate the energy loss of electrons and hadrons
//    for elemental and compound materials, assuming that the  relevant
//    physics parameters are tabulated. In the contrary, the  tables at
//    the end of the section should be implemented using references:
//
//    M.J. Berger and S.M. Seltzer, National Bureau of Standards Report
//    82-2550A (1983).
//    R.M. Sternheimer, M.J. Berger and S.M. Seltzer,Atomic and Nuclear
//    Data Tables 30 (1984) 261.
//
//-----------------------------------------------------------------------
//
//    These routines are based on the written document  
//
//    ESPACE Energy Loss Corrections Revisited
//    J. Mougey, E89-044 Analysis Progress Report, November 2000.
//
//    which PostScript version can be downloaded from the WebSite
//
//    http://isnwww.in2p3.fr/hadrons/helium3/Anal/AnaPag.html
//
//_____________________________________________________________________________
Double_t THaElossCorrection::ElossElectron( Double_t beta, Double_t z_med,
					    Double_t a_med, Double_t d_med, 
					    Double_t pathlength )
{
  //-----------------------------------------------------------------------
  // Energy loss of electrons taking into account the usual Bethe-Bloch
  // stopping power and the Density Effect Corrections that are important
  // at high electron energy. Additional  shell effects  can  be safely
  // neglected for ultrarelativistic electrons.
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  //
  // Passed variables
  //
  //            beta     electron velocity              (relative to light) 
  //
  //           z_med     effective charge of the medium
  //           a_med     effective atomic mass          (AMU)
  //           d_med     medium density                 (g/cm^3)
  //      pathlength     flight path through medium     (m)
  //
  //  z_med = sum(i)(w(i)*z(i))/sum(i)w(i)
  //            w(i) is the abundacy of element i in the medium
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  //
  // Return value:       Energy loss of electron        (GeV)
  //
  //-----------------------------------------------------------------------


  Double_t BETA2,BETA3,PLAS,EKIN,TAU,FTAU;
  Double_t BETH,DENS,ESTP,EXEN,GAMMA;

  //---- Constant factor corresponding to 2*pi*N_a*(r_e)^2*m_e*c^2
  //     Its units are MeV.cm2/g          

  static const Double_t COEF = 0.1535374581;

  //---- Electron mass (MeV)

  static const Double_t EMAS = 0.5109989020;

  //---- Numerical constants

  static const Double_t log2 = TMath::Log(2.0);

  //---- Input variables consistency check

  if( beta <= 0.0 || beta >= 1.0 || z_med == 0.0 ||
      a_med == 0.0 || pathlength == 0.0 )
    return 0.0;

  pathlength *= 1e2;  // internal units are cm

  BETA2 = beta * beta;
  BETA3 = 1.0 - BETA2;
  GAMMA = 1.0/TMath::Sqrt(BETA3);

  //---- Reduced Bethe-Bloch stopping power

  EXEN = ExEnerg(z_med,d_med);
  if( EXEN == 0.0 )
    return 0.0;

  EKIN = EMAS * ( GAMMA - 1.0 );
  TAU  = EKIN / EMAS;
  FTAU = 1.0+(TAU*TAU/8.0)-((2.0*TAU+1.0)*log2);
  BETH = 2.0 * TMath::Log(1e6*EKIN/EXEN) + BETA3 * FTAU;
  BETH = BETH + TMath::Log(1.0+(0.5*TAU));

  //---- Reduced density correction

  PLAS = 28.8084 * TMath::Sqrt(d_med*z_med/a_med);
  DENS = 2.0*(TMath::Log(PLAS)+TMath::Log(beta*GAMMA)-TMath::Log(EXEN))-1.0;
  if(DENS < 0.0) DENS = 0.0;

  //---- Total electron stopping power

  ESTP = COEF * z_med * ( BETH - DENS ) / a_med / BETA2;

  //---- Electron energy loss

  Double_t eloss = ESTP * d_med * pathlength * 1e-3; // GeV

  return eloss;
}

//_____________________________________________________________________________
Double_t THaElossCorrection:: ElossHadron( Int_t Z_hadron, Double_t beta, 
					   Double_t z_med, Double_t a_med, 
					   Double_t d_med, 
					   Double_t pathlength )
{
  //-----------------------------------------------------------------------
  //
  // Energy loss of hadrons taking into account the  usual  Bethe-Bloch
  // stopping power + the Density Effect Corrections (important at high
  // energy) + the Shell Corrections (important at small energy). 
  // The approximation 2 * gamma * m_e / M << 1 is used for the  Bethe-
  // Bloch formulae: for a 4 GeV/c pion, this leads to an effect about
  // 0.5-0.7 % on the total stopping power.
  // 
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  //
  // Passed variables
  //
  //        Z_hadron     hadron charge 
  //            beta     hadron velocity                (relative to light) 
  //
  //           z_med     effective charge of the medium
  //           a_med     effective atomic mass          (AMU)
  //           d_med     medium density                 (g/cm^3)
  //      pathlength     flight path through medium     (m)
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  //
  // Return value
  //
  //                     Energy loss of hadron          (GeV)
  //
  //-----------------------------------------------------------------------


  Double_t BETA2,GAMA2,GAMA,PLAS,ETA,ETA2,ETA4,ETA6;
  Double_t BETH,DENS,SHE0,SHE1,SHEL,HSTP;
  Double_t X,X0,X1,M,EXEN,C,A;

  //---- Constant factor corresponding to 4*pi*N_a*(r_e)^2*m_e*c^2
  //     Its units are MeV.cm2/g          

  static const Double_t COEF = 0.3070749162;

  //---- Electron mass (MeV)

  static const Double_t EMAS = 0.5109989020;

  //---- Numerical constants

  static const Double_t log100 = TMath::Log(1e2);

  //---- Input variables consistency check

  if( Z_hadron == 0 || beta <= 0.0 || beta >= 1.0 || z_med == 0.0 ||
      a_med == 0.0 || pathlength == 0.0 )
    return 0.0;

  pathlength *= 1e2;  // internal units are cm

  BETA2 = beta * beta;
  GAMA2 = 1.0 / (1.0 - BETA2);
  GAMA  = TMath::Sqrt( GAMA2 );
  ETA  = beta * GAMA ;

  //---- Reduced Bethe-Bloch stopping power

  EXEN = ExEnerg(z_med,d_med);  // in eV!
  //-    Tabulated material check
  if( EXEN == 0.0 ) 
    return 0.0;

  BETH = TMath::Log(2e6*EMAS*BETA2*GAMA2/EXEN) - BETA2;

  //---- Reduced density correction

  PLAS = 28.8084 * TMath::Sqrt(d_med*z_med/a_med);
  C = 2.0*TMath::Log(PLAS) - 2.0*TMath::Log(EXEN) - 1.0;
  X = TMath::Log10(ETA);
  HaDensi(z_med,d_med,X0,X1,M);
  //-    Tabulated density consistency
  if((X0+X1+M) == 0.0)
    return 0.0;
  A = -1.0 * ( C + log100*X0 ) / TMath::Power(X1-X0,M);
  if(X<X0)
    DENS = 0.0;
  else if(X<X1)
    DENS = log100*X + C + A*TMath::Power(X1-X,M);
  else
    DENS = log100*X + C;
  DENS *= 0.5;

  ETA2 = 1.0 / (ETA*ETA) ;
  ETA4 = ETA2 * ETA2 ;
  ETA6 = ETA4 * ETA2 ;

  //---- Reduced shell correction

  SHE0 = 4.2237e-1*ETA2 + 3.040e-2*ETA4 - 3.80e-4*ETA6;
  SHE1 = 3.8580e0 *ETA2 - 1.668e-1*ETA4 + 1.58e-3*ETA6;
  SHEL = 1e-6 * EXEN * EXEN * (SHE0 + 1e-3*SHE1*EXEN) / z_med;

  //---- Total hadron stopping power
   
  HSTP = COEF * z_med * Double_t(Z_hadron*Z_hadron) / a_med;
  HSTP = HSTP * ( BETH - DENS - SHEL ) / BETA2;

  //---- Electron energy loss

  Double_t eloss = HSTP * d_med * pathlength * 1e-3; // in GeV

  return eloss;
}

//_____________________________________________________________________________
Double_t THaElossCorrection::ExEnerg( Double_t z_med, Double_t d_med )
{

  Double_t EXEN;

//---- Some Z=1 isotopes

  if( TMath::Abs(z_med-1.0)<0.5) {
    //-    Gaseous hydrogen
    if(d_med<0.01)
      EXEN = 19.2;
    //-    Liquid Hydrogen
    //    else if(d_med<0.1)
    //      EXEN = 21.8;
    //---- Liquid Deuterium
    else
      EXEN = 21.8;
  }
  //---- Some Z=2 isotopes

  //-    Gaseous/Liquid Helium

  else if( TMath::Abs(z_med-2.0)<0.1 )
    EXEN = 41.8;

  //---- Plastic scintillator (Polyvinyltolulene 2-CH[3]C[6]H[4]CH=CH[2])
  //     
  //     z_eff = 3.36842    a_eff =  6.21996    d_med = 1.03200

  else if( TMath::Abs(z_med-3.37)<0.1 )
    EXEN = 64.7;

  //---- Kapton (Polymide film C[22]H[10]N[2]O[5])
  //     
  //     z_eff = 5.02564    a_eff =  9.80345    d_med = 1.42000

  else if( TMath::Abs(z_med-5.03)<0.1 )
    EXEN = 79.6;

  //---- Some Z=6 isotopes

  else if( TMath::Abs(z_med-6.0)<0.1 )
    EXEN = 78.0;

  //---- Air (dry, near sea level, 78% N2 + 22% O2)
  //     
  //     z_eff = 7.22000    a_eff = 14.46343    d_med = 1.20480E-03

  else if( TMath::Abs(z_med-7.22)<0.1 )
    EXEN = 85.7;

//---- Aluminum

  else if( TMath::Abs(z_med-13.0)<0.1 )
    EXEN = 166.0;

//---- Copper

  else if( TMath::Abs(z_med-29.0)<0.1 )
    EXEN = 322.0;

//---- Titanium

  else if( TMath::Abs(z_med-22.0)<0.1 )
    EXEN = 233.0;

//---- Table overflow

  else {
    cout << endl;
    cout << "Warning... Unknown selected material !!" << endl;
    cout << "z_med = " << z_med << " d_med = " << d_med << endl;
    cout << "Implement THaElossCorrection::ExEnerg routine for proper use." 
	 << endl << endl;
    EXEN = 0.0;
  }

  return EXEN;
}

//_____________________________________________________________________________
void THaElossCorrection::HaDensi( Double_t z_med, Double_t d_med,
				  Double_t& X0, Double_t& X1, Double_t& M )
{

  //---- Some Z=1 isotopes

  if( TMath::Abs( z_med-1.0 ) < 0.1 ) {
    //-    Gaseous hydrogen
    if( d_med < 0.01 ) {
      X0 =  1.8639;
      X1 =  3.2718;
      M  =  5.7273;
    }
    //-    Liquid Hydrogen
    else if( d_med < 0.1 ) {
      X0 =  0.4759;
      X1 =  1.9215;
      M  =  5.6249;
    }
    //---- Liquid Deuterium
    else if( d_med >= 0.1 ) {
      X0 =  0.4759;
      X1 =  1.9215;
      M  =  5.6249;
    }
  }
  //---- Some Z=2 isotopes

  else if( TMath::Abs( z_med-2.0 ) < 0.1 ) {
    //-    Gaseous/Liquid Helium
    X0 =   2.2017;
    X1 =   3.6122;
    M  =   5.8347;
  }

  //---- Plastic scintillator (Polyvinyltolulene 2-CH[3]C[6]H[4]CH=CH[2])
  //     
  //     z_eff = 3.36842    a_eff =  6.21996    d_med = 1.03200

  else if( TMath::Abs( z_med-3.37 ) < 0.1 ) {
    X0 =  0.1464;
    X1 =  2.4855;
    M  =  3.2393;
  }

  //---- Kapton (Polymide film C[22]H[10]N[2]O[5])
  //     
  //     z_eff = 5.02564    a_eff =  9.80345    d_med = 1.42000

  else if( TMath::Abs( z_med-5.03 ) < 0.1 ) {
    X0 =  0.1509;
    X1 =  2.5631;
    M  =  3.1921;
  }

  //---- Some Z=6 isotopes

  else if( TMath::Abs( z_med-6.0 ) < 0.1 ) {
    //-    Carbon (graphite, density 1.700 g/cm3)
    if( d_med < 1.750) {
      X0 =  0.0480;
      X1 =  2.5387;
      M  =  2.9532;
    }
    //-    Carbon (graphite, density 2.000 g/cm3)
    else if( d_med < 2.050) {
      X0 = -0.0351;
      X1 =  2.4860;
      M  =  3.0036;
    }
    //-    Carbon (graphite, density 2.265 g/cm3)
    else if( d_med < 2.270) {
      X0 = -0.0178;
      X1 =  2.3415;
      M  =  2.8697;
    }
  }

//---- Air (dry, near sea level, 78% N2 + 22% O2)
//     
//     z_eff = 7.22000    a_eff = 14.46343    d_med = 1.20480E-03

  else if( TMath::Abs( z_med-7.22 ) < 0.1 ) {
    X0 =   1.7418;
    X1 =   4.2759;
    M  =   3.3994;
  }

//---- Aluminum

  else if( TMath::Abs( z_med-13.0 ) < 0.1 ) {
    X0 =  0.1708;
    X1 =  3.0127;
    M  =  3.6345;
  }

//---- Titanium

  else if( TMath::Abs( z_med-22.0 ) < 0.1 ) {
    X0 =  0.0957;
    X1 =  3.0386;
    M  =  3.0302;
  }

//---- Table overflow

  else {
    cout << endl;
    cout << "Warning... Inconsistent material density... " << endl;
    cout << "z_med = " << z_med << " d_med = " << d_med << endl;
    cout << "Implement THaElossCorrection::HaDensi routine for proper use."
	 << endl << endl;
    X0 =  0.;
    X1 =  0.;
    M  =  0.;
  }
     
  return;
}

//_____________________________________________________________________________
ClassImp(THaElossCorrection)

