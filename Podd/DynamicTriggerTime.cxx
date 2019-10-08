//*-- Author :    Ole Hansen   23-Sep-19

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::DynamicTriggerTime                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "DynamicTriggerTime.h"
#include "THaDetMap.h"
#include "THaEvData.h"
#include "THaGlobals.h"
#include "THaCutList.h"
#include "TMath.h"
#include "VarDef.h"
#include "VarType.h"
#include "TError.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <cassert>

using namespace std;

namespace Podd {

//____________________________________________________________________________
DynamicTriggerTime::DynamicTriggerTime( const char *name, const char *desc,
                                        THaApparatus *apparatus ) :
  THaTriggerTime( name, desc, apparatus )
{
  // Basic constructor
}

//____________________________________________________________________________
DynamicTriggerTime::DynamicTriggerTime( const char *name, const char *desc,
                                        const char *expr, const char *cond,
                                        THaApparatus *apparatus ) :
  THaTriggerTime( name, desc, apparatus )
{
  // Construct with formula 'expr' for calculating the time correction and
  // optional condition 'cond' that must be true for applying the correction.
  // This formula is evaluated /after/ any other definitions read from the
  // database (like a default value).

  if( !expr )
    return;     // behave like the basic constructor
  if( !cond )
    cond = "";

  TimeCorrDef def(expr, cond, kMaxUInt);
  def.fFromDB = false;
  pair<set<Podd::DynamicTriggerTime::TimeCorrDef>::iterator,bool> ins =
    fDefs.insert( std::move(def) );
  assert( ins.second );
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::ReadDatabase( const TDatime &date )
{
  // Read this detector's parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char *const here = "ReadDatabase";

  FILE *file = OpenFile( date );
  if( !file )
    return kFileError;

  fGlOffset = 0.0;
  // Delete all definitions that came from the database, i.e. keep the one
  // given to the constructor, if any
  for( auto it = fDefs.begin(); it != fDefs.end(); ) {
    if( it->fFromDB )
      it = fDefs.erase(it);
    else
      ++it;
  }

  // Read configuration parameters
  TString defstr, stagestr;
  DBRequest config_request[] = {
    { "t_corr",   &defstr,    kTString, 0, true },
    { "eval_at",  &stagestr,  kTString, 0, true },
    { "glob_off", &fGlOffset, kDouble,  0, true },
    { nullptr }
  };
  Int_t err = LoadDB( file, date, config_request, fPrefix );
  fclose( file );
  if( err )
    return err;

  if( !defstr.IsNull() ) {
    // Parse the definition string. The format is
    //
    // <condition>   <formula for calculating time correction>
    //
    // No spaces are allowed in the condition. Conditions are evaluated in
    // the order listed. Comments (starting with '#') are ignored.
    // Definitions may have trailing comments.
    //
    // If only a single value string is given, it is taken as a global formula
    // applied to all events.
    //
    // Both the condition and formula may contain any global variable and/or
    // cut name that has been calculated at the corresponding stage.
    //
    // Examples:
    //
    // L.trg.eval_at = Decode   # Evaluate expressions at Decode stage
    // L.trg.t_corr =
    //   DL.evtypebits&0x20!=0
    //      L.s2.rt_c[L.s2.t_pads[0]]-R.s2.rt_c[R.s2.t_pads[0]]  # s2 L-TDC time diff
    //   DL.evtypebits&0x10!=0
    //      L.s2.lt_c[L.s2.t_pads[0]]-R.s2.lt_c[R.s2.t_pads[0]]  # s2 R-TDC time diff
    //

    unique_ptr<TObjArray> tokarr( defstr.Tokenize( " ") );
    if( !tokarr->IsEmpty() ) {
      Int_t nelem = tokarr->GetLast() + 1;
      assert( nelem > 0 );   // else IsEmpty()

      TObjArray* tokens = tokarr.get();
      if( nelem == 1 ) {
        if( !fDefs.empty() ) {
          Warning( Here(here), "Unconditional formula in database will cause "
                               "formula given to constructor to be ignored. "
                               "Fix database." );
        }
        fDefs.insert( TimeCorrDef( GetObjArrayString(tokens,0) ));
      }
      else if( nelem % 2 == 0 ) {
        for( int ip = 0; ip < nelem; ip += 2 ) {
          fDefs.insert( TimeCorrDef( GetObjArrayString(tokens,ip),
                                    GetObjArrayString(tokens,ip+1),
                                    ip/2 ));
        }
      }
      else {
        Error( Here(here), "Incorrect number of parameters = %d in database "
                           "key %st_torr. Must be pairs of formulas and "
                           "conditions. Fix database.", nelem , GetPrefix() );
        return kInitError;
      }

    } else {
      Warning( Here( here ), "Empty database key %st_corr.", GetPrefix());
    }
  }


  fIsInit = true;
  return kOK;
}

//____________________________________________________________________________
void DynamicTriggerTime::Clear( Option_t *opt )
{
  // Reset all variables to their default/unknown state
  THaTriggerTime::Clear( opt );
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::Process()
{
  // Process the defined tests in order of priority. For the first succeeding
  // test, evaluate its corresponding formula and take the result as this
  // event's trigger time correction.
  // Returns 1 if the formula was successfully evaluated, else 0.

  // To get meaningful statistics, we evaluate all tests in this module's test
  // block. This incurs a slight performance penalty with multiple defined tests.

  Int_t ret = 0;

  //TODO: call InitDefs here if necessary
  gHaCuts->EvalBlock(fTestBlockName);

  for( auto& def : fDefs ) {
    if( !def.fCond->IsInvalid() and def.fCond->GetResult() ) {
      fEvtTime = def.fForm->Eval();
      if( def.fForm->IsInvalid() )
        fEvtTime = kBig;
      else
        ret = 1;
      break;
    }
  }

  return ret;
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::Decode( const THaEvData &evdata )
{
  // Look through the channels to determine the trigger type.
  // Be certain to decode only once per event.

  //TODO: implement

//  if( fEvtType > 0 ) return kOK;
//
//  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
//    THaDetMap::Module *d = fDetMap->GetModule( i );
//
//    // Loop over our short-list of relevant channels.
//    // Continuing with the assumption that each channel has its own entry in
//    // fDetMap, so that fDetMap, fTrgTimes, fTrgTypes, and fToffsets are
//    // parallel.
//
//    // look through each channel and take the earliest hit
//    if( fCommonStop ) fTDCRes *= -1;
//
//    for( Int_t j = 0; j < evdata.GetNumHits( d->crate, d->slot, d->lo ); j++ ) {
//      Double_t t = fTDCRes * evdata.GetData( d->crate, d->slot, d->lo, j );
//      if( t < fTrgTimes[i] ) fTrgTimes[i] = t;
//    }
//  }
//
//  int earliest = -1;
//  Double_t reftime = 0;
//  for( Int_t i = 0; i < fNTrgType; i++ ) {
//    if( earliest < 0 || fTrgTimes[i] < reftime ) {
//      earliest = i;
//      reftime = fTrgTimes[i];
//    }
//  }
//
  Double_t offset = fGlOffset;
//  if( earliest >= 0 ) {
//    offset += fToffsets[earliest];
//    fEvtType = fTrgTypes[earliest];
//  }
  fEvtTime = offset;
  return kOK;
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::CoarseProcess( TClonesArray &array )
{
  return THaTriggerTime::CoarseProcess( array );
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::FineProcess( TClonesArray &array )
{
  return THaTriggerTime::FineProcess( array );
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::DefineVariables( EMode mode )
{
  // Define/delete standard event-by-event time offsets

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = (mode == kDefine);

  RVarDef vars[] = {
//          { "evtime",   "Time-offset for event (trg based)", "fEvtTime" },
//          { "evtype",   "Earliest trg-bit for the event",    "fEvtType" },
//          { "trgtimes", "Times for each trg-type",           "fTrgTimes" },
          { nullptr }
  };

  return DefineVarsFromList( vars, mode );
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::InitDefs()
{
  // Initialize formulae and tests based on the string expressions read
  // from the database. Return number of successful definitions.

  Int_t ndef = 0;
  for( auto& def : fDefs ) {
    // These should be nullptr, but it doesn't hurt to be safe
    delete def.fCond;
    delete def.fForm;

    // Parse formula
    def.fForm = new THaFormula( "form", def.fFormStr );
    def.fCond = nullptr;
    // Expression error? (Message already printed)
    if( def.fForm->IsZombie() or def.fForm->IsError() ) {
      delete def.fForm;
      def.fForm = nullptr;
      continue;
    }
    //TODO: disallow array formulae

    // Parse condition, if any
    if( !def.fCondStr.IsNull() ) {
      def.fCond = new THaCut( "cond", def.fCondStr, "Decode" );
      // If the cut cannot be parsed, the whole definition is unusable
      if( def.fCond->IsZombie() or def.fCond->IsError() ) {
        delete def.fCond;
        delete def.fForm;
        def.fForm = def.fCond = nullptr;
        continue;
      }
    }
    ++ndef;
  }

  return ndef;
}

} // namespace Podd

ClassImp( Podd::DynamicTriggerTime )
