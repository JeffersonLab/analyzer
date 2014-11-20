#ifndef ROOT_THaCut
#define ROOT_THaCut

//////////////////////////////////////////////////////////////////////////
//
// THaCut
//
//////////////////////////////////////////////////////////////////////////

#include "THaFormula.h"
#include "TString.h"
#include "TMath.h"

class THaCut : public THaFormula {

public:
  THaCut();
  THaCut( const char* name, const char* expression, const char* block,
	  const THaVarList* vlst = gHaVars, const THaCutList* clst = gHaCuts );
  THaCut( const THaCut& rhs );
  THaCut& operator=( const THaCut& rhs );
  virtual ~THaCut();

          void         ClearResult()        { fLastResult = kFALSE; }
  // Requires ROOT >= 4.00/00
  virtual Int_t        DefinedVariable( TString& variable, Int_t& action );
  virtual Bool_t       EvalCut();
          Bool_t       GetResult()    const { return fLastResult; }
          const char*  GetBlockname() const { return fBlockname.Data(); }
          UInt_t       GetNCalled()   const { return fNCalled; }
          UInt_t       GetNPassed()   const { return fNPassed; }
  virtual Bool_t       IsArray()      const { return kFALSE; }
  virtual Bool_t       IsVarArray()   const { return kFALSE; }
  virtual void         Print( Option_t *opt="" ) const;
  virtual void         Reset();
  virtual void         SetBlockname( const Text_t* name );
  virtual void         SetName( const Text_t* name );
  virtual void         SetNameTitle( const Text_t *name, const Text_t *title );

protected:
  Bool_t      fLastResult;   //Result of last evaluation of this formula
  TString     fBlockname;    //Name of block this cut belongs to
  UInt_t      fNCalled;      //Number of times this cut has been evaluated
  UInt_t      fNPassed;      //Number of times this cut was true when evaluated

  ClassDef(THaCut,0)   //A logical cut (a.k.a. test)
};

//________________ inlines ____________________________________________________
inline
Bool_t THaCut::EvalCut()
{
  // Evaluate the cut and increment counters

  ResetBit(kInvalid);
  fNCalled++;
  if( TestBit(kError) )
    fLastResult = false;
  else {
    fLastResult = (TMath::Nint( Eval() ) != 0);
    if( TestBit(kInvalid) ) {
      fLastResult = false;
    } else if( fLastResult ) {
      fNPassed++;
    }
  }
  return fLastResult;
}

//_____________________________________________________________________________
inline
void THaCut::Reset()
{
  // Reset cut result and statistics counters

  ClearResult();
  fNCalled = fNPassed = 0;
}

#endif

