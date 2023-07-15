/* ---------------------------------------------------------------------- */
/* Plot - 24-05-94 SSi    -    Modul zur Fehleranzeige und Iter.Abbruch   */
/* ---------------------------------------------------------------------- */

#include "Plot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <exec/interrupts.h>
#include <libraries/dosextens.h>
#include <intuition/intuitionbase.h>
#include <user/user.h>
#include <user/pointerlists.h>

/* Globale Daten -------------------------------------------------------- */

extern struct Screen *OurS;
extern short Options;

struct Window *ErrW=0;	/* ^ Errorwindow */
short UseErrorWindow=0;	/* soll es benutzt werden ? nein -> stdout */

char __stdiowin[]= "CON:20/50/600/50/3dPlot Error Window";
char __stdiov37[]= "/AUTO/CLOSE";

TIME *Stdout_Error_TH;

PLIST Msgs=0;	/* ^Messages (char []) */
PLIST Times=0;/* ^entspr. Zeiten (timeval) */

/* ---------------------------------------------------------------------- */

void __interrupt _Error(char *,short);

void __interrupt Error(char *str)
{
	_Error(str,-1);
}

void __interrupt Message(char *str)
{
	_Error(str,0);
}

void __interrupt ClrError(void)
{
	_Error(0,1);
}


void __interrupt _Error(char *str,short err)
{	/* es wird der String als Fehler ausgegeben */
	static char Buf[512];
	short changed=0;
	
	if (!str && err!=1) str=l[58];	/* keine Fehlerinformation */
	
	if (ErrW && OurS && UseErrorWindow)
	{
		struct timeval *tv;
		struct timeval ct;
		short a;
		
		CurrentTime(&ct.tv_secs,&ct.tv_micro);
		
		for (a=0;a<PLGetNum(&Msgs);a++)	/* doppelte u. alte Fehler löschen */
		{
			short loeschen=0;
			struct timeval *tv;
			
			tv=(struct timeval *)PLGetPos(&Times,a);
				
			if (str) if (!strcmp((char *)PLGetPos(&Msgs,a),str)) loeschen=1;
			if (tv) if (ct.tv_secs-tv->tv_secs>2) loeschen=1;
			if (loeschen)
			{
				free(PLRemPos(&Msgs,a));
				free(PLRemPos(&Times,a));
				a--;
				changed=1;
			}
		}
		if (str)
		{
			if (tv=malloc(sizeof(*tv)))	/* Fehler zur Liste fügen */
			{
				CurrentTime(&tv->tv_secs,&tv->tv_micro);
				PLAddTail(&Times,tv);
				PLAddTail(&Msgs,strdup(str));
				changed=1;
			}
		}
		if (err==-1) strcpy(Buf,l[57]);	/* evtl."ERROR:" */
		else Buf[0]=0;
		for (a=0;a<PLGetNum(&Msgs);a++)
		{
			strncat(Buf,a?" - ":" ",511);
			strncat(Buf,(char *)PLGetPos(&Msgs,a),511);
		}	

		if (err==-1) DisplayBeep(OurS);
		if (changed)
		{
			SetWindowTitles(ErrW,PLGetNum(&Msgs)?Buf:l[59],OurS->Title);
			if (PLGetNum(&Msgs)) WindowToFront(ErrW); else WindowToBack(ErrW);
		}
	}
	else
	{
		if (err)
		{
			if (!(Options&OPT_WorkBench)) Options|=OpenWorkBench()?OPT_WorkBench:0; 
			WBenchToFront();
			puts(str);
			StartTime(Stdout_Error_TH,3);
		}
	}
	
	return;
}

/****************************************************************************/

/* Ctrl-C Handler als Abbruchcode für unendliche Iterationen -------------- */

unsigned long _IterationBrk;
short _CtrlBrkHandler_active=0;

struct IOStdReq InputReq;
struct Interrupt HandlerStruct;

struct InputEvent * __interrupt __asm __saveds _BrkHandler
	(register __a0 struct InputEvent *,register __a1 struct Screen **);

struct InputEvent * __interrupt __asm __saveds _BrkHandler
(register __a0 struct InputEvent *Event,register __a1 struct Screen **Screen)
{
	static short _pressed=0;
	static ULONG _time;
	ULONG lockptr;
	
	if (_pressed)	/* länger als 3 Sekunden gedrückt ? */
	{
		if (Event->ie_TimeStamp.tv_secs-_time>2) _IterationBrk++;
	}
	lockptr=LockIBase(0);
	if	(Event->ie_Class==IECLASS_RAWKEY && IntuitionBase->ActiveScreen==*Screen)
	{
		if (Event->ie_Code==0x33 && Event->ie_Qualifier&IEQUALIFIER_CONTROL)
		{
			if (!_pressed) _time=Event->ie_TimeStamp.tv_secs;
			_pressed=1;
		}
		if (Event->ie_Code==(0x33|IECODE_UP_PREFIX)) _pressed=0;
	}
	UnlockIBase(lockptr);
	return(Event);
}

/* Ctrl-C Handler Kontrollroutinen ---------------------------------------- */

void AddCtrlBrkHandler() /* - - - - - - - - - - - - - - - - - - - - - */
{
	if (_CtrlBrkHandler_active) return;

	if (OpenDevice("input.device",0,(struct IORequest *)&InputReq,0)) return;
	
	HandlerStruct.is_Code=(void(*)())&_BrkHandler;
	HandlerStruct.is_Data=(APTR)&OurS;
	HandlerStruct.is_Node.ln_Pri=120;
	HandlerStruct.is_Node.ln_Type=NT_INTERRUPT;

	InputReq.io_Command=IND_ADDHANDLER;
	InputReq.io_Message.mn_ReplyPort=&((struct Process *)FindTask(0))->pr_MsgPort;
	InputReq.io_Data=(void *)&HandlerStruct;

	if (!(_CtrlBrkHandler_active=!DoIO((struct IORequest *)&InputReq))) Error(l[134]);
}

void RemCtrlBrkHandler() /* - - - - - - - - - - - - - - - - - - - - - */
{
	if (InputReq.io_Device)
	{
		if (_CtrlBrkHandler_active)
		{
			InputReq.io_Command=IND_REMHANDLER;
			DoIO((struct IORequest *)&InputReq);	/* Handler löschen */
			_CtrlBrkHandler_active=0;
		}
		CloseDevice((struct IORequest *)&InputReq);
	}
	InputReq.io_Device=0;
}	
