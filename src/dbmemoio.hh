/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1994,95 Mark-Andre' Hopf                                    *
 *  filename: dbmemoio.h                                                    *
 *  relation: dbfileio.cpp                                                  *
 *  author  : Mark-Andre' Hopf                                              *
 *  date    : 12.04.1995                                                    *
 *                                                                          *
 ****************************************************************************/

#ifndef __DBMEMOIO_HH
#define __DBMEMOIO_HH

class dbMemoIO: virtual public dbError
{
  private:
    int  fdMemo;
    char *filename;
    unsigned char *buffer;

    bool loadblock(ULONG nr);
    bool saveblock(ULONG nr);

  protected:
    dbMemoIO(){filename=NULL; buffer=NULL; fdMemo=0;}
    ~dbMemoIO();
    bool memoopen(const char *dbfile);
    bool memoclose();
    bool CopyMemoToFile(ULONG block, const char* file, bool additive=false);
};
#endif
