//*-- Author :    Ole Hansen     8-Oct-2001

////////////////////////////////////////////////////////////////////////
//
// THaRawEvent
//
// This is an example of an event structure. It contains raw data
// and coarse tracking information for a number of detectors in
// the HRS-Left, HRS-Right, and Beamline.
// 
// NOTE: Output generated with this event object will be LARGE.
//
// This class will be obsoleted in v0.80.
//
////////////////////////////////////////////////////////////////////////

#include "THaRawEvent.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TMath.h"

ClassImp(THaRawEvent)

//______________________________________________________________________________
  THaRawEvent::THaRawEvent() : THaEvent(), fMaxhit(50), fMaxclu(10), fMaxtrk(5)
{
  // Create a THaRawEvent object.

  SetupDatamap( kAll );
  Clear();
}

//______________________________________________________________________________
THaRawEvent::~THaRawEvent()
{
  // Destructor. Clean up all my objects.

  // Note fDataMap is deleted by the base class destructor!

  DeleteVariableArrays( kAll );
}

//______________________________________________________________________________
void THaRawEvent::Clear( Option_t* opt )
{
  // Reset all contained data members to zero, i.e. scalars and
  // fixed-size arrays, but not variable size arrays since those
  // are simply reset by setting their dimension variable to zero.

  // Clear the base class (i.e. histograms & event header)
  THaEvent::Clear( opt );

  size_t len = reinterpret_cast<const char*>( &fL_S2_try )
    + sizeof( fL_S2_try )
    - reinterpret_cast<const char*>( &fB_X4a );
  memset( fB_X4a, 0, len );

}

//______________________________________________________________________________
void THaRawEvent::CreateVariableArrays( EBlock which )
{
  // Set up the variable-size arrays for the VDCs. 
  // Required heap memory ~4kB with the defaults fMaxhit=50, fMaxclu=10.


  if( which == kHits || which == kAll ) {
    // Right HRS VDC
    fR_U1_wire  = new Int_t   [ fMaxhit ];
    fR_U1_time  = new Double_t[ fMaxhit ];
    fR_V1_wire  = new Int_t   [ fMaxhit ];
    fR_V1_time  = new Double_t[ fMaxhit ];
    fR_U2_wire  = new Int_t   [ fMaxhit ];
    fR_U2_time  = new Double_t[ fMaxhit ];
    fR_V2_wire  = new Int_t   [ fMaxhit ];
    fR_V2_time  = new Double_t[ fMaxhit ];
    // Left HRS VDC
    fL_U1_wire  = new Int_t   [ fMaxhit ];
    fL_U1_time  = new Double_t[ fMaxhit ];
    fL_V1_wire  = new Int_t   [ fMaxhit ];
    fL_V1_time  = new Double_t[ fMaxhit ];
    fL_U2_wire  = new Int_t   [ fMaxhit ];
    fL_U2_time  = new Double_t[ fMaxhit ];
    fL_V2_wire  = new Int_t   [ fMaxhit ];
    fL_V2_time  = new Double_t[ fMaxhit ];
  }
  if( which == kClusters || which == kAll ) {
    // Right HRS VDC
    fR_U1_clpos = new Double_t[ fMaxclu ];
    fR_U1_clsiz = new Int_t   [ fMaxclu ];
    fR_V1_clpos = new Double_t[ fMaxclu ];
    fR_V1_clsiz = new Int_t   [ fMaxclu ];
    fR_U2_clpos = new Double_t[ fMaxclu ];
    fR_U2_clsiz = new Int_t   [ fMaxclu ];
    fR_V2_clpos = new Double_t[ fMaxclu ];
    fR_V2_clsiz = new Int_t   [ fMaxclu ];
    // Left HRS VDC
    fL_U1_clpos = new Double_t[ fMaxclu ];
    fL_U1_clsiz = new Int_t   [ fMaxclu ];
    fL_V1_clpos = new Double_t[ fMaxclu ];
    fL_V1_clsiz = new Int_t   [ fMaxclu ];
    fL_U2_clpos = new Double_t[ fMaxclu ];
    fL_U2_clsiz = new Int_t   [ fMaxclu ];
    fL_V2_clpos = new Double_t[ fMaxclu ];
    fL_V2_clsiz = new Int_t   [ fMaxclu ];
  }
  if( which == kTracks || which == kAll ) {
    // Right HRS VDC
    fR_TR_x     = new Double_t[ fMaxtrk ];
    fR_TR_y     = new Double_t[ fMaxtrk ];
    fR_TR_th    = new Double_t[ fMaxtrk ];
    fR_TR_ph    = new Double_t[ fMaxtrk ];
    fR_TR_p     = new Double_t[ fMaxtrk ];
    // Left HRS VDC
    fL_TR_x     = new Double_t[ fMaxtrk ];
    fL_TR_y     = new Double_t[ fMaxtrk ];
    fL_TR_th    = new Double_t[ fMaxtrk ];
    fL_TR_ph    = new Double_t[ fMaxtrk ];
    fL_TR_p     = new Double_t[ fMaxtrk ];
  }
}

