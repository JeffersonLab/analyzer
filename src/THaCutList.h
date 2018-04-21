#ifndef Podd_THaCutList_h_
#define Podd_THaCutList_h_

//////////////////////////////////////////////////////////////////////////
//
// THaCutList.h
//
//////////////////////////////////////////////////////////////////////////

#include "THashList.h"
#include "THaCut.h"
#include "THaNamedList.h"

class TList;
class THaVarList;
class THaPrintOption;

// Utility class that provides the PrintOpt method
class THaHashList : public THashList {
public:
  THaHashList(Int_t capacity = TCollection::kInitHashTableCapacity, 
	      Int_t rehash = 0) : THashList(capacity,rehash) {}
  THaHashList(TObject* parent, 
	      Int_t capacity = TCollection::kInitHashTableCapacity, 
	      Int_t rehash = 0) : THashList(parent,capacity,rehash) {}
  virtual ~THaHashList() {}

  virtual void PrintOpt( Option_t* opt ) const;
  ClassDef(THaHashList,1) //A THashList list with a PrintOpt method
};
  
class THaCutList {

public:
  static const char* const kDefaultBlockName;
  static const char* const kDefaultCutFile;

  enum EWarnMode { kWarn, kNoWarn };

  THaCutList();
  THaCutList( const THaCutList& clst );
  THaCutList( const THaVarList* lst );
  virtual    ~THaCutList();

  virtual void      Clear( Option_t* opt="" );
  virtual void      ClearAll( Option_t* opt="" );
  virtual void      ClearBlock( const char* block=kDefaultBlockName,
				Option_t* opt="" );
  virtual void      Compile();
  virtual Int_t     Define( const char* cutname, const char* expr, 
			    const char* block=kDefaultBlockName );
  virtual Int_t     Define( const char* cutname, const char* expr,
			    const THaVarList* lst, 
			    const char* block=kDefaultBlockName );
  virtual Int_t     Eval();
  virtual Int_t     EvalBlock( const char* block=kDefaultBlockName );
  THaCut*           FindCut( const char* name ) const
    { return static_cast<THaCut*>(fCuts->FindObject( name )); }
  THaNamedList*     FindBlock( const char* block ) const
    { return static_cast<THaNamedList*>(fBlocks->FindObject( block )); }
  const THashList*  GetCutList()   const { return fCuts; }   //These might disappear
  const THashList*  GetBlockList() const { return fBlocks; } //in future versions
          Int_t     GetNblocks()   const { return fBlocks->GetSize(); }
          Int_t     GetSize()      const { return fCuts->GetSize(); }
  virtual Int_t     Load( const char* filename=kDefaultCutFile );
  virtual void      Print( Option_t* option="" ) const;
  virtual void      PrintCut( const char* cutname, Option_t* option="" ) const;
  virtual void      PrintBlock( const char* block=kDefaultBlockName, 
				Option_t* option="" ) const;
  virtual void      Reset();
  virtual Int_t     Result( const char* cutname = "", EWarnMode mode=kWarn );
  virtual Int_t     Remove( const char* cutname );
  virtual Int_t     RemoveBlock( const char* block=kDefaultBlockName );
  virtual void      SetList( THaVarList* lst );

  static  Int_t     EvalBlock( const TList* plist );

protected:
  THaHashList*      fCuts;    //Hash list holding all cuts
  THaHashList*      fBlocks;  //Hash list holding blocks of cuts.
                              //Elements of this table are THaNamedLists of THaCuts
  const THaVarList* fVarList; //Pointer to list of variables

  static  void      MakePrintOption( THaPrintOption& opt, 
				     const TList* plist );

  virtual void      PrintHeader( const THaPrintOption& opt ) const;

  ClassDef(THaCutList,0)  //Hash list of TCuts with support for blocks of cuts
};

// Global utitlity function
UInt_t IntDigits( Int_t n );

#endif
