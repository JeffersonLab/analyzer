#ifndef ROOT_THaCutList
#define ROOT_THaCutList

//////////////////////////////////////////////////////////////////////////
//
// THaCutList.h
//
//////////////////////////////////////////////////////////////////////////

#include "THashList.h"

class TList;
class THaVarList;
class THaPrintOption;
class THaNamedList;
class TString;

class THaCutList {

public:
  static const char* const kDefaultBlockName;
  static const char* const kDefaultCutFile;

  enum EWarnMode { kWarn, kNoWarn };

  THaCutList();
  THaCutList( const THaVarList& lst );
  virtual    ~THaCutList();

  virtual Int_t     Define( const char* cutname, const char* expr, 
			    const char* block=kDefaultBlockName );
  virtual Int_t     Define( const char* cutname, const char* expr,
			    const THaVarList& lst, 
			    const char* block=kDefaultBlockName );
  virtual Int_t     Load( const char* filename=kDefaultCutFile );
  virtual void      Clear( Option_t* opt="" );
  virtual Int_t     Eval();
  virtual Int_t     EvalBlock( const char* block=kDefaultBlockName );
  virtual Int_t     EvalBlock( const TString& block ) 
    { return EvalBlock(block.Data()); }
  virtual Int_t     EvalBlock( const THaNamedList* plist );
  THaNamedList*     FindBlock( const char* block ) const
    { return reinterpret_cast<THaNamedList*>(fBlocks->FindObject( block )); }
  const THashList*  GetCutList()   const { return fCuts; }   //These might disappear
  const THashList*  GetBlockList() const { return fBlocks; } //in future versions
          Int_t     GetNblocks()   const { return fBlocks->GetSize(); }
          Int_t     GetSize()      const { return fCuts->GetSize(); }
  virtual void      Reset();
  virtual Int_t     Result( const char* cutname = "", EWarnMode mode=kWarn );
          Int_t     Result( const TString& cutname, EWarnMode mode=kWarn ) 
    { return Result(cutname.Data(),mode); }
  virtual Int_t     Remove( const char* cutname );
  virtual Int_t     RemoveBlock( const char* block=kDefaultBlockName );
  virtual void      SetList( THaVarList& lst ) { fVarList = &lst; }
  virtual void      Print( Option_t* option="" ) const;
  virtual void      PrintCut( const char* cutname, Option_t* option="" ) const;
  virtual void      PrintBlock( const char* block=kDefaultBlockName, 
				Option_t* option="" ) const;

protected:
  THashList*        fCuts;      //Hash list holding all cuts
  THashList*        fBlocks;    //Hash list holding blocks of cuts.
                                //Elements of this table are THaNamedLists of THaCuts
  const THaVarList* fVarList;   //Pointer to list of variables

          UInt_t    IntDigits( Int_t n ) const;
  virtual void      PrintHeader( const THaPrintOption& opt ) const;
  virtual void      MakePrintOption( THaPrintOption& opt, 
				     const TList* plist ) const;


  ClassDef(THaCutList,0)  //HashList with support for blocks of objects
};

#endif
