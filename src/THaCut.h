#ifndef ROOT_THaCut
#define ROOT_THaCut

//////////////////////////////////////////////////////////////////////////
//
// THaCut
//
//////////////////////////////////////////////////////////////////////////

#include "THaFormula.h"
#include "TString.h"

class THaVarList;
class THaCutList;

class THaCut : public THaFormula {

public:
  THaCut() : THaFormula(), fLastResult(kFALSE), fNCalled(0), fNPassed(0),
	     fCutList(NULL), fIsCut(NULL) {}
  THaCut( const char* name, const char* expression, const char* block, 
	  const THaVarList& lst, const THaCutList* cutlist = NULL );
  virtual ~THaCut();

  virtual Int_t        Compile( const char* expression="" );
  virtual Double_t     DefinedValue( Int_t i );
  virtual Int_t        DefinedVariable( TString& variable );
  virtual Bool_t       EvalCut();
          Bool_t       GetResult()    const { return fLastResult; }
          const char*  GetBlockname() const { return fBlockname.Data(); }
          UInt_t       GetNCalled()   const { return fNCalled; }
          UInt_t       GetNPassed()   const { return fNPassed; }
  virtual void         Print( Option_t *opt="" ) const;
  virtual void         Reset() { fLastResult=kFALSE; fNCalled=fNPassed=0; }
  virtual void         SetName( const Text_t* name );
  virtual void         SetBlockname( const Text_t* name ) { fBlockname = name; }
  virtual void         SetNameTitle( const Text_t *name, const Text_t *title );
          void         SetCutList( const THaCutList* lst )  { fCutList = lst; }

protected:
  Bool_t      fLastResult;   //Result of last evaluation of this formula
  TString     fBlockname;    //Name of block this cut belongs to
  UInt_t      fNCalled;      //Number of times this cut has been evaluated
  UInt_t      fNPassed;      //Number of times this cut was true when evaluated
  const THaCutList* fCutList; //List of cuts (required if cuts are to contain other cuts)
  Bool_t*     fIsCut;        //Flags indicating that fCodes[i] is pointer to a cut

  ClassDef(THaCut,0)   //A logical cut (a.k.a. test)
};

//________________ inlines ____________________________________________________
inline
Bool_t THaCut::EvalCut()
{
  // Evaluate the cut and increment counters

  fNCalled++;
  fLastResult = ( Eval() > 0.5 );
  if( fLastResult ) fNPassed++;
  return fLastResult;
}

#endif

