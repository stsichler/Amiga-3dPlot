/* -------------------------------------------------------------------------- *
 * Plot - 25-12-93 SSi  -  Close.c ------------------------------------------ *
 * -------------------------------------------------------------------------- */

#include "Plot.h"

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <proto/gadtools.h>
#include <graphics/rastport.h>
#include <graphics/displayinfo.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <string.h>
#include <stdlib.h>
#include <dos/dosextens.h>

#define Line(RPort,x1,y1,x2,y2,col) { SetAPen(RPort,col); Move(RPort,x1,y1); Draw(RPort,x2,y2); }

/* Taskkommunikation ----------------------------------------------------- */

#define SendMainMsg(type) { while (MainMsg.Type); MainMsg.Msg.mn_Length=2; MainMsg.Type=type; if (DiskTask_MsgPort) PutMsg(DiskTask_MsgPort,(struct Message *)&MainMsg); else MainMsg.Type=0; }
#define ReplyOwnMsg(msg) ((struct OwnMsg *)msg)->Type=0;
#define ClearMsgPort(port) { struct Message *msg; if (port) while (msg=GetMsg(port)) ReplyMsg(msg); }

extern struct MsgPort *MainTask_MsgPort;
extern struct MsgPort *DiskTask_MsgPort;

extern volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */
extern volatile struct OwnMsg DiskMsg;		/* Nachricht von Disk->Main */

extern struct IntuiMessage *ActMsg,*ActGtmsg;	/* "laufende" Nachrichten */

/* externe Grafikdaten ---------------------------------------------- */

extern far struct Image _3dPlotImage;	/* Image mit Tiefe 4 */
																		/* Farbnummer 0 ist Hintergrundf. */
extern far struct BitMap _3dPlotImBorderBM; /* BitMap mit Tiefe 1 */
																		/* ist 1, wo Image Hintergrundf. ist */
																		
/* private Daten ---------------------------------------------------- */

extern TIME *Image_TH;

/* Globale Zeiger/Daten --------------------------------------------- */

extern struct PlotPrefs *activePrefs,*editPrefs;

extern void *VisualInfo;

extern struct Gadget *FirstGad;	/* erstes Gadget von GadTools.lib */
extern struct Gadget *FirstOpt1Gad;/* erstes Gadget des Options(1)-window */
extern struct Gadget *FirstOpt2Gad;/* erstes Gadget des Options(2)-window */
extern struct Gadget *FirstDiskGad;/*         -"-       Disk-window */
extern struct Gadget *FirstPrefsGad;	/* siehe PPrefs.c */

extern short maxx;		/* letzte Bildspalte (Bildbreite-1) */
extern short maxy;		/* zeigt auf die max. unterste Zeile des Fnkt-Zeichenbereichs */

extern struct Screen *OurS;
extern struct DimensionInfo OurS_DIM;

extern struct Screen *TempS;	/* temporärer Screen zur Überbrückung von CloseAll- */
                      /* OpenAll- Durchläufen (damit WB nicht geöffnet wird) */

extern struct Window *OurW;
extern struct Window *Opt1W;	/* Options-window für Programm (ACHTUNG: Optionen(2)) */
extern struct Window *Opt2W;	/* Options-window für Zeichnen (ACHTUNG: Optionen(1)) */
extern struct Window *DiskW;	/* Disk-window */
extern struct Window *ErrW;	/* Error-window */
extern struct Window *ApsW;	/* AutoPicSave-window */

extern short UseErrorWindow;

extern short BorderCleared;
extern short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY; /* (fest) */
extern short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. für die Funktion */
									/* auf dem Bildschirm (verän.) */
extern double Midx,Midy;				/* Bildschirmmitte (veränderlich) */
extern double _Midx,_Midy;				/* Bildschirmmitte (fest) */

extern short COLm,COLcnt;	/* min. Farbe und Farbzahl für die Funktion */

extern double COL_CC1,COL_CC2,COL_CC3;	/* Farben für DrawCCross() (0-1) */

