/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1994 Mark-Andre' Hopf                                       *
 *  filename: dbLckLst.h                                                    *
 *  relation: dbLckLst.cpp                                                  *
 *  author  : Mark-Andre' Hopf                                              *
 *  date    : 13.02.1995                                                    *
 *  about   : dbLockList fuehrt eine Liste ueber alle gesperrten Datensaetze*
 *                                                                          *
 ****************************************************************************/

#ifndef __DBLCKLST_HH
#define __DBLCKLST_HH

#define LBSIZE 50

class dbLockList
{
  private:
    struct defLock
    {
      ULONG record;
      defLock(){record=(ULONG)-1L;}
    };
    struct defLockBlock
    {
      defLockBlock* next;
      unsigned number;
      defLock Lock[LBSIZE];
      defLockBlock(){number=0;}
    };
    defLockBlock* LockList;
    defLockBlock* FreeList;
    defLockBlock* ListPtr;
    int nPtr;
  public:
    dbLockList(){ListPtr=LockList=FreeList=NULL;}
    ~dbLockList();
    bool add(ULONG record);   // Nimmt Datensatz in Locklist auf
    bool del(ULONG record);   // Loescht Datensatz aus Locklist
    bool is (ULONG record);   // true, wenn Datensatz in Locklist

    bool first(ULONG* nr);    // Liefert ersten Datensatz in Locklist
    bool next(ULONG* nr);     // Liefert folgenden Datensatz in Locklist

    bool clear();             // clear all entries
};

#endif
