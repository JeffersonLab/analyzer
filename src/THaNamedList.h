#ifndef Podd_THaNamedList_h_
#define Podd_THaNamedList_h_

//////////////////////////////////////////////////////////////////////////
//
// THaNamedList
//
// A list with a name. Used to store blocks of THaCuts (tests) in a 
// THashTable. 
//
//////////////////////////////////////////////////////////////////////////

#include "TList.h"
#include "TNamed.h"

class THaNamedList : public TList {
  
public:
  THaNamedList();
  THaNamedList( const char* name );
  THaNamedList( const char* name, const char* descript );
  virtual  ~THaNamedList();

  virtual Int_t    Compare( const TObject* obj) const    
    { return fNamed->Compare(obj); }
  virtual void     FillBuffer(char*& buffer)    { fNamed->FillBuffer(buffer); }
          const Text_t*  GetName() const        { return fNamed->GetName(); }
          const Text_t*  GetTitle() const       { return fNamed->GetTitle(); }
  virtual ULong_t  Hash() const                 { return fNamed->Hash(); }
          Bool_t   IsSortable() const           { return kTRUE; }
  virtual void     PrintOpt( Option_t* opt="" ) const;
  virtual void     SetName(const Text_t *name); // *MENU*
  virtual void     SetNameTitle(const Text_t *name, const Text_t *title);
  virtual void     SetTitle(const Text_t *title="") 
    { fNamed->SetTitle(title); } // *MENU*
  virtual void     ls(Option_t *option="") const;
          Int_t    Sizeof() const               { return fNamed->Sizeof(); }

protected:
  TNamed*    fNamed;   //Name of the list

  ClassDef(THaNamedList,0)   //A list with a name
};

#endif

