/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1994,95 Mark-André Hopf                                    *
 *  filename: dbdata.cc		                                                  *
 *  relation: ?						                                                  *
 *  author  : Mark-André Hopf                                              *
 *  date    : 13.02.1995                                                    *
 *  about   : -								                                             	*
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

#include <stdlib.h>

void dbData::update()
{
  ERRORMSG1("dbData.update: update from database @ %8lx\n",(ULONG)fileio);

  if(!fileio->readfield(field,pString,len)) // FileIO Buffer nach pString
    memset(pString,' ',len);                // ging nicht: leerer String

  ERRORMSG2("dbData.update: created pString=\"%s\" (len=%i)\n",pString,len);
}

dbData::dbData()
{
  ERRORMSG("dbData: constructor\n");
  fileio=NULL;
  pString=NULL;
}  

dbData::~dbData()
{
  ERRORMSG("dbData.~dbData: entry\n");
  if (pString)
  {
    ERRORMSG("dbData.~dbData: delete[] pString\n");
    delete[] pString;
  }
  ERRORMSG("dbData.~dbData: exit\n");
}

void dbData::Init(dbFileIO* io, WORD f)
{
  fileio=io;
  field =f;
  len   =fileio->FieldLen (field);
  dec   =fileio->FieldDec (field);
  type  =fileio->FieldType(field);
  bHaveString=false;

  if (pString)
  {
    ERRORMSG("dbData.Init: delete[] pString\n");
    delete[] pString;
    pString=NULL;
  } 
  if (!pString)
  {
    ERRORMSG1("dbData.Init: pString=new char[%i]\n",(int)(len+1));
    pString=new char[len+1];
    if (pString==NULL)
    {
      ERRORMSG("dbData.Init : out of memory\n");
    }
    pString[len]='\0';
  }
  ERRORMSG1("        dbFileIO* = %8lx\n",(ULONG)fileio);
  ERRORMSG1("        nr        = %u\n",(unsigned)f);
  ERRORMSG1("        len       = %u\n",(unsigned)len);
  ERRORMSG1("        dec       = %u\n",(unsigned)dec);
  ERRORMSG1("        type      = %c\n",(char)type);
}

// Lesezugriffe auf die Datenbank
//operator void*() {return s;}
dbData::operator char*()
{
  ERRORMSG("dbData: operator char*()\n");
  update();
//fprintf(stderr, "dbData::operator char*() liefert \"%s\"\n",pString);
  return pString;
}

dbData::operator int()
{
  ERRORMSG("dbData: operator int()\n");
  update();
  return atoi(pString);
}

dbData::operator unsigned long()
{
  ERRORMSG("dbData: operator unsigned long()\n");
  update();
	return strtoul(pString,NULL,10);
}