//______________________________________________________________________________
void THaRawEvent::DeleteVariableArrays( EBlock which )
{
  // Delete variable arrays.

  if( which == kHits || which == kAll ) {
    // Right HRS VDC
    delete [] fR_U1_wire ;
    delete [] fR_U1_time ;
    delete [] fR_V1_wire ;
    delete [] fR_V1_time ;
    delete [] fR_U2_wire ;
    delete [] fR_U2_time ;
    delete [] fR_V2_wire ;
    delete [] fR_V2_time ;
    // Left HRS VDC
    delete [] fL_U1_wire ;
    delete [] fL_U1_time ;
    delete [] fL_V1_wire ;
    delete [] fL_V1_time ;
    delete [] fL_U2_wire ;
    delete [] fL_U2_time ;
    delete [] fL_V2_wire ;
    delete [] fL_V2_time ;
  }
  if( which == kClusters || which == kAll ) {
    // Right HRS VDC
    delete [] fR_U1_clpos;
    delete [] fR_U1_clsiz;
    delete [] fR_V1_clpos;
    delete [] fR_V1_clsiz;
    delete [] fR_U2_clpos;
    delete [] fR_U2_clsiz;
    delete [] fR_V2_clpos;
    delete [] fR_V2_clsiz;
    // Left HRS VDC
    delete [] fL_U1_clpos;
    delete [] fL_U1_clsiz;
    delete [] fL_V1_clpos;
    delete [] fL_V1_clsiz;
    delete [] fL_U2_clpos;
    delete [] fL_U2_clsiz;
    delete [] fL_V2_clpos;
    delete [] fL_V2_clsiz;
  }
  if( which == kTracks || which == kAll ) {
    // Right HRS VDC
    delete [] fR_TR_x;
    delete [] fR_TR_y;
    delete [] fR_TR_th;
    delete [] fR_TR_ph;
    delete [] fR_TR_p;
    // Left HRS VDC
    delete [] fL_TR_x;
    delete [] fL_TR_y;
    delete [] fL_TR_th;
    delete [] fL_TR_ph;
    delete [] fL_TR_p;
  }
}

//______________________________________________________________________________
Int_t THaRawEvent::Fill()
{
  // Custom Fill() method: Check first if the maximum sizes of any of
  // the variable arrays are exceeded.  If so, recreate the arrays,
  // the datamap, and reinitialize everything. Otherwise, just call
  // the standard THaEvent::Fill().

  if( !fInit )
    InitCounters();

  Int_t maxhit = -1, maxclu = -1, maxtrk = -1, n;
  for( int i = 0; i < NVDC; i++ ) {
    if( fNhitVar[i] && (n = static_cast<Int_t>(fNhitVar[i]->GetValue()))
	> maxhit )
      maxhit = n;
    if( fNcluVar[i] && (n = static_cast<Int_t>(fNcluVar[i]->GetValue()))
	> maxclu )
      maxclu = n;
  }
  for( int i = 0; i < 2; i++ ) {
    if( fNtrkVar[i] && (n = static_cast<Int_t>(fNtrkVar[i]->GetValue()))
	> maxtrk )
      maxtrk = n;
  }

  if( maxhit > fMaxhit ) {
    DeleteVariableArrays( kHits );
    fMaxhit = TMath::Max( 2*fMaxhit, maxhit );
    SetupDatamap( kHits );
  }
  if( maxclu > fMaxclu ) {
    DeleteVariableArrays( kClusters );
    fMaxclu = TMath::Max( 2*fMaxclu, maxclu );
    SetupDatamap( kClusters );
  }
  if( maxtrk > fMaxtrk ) {
    DeleteVariableArrays( kTracks );
    fMaxtrk = TMath::Max( 2*fMaxtrk, maxtrk );
    SetupDatamap( kTracks );
  }

  return THaEvent::Fill();
}

