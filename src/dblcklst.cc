/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1994,95 Mark-André Hopf                                    	*
 *  filename: dbLckLst.cpp                                                  *
 *  relation: dbFileIO.cpp                                                  *
 *  author  : Mark-Andre' Hopf                                              *
 *  date    : 13.02.1995                                                    *
 *  about   : (see dbLckLst.hh)                                             *
 *                                                                          *
 ****************************************************************************/

//#include <typeinfo>
#include <cstddef>
#include <cstring>
#include <moonbase/owndef.hh>
#include <moonbase/dbheader.hh>
#include <moonbase/dbmemoio.hh>
#include <moonbase/dblcklst.hh>
#include <moonbase/dbfileio.hh>
#include <moonbase/dbdata.hh>
#include <moonbase/moonbase.hh>

// liefert true bei Erfolg, und false, wenn der Record schon eingetragen war
bool dbLockList::add(ULONG record)
{
  int nr;
  // first, next beenden
  ListPtr=NULL;
	#ifdef SECURE
  if (record==0L)
  {
    printf("dbLockList::add : illegal record number\n");
    return false;
  }
  #endif
  // doppelte Belegungen vermeiden
  if (is(record)) return false;
  // alle Lock-Bloecke nach einem freien Eintrag absuchen
  defLockBlock* ptr=LockList;
  while(ptr!=NULL)
  {
    if (ptr->Lock[LBSIZE-1].record==0L)   // hat Block noch freie Eintraege?
    {                                     // wenn ja, dann such sie:
      for(nr=0; nr<LBSIZE; nr++)
      {
        if (ptr->Lock[nr].record==0L) goto found;
      }
    }
    ptr=ptr->next;                        // Block voll, naechster Block
  }
  // wenn kein freier Lock-Block gefunden wurde, dann einen neuen einfuegen
  if (ptr==NULL)
  {
    if (FreeList)               // einen Block aus der FreeList nehmen wenn..
    {                           // ..vorhanden..
      ptr=FreeList;
      FreeList=ptr->next;
    }
    else                        // ..oder einen neuen erzeugen und dann..
    {
      ptr=new(defLockBlock);
    }
    ptr->next=LockList;         // ..den Block in die LockList haengen
    LockList=ptr;
    nr=0;
  }
  // die Sperre eintragen
  found:
  ptr->Lock[nr].record=record;
  return true;
}

bool dbLockList::del(ULONG record)
{
  // first, next beenden
  ListPtr=NULL;
  // alle Lock-Bloecke nach Eintrag absuchen
  defLockBlock* ptr=LockList;
  defLockBlock* bptr=NULL;
  while(ptr!=NULL)
  {
    for(int nr=0; nr<LBSIZE; nr++)
    {
      if (ptr->Lock[nr].record==record)
      {
        ptr->Lock[nr].record=0L;    // Eintrag loeschen
        for(nr=0; nr<LBSIZE; nr++)  // Pruefen, ob der Lock-Block leer ist..
        {
          if (ptr->Lock[nr].record!=0L) break;
        }
        if (nr>=LBSIZE)             // ..und wenn ja, dann den LockBlock in..
        {                           // ..die FreeList haengen
          if (bptr)                 // (innerhalb der Liste)
          {
            bptr->next=ptr->next;
            ptr->next=FreeList;
            FreeList=ptr;
          }
          else                      // (am Anfang der Liste)
          {
            LockList=ptr->next;
            ptr->next=FreeList;
            FreeList=ptr;
          }
        }
        return true;
      }
    }
    bptr=ptr;
    ptr=ptr->next;
  }
  return false;
}

bool dbLockList::is(ULONG record)
{
  // alle Lock-Bloecke nach Eintrag absuchen
  defLockBlock* ptr=LockList;
  while(ptr!=NULL)
  {
    for(int nr=0; nr<LBSIZE; nr++)
    {
      if (ptr->Lock[nr].record==record) return true;
    }
    ptr=ptr->next;
  }
  return false;
}

bool dbLockList::first(ULONG *nr)
{
  ListPtr=LockList;
  nPtr=0;
  return next(nr);
}

bool dbLockList::next(ULONG *nr)
{
  if (ListPtr)
  {
    *nr=ListPtr->Lock[nPtr].record;
    nPtr++;
    while(ListPtr)
    {
      while(nPtr<LBSIZE)
      {
        if (ListPtr->Lock[nPtr].record != 0L) return true;
        nPtr++;
      }
      nPtr=0;
      ListPtr=ListPtr->next;
    }
    return true;
  }
  return false;
}

bool dbLockList::clear()
{
  defLockBlock* ptr=LockList;
  defLockBlock* lptr=NULL;
  while(ptr)                        // Alle Lock-Bloecke durchgehen..
  {
    for(int i=0; i<LBSIZE; i++)     // ..und darin alle Records loeschen.
    {
      ptr->Lock[i].record=0L;
    }
    lptr=ptr;
    ptr=ptr->next;
  }
  if (LockList)                     // Leere LockList an die FreeList haengen
  {
    lptr->next=FreeList;            // (Freelist an LockList)
    FreeList=LockList;
    LockList=NULL;
  }
  return true;
}

dbLockList::~dbLockList()
{
  defLockBlock* ptr;
  // FreeList loeschen
  while(FreeList)
  {
    ptr=FreeList;
    FreeList=ptr->next;
    delete ptr;
  }
  // LockList loeschen
  while(LockList)
  {
    ptr=LockList;
    LockList=ptr->next;
    delete ptr;
  }
}