extern short ModeDisplay_x;	/* x/y-Koordinaten der Modus-Anzeige */
extern short ModeDisplay_y;

extern short DrawMode;	/* Zeichenmodus */

extern short Options;

extern short SpacingX,SpacingY;	/* x/y-Zeichenmusterabstand */
extern double xMin,xMax,yMin,yMax,zMin,zMax;
extern short xRes,yRes;

extern vector LightVect[MAXLIGHTS];	/* Lichtquellen */
extern double LightIntens[MAXLIGHTS];	/* Lichtintensitäten */
extern short LightsOn;
extern short SLS;	/* (ShowedLightSource) Nummer der gerade edit. Lichtquelle */
extern struct Gadget *LightSelG;	/* Lichtselektionsgadget (ID 55) */

extern double Diffuse_Rfx;			/* Reflexionsdaten der Funktion für diffuse(d) */
extern double Direct_Rfx;			/* und direkte(p) Reflexion */

extern TIME *TimeEstTH;	/* für OPT_Time */
extern TIME *APS_TH;		/* für OPT_APS */
extern TIME *Edit_TH;		/* für Edit function */

extern fnctcode *FunctionCode;

extern char *PROGTYPE;	/* Konfigurationsabhängige Anzeige für Screentitel */

extern LONG TimeMinThreshold,APS_MinIntervall;

extern char BackupFileName[];

extern char FunctionString[];		/* Funktion im ASCII-Format */

extern BPTR CD_at_start;			/* CD bei Start des Programms */
extern BPTR FnctDir_Lock;		/* gets UnLock()-ed in CloseAll() */
extern BPTR HomeDir_Lock;	/* Homedir Lock des Programms (needs not to be UnLock()-ed */

extern BPTR DefaultDir_Lock;	/* needs not to be UnLock()-ed */
extern char *Default_fnct_Name;	/* needs not to be free()-ed */


extern char Reset_fnct_Name[];	/* Name der Reset-Funktion */
extern char *_Filename;			/* Beim1. Aufruf von OpenAll() aus main() */
								/* steht hier evtl. der Pfad für  */
								/* Funktionen */

extern short ConsoleState;

extern short OpenAllHitCnt;	/* Zähler für OpenAll()-Durchläufe */

short IDCMP_locked=-1;

/* Garfikstrukturen ----------------------------------------------------- */

extern WORD areaBuffer[25];		/* Areastrukturen */
extern struct AreaInfo areaInfo;
extern struct TmpRas tmpras;

extern char *Title[5];

/* Gadgetzeiger für die Funktions-Laderoutine -------------------------- */

extern struct Gadget *_FuncG,*_xResG,*_yResG,*_xMinG,*_xMaxG,*_yMinG,*_yMaxG,
		*_zMinG,*_zMaxG,*_OPattG,*_OxSpaceG,*_OySpaceG,*_OBodyG,
		*_OxLightG,*_OyLightG,*_OzLightG,*_OPT_ACR_G,*_OPT_ZLimit_G,
		*_OPT_Time_G,*_OPT_Quad_G,*_OPT_WorkBench_G,*_OPT_DitherCol_G,
		*_OPT_LowPri_G,*_OPT_APS_G,*_OGLightStateG,*_dRfxG,*_pRfxG,
		*_OGPatColG,*_OLIntensG,*_OGBakFile,*_OPT_RG3d_G,*_APS_Int_G;
		
/* Verbindung zum Disktask --------------------------------------------- */

extern struct Process *DiskTaskPtr;
extern volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */

extern struct Gadget *_DGPath;
extern struct Gadget *_DGPat;
extern struct Gadget *_DGFile;
extern struct Gadget *_DGList;

/* Fontdefinitionen ---------------------------------------------------- */

extern struct TextAttr font;
extern struct TextFont *textfont;

/* Gadgetprototypen ---------------------------------------------------- */

extern struct NewGadget GZFunct,GSKnob,GLText,GChoose,GStep,GRange1,
                 GRange2,GRText,GAct;

