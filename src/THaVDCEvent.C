//*-- Author :    Ole Hansen     12-Nov-2002

////////////////////////////////////////////////////////////////////////
//
// THaVDCEvent
//
// Event containing VDC information in full detail, but nothing else.
//
// This class will be obsoleted in v0.80.
//
////////////////////////////////////////////////////////////////////////

#include "THaVDCEvent.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TMath.h"

ClassImp(THaVDCEvent)

//______________________________________________________________________________
THaVDCEvent::THaVDCEvent() : THaEvent(), fMaxhit(50), fMaxclu(10), fMaxtrk(5)
{
  // Create a THaVDCEvent object.

  SetupDatamap( kAll );
  Clear();
}

//______________________________________________________________________________
THaVDCEvent::~THaVDCEvent()
{
  // Destructor. Clean up all my objects.

  // Note fDataMap is deleted by the base class destructor!

  DeleteVariableArrays( kAll );
}

//______________________________________________________________________________
void THaVDCEvent::Clear( Option_t* opt )
{
  // Reset all contained data members to zero, i.e. scalars and
  // fixed-size arrays, but not variable size arrays since those
  // are simply reset by setting their dimension variable to zero.

  // Clear the base class (i.e. histograms & event header)
  THaEvent::Clear( opt );

  size_t len = reinterpret_cast<const char*>( &fL_TR_n )
    + sizeof( fL_TR_n )
    - reinterpret_cast<const char*>( &fR_U1_nhit );
  memset( &fR_U1_nhit, 0, len );

}