//______________________________________________________________________________
void THaRawEvent::InitCounters()
{
  // Initialize global variables holding variable array sizes.

  fNhitVar[0] = gHaVars->Find( "R.vdc.u1.nhit" );
  fNhitVar[1] = gHaVars->Find( "R.vdc.v1.nhit" );
  fNhitVar[2] = gHaVars->Find( "R.vdc.u2.nhit" );
  fNhitVar[3] = gHaVars->Find( "R.vdc.v2.nhit" );
  fNhitVar[4] = gHaVars->Find( "L.vdc.u1.nhit" );
  fNhitVar[5] = gHaVars->Find( "L.vdc.v1.nhit" );
  fNhitVar[6] = gHaVars->Find( "L.vdc.u2.nhit" );
  fNhitVar[7] = gHaVars->Find( "L.vdc.v2.nhit" );
  fNcluVar[0] = gHaVars->Find( "R.vdc.u1.nclust" );
  fNcluVar[1] = gHaVars->Find( "R.vdc.v1.nclust" );
  fNcluVar[2] = gHaVars->Find( "R.vdc.u2.nclust" );
  fNcluVar[3] = gHaVars->Find( "R.vdc.v2.nclust" );
  fNcluVar[4] = gHaVars->Find( "L.vdc.u1.nclust" );
  fNcluVar[5] = gHaVars->Find( "L.vdc.v1.nclust" );
  fNcluVar[6] = gHaVars->Find( "L.vdc.u2.nclust" );
  fNcluVar[7] = gHaVars->Find( "L.vdc.v2.nclust" );
  fNtrkVar[0] = gHaVars->Find( "R.tr.n" );
  fNtrkVar[1] = gHaVars->Find( "L.tr.n" );
}