extern struct NewGadget OGChoose;
extern char *_OGChoose1[6];
extern char *_OGChoose2[5];

extern struct NewGadget OGText,OGInput,OGSwitch,OGPrefs,OGMinIn,OGText2,
                 OGLightSel,OGLightChoose,OGBackup;

extern char *_OGLightChoose[3];
extern char *_OGPatColChoose[4];

extern struct NewGadget DGList,DGText,DGLoad,DGSave;

extern struct NewGadget GHelpScroller;

/* importierte Gadgetprototypen ----------------------------------------- */

extern struct NewGadget PGMeasure,PGDArea,PGResol,PGAction,PGSpecial;

/* modulinterne Prototypen ---------------------------------------------- */

void __interrupt CloseWindowSafely(struct Window *);

/* ---------------------------------------------------------------------- *
 * Schließroutine ------------------------------------------------------- *
 * ---------------------------------------------------------------------- */

void CloseAll(int notall)
{
	/* aktuelle Nachricht ggf. zurücksenden */

	if (!notall) RemCtrlBrkHandler();

	if (ActGtmsg) GT_PostFilterIMsg(ActGtmsg);
	ActGtmsg=0;
	if (ActMsg) ReplyMsg((struct Message *)ActMsg); /* und zurück */
	ActMsg=0;
	{
		struct OwnMsg *msg;	/* Disktask stoppen/beenden */
		
		if (DiskTaskPtr)
		{
			SendMainMsg(notall?MT_DETACH:MT_ENDREQ);
			for (msg=0;;)
			{
				if (msg) ReplyMsg((struct Message *)msg);
				WaitPort(MainTask_MsgPort);
				if (!(msg=(struct OwnMsg *)GetMsg(MainTask_MsgPort))) continue;
				if (msg->Msg.mn_Length!=2) continue;
				if (msg->Type==(notall?MT_OK:MT_ENDED)) break;
			}
			ReplyOwnMsg(msg);
		}
	}
	BorderCleared=1;
	UseErrorWindow=0;
	if (OurS) ReduceColors(1);

	if (!notall) UnLock(FnctDir_Lock);
	
	if (tmpras.RasPtr) FreeMem(tmpras.RasPtr,tmpras.Size);
	tmpras.RasPtr=0;

	CloseResolutionWindow(-1);
	ClosePaletteWindow(-1);
	
	CloseWindowSafely(DiskW); DiskW=0;
	CloseWindowSafely(ApsW); ApsW=0;
	CloseWindowSafely(ErrW); ErrW=0;
	CloseWindowSafely(Opt1W); Opt1W=0;
	CloseWindowSafely(Opt2W); Opt2W=0;
	
	IDCMP_locked=-1;

	SetDrawGadget(-1);

	((struct Process *)FindTask(0))->pr_WindowPtr=0;	
	if (OurW)
	{
		struct Window *buf=OurW;
		OurW=0;
		buf->RPort->AreaInfo=0;
		buf->RPort->TmpRas=0;
		CloseWindowSafely(buf);
	}

	if (FirstGad) FreeGadgets(FirstGad);
	if (FirstOpt1Gad) FreeGadgets(FirstOpt1Gad);
	if (FirstOpt2Gad) FreeGadgets(FirstOpt2Gad);	
	if (FirstDiskGad) FreeGadgets(FirstDiskGad);
	if (FirstPrefsGad) FreeGadgets(FirstPrefsGad);
	FirstGad=FirstOpt1Gad=FirstOpt2Gad=FirstDiskGad=FirstPrefsGad=0;
	if (VisualInfo) FreeVisualInfo(VisualInfo);
	VisualInfo=0;

	if (OpenAllHitCnt && notall && OurS)	/* Überbrückungsscreen öffnen */
	{
		static WORD TempCols[] = { 0,0,0,0, 1,0,0,0, -1 };
			
		TempS=OpenScreenTags(0,SA_Depth,(ULONG)1,SA_Width,32,SA_Height,8,
		                     SA_Behind,FALSE,SA_Colors,&TempCols,TAG_DONE);
	}
	if (!notall && TempS) { CloseScreen(TempS); TempS=0; }
	
	if (OurS) CloseScreen(OurS);
	OurS=0;
	ReduceColors(0);	/* um der Routine mitzuteilen, daß sich OurS */
										/* verändern wird. */
	if (!notall)
	{
		Free_Prefs(editPrefs);
		if (Edit_TH) CloseTime(Edit_TH);
		if (TimeEstTH) CloseTime(TimeEstTH);
		if (APS_TH) CloseTime(APS_TH);
		if (Image_TH) CloseTime(Image_TH);
		if (MainTask_MsgPort)
		{
			ClearMsgPort(MainTask_MsgPort);
			DeleteMsgPort(MainTask_MsgPort);
		}
		ClearFunctionCode(&FunctionCode);
	}
	freestr(Title[0]); Title[0]=0;
	freestr(Title[1]); Title[1]=0;
	freestr(Title[2]); Title[2]=0;
	freestr(Title[3]); Title[3]=0;
	freestr(Title[4]); Title[4]=0;

	if (textfont) { CloseFont(textfont); textfont=0; }
	if (!notall)
	{
		/* if (!(Options&OPT_WorkBench)) */ OpenWorkBench();
		DelAllVars(&Vars);
		Free_Prefs(activePrefs);
		activePrefs=0;
	}
}