//______________________________________________________________________________
void THaVDCEvent::CreateVariableArrays( EBlock which )
{
  // Set up the variable-size arrays for the VDCs. 
  // Required heap memory ~4kB with the defaults fMaxhit=50, fMaxclu=10.


  if( which == kHits || which == kAll ) {
    // Right HRS VDC
    fR_U1_wire    = new Int_t   [ fMaxhit ];
    fR_U1_rawtime = new Int_t   [ fMaxhit ];
    fR_U1_time    = new Double_t[ fMaxhit ];
    fR_U1_dist    = new Double_t[ fMaxhit ];
    fR_V1_wire    = new Int_t   [ fMaxhit ];
    fR_V1_rawtime = new Int_t   [ fMaxhit ];
    fR_V1_time    = new Double_t[ fMaxhit ];
    fR_V1_dist    = new Double_t[ fMaxhit ];
    fR_U2_wire    = new Int_t   [ fMaxhit ];
    fR_U2_rawtime = new Int_t   [ fMaxhit ];
    fR_U2_time    = new Double_t[ fMaxhit ];
    fR_U2_dist    = new Double_t[ fMaxhit ];
    fR_V2_wire    = new Int_t   [ fMaxhit ];
    fR_V2_rawtime = new Int_t   [ fMaxhit ];
    fR_V2_time    = new Double_t[ fMaxhit ];
    fR_V2_dist    = new Double_t[ fMaxhit ];
    // Left HRS VDC
    fL_U1_wire    = new Int_t   [ fMaxhit ];
    fL_U1_rawtime = new Int_t   [ fMaxhit ];
    fL_U1_time    = new Double_t[ fMaxhit ];
    fL_U1_dist    = new Double_t[ fMaxhit ];
    fL_V1_wire    = new Int_t   [ fMaxhit ];
    fL_V1_rawtime = new Int_t   [ fMaxhit ];
    fL_V1_time    = new Double_t[ fMaxhit ];
    fL_V1_dist    = new Double_t[ fMaxhit ];
    fL_U2_wire    = new Int_t   [ fMaxhit ];
    fL_U2_rawtime = new Int_t   [ fMaxhit ];
    fL_U2_time    = new Double_t[ fMaxhit ];
    fL_U2_dist    = new Double_t[ fMaxhit ];
    fL_V2_wire    = new Int_t   [ fMaxhit ];
    fL_V2_rawtime = new Int_t   [ fMaxhit ];
    fL_V2_time    = new Double_t[ fMaxhit ];
    fL_V2_dist    = new Double_t[ fMaxhit ];
  }
  if( which == kClusters || which == kAll ) {
    // Right HRS VDC
    fR_U1_clpos   = new Double_t[ fMaxclu ];
    fR_U1_slope   = new Double_t[ fMaxclu ];
    fR_U1_clpiv   = new Int_t   [ fMaxclu ];
    fR_U1_clsiz   = new Int_t   [ fMaxclu ];
    fR_V1_clpos   = new Double_t[ fMaxclu ];
    fR_V1_slope   = new Double_t[ fMaxclu ];
    fR_V1_clpiv   = new Int_t   [ fMaxclu ];
    fR_V1_clsiz   = new Int_t   [ fMaxclu ];
    fR_U2_clpos   = new Double_t[ fMaxclu ];
    fR_U2_slope   = new Double_t[ fMaxclu ];
    fR_U2_clpiv   = new Int_t   [ fMaxclu ];
    fR_U2_clsiz   = new Int_t   [ fMaxclu ];
    fR_V2_clpos   = new Double_t[ fMaxclu ];
    fR_V2_slope   = new Double_t[ fMaxclu ];
    fR_V2_clpiv   = new Int_t   [ fMaxclu ];
    fR_V2_clsiz   = new Int_t   [ fMaxclu ];
    // Left HRS VDC
    fL_U1_clpos   = new Double_t[ fMaxclu ];
    fL_U1_slope   = new Double_t[ fMaxclu ];
    fL_U1_clpiv   = new Int_t   [ fMaxclu ];
    fL_U1_clsiz   = new Int_t   [ fMaxclu ];
    fL_V1_clpos   = new Double_t[ fMaxclu ];
    fL_V1_slope   = new Double_t[ fMaxclu ];
    fL_V1_clpiv   = new Int_t   [ fMaxclu ];
    fL_V1_clsiz   = new Int_t   [ fMaxclu ];
    fL_U2_clpos   = new Double_t[ fMaxclu ];
    fL_U2_slope   = new Double_t[ fMaxclu ];
    fL_U2_clpiv   = new Int_t   [ fMaxclu ];
    fL_U2_clsiz   = new Int_t   [ fMaxclu ];
    fL_V2_clpos   = new Double_t[ fMaxclu ];
    fL_V2_slope   = new Double_t[ fMaxclu ];
    fL_V2_clpiv   = new Int_t   [ fMaxclu ];
    fL_V2_clsiz   = new Int_t   [ fMaxclu ];
  }
  if( which == kTracks || which == kAll ) {
    // Right HRS VDC
    fR_TR_x     = new Double_t[ fMaxtrk ];
    fR_TR_y     = new Double_t[ fMaxtrk ];
    fR_TR_th    = new Double_t[ fMaxtrk ];
    fR_TR_ph    = new Double_t[ fMaxtrk ];
    fR_TR_p     = new Double_t[ fMaxtrk ];
    fR_TR_flag  = new UInt_t  [ fMaxtrk ];

    fR_TD_x     = new Double_t[ fMaxtrk ];
    fR_TD_y     = new Double_t[ fMaxtrk ];
    fR_TD_th    = new Double_t[ fMaxtrk ];
    fR_TD_ph    = new Double_t[ fMaxtrk ];

    fR_TR_rx    = new Double_t[ fMaxtrk ];
    fR_TR_ry    = new Double_t[ fMaxtrk ];
    fR_TR_rth   = new Double_t[ fMaxtrk ];
    fR_TR_rph   = new Double_t[ fMaxtrk ];

    fR_TG_y     = new Double_t[ fMaxtrk ];
    fR_TG_th    = new Double_t[ fMaxtrk ];
    fR_TG_ph    = new Double_t[ fMaxtrk ];
    fR_TG_dp    = new Double_t[ fMaxtrk ];
    // Left HRS VDC
    fL_TR_x     = new Double_t[ fMaxtrk ];
    fL_TR_y     = new Double_t[ fMaxtrk ];
    fL_TR_th    = new Double_t[ fMaxtrk ];
    fL_TR_ph    = new Double_t[ fMaxtrk ];
    fL_TR_p     = new Double_t[ fMaxtrk ];
    fL_TR_flag  = new UInt_t  [ fMaxtrk ];

    fL_TD_x     = new Double_t[ fMaxtrk ];
    fL_TD_y     = new Double_t[ fMaxtrk ];
    fL_TD_th    = new Double_t[ fMaxtrk ];
    fL_TD_ph    = new Double_t[ fMaxtrk ];

    fL_TR_rx    = new Double_t[ fMaxtrk ];
    fL_TR_ry    = new Double_t[ fMaxtrk ];
    fL_TR_rth   = new Double_t[ fMaxtrk ];
    fL_TR_rph   = new Double_t[ fMaxtrk ];

    fL_TG_y     = new Double_t[ fMaxtrk ];
    fL_TG_th    = new Double_t[ fMaxtrk ];
    fL_TG_ph    = new Double_t[ fMaxtrk ];
    fL_TG_dp    = new Double_t[ fMaxtrk ];
  }
}

