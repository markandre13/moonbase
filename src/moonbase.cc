/*****************************************************************************

  MoonBase - a dBASE compatible database for C++
  (C)opyright 1993,95 by MAGIC INCLUDED
  (C)opyright 1993,95,96 by Mark-André Hopf
  
*****************************************************************************/
const char *MOONBASE_VERSION = "v0.03 Rel.02.05.96";

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

#include <stdio.h>

/****************************************************************************
 *                                                                          *
 * Konstruktor                                                              *
 *                                                                          *
 ****************************************************************************/
database::database()
{
  dbDatas=NULL;

  // Bereich "1" mit Datenbank erzeugen
  pArea				=	new defArea;
  pArea->name	=	new char[2];
  pArea->name[0]='1'; pArea->name[1]='\0';
  pArea->db		=	new dbFileIO;
  pArea->next	=	pArea;
	pArea->prev	=	pArea;
  pAreaList		=	pArea;
  db					=	pArea->db;
}

/****************************************************************************
 *                                                                          *
 * Destruktor                                                               *
 *                                                                          *
 ****************************************************************************/
database::~database()
{
	if (pArea==NULL)
		THROW("database.~database: FATAL! pArea==NULL");
	defArea *ptr;
	pAreaList->prev->next	=	NULL;
	while(pAreaList!=NULL)
	{
		ptr=pAreaList;
		pAreaList=pAreaList->next;
		#ifdef DEBUG
		printf("Schließe Bereich \"%s\" mit Datenbank \"%s\"...\n",ptr->name, ptr->db->Dbf());
		#endif
		delete[] ptr->name;
		if (ptr->db->handle) ptr->db->fileclose();
		delete	 ptr->db;
		delete   ptr;
	}
}

/****************************************************************************
 *                                                                          *
 * Datei öffnen                                                             *
 *                                                                          *
 ****************************************************************************/
bool database::fileopen(const char* file)
{
  ERRORMSG("database.fileopen: entry\n");
  if (db->handle)
    THROW("database::fileopen: FATAL! File is already open");

  if(db->fileopen(file))
  {
  	create_dbdatas();
    ERRORMSG("database.fileopen: exit\n");
    return true;
  }
  ERRORMSG("database.fileopen: exit\n");
  return false;
}

/****************************************************************************
 *                                                                          *
 * SELECT                                                                   *
 * Einen Bereich für die Datenbank auswählen (inklusive Löschen & Erzeugen) *
 *                                                                          *
 ****************************************************************************/
bool database::Select(const char *name)
{
	#ifdef SECURE
	if (pArea==NULL)
	{
		printf("database.Select: FATAL! pArea==NULL\n");
		exit(1);
	}
	#endif

	// Ist der angegebene Bereich der gerade genutzte, dann sind wir schon fertig
	if (stricmp(name, pArea->name)==0)
		return true;

	// nach dem angegebenen Bereich suchen
	defArea *ptr = pAreaList;
	int nAction=2;
	do
	{
		#ifdef DEBUG
		printf("vergleiche %s mit %s => ",name,ptr->name);
		#endif
		if (stricmp(name, ptr->name)==0)
		{
			#ifdef DEBUG
			printf("gleich\n");
			#endif
			nAction=0;
			break;
		}
		#ifdef DEBUG
		printf("ungleich\n");
		#endif
		ptr=ptr->next;	
	}while(ptr!=pAreaList);
	
	if (db->handle) delete_dbdatas();
	if (db->handle!=0) nAction++;

	switch(nAction)
	{
		case 0:						// Bereich existiert und alter Bereich ist unbenutzt
			#ifdef DEBUG
			printf("Bereich %s ist leer und wird vor Verlassen gelöscht\n",pArea->name);
			#endif
			pArea->prev->next=pArea->next;
			pArea->next->prev=pArea->prev;
			delete[] pArea->name;
			delete   pArea->db;
			if (pAreaList==pArea)
				pAreaList=pArea->next;
			delete	 pArea;									
		case 1:						// Bereich existiert und alter Bereich ist benutzt
			#ifdef DEBUG
			printf("neuen Bereich %s gewählt\n",ptr->name);
			#endif
			pArea=ptr;
			db=pArea->db;
			break;
		case 2:						// Bereich existiert nicht und alter Bereich ist unbenutzt
			#ifdef DEBUG
			printf("alter Bereich leer, benenne %s in %s um\n",pArea->name,name);
			#endif
			delete[] pArea->name;
			pArea->name=new char[strlen(name)];
			strcpy(pArea->name, name);
			break;
		case 3:						// Bereich existiert nicht und alter Bereich ist benutzt
			ptr				=	new defArea;
			ptr->db		= new dbFileIO;
			ptr->name	= new char[strlen(name)];
			strcpy(ptr->name, name);
			ptr->next				=	pArea;
			ptr->prev				= pArea->prev;
			ptr->next->prev	=	ptr;
			ptr->prev->next	= ptr; 
			pArea=ptr;
			db=pArea->db;
			break;
	}
	if (db->handle) create_dbdatas();
	return true;
}

