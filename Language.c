/* ---------------------------------------------------------------------- */
/* Plot - 23-05-94 SSi    -    Sprachenmodul                              */
/* ---------------------------------------------------------------------- */

#include "Plot.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <proto/intuition.h>

/* Globale Daten -------------------------------------------------------- */

char **l;				/* Zeiger auf aktiven Wortschatz */
extern char *lang[];	/* Zeiger auf Defaultwortschat */
char *Language;

extern struct Screen *OurS;

/* ---------------------------------------------------------------------- */

void LoadLanguage(void)	/* versucht den in "Language" angegebenen */
						/* Sprachfile zu öffnen und zu übernehmen */
{
	char buf[16];
	short lvers,lsubvers;
	FILE *fp;
	
	l=lang;	/* Defaultwortschatz setzen */
	
	if (!Language) return;
	
	if (!(fp=fopen(Language,"rb"))) { Error(l[84]); return; }
	
	fread(buf,sizeof(char),16,fp);	/* ID lesen */
	
	if (!memcmp(buf,"3dPlot__Language",16))
	{
		fread(&lvers,sizeof(short),1,fp);	/* richtige Version ? */
		fread(&lsubvers,sizeof(short),1,fp);
		if (lvers==PLOTVERSION && lsubvers==PLOTSUBVERSION)
		{
			long size;
			char **list;
	
			fread(&size,sizeof(long),1,fp);	/* Größe einlesen */
			
			if (list=(char **)malloc(size))	/* und allokieren */
			{
				short a;
				
				fread((APTR)list,size,1,fp);	/* Liste einlesen */
												/* und relozieren */
				for (a=0; list[a]; a++) list[a]=(char *)((ULONG)list[a]+(ULONG)list);
			
				l=list;	/* und Wortschatz aktivieren */
			} else Error(l[68]);
		} else Error(l[85]);
	} else Error(l[86]);
	fclose(fp);
}