//______________________________________________________________________________
void THaVDCEvent::DeleteVariableArrays( EBlock which )
{
  // Delete variable arrays.

  if( which == kHits || which == kAll ) {
    // Right HRS VDC
    delete [] fR_U1_wire ;
    delete [] fR_U1_rawtime ;
    delete [] fR_U1_time ;
    delete [] fR_U1_dist ;
    delete [] fR_V1_wire ;
    delete [] fR_V1_rawtime ;
    delete [] fR_V1_time ;
    delete [] fR_V1_dist ;
    delete [] fR_U2_wire ;
    delete [] fR_U2_rawtime ;
    delete [] fR_U2_time ;
    delete [] fR_U2_dist ;
    delete [] fR_V2_wire ;
    delete [] fR_V2_rawtime ;
    delete [] fR_V2_time ;
    delete [] fR_V2_dist ;
    // Left HRS VDC
    delete [] fL_U1_wire ;
    delete [] fL_U1_rawtime ;
    delete [] fL_U1_time ;
    delete [] fL_U1_dist ;
    delete [] fL_V1_wire ;
    delete [] fL_V1_rawtime ;
    delete [] fL_V1_time ;
    delete [] fL_V1_dist ;
    delete [] fL_U2_wire ;
    delete [] fL_U2_rawtime ;
    delete [] fL_U2_time ;
    delete [] fL_U2_dist ;
    delete [] fL_V2_wire ;
    delete [] fL_V2_rawtime ;
    delete [] fL_V2_time ;
    delete [] fL_V2_dist ;
  }
  if( which == kClusters || which == kAll ) {
    // Right HRS VDC
    delete [] fR_U1_clpos;
    delete [] fR_U1_clpiv;
    delete [] fR_U1_slope;
    delete [] fR_U1_clsiz;
    delete [] fR_V1_clpos;
    delete [] fR_V1_clpiv;
    delete [] fR_V1_slope;
    delete [] fR_V1_clsiz;
    delete [] fR_U2_clpos;
    delete [] fR_U2_clpiv;
    delete [] fR_U2_slope;
    delete [] fR_U2_clsiz;
    delete [] fR_V2_clpos;
    delete [] fR_V2_clpiv;
    delete [] fR_V2_slope;
    delete [] fR_V2_clsiz;
    // Left HRS VDC
    delete [] fL_U1_clpos;
    delete [] fL_U1_clpiv;
    delete [] fL_U1_slope;
    delete [] fL_U1_clsiz;
    delete [] fL_V1_clpos;
    delete [] fL_V1_clpiv;
    delete [] fL_V1_slope;
    delete [] fL_V1_clsiz;
    delete [] fL_U2_clpos;
    delete [] fL_U2_clpiv;
    delete [] fL_U2_slope;
    delete [] fL_U2_clsiz;
    delete [] fL_V2_clpos;
    delete [] fL_V2_clpiv;
    delete [] fL_V2_slope;
    delete [] fL_V2_clsiz;
  }
  if( which == kTracks || which == kAll ) {
    // Right HRS VDC
    delete [] fR_TR_x;
    delete [] fR_TR_y;
    delete [] fR_TR_th;
    delete [] fR_TR_ph;
    delete [] fR_TR_p;
    delete [] fR_TR_flag;

    delete [] fR_TR_rx;
    delete [] fR_TR_ry;
    delete [] fR_TR_rth;
    delete [] fR_TR_rph;

    delete [] fR_TD_x;
    delete [] fR_TD_y;
    delete [] fR_TD_th;
    delete [] fR_TD_ph;

    delete [] fR_TG_y;
    delete [] fR_TG_th;
    delete [] fR_TG_ph;
    delete [] fR_TG_dp;
    // Left HRS VDC
    delete [] fL_TR_x;
    delete [] fL_TR_y;
    delete [] fL_TR_th;
    delete [] fL_TR_ph;
    delete [] fL_TR_p;
    delete [] fL_TR_flag;

    delete [] fL_TR_rx;
    delete [] fL_TR_ry;
    delete [] fL_TR_rth;
    delete [] fL_TR_rph;

    delete [] fL_TD_x;
    delete [] fL_TD_y;
    delete [] fL_TD_th;
    delete [] fL_TD_ph;

    delete [] fL_TG_y;
    delete [] fL_TG_th;
    delete [] fL_TG_ph;
    delete [] fL_TG_dp;
  }
}