/****************************************************************************
 *                                                                          *
 * Datei schließen                                                          *
 *                                                                          *
 ****************************************************************************/
bool database::fileclose()
{
  if (!db->handle) 
  	return true;
  if (!dbDatas)
    THROW("database::fileclose(): FATAL! No dbDatas\n");
	delete_dbdatas();

  bool bStatus=db->fileclose();
  return bStatus;
}

bool database::create_dbdatas()
{
	#ifdef DEBUG2
	printf("database::create_dbdatas\n");
	printf("this    : 0x%lx\n",(long)this);
	printf("dbDatas : 0x%lx\n",(long)dbDatas);
	#endif

// interner Compilerfehler von GCC 2.7.2
  if (dbDatas) 
  	THROW("database::create_dbdatas(): FATAL! Found old dbDatas");

  // für jedes Feld ein dbData Objekt einrichten, das den Zugriff regelt
  dbDatas=new dbData[FldCount()];
  for(int i=0; i<FldCount(); i++)
    dbDatas[i].Init(db, i+1 );
  return true;
}

bool database::delete_dbdatas()
{
  ERRORMSG("database.fileclose: delete[] dbDatas\n");
  delete[] dbDatas;
  ERRORMSG("database.fileclose: dbDatas deleted\n");
  dbDatas=NULL;
  return true;
}

/****************************************************************************
 *                                                                          *
 * Use                                                                      *
 *                                                                          *
 ****************************************************************************/
bool database::Use(const char *file)
{
  register bool bStatus=true;

	// printf("file: %s line: %lu\n",__FILE__,(ULONG)__LINE__);
  ERRORMSG("database.Use: entry\n");
  if (file)
  {
    if (db->handle) bStatus=fileclose();
    if (bStatus) bStatus=fileopen(file);
  }
  else
    if (db->handle) bStatus=fileclose();
  ERRORMSG("database.Use: exit\n");
  return bStatus;
}

/****************************************************************************
 *                                                                          *
 * CreateBegin                                                              *
 *                                                                          *
 ****************************************************************************/
bool database::CreateBegin(const char *file, WORD fieldcount)
{
	if (db->handle) fileclose();
	return db->create_begin(file, fieldcount);
}

bool database::CreateAdd(const char *name, char type, BYTE len, BYTE dec)
{
	return db->create_add(name, type, len, dec);
}

bool database::CreateEnd()
{
	if (db->create_end())
	{
		if (db->handle==0)
		{
			printf("database.CreateEnd(): FATAL! handle==0\n");
			exit(1);
		}
		create_dbdatas();
		return true;
	}
	return false;
}

/****************************************************************************
 *                                                                          *
 * Replace                                                                  *
 *                                                                          *
 ****************************************************************************/
bool database::Replace(WORD field, const char *text)
{
  return db->writefield(field,text);
}

bool database::Replace(WORD field, unsigned long value)
{
	char text[255];
	sprintf(text,"%lu",value);
  return db->writefield(field,text);
}