/* Hilsroutinen ----------------------------------------------------------- */

void __interrupt CloseWindowSafely(struct Window *win)	/* koppelt den UserPort */
{								/* des Windows ab und schließt es. */
	if (win)
	{
		Forbid();
		StripIntuiMessages(win->UserPort,win);
		win->UserPort=0;
		ModifyIDCMP(win,0);
		Permit();
		CloseWindow(win);
	}
}

void __interrupt StripIntuiMessages(struct MsgPort *port,struct Window *win)
{
	struct IntuiMessage *msg,*succ;

	if (port && win)
	{	
		Forbid();
		msg=(struct IntuiMessage *)port->mp_MsgList.lh_Head;
		
		for (;succ=(struct IntuiMessage *)msg->ExecMessage.mn_Node.ln_Succ;msg=succ)
		{
			if (msg->IDCMPWindow==win)
			{
				Remove((struct Node *)msg);
				ReplyMsg((struct Message *)msg);
			}
		}
		Permit();
	}
}

/* Hilfsroutinen ------------------------------------------------------------ */

void LockIDCMP(void)	/* alle IDCMP Ports löschen */
{
	if (!IDCMP_locked)
	{
		IDCMP_locked=-1;
		Forbid();
		ModifyIDCMP(OurW,IDCMP_INTUITICKS);
		ModifyIDCMP(Opt1W,IDCMP_INTUITICKS);
		ModifyIDCMP(Opt2W,IDCMP_INTUITICKS);
		ModifyIDCMP(ErrW,IDCMP_INTUITICKS);
		ModifyIDCMP(ApsW,IDCMP_INTUITICKS);
		Permit();
	}
}

void UnlockIDCMP(void)	/* alle IDCMP Ports wiederherstellen */
{
	if (IDCMP_locked)
	{
		OurW->UserPort=MainTask_MsgPort;
		ModifyIDCMP(OurW, IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_GADGETDOWN|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|ARROWIDCMP|SCROLLERIDCMP|IDCMP_RAWKEY);
		Opt1W->UserPort=MainTask_MsgPort;
		ModifyIDCMP(Opt1W,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY|IDCMP_CHANGEWINDOW);
		Opt2W->UserPort=MainTask_MsgPort;
		ModifyIDCMP(Opt2W,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY|SCROLLERIDCMP|ARROWIDCMP|IDCMP_CHANGEWINDOW);
		ErrW->UserPort=MainTask_MsgPort;
		ModifyIDCMP(ErrW, IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY);
		ApsW->UserPort=MainTask_MsgPort;
		ModifyIDCMP(ApsW, IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY);
		IDCMP_locked=0;
	}
}
