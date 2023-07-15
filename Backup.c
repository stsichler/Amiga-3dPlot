/* ---------------------------------------------------------------------- */
/* Plot - 12-8-94 SSi     -    Backup.c                                   */
/* ---------------------------------------------------------------------- */

#include "Plot.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <dos.h>
#include <math.h>
#include <exec/memory.h>

/* globale Daten ------------------------------------------------------------ */

extern short Options;

char BackupFileName[560];

/* -------------------------------------------------------------------------- */

void BackupFile(char *Filename)	/* legt ein Backup des angegebenen Files an */
{
	BPTR in,out;
	
	APTR Buf;
	LONG BufLen;
	LONG bip;	/* BytesInBuffer */
	
	static char CommentString[80];
	
	if (!BackupFileName[0]) return;
	if (!Filename) return;
	if (!*Filename) return;
	
	{								/* Filekommentar vorbereiten */
		static char node[FESIZE];
		strcpy(CommentString,"Backup of ");
		stcgfn(node,Filename);
		strncat(CommentString,node,69);
	}
	
	if (in=Open(Filename,MODE_OLDFILE))
	{
		Seek(in,0,OFFSET_END);				/* Buffergröße ermitteln */
		BufLen=Seek(in,0,OFFSET_BEGINNING);

		Forbid();
		{
			LONG avmem=AvailMem(MEMF_LARGEST)/2;
			BufLen=min(BufLen,avmem);
		}
		if (Buf=AllocMem(BufLen,0))
		{
			Permit();
			if (out=Open(BackupFileName,MODE_NEWFILE))
			{
				for (;;)		/* Kopierschleife */
				{
					if ((bip=Read(in,Buf,BufLen))<=0) break;
					
					Write(out,Buf,bip);
				}
				Close(out);
				SetComment(BackupFileName,CommentString);
			}
			FreeMem(Buf,BufLen);
		}
		else Permit();
		Close(in);
	}
	return;	
}