//______________________________________________________________________________
void THaRawEvent::SetupDatamap( EBlock which )
{
  // Set up the data map, which relates global variable names to
  // THaRawEvent member variables.

  CreateVariableArrays( which );

  // Note: Before you freak out here, recall that string literals
  // are always of storage class "static". So, the pointers to
  // const char* are not dangling when datamap goes out of scope.

  const DataMap datamap[] = {

    //=== Beamline

    { 2*NBPM, "B.bpm4a.x",         fB_X4a },
    { 2*NBPM, "B.bpm4b.x",         fB_X4b },
    { 4,      "B.rast.Xcur",       &fB_Xcur },

    //=== Right HRS

    // VDC
    { 1,      "R.vdc.u1.nhit",     &fR_U1_nhit },
    {-1,      "R.vdc.u1.wire",     fR_U1_wire  },
    {-1,      "R.vdc.u1.time",     fR_U1_time, },
    { 1,      "R.vdc.u1.nclust",   &fR_U1_nclust },
    {-1,      "R.vdc.u1.clpos",    fR_U1_clpos },
    {-1,      "R.vdc.u1.clsiz",    fR_U1_clsiz },
    { 1,      "R.vdc.v1.nhit",     &fR_V1_nhit },
    {-1,      "R.vdc.v1.wire",     fR_V1_wire  },
    {-1,      "R.vdc.v1.time",     fR_V1_time  },
    { 1,      "R.vdc.v1.nclust",   &fR_V1_nclust },
    {-1,      "R.vdc.v1.clpos",    fR_V1_clpos },
    {-1,      "R.vdc.v1.clsiz",    fR_V1_clsiz },
    { 1,      "R.vdc.u2.nhit",     &fR_U2_nhit },
    {-1,      "R.vdc.u2.wire",     fR_U2_wire  },
    {-1,      "R.vdc.u2.time",     fR_U2_time  },
    { 1,      "R.vdc.u2.nclust",   &fR_U2_nclust },
    {-1,      "R.vdc.u2.clpos",    fR_U2_clpos },
    {-1,      "R.vdc.u2.clsiz",    fR_U2_clsiz },
    { 1,      "R.vdc.v2.nhit",     &fR_V2_nhit },
    {-1,      "R.vdc.v2.wire",     fR_V2_wire  },
    {-1,      "R.vdc.v2.time",     fR_V2_time  },
    { 1,      "R.vdc.v2.nclust",   &fR_V2_nclust },
    {-1,      "R.vdc.v2.clpos",    fR_V2_clpos },
    {-1,      "R.vdc.v2.clsiz",    fR_V2_clsiz },
    { 1,      "R.tr.n",            &fR_TR_n },
    {-1,      "R.tr.x",            fR_TR_x },
    {-1,      "R.tr.y",            fR_TR_y },
    {-1,      "R.tr.th",           fR_TR_th },
    {-1,      "R.tr.ph",           fR_TR_ph },
    {-1,      "R.tr.p",            fR_TR_p },
    
    // S1
    { 1,      "R.s1.nlthit",       &fR_S1L_nthit },
    { -1,     "R.s1.lt",           fR_S1L_tdc },
    { -1,     "R.s1.lt_c",         fR_S1L_tdc_c },
    { 1,      "R.s1.nrthit",       &fR_S1R_nthit },
    { -1,     "R.s1.rt",           fR_S1R_tdc },
    { -1,     "R.s1.rt_c",         fR_S1R_tdc_c },
    { 1,      "R.s1.nlahit",       &fR_S1L_nahit },
    { -1,     "R.s1.la",           fR_S1L_adc },
    { -1,     "R.s1.la_p",         fR_S1L_adc_p },
    { -1,     "R.s1.la_c",         fR_S1L_adc_c },
    { 1,      "R.s1.nrahit",       &fR_S1R_nahit },
    { -1,     "R.s1.ra",           fR_S1R_adc },
    { -1,     "R.s1.ra_p",         fR_S1R_adc_p },
    { -1,     "R.s1.ra_c",         fR_S1R_adc_c },
    { 1,      "R.s1.trx",          &fR_S1_trx },   
    { 1,      "R.s1.try",          &fR_S1_try },

    // S2
    { 1,      "R.s2.nlthit",       &fR_S2L_nthit },
    { -1,     "R.s2.lt",           fR_S2L_tdc },
    { -1,     "R.s2.lt_c",         fR_S2L_tdc_c },
    { 1,      "R.s2.nrthit",       &fR_S2R_nthit },
    { -1,     "R.s2.rt",           fR_S2R_tdc },
    { -1,     "R.s2.rt_c",         fR_S2R_tdc_c },
    { 1,      "R.s2.nlahit",       &fR_S2L_nahit },
    { -1,     "R.s2.la",           fR_S2L_adc },
    { -1,     "R.s2.la_p",         fR_S2L_adc_p },
    { -1,     "R.s2.la_c",         fR_S2L_adc_c },
    { 1,      "R.s2.nrahit",       &fR_S2R_nahit },
    { -1,     "R.s2.ra",           fR_S2R_adc },
    { -1,     "R.s2.ra_p",         fR_S2R_adc_p },
    { -1,     "R.s2.ra_c",         fR_S2R_adc_c },
    { 1,      "R.s2.trx",          &fR_S2_trx },   
    { 1,      "R.s2.try",          &fR_S2_try },

    // Aerogel
    { 1,      "R.aero1.nthit",     &fR_AR_nthit },
    { -1,     "R.aero1.t",         fR_AR_tdc },
    { -1,     "R.aero1.t_c",       fR_AR_tdc_c },
    { 1,      "R.aero1.nahit",     &fR_AR_nahit },
    { -1,     "R.aero1.a",         fR_AR_adc },
    { -1,     "R.aero1.a_p",       fR_AR_adc_p },
    { -1,     "R.aero1.a_c",       fR_AR_adc_c },
    { 1,      "R.aero1.asum_p",    &fR_AR_asum_p },
    { 1,      "R.aero1.asum_c",    &fR_AR_asum_c },
    { 1,      "R.aero1.trx",       &fR_AR_trx },   
    { 1,      "R.aero1.try",       &fR_AR_try },

    // Cherenkov
    { 1,      "R.cer.nthit",       &fR_CH_nthit },
    { -1,     "R.cer.t",           fR_CH_tdc },
    { -1,     "R.cer.t_c",         fR_CH_tdc_c },
    { 1,      "R.cer.nahit",       &fR_CH_nahit },
    { -1,     "R.cer.a",           fR_CH_adc },
    { -1,     "R.cer.a_p",         fR_CH_adc_p },
    { -1,     "R.cer.a_c",         fR_CH_adc_c },
    { 1,      "R.cer.asum_p",      &fR_CH_asum_p },
    { 1,      "R.cer.asum_c",      &fR_CH_asum_c },
    { 1,      "R.cer.trx",         &fR_CH_trx },   
    { 1,      "R.cer.try",         &fR_CH_try },

    // Preshower
    { 1,      "R.ps.nhit",         &fR_PSH_nhit },
    { -1,     "R.ps.a",            fR_PSH_adc },
    { -1,     "R.ps.a_p",          fR_PSH_adc_p },
    { -1,     "R.ps.a_c",          fR_PSH_adc_c },
    { 1,      "R.ps.asum_p",       &fR_PSH_asum_p },
    { 1,      "R.ps.asum_c",       &fR_PSH_asum_c },
    { 1,      "R.ps.nclust",       &fR_PSH_nclust },
    { 1,      "R.ps.e",            &fR_PSH_e },
    { 1,      "R.ps.x",            &fR_PSH_x },
    { 1,      "R.ps.y",            &fR_PSH_y },
    { 1,      "R.ps.mult",         &fR_PSH_mult },
    { 6,      "R.ps.nblk",         fR_PSH_nblk },
    { 6,      "R.ps.eblk",         fR_PSH_eblk },
    { 1,      "R.ps.trx",          &fR_PSH_trx },   
    { 1,      "R.ps.try",          &fR_PSH_try },

    // Shower
    { 1,      "R.sh.nhit",         &fR_SHR_nhit },
    { -1,     "R.sh.a",            fR_SHR_adc },
    { -1,     "R.sh.a_p",          fR_SHR_adc_p },
    { -1,     "R.sh.a_c",          fR_SHR_adc_c },
    { 1,      "R.sh.asum_p",       &fR_SHR_asum_p },
    { 1,      "R.sh.asum_c",       &fR_SHR_asum_c },
    { 1,      "R.sh.nclust",       &fR_SHR_nclust },
    { 1,      "R.sh.e",            &fR_SHR_e },
    { 1,      "R.sh.x",            &fR_SHR_x },
    { 1,      "R.sh.y",            &fR_SHR_y },
    { 1,      "R.sh.mult",         &fR_SHR_mult },
    { 9,      "R.sh.nblk",         fR_SHR_nblk },
    { 9,      "R.sh.eblk",         fR_SHR_eblk },
    { 1,      "R.sh.trx",          &fR_SHR_trx },   
    { 1,      "R.sh.try",          &fR_SHR_try },

    // Total Shower
    { 1,      "R.ts.e",            &fR_TSH_e },
    { 1,      "R.ts.id",           &fR_TSH_id },

    //=== Left HRS

    // VDC
    { 1,      "L.vdc.u1.nhit",     &fL_U1_nhit },
    {-1,      "L.vdc.u1.wire",     fL_U1_wire },
    {-1,      "L.vdc.u1.time",     fL_U1_time },
    { 1,      "L.vdc.u1.nclust",   &fL_U1_nclust },
    {-1,      "L.vdc.u1.clpos",    fL_U1_clpos },
    {-1,      "L.vdc.u1.clsiz",    fL_U1_clsiz },
    { 1,      "L.vdc.v1.nhit",     &fL_V1_nhit },
    {-1,      "L.vdc.v1.wire",     fL_V1_wire },
    {-1,      "L.vdc.v1.time",     fL_V1_time },
    { 1,      "L.vdc.v1.nclust",   &fL_V1_nclust },
    {-1,      "L.vdc.v1.clpos",    fL_V1_clpos },
    {-1,      "L.vdc.v1.clsiz",    fL_V1_clsiz },
    { 1,      "L.vdc.u2.nhit",     &fL_U2_nhit },
    {-1,      "L.vdc.u2.wire",     fL_U2_wire },
    {-1,      "L.vdc.u2.time",     fL_U2_time },
    { 1,      "L.vdc.u2.nclust",   &fL_U2_nclust },
    {-1,      "L.vdc.u2.clpos",    fL_U2_clpos },
    {-1,      "L.vdc.u2.clsiz",    fL_U2_clsiz },
    { 1,      "L.vdc.v2.nhit",     &fL_V2_nhit },
    {-1,      "L.vdc.v2.wire",     fL_V2_wire },
    {-1,      "L.vdc.v2.time",     fL_V2_time },
    { 1,      "L.vdc.v2.nclust",   &fL_V2_nclust },
    {-1,      "L.vdc.v2.clpos",    fL_V2_clpos },
    {-1,      "L.vdc.v2.clsiz",    fL_V2_clsiz },
    { 1,      "L.tr.n",            &fL_TR_n },
    {-1,      "L.tr.x",            fL_TR_x },
    {-1,      "L.tr.y",            fL_TR_y },
    {-1,      "L.tr.th",           fL_TR_th },
    {-1,      "L.tr.ph",           fL_TR_ph },
    {-1,      "L.tr.p",            fL_TR_p },

    // S1
    { 1,      "L.s1.nlthit",       &fL_S1L_nthit },
    { -1,     "L.s1.lt",           fL_S1L_tdc },
    { -1,     "L.s1.lt_c",         fL_S1L_tdc_c },
    { 1,      "L.s1.nrthit",       &fL_S1R_nthit },
    { -1,     "L.s1.rt",           fL_S1R_tdc },
    { -1,     "L.s1.rt_c",         fL_S1R_tdc_c },
    { 1,      "L.s1.nlahit",       &fL_S1L_nahit },
    { -1,     "L.s1.la",           fL_S1L_adc },
    { -1,     "L.s1.la_p",         fL_S1L_adc_p },
    { -1,     "L.s1.la_c",         fL_S1L_adc_c },
    { 1,      "L.s1.nrahit",       &fL_S1R_nahit },
    { -1,     "L.s1.ra",           fL_S1R_adc },
    { -1,     "L.s1.ra_p",         fL_S1R_adc_p },
    { -1,     "L.s1.ra_c",         fL_S1R_adc_c },
    { 1,      "L.s1.trx",          &fL_S1_trx },   
    { 1,      "L.s1.try",          &fL_S1_try },

    // S2
    { 1,      "L.s2.nlthit",       &fL_S2L_nthit },
    { -1,     "L.s2.lt",           fL_S2L_tdc },
    { -1,     "L.s2.lt_c",         fL_S2L_tdc_c },
    { 1,      "L.s2.nrthit",       &fL_S2R_nthit },
    { -1,     "L.s2.rt",           fL_S2R_tdc },
    { -1,     "L.s2.rt_c",         fL_S2R_tdc_c },
    { 1,      "L.s2.nlahit",       &fL_S2L_nahit },
    { -1,     "L.s2.la",           fL_S2L_adc },
    { -1,     "L.s2.la_p",         fL_S2L_adc_p },
    { -1,     "L.s2.la_c",         fL_S2L_adc_c },
    { 1,      "L.s2.nrahit",       &fL_S2R_nahit },
    { -1,     "L.s2.ra",           fL_S2R_adc },
    { -1,     "L.s2.ra_p",         fL_S2R_adc_p },
    { -1,     "L.s2.ra_c",         fL_S2R_adc_c },
    { 1,      "L.s2.trx",          &fL_S2_trx },   
    { 1,      "L.s2.try",          &fL_S2_try },

    { 0 }  //End of data
  };

  // Save the datamap
  int nvar = sizeof(datamap)/sizeof(DataMap);
  delete [] fDataMap;
  fDataMap = new DataMap[ nvar ];
  memcpy( fDataMap, datamap, sizeof(datamap) );

  fInit = kFALSE;
}