//______________________________________________________________________________
Int_t THaVDCEvent::Fill()
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
void THaVDCEvent::InitCounters()
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
void THaVDCEvent::SetupDatamap( EBlock which )
{
  // Set up the data map, which relates global variable names to
  // THaVDCEvent member variables.

  CreateVariableArrays( which );

  // Note: Before you freak out here, recall that string literals
  // are always of storage class "static". So, the pointers to
  // const char* are not dangling when datamap goes out of scope.

  const DataMap datamap[] = {

    //=== Right HRS

    // VDC
    { 1,      "R.vdc.u1.nhit",     &fR_U1_nhit },
    {-1,      "R.vdc.u1.wire",     fR_U1_wire  },
    {-1,      "R.vdc.u1.rawtime",  fR_U1_rawtime, },
    {-1,      "R.vdc.u1.time",     fR_U1_time, },
    {-1,      "R.vdc.u1.dist",     fR_U1_dist },
    { 1,      "R.vdc.u1.nclust",   &fR_U1_nclust },
    {-1,      "R.vdc.u1.clpos",    fR_U1_clpos },
    {-1,      "R.vdc.u1.clpivot",    fR_U1_clpiv },
    {-1,      "R.vdc.u1.slope",    fR_U1_slope },
    {-1,      "R.vdc.u1.clsiz",    fR_U1_clsiz },
    { 1,      "R.vdc.v1.nhit",     &fR_V1_nhit },
    {-1,      "R.vdc.v1.wire",     fR_V1_wire  },
    {-1,      "R.vdc.v1.rawtime",  fR_V1_rawtime, },
    {-1,      "R.vdc.v1.time",     fR_V1_time  },
    {-1,      "R.vdc.v1.dist",     fR_V1_dist },
    { 1,      "R.vdc.v1.nclust",   &fR_V1_nclust },
    {-1,      "R.vdc.v1.clpos",    fR_V1_clpos },
    {-1,      "R.vdc.v1.clpivot",    fR_V1_clpiv },
    {-1,      "R.vdc.v1.slope",    fR_V1_slope },
    {-1,      "R.vdc.v1.clsiz",    fR_V1_clsiz },
    { 1,      "R.vdc.u2.nhit",     &fR_U2_nhit },
    {-1,      "R.vdc.u2.wire",     fR_U2_wire  },
    {-1,      "R.vdc.u2.rawtime",  fR_U2_rawtime, },
    {-1,      "R.vdc.u2.time",     fR_U2_time  },
    {-1,      "R.vdc.u2.dist",     fR_U2_dist },
    { 1,      "R.vdc.u2.nclust",   &fR_U2_nclust },
    {-1,      "R.vdc.u2.clpos",    fR_U2_clpos },
    {-1,      "R.vdc.u2.clpivot",    fR_U2_clpiv },
    {-1,      "R.vdc.u2.slope",    fR_U2_slope },
    {-1,      "R.vdc.u2.clsiz",    fR_U2_clsiz },
    { 1,      "R.vdc.v2.nhit",     &fR_V2_nhit },
    {-1,      "R.vdc.v2.wire",     fR_V2_wire  },
    {-1,      "R.vdc.v2.rawtime",  fR_V2_rawtime, },
    {-1,      "R.vdc.v2.time",     fR_V2_time  },
    {-1,      "R.vdc.v2.dist",     fR_V2_dist },
    { 1,      "R.vdc.v2.nclust",   &fR_V2_nclust },
    {-1,      "R.vdc.v2.clpos",    fR_V2_clpos },
    {-1,      "R.vdc.v2.clpivot",    fR_V2_clpiv },
    {-1,      "R.vdc.v2.slope",    fR_V2_slope },
    {-1,      "R.vdc.v2.clsiz",    fR_V2_clsiz },
    { 1,      "R.tr.n",            &fR_TR_n },
    {-1,      "R.tr.x",            fR_TR_x },
    {-1,      "R.tr.y",            fR_TR_y },
    {-1,      "R.tr.th",           fR_TR_th },
    {-1,      "R.tr.ph",           fR_TR_ph },
    {-1,      "R.tr.p",            fR_TR_p },
    {-1,      "R.tr.flag",         fR_TR_flag },
    
    {-1,      "R.tr.d_x",            fR_TD_x },
    {-1,      "R.tr.d_y",            fR_TD_y },
    {-1,      "R.tr.d_th",           fR_TD_th },
    {-1,      "R.tr.d_ph",           fR_TD_ph },

    {-1,      "R.tr.r_x",            fR_TR_rx },
    {-1,      "R.tr.r_y",            fR_TR_ry },
    {-1,      "R.tr.r_th",           fR_TR_rth },
    {-1,      "R.tr.r_ph",           fR_TR_rph },

    {-1,      "R.tr.tg_y",            fR_TG_y },
    {-1,      "R.tr.tg_th",           fR_TG_th },
    {-1,      "R.tr.tg_ph",           fR_TG_ph },
    {-1,      "R.tr.tg_dp",            fR_TG_dp },

    //=== Left HRS

    // VDC
    { 1,      "L.vdc.u1.nhit",     &fL_U1_nhit },
    {-1,      "L.vdc.u1.wire",     fL_U1_wire },
    {-1,      "L.vdc.u1.rawtime",  fL_U1_rawtime, },
    {-1,      "L.vdc.u1.time",     fL_U1_time },
    {-1,      "L.vdc.u1.dist",     fL_U1_dist },
    { 1,      "L.vdc.u1.nclust",   &fL_U1_nclust },
    {-1,      "L.vdc.u1.clpos",    fL_U1_clpos },
    {-1,      "L.vdc.u1.clpivot",    fL_U1_clpiv },
    {-1,      "L.vdc.u1.slope",    fL_U1_slope },
    {-1,      "L.vdc.u1.clsiz",    fL_U1_clsiz },
    { 1,      "L.vdc.v1.nhit",     &fL_V1_nhit },
    {-1,      "L.vdc.v1.wire",     fL_V1_wire },
    {-1,      "L.vdc.v1.rawtime",  fL_V1_rawtime, },
    {-1,      "L.vdc.v1.time",     fL_V1_time },
    {-1,      "L.vdc.v1.dist",     fL_V1_dist },
    { 1,      "L.vdc.v1.nclust",   &fL_V1_nclust },
    {-1,      "L.vdc.v1.clpos",    fL_V1_clpos },
    {-1,      "L.vdc.v1.clpivot",    fL_V1_clpiv },
    {-1,      "L.vdc.v1.slope",    fL_V1_slope },
    {-1,      "L.vdc.v1.clsiz",    fL_V1_clsiz },
    { 1,      "L.vdc.u2.nhit",     &fL_U2_nhit },
    {-1,      "L.vdc.u2.wire",     fL_U2_wire },
    {-1,      "L.vdc.u2.rawtime",  fL_U2_rawtime, },
    {-1,      "L.vdc.u2.time",     fL_U2_time },
    {-1,      "L.vdc.u2.dist",     fL_U2_dist },
    { 1,      "L.vdc.u2.nclust",   &fL_U2_nclust },
    {-1,      "L.vdc.u2.clpos",    fL_U2_clpos },
    {-1,      "L.vdc.u2.clpivot",    fL_U2_clpiv },
    {-1,      "L.vdc.u2.slope",    fL_U2_slope },
    {-1,      "L.vdc.u2.clsiz",    fL_U2_clsiz },
    { 1,      "L.vdc.v2.nhit",     &fL_V2_nhit },
    {-1,      "L.vdc.v2.wire",     fL_V2_wire },
    {-1,      "L.vdc.v2.rawtime",  fL_V2_rawtime, },
    {-1,      "L.vdc.v2.time",     fL_V2_time },
    {-1,      "L.vdc.v2.dist",     fL_V2_dist },
    { 1,      "L.vdc.v2.nclust",   &fL_V2_nclust },
    {-1,      "L.vdc.v2.clpos",    fL_V2_clpos },
    {-1,      "L.vdc.v2.clpivot",    fL_V2_clpiv },
    {-1,      "L.vdc.v2.slope",    fL_V2_slope },
    {-1,      "L.vdc.v2.clsiz",    fL_V2_clsiz },
    { 1,      "L.tr.n",            &fL_TR_n },
    {-1,      "L.tr.x",            fL_TR_x },
    {-1,      "L.tr.y",            fL_TR_y },
    {-1,      "L.tr.th",           fL_TR_th },
    {-1,      "L.tr.ph",           fL_TR_ph },
    {-1,      "L.tr.p",            fL_TR_p },
    {-1,      "L.tr.flag",         fL_TR_flag },

    {-1,      "L.tr.d_x",            fL_TD_x },
    {-1,      "L.tr.d_y",            fL_TD_y },
    {-1,      "L.tr.d_th",           fL_TD_th },
    {-1,      "L.tr.d_ph",           fL_TD_ph },

    {-1,      "L.tr.r_x",            fL_TR_rx },
    {-1,      "L.tr.r_y",            fL_TR_ry },
    {-1,      "L.tr.r_th",           fL_TR_rth },
    {-1,      "L.tr.r_ph",           fL_TR_rph },

    {-1,      "L.tr.tg_y",            fL_TG_y },
    {-1,      "L.tr.tg_th",           fL_TG_th },
    {-1,      "L.tr.tg_ph",           fL_TG_ph },
    {-1,      "L.tr.tg_dp",           fL_TG_dp },

    { 0 }  //End of data
  };

  // Save the datamap
  int nvar = sizeof(datamap)/sizeof(DataMap);
  delete [] fDataMap;
  fDataMap = new DataMap[ nvar ];
  memcpy( fDataMap, datamap, sizeof(datamap) );

  fInit = kFALSE;
}
