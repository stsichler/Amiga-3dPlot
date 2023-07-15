/* ------------------------------------------------------------------------- *
 * Plot V2.6+ SSi - Hilferoutinen 2-6-94 ----------------------------------- *
 * ------------------------------------------------------------------------- *
 * Plot V4.00 SSi - ersetzt 13-10-94                                         *
 * ------------------------------------------------------------------------- */

#include "Plot.h"
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <user/pointerlists.h>

/* globale Daten ----------------------------------------------------------- */

extern struct Screen *OurS;
extern struct Window *OurW;

extern short Options;

extern void *VisualInfo;
extern short BorderDrawx,BorderDrawy,BorderDrawX,BorderDrawY;

char *HelpFile;

extern struct TextFont *textfont;

extern double COL_CC3;	/* Farbnummer (0-1) für Hilfe (u. DrawCCross()) */

struct Gadget *HelpScrollerGad=0,*FirstHelpScrollerGad=0;
extern struct NewGadget GHelpScroller;

/* private Daten ----------------------------------------------------------- */

short Help_displayed;

PLIST TextLines=0;
short FirstDisplayedLine,VisibleLines,MaxVisible;

/* private Prototypen ------------------------------------------------------ */

int AddLine(char *,short);
void ClearLines(void);
void PrintLines(short);

void MakeHelpScroller(short,short);
/* void RemoveHelpScroller(void); */	/* in Plot.h */

/* Routinen ---------------------------------------------------------------- */

void DisplayHelp(short HelpNum)				/* ruft Hilfeinformation ab. */
{
	FILE *hf;
	static char Buffer[512];
	char HB[10];
	short Found=0;
	short maxwidth_pixels;

	if (!HelpNum) return;

	MaxVisible=(BorderDrawY-BorderDrawy-1)/H(8);
	maxwidth_pixels=GHelpScroller.ng_LeftEdge-BorderDrawx-5;

	ClearLines();	
	
	if (HelpFile && (hf=fopen(HelpFile,"ra")))
	{
		HB[0]='.';				/* Helpnode-Name bilden */
		stci_d(&HB[1],HelpNum);
		strcat(HB,".");
		
		while (!(feof(hf) || Found))		/* und Node suchen */
		{
			fgets(Buffer,512,hf);
			if (HB[0]!='.') continue;
			if (strstr(Buffer,HB)) Found=1;
		}
		
		if (Found)						/* wenn gefunden */
		{						
			while (!feof(hf))			/* einlesen */
			{
				short len;
				if (!(fgets(Buffer,512,hf))) break;
				
				if (len=strlen(Buffer)) Buffer[--len]=0;	/* \n löschen */
				if (Buffer[0]=='\\') continue;
				if (!stricmp(Buffer,".end")) break;
				
				if (AddLine(Buffer,maxwidth_pixels)) break;
			}
			if (Options&OPT_AWH) WindowToFront(OurW);

			Help_displayed=1;
			{
				PrintLines(0);			/* und ausdrucken */
				
				if (VisibleLines<PLGetNum(&TextLines))	/* Scroller nötig ? */
				{					
					MakeHelpScroller(VisibleLines,PLGetNum(&TextLines));/* Scroller einblenden */
				}
			}
		}
		fclose(hf);
	}
	else Error(l[72]);
	
	if (!Found)
	{
/*		SetColor(COL_CC3,0);
		SetDrMd(OurW->RPort,JAM1);
		Move(OurW->RPort,W(20),H(40));
		Text(OurW->RPort,l[7],strlen(l[7]));
*/		NormalColor();
		Error(l[7]);
	}
	return;	
}

/* ------------------------------------------------------------------------ */

void EndDisplayHelp(void)		/* beendet die Einblendung von Hilfeinf. */
{
	if (Help_displayed)
	{
		Help_displayed=0;
		RemoveHelpScroller();
		ClearLines();
	}
}

/* ------------------------------------------------------------------------ */

int AddLine(str,len)	/* fügt eine Zeile zur Liste "FirstLine->" hinzu */
char *str;								/* mit max. Pixelbreite len und Höhe height */
short len;								/* RETURN: error */
{
	short accLen,i;					/* accepted Len */
	char z;
	int err=0;
	char *text;
	
	if (!str) return(0);
		
	if (accLen=strlen(str))
	{	
		struct TextExtent tf_result;
		accLen=TextFit(OurW->RPort,str,accLen,&tf_result,0,1,len,H(8)+1);
	
		if (str[accLen])  /* ggf. auf Wörtergrenze bringen */
		{
			for (i=accLen;str[i-1]!=' ' && i;i--);
			if (i) accLen=i;
		}
	
		if (!accLen) return(-1);
	}
	z=str[accLen];
	str[accLen]=0;
	
	if (!(text=(char *)malloc(accLen+1))) { Error(l[68]); return(-1); }

	strcpy(text,str);
	PLAddTail(&TextLines,text);				/* und in Liste hängen */
	
	if (str[accLen]=z) err=AddLine(&str[accLen],len);
	return(err);
}

