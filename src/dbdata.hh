/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1995 Mark-Andre' Hopf                                       *
 *  filename: dbdata.hh                                                     *
 *  relation: dbdata.cc                                                     *
 *  author  : Mark-Andre' Hopf                                              *
 *  date    : 04.05.1995                                                    *
 *                                                                          *
 ****************************************************************************/

#ifndef __DBDATA_HH
#define __DBDATA_HH

class dbData
{
  private:
    dbFileIO *fileio; // Verweis auf die Klasse mit dem Record Buffer
    WORD field;       // Feld im Record

    char* pString;    // Nullterminierter String mit Bufferinhalt
    WORD len;
    WORD dec;
    char type;

    bool bHaveString;
    void update();

  public:
    dbData();
    ~dbData();
    void Init(dbFileIO* io, WORD f);

//    operator =(char*);
//    operator =(int);
    operator int();
    operator unsigned long();
    operator char*();
};

#endif
