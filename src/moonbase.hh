/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1995 MAGIC INCLUDED																					*
 *  (C)opyright 1995 Mark-André Hopf                                        *
 *  filename: database.hh                                                   *
 *  relation: database.cc                                                   *
 *  author  : Mark-André Hopf                                               *
 *  date    : 29.04.1996                                                    *
 *                                                                          *
 ****************************************************************************/

#ifndef __DATABASE_HH
#define __DATABASE_HH

extern const char *MOONBASE_VERSION;

class database
{
  private:
    bool fileopen(const char*);         // setzt dbDatas -> dbFileIO::fileopen()
    bool fileclose();             // löscht dbDatas -> dbFileIO::fileopen()

    dbData *dbDatas;
		bool create_dbdatas();				// dbDatas für aktuelle Datei erzeugen
		bool delete_dbdatas();				// dbDatas für aktuelle Datei löschen

		// Datenbankbereiche
		class defArea
		{
			public:
				char 		 *name;						// Name für den Bereich
				dbFileIO *db;							// Zeiger auf zugehörige Datenbank
				defArea  *prev,*next;			// doppelt verkettete Liste erleichtert
																	// das Löschen
		};
		defArea *pAreaList;						// Liste der Bereiche
		defArea *pArea;								// Zeiger auf den aktuellen Bereich
		dbFileIO *db;									// Zeiger auf die aktuelle Datenbank

  public:
    database();
    ~database();
    bool Use(const char *file=NULL);          // Datenbank öffnen/schließen
		bool Select(const char *area);						// Bereich auswählen
		bool CreateBegin(const char *file, WORD fieldcount);
		bool CreateAdd(const char *name, char type, BYTE len, BYTE dec);
		bool CreateEnd();

		const char* Dbf()				{return db->Dbf();}
		WORD FldCount()					{return db->FldCount();}
		WORD RecCount()					{return db->RecCount();}
		WORD RecSize()					{return db->RecSize();}
		const char* Field(WORD nr){return db->Field(nr);}
		WORD FieldNo(const char *name){return db->FieldNo(name);}
		BYTE FieldLen(WORD nr)	{return db->FieldLen(nr);}
		BYTE FieldDec(WORD nr)	{return db->FieldDec(nr);}
		char FieldType(WORD nr)	{return db->FieldType(nr);}
		const char* TypeName(char type){return db->TypeName(type);}
		bool ListStructure()		{return db->ListStructure();}

		bool Go(ULONG nr)				{return db->Go(nr);}
    bool Skip(long dir=1) 	{return db->Skip(dir);}
    ULONG RecNo()						{return db->RecNo();}
    bool Bof()							{return db->Bof();}
    bool Eof()							{return db->Eof();}
    bool AppendBlank()			{return db->AppendBlank();}
    bool RLock()						{return db->RLock();}
    bool Unlock()						{return db->Unlock();}
    bool UnlockAll()				{return db->UnlockAll();}
    bool Deleted()					{return db->Deleted();}
    bool Delete()						{return db->Delete();}
    bool Recall()						{return db->Recall();}
    bool SetDeleted(bool on){return db->SetDeleted(on);}
    bool CopyMemoTo(const char* field, const char* file, bool add=false)
    			{return db->CopyMemoTo(field,file,add);}

		const char* lasterror()	{return db->lasterror();}
    bool Replace(WORD,const char*);
    bool Replace(WORD,unsigned long);
		bool Replace(const char* field, const char *value){return Replace(FieldNo(field),value);}
    dbData& operator ()(const char *field) {return dbDatas[FieldNo(field)-1];}
    dbData& operator ()(int fn) {return dbDatas[fn-1];}
};

#endif