/* ------------------------------------------------------------------------ */

void ClearLines(void)		/* löscht alle Einträge der "TextLine" Liste */
{
	PLClear(&TextLines);
	FirstDisplayedLine=-1;
	VisibleLines=0;
	SetAPen(OurW->RPort,3);
	SetBPen(OurW->RPort,3);
	SetDrMd(OurW->RPort,JAM1);
	RectFill(OurW->RPort,BorderDrawx,BorderDrawy,BorderDrawX,BorderDrawY);
}

/* ------------------------------------------------------------------------ */

void PrintLines(short anf)		/* druckt die Zeilen ab Zeile #anf aufs Bild */
{                         
	short x,y;
	short delta;
	short FirstPrintPos=0,PrintLen=MaxVisible;
	
	if (FirstDisplayedLine==anf) return;
	
	if (Help_displayed)
	{
		SetAPen(OurW->RPort,3);
		SetBPen(OurW->RPort,3);
		SetDrMd(OurW->RPort,JAM1);
		
		if (Options&OPT_ACR) ReduceColors(1);

		/* Scrolling überprüfen */
		if (VisibleLines)
		{
	      delta=anf-FirstDisplayedLine;
	      	
				if (delta>-MaxVisible && delta<MaxVisible)
	      {
					FirstPrintPos=(delta<0)?0:(MaxVisible-delta);
					PrintLen=abs(delta);
					
					ScrollRaster(OurW->RPort,0,delta*H(8),BorderDrawx,BorderDrawy,GHelpScroller.ng_LeftEdge-2,BorderDrawY);
				} else delta=0;
		} else delta=0;
		
		FirstDisplayedLine=anf;
		if (delta>0) anf+=MaxVisible-delta;
		VisibleLines=min(VisibleLines,MaxVisible-delta);
	
		if (!delta) RectFill(OurW->RPort,BorderDrawx,BorderDrawy+FirstPrintPos*H(8),GHelpScroller.ng_LeftEdge-2,BorderDrawy+(FirstPrintPos+PrintLen)*H(8)-1);
	
		SetColor(COL_CC3,0);
		SetDrMd(OurW->RPort,JAM1);
		x=BorderDrawx;	/* Anfangskoordinaten */
		y=BorderDrawy+FirstPrintPos*H(8)+textfont->tf_Baseline;								
		
		for (;anf<PLGetNum(&TextLines) && PrintLen>0;anf++,VisibleLines++,PrintLen--)
		{
			Move(OurW->RPort,x,y);
			Text(OurW->RPort,(char *)PLGetPos(&TextLines,anf),strlen((char *)PLGetPos(&TextLines,anf)));
			y+=H(8);
			if (y>=BorderDrawY-textfont->tf_YSize+textfont->tf_Baseline) break;
		}
		if (Options&OPT_ACR) ReduceColors(0);
	}
	NormalColor();
	return;
}

/* ------------------------------------------------------------------------ */

void MakeHelpScroller(short visible,short total)
{
	struct Gadget *gad;
	gad=CreateContext(&FirstHelpScrollerGad);

	if (Options&OPT_ACR) ReduceColors(1);		
	HelpScrollerGad=gad=CreateGadget(SCROLLER_KIND,gad,&GHelpScroller,GTSC_Arrows,10,GA_RelVerify,TRUE,PGA_Freedom,LORIENT_VERT,GTSC_Visible,visible,GTSC_Total,total,TAG_END);
	if (!gad) { Error(l[80]); return; }
	AddGList(OurW,FirstHelpScrollerGad,-1,-1,0);
	GT_RefreshWindow(OurW,0);
	RefreshGadgets(FirstHelpScrollerGad,OurW,0);
	if (Options&OPT_ACR) ReduceColors(0);		
}

/* ------------------------------------------------------------------------ */

void RemoveHelpScroller(void)
{
	if (HelpScrollerGad)
	{
		RemoveGList(OurW,FirstHelpScrollerGad,-1);
		FreeGadgets(FirstHelpScrollerGad);
		HelpScrollerGad=0;
	}
}
