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
#include "THaFormula.h"
#include "THaCut.h"
#include "VarDef.h"
#include "VarType.h"
#include "TError.h"
#include <cstdio>
#include <memory>
#include <cassert>
#include <map>

using namespace std;

namespace Podd {

//____________________________________________________________________________
DynamicTriggerTime::DynamicTriggerTime( const char* name, const char* desc,
                                        THaApparatus* app ) :
  THaTriggerTime(name, desc, app), fEvalAt(kDecode), fDidFormInit(false)
{
  // Basic constructor
}

//____________________________________________________________________________
DynamicTriggerTime::DynamicTriggerTime( const char* name, const char* desc,
                                        const char* expr, const char* cond,
                                        THaApparatus* app ) :
  THaTriggerTime(name, desc, app), fEvalAt(kDecode), fDidFormInit(false)
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
Int_t DynamicTriggerTime::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char *const here = "ReadDatabase";

  FILE *file = OpenFile( date );
  if( !file )
    return kFileError;

  // Make the name of our block of tests. We need fPrefix to be set
  MakeBlockName();

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
  TString defstr, evalat;
  DBRequest config_request[] = {
    { "t_corr",   &defstr,    kTString, 0, true },
    { "eval_at",  &evalat,    kTString, 0, true },
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
    // L.trg.eval_at = Decode   # Evaluate expressions at Decode stage (default)
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
                           "conditions. Fix database.", nelem , fPrefix );
        return kInitError;
      }

    } else {
      Warning( Here( here ), "Empty database key %st_corr.", fPrefix);
    }
  }

  if( !evalat.IsNull() ) {
    map<TString,EEvalAt> stagelookup{{ "Decode",        kDecode },
                                     { "CoarseProcess", kCoarseProcess },
                                     { "FineProcess",   kFineProcess }};
    auto item = stagelookup.find( evalat);
    if( item == stagelookup.end() ) {
      Error( Here(here), "\"%seval_at = %s\": Unrecognized processing stage name.\n"
                         "Must be one of \"Decode\", \"CoarseProcess\" or \"FineProcess\". "
                         "Fix database.", fPrefix, evalat.Data() );
      return kInitError;
    }
    fEvalAt = item->second;
    if( fDebug > 0 )
      Info( Here(here), "Configured for stage = \"%s\"", item->first.Data() );
  }

  //FIXME: debug
  InitDefs();

  fIsInit = true;
  return kOK;
}

//____________________________________________________________________________
void DynamicTriggerTime::Clear( Option_t* opt )
{
  // Reset all variables to their default/unknown state
  THaTriggerTime::Clear(opt);
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::CalcCorrection()
{
  // Process the defined tests in order of priority. For the first succeeding
  // test, evaluate its corresponding formula and take the result as this
  // event's trigger time correction.
  // Returns 1 if the formula was successfully evaluated, else 0.

  // To get meaningful statistics, we evaluate all tests in this module's test
  // block. This incurs a slight performance penalty with multiple defined tests.

  Int_t ret = 0;

  if( !fDidFormInit ) {
    InitDefs();
    fDidFormInit = true;
  }

  gHaCuts->EvalBlock(fTestBlockName);

  for( auto& def : fDefs ) {
    if( !def.fCond->IsInvalid() and def.fCond->GetResult() ) {
      fEvtTime = def.fForm->Eval();
      if( def.fForm->IsInvalid() )
        fEvtTime = kBig;
      else {
        fEvtTime += fGlOffset;
        ret = 1;
      }
      break;
    }
  }

  return ret;
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::Decode( const THaEvData& /*evdata*/ )
{
  // Calculate the trigger time correction for the given event, if
  // configured for the Decode stage.
  //
  // Doing the calculation in Decode is what one normally wants
  // to apply trigger jitter corrections to raw tracking detector
  // TDC values.
  //
  // Important: the processing order of the detectors needs to be set
  // such that this modules runs AFTER all the variables used in
  // the time correction formulas have been calculated at BEFORE
  // the result is used by other detectors. Typically, that means
  // that scintillators need to be processed first, then this module,
  // then the tracking detectors.

  if( fEvalAt == kDecode )
    CalcCorrection();

// return THaTriggerTime::Decode(evdata); //FIXME: run this too?
  return kOK;
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::CoarseProcess( TClonesArray& )
{
  // Calculate the trigger time correction for the given event, if
  // configured for the CoarseProcess stage.
  //
  // Important: Since the standard spectrometer class THaSpectrometer always
  // runs CoarseTrack before CoarseProcess, doing this calculation in
  // CoarseProcess is useful only if needed by tracking detectors in FineTrack.

  if( fEvalAt == kCoarseProcess )
    CalcCorrection();

  return kOK;
}

//____________________________________________________________________________
Int_t DynamicTriggerTime::FineProcess( TClonesArray& )
{
  // Calculate the trigger time correction for the given event, if
  // configured for the FineProcess stage.
  //
  // Doing the calculation in FineProcess is only useful if the result is
  // needed by other non-tracking detectors and/or physics modules.

  if( fEvalAt == kFineProcess )
    CalcCorrection();

  return kOK;
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
    // Delete existing formula/test, if any, to allow reinitialization
    delete def.fForm;
    delete def.fCond;
    def.fCond = nullptr;

    // Parse formula
    def.fForm = new THaFormula( "form", def.fFormStr );
    // Expression error? (Message already printed)
    if( def.fForm->IsZombie() or def.fForm->IsError() ) {
      delete def.fForm;
      def.fForm = nullptr;
      continue;
    }
    //TODO: disallow array formulas

    // Parse condition, if any
    if( !def.fCondStr.IsNull() ) {
      def.fCond = new THaCut( "cond", def.fCondStr, fTestBlockName );
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

//____________________________________________________________________________
void DynamicTriggerTime::MakeBlockName()
{
  // Create the test block name

  if( fPrefix && *fPrefix ) {
    TString no_dot_prefix(fPrefix);
    no_dot_prefix.Chop();
    fTestBlockName = no_dot_prefix;
  } else
    fTestBlockName = fName;
  fTestBlockName.Append("_Tests");
}

//____________________________________________________________________________
DynamicTriggerTime::TimeCorrDef::~TimeCorrDef()
{
  delete fForm;
  delete fCond;
}

//____________________________________________________________________________

} // namespace Podd

ClassImp( Podd::DynamicTriggerTime )
