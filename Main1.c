/* ---------------------------------------------------------------------- */
/* Plot - 25-12-93 SSi    -    HAUPTMODUL  (Teil 1)                       */
/* ---------------------------------------------------------------------- */
/* Moduskontrollroutinen und zentrale Hilfsroutinen */

#include "Plot.h"

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <graphics/gfxbase.h>
#include <graphics/gfx.h>
#include <string.h>
#include <dos.h>
#include <user/functions.h>
#include <user/user.h>
#include <exec/execbase.h>

extern struct IntuitionBase *IntuitionBase;

long __oslibversion = 37;

long __stack = 0x2800;

/* globale Zeiger etc. ------------------------------------------------- */

struct Screen *OurS;
struct Window *OurW;
struct Window *Opt1W;
struct Window *Opt2W;
struct Window *DiskW;
struct Window *ApsW;
extern struct Window *ResW;
extern struct Window *PalW;
extern void *VisualInfo;

extern struct NewGadget GAct;

extern struct Gadget *_FuncG,*_xResG,*_yResG,*_xMinG,*_xMaxG,*_yMinG,*_yMaxG,
		*_zMinG,*_zMaxG,*_OPattG,*_OxSpaceG,*_OySpaceG,*_OBodyG,
		*_OxLightG,*_OyLightG,*_OzLightG,*_OPT_ACR_G,*_OPT_ZLimit_G,
		*_OPT_Time_G,*_OPT_Quad_G,*_OPT_WorkBench_G,*_OPT_DitherCol_G,
		*_OPT_APS_G,*_OGLightStateG,*_dRfxG,*_pRfxG,*_OGPatColG,
		*_OLIntensG;

extern char *TOOLTYPES;		/* Tooltype Definitionen */
extern char *PROGTYPE;	/* Konfigurationsabhängige Anzeige für Screentitel */

short BorderCleared;			/* Zeigt an, daß der BorderDraw Bereich */
							/* verändert ist */
short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY;	/* min. und max. Koor. allgem. */
short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. für die Funktion */
								/* auf dem Bildschirm */
double Midx,Midy;					/* Koordinatenmitte (veränderlich) */
double _Midx,_Midy;				/*      -"-         (fest) */

extern struct RGColSupp *RGColSupp;	/* RG-3d ColorSupp - Struktur */

extern TIME *Stdout_Error_TH;	/* wird bei Fehlerausgabe gestartet */

/* Daten der Funktion *****************************************************/

char FunctionString[MAX_FNCTSTR_LEN+2];	/* Funktion im ASCII-Format */

fnctcode *FunctionCode;		/* RAW-Code der Funktion */

short Status=ST_NO_FUNC;	/* Funktion zu berechnen 0 - ja / !=0 - nein */

struct VarList Vars;					/* VARIABLEN-WURZELSTRUKTUR */

double *X;					/* Zeiger auf x,y Variablen */
double *Y;

double xMin=-4.0;
double xMax=4.0;
double yMin=-4.0;
double yMax=4.0;			/* Bereich der Funktion */
double zMin=-4.0;
double zMax=4.0;

vector FuncMid;	// Funktionsmitte

short xRes=40;			/* Berechnungsschritte */
short yRes=40;

short SpacingX=0,SpacingY=0;		/* x/y - Musterabstand */
double xResPac=40.0,yResPac=40.0;	/* =Resol./(Spacing+1) */

double yAngle=330.0*pi/180.0;			/* Kippwinkel der Funktion */
double zAngle=20.0*pi/180.0;

double SizeFact;			/* Vergrößerungsfaktor der Funktion */

extern vector LightVect[MAXLIGHTS];	/* Lichtquellen */
extern double LightIntens[MAXLIGHTS];	/* Lichtquellenintensitäten (0-1) */
extern short LightsOn;
short SLS;	/* (ShowedLightSource) Nummer der gerade edit. Lichtquelle */
extern struct Gadget *LightSelG;	/* Lichtselektionsgadgets (ID 55) */

double Diffuse_Rfx=1.0;		/* Reflexionsdaten der Funktion für diffuse */
double Direct_Rfx=0.0;			/* und direkte Reflexion */

double PatternCol=0.5,PatternDeltaCol=0.15;	/* Funktionsfarben */
double BodyCol=0.3;

extern struct DateStamp PicLastSave;/* Datum+Zeit der letzten Bildspeicherung */
extern LONG PicSecWorth;	/* Anzahl Sekunden, die das Bild "wert" ist. */
						/* -1: kein Bild auf dem Schirm */
						/*  0: wertloses Bild af dem Schirm */
						/* >0: Bild, das ## sec wert ist, auf dem Schirm */
						
/*************************************************************************/

/* Globale Daten ------------------------------------------------------- */

short mode=MODE_START;		/* derz. Arbeitsmodus des Hauptprogramms */

short ConsoleState=CS_CLEAR;	/* derz. Anzeigen auf der Konsole */

short submode=0;				/* im Editmode : GadgetID des Editgadgets */
							/* im Drawmode :	0 - Zeichenbeginn */
							/*					1 - beim Zeichnen */
							/*					-1 - Zeichenende */

double _EditFact;	/* Faktor für logarithm. Editieren */
#define _EDITFACT_REL (1.4)

short DrawMode=DMF_LX|DMF_LY|DMF_FL;		/* Zeichenmodus */

short Options=OPT_ACR|OPT_Quad|OPT_WorkBench|OPT_DitherCol /* Optionen */
			|OPT_AWH;
			
LONG TimeMinThreshold=2;		/* Anzahl der Minuten, die ein Bild wert */
							/* sein muß, daß bei Löschgefahr Sicherheits- */
							/* Abfragen gemacht werden (in min.) */
				
LONG APS_MinIntervall=5;		/* Zeitintervall, in dem bei OPT_APS das */
							/* Bild automatisch gesichert wird. (in min.) */

char APS_Filename[560]= "";	/* AutoPicSave-Filename */

TIME *APS_TH,*Edit_TH;					/* TimeHandle */

extern struct DrawingInfo DI;

extern char BackupFileName[];	/* Filename für OPT_Backup */

extern struct ExecBase *SysBase;

ULONG Workbench_DisplayID;

extern short ResWx,ResWy,PalWx,PalWy;

extern short maxx,maxy;
						
/* Verbindung zum Disktask --------------------------------------------- */

struct Process *DiskTaskPtr;

volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */

extern char *_Filename;			/* Filename vom Disktask */

char Filename[560];					/* Filename für Laden/Speichern */

BPTR CD_at_start;					/* Current Dir bei Programmstart */

BPTR HomeDir_Lock;	/* Homedir Lock des Programms (needs not to be UnLock()-ed */

/* Taskkommunikation ---------------------------------------------------- */

#define SendMainMsg(type) { while (MainMsg.Type); MainMsg.Msg.mn_Length=2; MainMsg.Type=type; if (DiskTask_MsgPort) PutMsg(DiskTask_MsgPort,(struct Message *)&MainMsg); else MainMsg.Type=0; }
#define ReplyOwnMsg(msg) ((struct OwnMsg *)msg)->Type=0;

struct MsgPort *MainTask_MsgPort;
struct MsgPort *DiskTask_MsgPort;

       volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */
extern volatile struct OwnMsg DiskMsg;		/* Nachricht von Disk->Main */

struct IntuiMessage *ActMsg,*ActGtmsg;		/* "laufende" Nachrichten */

/* Gadgethandlerliste (siehe GList.c) ---------------------------------- */

extern void (*GadgetDownHandle[])(struct Gadget *,USHORT);
extern void (*GadgetUpHandle[])(struct Gadget *,USHORT);

/* externe Prototypen -------------------------------------------------- */

void GUH1(struct Gadget *,USHORT);
void GUH2(struct Gadget *,USHORT);
void GUH3(struct Gadget *,USHORT);
void GUH25(struct Gadget *,USHORT);
void GUH4to9(struct Gadget *,USHORT);
void GUH63(struct Gadget *,USHORT);
void GUH82(struct Gadget *,USHORT);
void GUH83(struct Gadget *,USHORT);
void GUH84(struct Gadget *,USHORT);
void GUH91(struct Gadget *,USHORT);

void goto_exit(int code)					/* EXIT-Routine ***********************/
{
	if (CD_at_start) UnLock(CurrentDir(CD_at_start));
	if (Stdout_Error_TH)
	{
		WaitTime(Stdout_Error_TH);
		CloseTime(Stdout_Error_TH);
	}
	Stdout_Error_TH=0;
	exit(code);
}

/* --------------------------------------------------------------------- */
/* Hauptprogramm ------------------------------------------------------- */
/* --------------------------------------------------------------------- */

int BreakFunc(void) { return(0); }

void main(int argc, char *argv[])
{
	USHORT ID;

	Stdout_Error_TH=argc?0:OpenTime();	/* Bei Worchbenchstart Timer alloc. */
	
	if (argc==2 && argv[1][0]=='?' && !argv[1][1])
	{
		char progname[FESIZE+1];
		
		stcgfn(progname,argv[0]);
		
		printf("\n[1m3dPlot V%hd.%02hd %s[0m - ®SSi\\93-97\n\nSYNTAX: %s <Option 1> [<Opt.2> [<Opt.3> ...]]\n\nAccepted OPTIONS (also accepted as tooltypes) are:\n",PLOTVERSION,PLOTSUBVERSION,PROGTYPE,progname);
		puts(TOOLTYPES);
		goto_exit(0);
	}

	OSVERSION=SysBase->LibNode.lib_Version;
	
	{
		struct Screen *wbscreen;
		if (wbscreen=LockPubScreen(0))
		{
			Workbench_DisplayID=GetVPModeID(&wbscreen->ViewPort);
			UnlockPubScreen(0,wbscreen);
		} else Workbench_DisplayID=DEFAULT_MONITOR_ID|HIRES_KEY;
	}
	
	onbreak(BreakFunc);
	
	/* original-Pfad (CD) sichern und verändern */

	CD_at_start=CurrentDir(DupLock(HomeDir_Lock=GetProgramDir()));
	
	ComputeWBMsg(argc,argv);	/* Übergabeparameter auswerten / Spr. laden */

	/* ----------------------------------------------------------------- */

	if (Mode(MODE_START))			/* und starten */
	{
		short Last_RAWKEY_Msgcode=-1;
		for (;;)
		{													
			DoMode();				/* Modus ausführen */
	
			if (ActMsg=(struct IntuiMessage *)GetMsg(MainTask_MsgPort))
			{
				if (ActMsg->ExecMessage.mn_Length==2)/* eigene Msg. */
				{
					ReplyOwnMsg(ActMsg); /* keine erwartet -> ignorieren */
					ActMsg=0;
				}
				else	/* IntuitionMessage */
				{
					if (ActGtmsg=GT_FilterIMsg(ActMsg)) /* Message auswerten */
					{
						if (!(ActGtmsg->Class&INTUITICKS)) ClrError();
						/***********/
/*						{
							char buf[16];
							sprintf(buf,"%p",(ULONG)ActGtmsg->Class);
							SetAPen(OurW->RPort,1);
							SetBPen(OurW->RPort,3);
							SetDrMd(OurW->RPort,JAM2);
							Move(OurW->RPort,(short)Drawx,(short)Drawy+8);
							Text(OurW->RPort,buf,strlen(buf));
							sprintf(buf,"%p",(ULONG)ActGtmsg->Code);
							Move(OurW->RPort,(short)Drawx,(short)Drawy+16);
							Text(OurW->RPort,buf,strlen(buf));
						}
*/						/***********/
						/************* DO NOT SPECIFY IDCMP_VANILLAKEY !!!! **********/
						if (ActGtmsg->Class&IDCMP_RAWKEY)
						{
							short oldsm=submode;
							static char PressedKeys[128];

							if (ActGtmsg->Code!=Last_RAWKEY_Msgcode)
							{
								Last_RAWKEY_Msgcode=ActGtmsg->Code;
								submode=-1;
								if (ActGtmsg->Code>=0 && ActGtmsg->Code<256) PressedKeys[ActGtmsg->Code&0x7f]=(ActGtmsg->Code&0x80)?0:1;
									
								if (ConsoleState==CS_MAIN)
								{
								  submode=0;
									     if (PressedKeys[0x2d]) submode=8; // ay+
									else if (PressedKeys[0x2e]) submode=5; // ay-
									else if (PressedKeys[0x2f]) { GUH2(0,0); PressedKeys[0x2f]=0; } // ay 0
									else if (PressedKeys[0x1d]) submode=9; // az+
									else if (PressedKeys[0x1e]) submode=6; // az-
									else if (PressedKeys[0x1f]) { GUH3(0,0); PressedKeys[0x1f]=0; } // az 0
									else if (PressedKeys[0x3c]) { GUH25(0,0);PressedKeys[0x3d]=0; } // Bild zentrieren
									else if (PressedKeys[0x3d]) submode=7; // size+
									else if (PressedKeys[0x3e]) submode=4; // size-
									else if (PressedKeys[0x3f]) { GUH1(0,0); PressedKeys[0x3f]=0; } // size A
									else if (PressedKeys[0x4c]) submode=23;// up
									else if (PressedKeys[0x4f]) submode=24;// left
									else if (PressedKeys[0x4e]) submode=26;// right
									else if (PressedKeys[0x4d]) submode=27;// down
								}
								if (submode>0)	/* Gadget(Key)Down */
								{
									if (mode!=MODE_EDIT) if (!Mode(MODE_STARTEDIT)) submode=oldsm;
								}
								else /* Gadget(Key)Up */
								if (!submode && (mode==MODE_EDIT || mode==MODE_STARTEDIT)) { submode=oldsm; Mode(MODE_DISPL); }
								else submode=oldsm;
								if (PressedKeys[0x5f])
								{
									short helpnum=0;
									PressedKeys[0x5f]=0;
									if (ActGtmsg->IDCMPWindow==OurW) helpnum=1;
									if (ActGtmsg->IDCMPWindow==Opt2W) helpnum=2;
									if (ActGtmsg->IDCMPWindow==Opt1W) helpnum=3;
									if (ActGtmsg->IDCMPWindow==DiskW) helpnum=4;
									if (ActGtmsg->IDCMPWindow==ResW) helpnum=5;
									if (ActGtmsg->IDCMPWindow==PalW) helpnum=6;
									
									DisplayHelp(helpnum);
								}
							}
						}
						else if (ActGtmsg->Class&IDCMP_MOUSEBUTTONS && ActGtmsg->Code==SELECTUP && mode==MODE_EDIT) GUH4to9(0,0);
						
						else if (ActGtmsg->Class&IDCMP_CLOSEWINDOW)	/* Ende oder anderes Fenster ? */
						{
							if (ActGtmsg->IDCMPWindow==OurW)	/* Ende einleiten */
							{
								if (Mode(MODE_TEST))
								{
									if (AskForExit()) Mode(MODE_ENDREQ);
								}
							}
							if (ActGtmsg->IDCMPWindow==Opt1W) WindowToBack(Opt1W);
							if (ActGtmsg->IDCMPWindow==Opt2W) WindowToBack(Opt2W);
							if (ActGtmsg->IDCMPWindow==DiskW) WindowToBack(DiskW);
							if (ActGtmsg->IDCMPWindow==ResW) CloseResolutionWindow(1);
							if (ActGtmsg->IDCMPWindow==PalW) GUH91(0,0);
						}
						else if (ActGtmsg->Class&IDCMP_CHANGEWINDOW)	/* Window verschoben */
						{
							short dx,dy;
							if (ActGtmsg->IDCMPWindow==Opt1W)
							{
								dx=Opt1W->LeftEdge-Opt2W->LeftEdge;
								dy=Opt1W->TopEdge-Opt2W->TopEdge;
								if (dx || dy) MoveWindow(Opt2W,dx,dy);
							}
							if (ActGtmsg->IDCMPWindow==Opt2W)
							{
								dx=Opt2W->LeftEdge-Opt1W->LeftEdge;
								dy=Opt2W->TopEdge-Opt1W->TopEdge;
								if (dx || dy) MoveWindow(Opt1W,dx,dy);
							}
							if (ActGtmsg->IDCMPWindow==ResW) { ResWx=ResW->LeftEdge; ResWy=ResW->TopEdge; }
							if (ActGtmsg->IDCMPWindow==PalW) { PalWx=PalW->LeftEdge; PalWy=PalW->TopEdge; }
						}
						else if (ActGtmsg->Class&(IDCMP_GADGETDOWN|IDCMP_GADGETUP|IDCMP_MOUSEMOVE))	/* Gadgetmessage */
						{
							if ((ID=((struct Gadget *)(ActGtmsg->IAddress))->GadgetID)<=GADGETNUM)
							{
								if (ActGtmsg->Class&IDCMP_GADGETDOWN && GadgetDownHandle[ID]) GadgetDownHandle[ID]((struct Gadget *)ActGtmsg->IAddress,ActGtmsg->Code);
								if (ActGtmsg->Class&IDCMP_MOUSEMOVE)
								{
									switch (ID)
									{
										case 63: GUH63((struct Gadget *)ActGtmsg->IAddress,ActGtmsg->Code); break;
										case 82: GUH82((struct Gadget *)ActGtmsg->IAddress,ActGtmsg->Code); break;
										case 83: GUH83((struct Gadget *)ActGtmsg->IAddress,ActGtmsg->Code); break;
										case 84: GUH84((struct Gadget *)ActGtmsg->IAddress,ActGtmsg->Code); break;
									}
								}
								if (ActGtmsg->Class&IDCMP_GADGETUP)
								{
									if (ActGtmsg->Code==0x5f)	/* ExitHelp Msg ? */
									{
										if (Mode(MODE_HELP)) submode=ID;
										ActivateGadget(ActGtmsg->IAddress,ActGtmsg->IDCMPWindow,0);
									}
									else if (GadgetUpHandle[ID]) GadgetUpHandle[ID]((struct Gadget *)ActGtmsg->IAddress,ActGtmsg->Code);
								}
							}
						}
						if (ActGtmsg) GT_PostFilterIMsg(ActGtmsg);
						ActGtmsg=0;
					}									
				}
				if (ActMsg) ReplyMsg((struct Message *)ActMsg); /* und zurück */
				ActMsg=0;
			}
			CloseResolutionWindow(2);
			ClosePaletteWindow(2);
		}
	}
}

/* Modus-Kontrollroutinen  ---------------------------------------------- */

int Mode(newmode)				/* Schaltet auf den Modus um und führt */
short newmode;				/* die dabei nötigen Schritte aus */
{							/* RETURN: 0 bei Fehler / -1 bei OK. */
	static short oldmode,oldsubmode;

	if (newmode==MODE_SAVE || newmode==MODE_LOAD)
	{
		oldmode=mode;
		oldsubmode=submode;
	}
	else
	{
		if (mode==MODE_DRAW && mode!=newmode)
		{
			short sm=submode;
			if (submode!=-1 && newmode!=MODE_ENDREQ) if (!AskForClear()) return(0);
			if (newmode==MODE_TEST) return(-1);
			submode=-1;
			DoMode();		/* um Zws. freizugeben */
			submode=sm;
			SetDrawGadget(0);
		}
		if (mode==MODE_EDIT && mode!=newmode) StopTime(Edit_TH);
	}
	switch(newmode)
	{
		case MODE_CLEAR:
			if (Options&OPT_ACR) ReduceColors(0);
		case MODE_DOCLEAR:
			if (mode!=MODE_CLEAR)
			{
				EndDisplayHelp();
				NormalColor();
				if (BorderCleared)
				{
					SetAPen(OurW->RPort,2);
					RectFill(OurW->RPort,BorderDrawx,BorderDrawy,BorderDrawX,BorderDrawY);
					BorderCleared=0;
				}
				SetAPen(OurW->RPort,(Options&OPT_RG3d)?RGColSupp->BGCol:3);
				RectFill(OurW->RPort,Drawx,Drawy,DrawX,DrawY);
				PicSecWorth=-1;
			}
			break;
		case MODE_DRAW:
			if (Options&OPT_ACR) ReduceColors(1);
			if (Options&OPT_AWH) WindowToFront(OurW);
			if (mode!=MODE_CLEAR) if (!Mode(MODE_DOCLEAR)) return(0);
			if (mode!=MODE_DRAW) submode=0;
			SetDrawGadget(1);
			break;
		case MODE_WAIT:
			if (Options&OPT_ACR) ReduceColors(0);
			break;
		case MODE_EDIT:
			break;
		case MODE_STARTEDIT:
			if (!AskForClear()) return(0);
			if (Options&OPT_ACR) ReduceColors(1);
			if (Options&OPT_AWH) WindowToFront(OurW);
			_EditFact=1.0;			
			break;
		case MODE_DISPL:
			if (!AskForClear()) return(0);
			if (Options&OPT_ACR) ReduceColors(1);
			PicSecWorth=-1;
			DrawCCross(0);
			if (Options&OPT_ACR) ReduceColors(0);
			break;
		case MODE_HELP:
			if (mode!=MODE_CLEAR) if (!Mode(MODE_TEST)) return(0);
			EndDisplayHelp();
			NormalColor();
			SetAPen(OurW->RPort,3);
			RectFill(OurW->RPort,BorderDrawx,BorderDrawy,BorderDrawX,BorderDrawY);			
			BorderCleared=1;
			submode=0;	/* muß dann auf Hilfenummer gesetzt werden */
			break;
		case MODE_LOAD:
		case MODE_SAVE:
			break;
		case MODE_OLD:
			newmode=oldmode;
			submode=oldsubmode;
			break;
		case MODE_TEST:						/* test, ob Bild löschbar */
			if (!AskForClear()) return(0);
			else return(-1);
			break;
	}			

	if (mode!=newmode)
	{
		mode=newmode;
		if (mode!=MODE_DOCLEAR) DisplayMode();
		else mode=MODE_CLEAR;
	}
	return(-1);
}

/* ------------------------------------------------------------------------ */

void DoMode(void)				/* führt den Modus mode aus */
{
	switch(mode)
	{
		case MODE_HELP:
			DisplayHelp(submode);	/* Hilfe auf den Schirm bringen */
			Mode(MODE_WAIT);
		case MODE_WAIT:
		case MODE_DISPL:
		case MODE_CLEAR: WaitPort(MainTask_MsgPort); return;		
		case MODE_START:
		{
			struct PlotPrefs *pp;
			
			if (!(pp=Load_Prefs())) goto_exit(20);
			
			Options&=~OPT_INTERLACE;	/**** obsolete ****/
				
			if (Use_Prefs(pp)<0) { CloseAll(0); goto_exit(20); }
			Free_Prefs(pp); 
			Mode(MODE_WAIT);
			_TestRange(-1);
			return;
		}
		case MODE_ENDREQ: CloseAll(0); goto_exit(0);

		case MODE_EDIT:
			if (CheckTime(Edit_TH))
			{
				switch (submode)
				{
					case 4: SizeFact/=1+(0.03*_EditFact); break;
					case 7: SizeFact*=1+(0.03*_EditFact); break;
					case 5: yAngle+=_EditFact*(pi/180.0); break;
					case 6: zAngle-=_EditFact*(pi/180.0); break;
					case 8: yAngle-=_EditFact*(pi/180.0); break;
					case 9: zAngle+=_EditFact*(pi/180.0); break;
					case 23: Midy-=(DrawY-Drawy+1.0)/(10.0+140.0/_EditFact); break;
					case 24: Midx-=(DrawX-Drawx+1.0)/(10.0+140.0/_EditFact); break;
					case 26: Midx+=(DrawX-Drawx+1.0)/(10.0+140.0/_EditFact); break;
					case 27: Midy+=(DrawY-Drawy+1.0)/(10.0+140.0/_EditFact); break;
				}
				while (yAngle<0.0) yAngle+=2.0*pi;
				while (yAngle>=2.0*pi) yAngle-=2.0*pi;
				while (zAngle<0.0) zAngle+=2.0*pi;
				while (zAngle>=2.0*pi) zAngle-=2.0*pi;
				ComputeBaseVects();
				DrawCCross(0);
				StartTimeExactly(Edit_TH,0,200000);
				_EditFact*=_EDITFACT_REL;
			}
			Forbid();
			if (!CheckTime(Edit_TH) && MainTask_MsgPort->mp_MsgList.lh_Head==(struct Node *)&MainTask_MsgPort->mp_MsgList.lh_Tail) Wait((1<<MainTask_MsgPort->mp_SigBit)|(1<<Edit_TH->ReplyPort.mp_SigBit));
			Permit();
			return;
		case MODE_STARTEDIT: Mode(MODE_EDIT); StartTimeExactly(Edit_TH,0,800000); return;
		case MODE_DRAW:
			DrawFunction();		/* Funktion zeichnen */
			if (_FPERR==_FPEbrk)
			{
				DisplayBeep(OurS); 
				_FPERR=0; 
				if (Mode(MODE_WAIT)) Message(l[135]);
			}
			return;
		default: return;
	}
}

/* zentrale Hilfsroutinen --------------------------------------------------------------- */

void PrepNewFunction(void)	/* bereitet Variablen und Rawspeicher */
{													/* für eine neue Funktion vor */
	DelAllVars(&Vars);
	
	SetVariable(GetVariable(&Vars,"pi",VAR_SYS),(double)(asin(1.0)*2.0),(double)0.0);
	SetVariable(GetVariable(&Vars,"e",VAR_SYS),(double)exp(1.0),(double)0.0);
	SetVariable(GetVariable(&Vars,"i",VAR_SYS),(double)0.0,(double)1.0);
	
	X=&GetVariable(&Vars,"x",VAR_SYS)->value.re;
	Y=&GetVariable(&Vars,"y",VAR_SYS)->value.re;
	
	ClearFunctionCode(&FunctionCode);
}

/* ------------------------------------------------------------------------ */

int _TestRange(short DispForbid)/* kontrolliert Range und geht evtl. auf MODE_DISPL */
{						/* und setzt zDist (RETURN: 0 - ok / -1 - Fehler) */
	short OldStatus;	/* 1 bei unmöglich in MODE_DISPL zu kommen */
	
	OldStatus=Status;	/* DispForbid!=0 verbietet, in den Displaymodus zu */
	Status&=~ST_RANGE;	/* gehen */
	if (xMin>=xMax || yMin>=yMax || zMin>=zMax)
	{
		Error(l[62]);
		Status|=ST_RANGE;
	}
	else
	{
		FuncMid.x=(xMin+xMax)/2.0;	/* Mitten berechnen */
		FuncMid.y=(yMin+yMax)/2.0;
		FuncMid.z=(zMin+zMax)/2.0;
	}
	if (!DispForbid) { if (!Mode(MODE_DISPL)) { Status=OldStatus; return(1); }}
	return((Status&ST_RANGE)?-1:0);
}

/* ------------------------------------------------------------------------ */

void ReduceColors(red)      /* reduziert die Farben bei red=1, */
short red;                  /* bei red=0 Tiefe max., bei red=-1 */
{                           /* Toggle. */
    static short RedOn=-1;  /* wurde schon einmal aufgerufen */
	static struct BitMap bm;
	short NewRed;

	if (!OurS) { RedOn=-1; return; }
		
	if (RedOn==-1)/* erster Aufruf-> originale BitMap-Strukt kopieren */
	{
		memcpy(&bm,&OurS->BitMap,sizeof(struct BitMap));
		OurS->ViewPort.RasInfo->BitMap=&bm;
		RedOn=0;
	}
	if (red==-1) NewRed=1-RedOn;
	else NewRed=red ? 1:0;
	
	if (NewRed!=RedOn)	/* Veränderung überhaupt nötig ? */
	{
		RedOn=NewRed;
		bm.Depth=RedOn ? 2:OurS->BitMap.Depth;

		MakeScreen(OurS);
		RethinkDisplay();
	}
}	

/* ------------------------------------------------------------------------ */

int AskWriteIfExists(void)		/* Prüft ob "Filename" schon auf der Disk */
{								/* vorhanden ist, und bildet dann einen */
								/* Requester. RET: 1 - Write / 0 - Cancel */
	static struct EasyStruct ReqStr =
	{
		sizeof(struct EasyStruct),
		0,"3dPlot Request",0,0
	};
	BPTR File;
	
	ReqStr.es_TextFormat=l[8];
	ReqStr.es_GadgetFormat=l[9];
	
	if (File=Open(Filename,MODE_OLDFILE))
	{
		int answer;
		
		Close(File);
		
		LockIDCMP();
		answer=EasyRequest(OurW,&ReqStr,0,Filename);
		UnlockIDCMP();
		return(answer);
	}
	else return(1);
}

/* ------------------------------------------------------------------------ */
	
int AskForExit(void)			/* Fragt den Benutzer, ob er das Prog. wirklich */
{							/* verlassen will RET: 1- Ja / 0- Nein */
	static struct EasyStruct ReqStr =
	{
		sizeof(struct EasyStruct),
		0,"3dPlot Request",0,0
	};
	int answer;
	
	ReqStr.es_TextFormat=l[10];
	ReqStr.es_GadgetFormat=l[11];
	
	LockIDCMP();
	answer=EasyRequest(OurW,&ReqStr,0);
	UnlockIDCMP();
	return(answer);
}

/* ------------------------------------------------------------------------ */
	
int AskForClear(void)			/* Fragt den Benutzer, ob das Bild wirklich */
{							/* gelöscht werden kann RET: 1- Ja / 0- Nein */
	static char buf[256];	/* jedoch nur falls es mehr als TimeMinThreshold */
	struct DateStamp Date;	/* Minuten wert ist. */
	LONG Seconds;
	int result=1;
	static struct EasyStruct ReqStr =
	{
		sizeof(struct EasyStruct),
		0,"3dPlot Request",buf,0
	};
	
	if (PicSecWorth>0)
	{
		if (mode==MODE_DRAW)
		{
			DateStamp(&Date);
			Seconds=(Date.ds_Days-PicLastSave.ds_Days)*(24*60*60);
			Seconds+=(Date.ds_Minute-PicLastSave.ds_Minute)*60;
			Seconds+=(Date.ds_Tick-PicLastSave.ds_Tick)/50;
		}
		else Seconds=PicSecWorth;
	
		if (Seconds)
		{
			LONG Minutes;
			
			Minutes=Seconds/60;
			
			if (Minutes>=TimeMinThreshold)
			{
				if (Minutes)
				{
					sprintf(buf,l[90],Minutes,(Minutes==1)?0:l[91]);
					ReqStr.es_TextFormat=buf;
				}
				else ReqStr.es_TextFormat=l[98];

				ReqStr.es_GadgetFormat=l[92];
				
				LockIDCMP();
				result=EasyRequest(OurW,&ReqStr,0);
				UnlockIDCMP();
				if (result!=0) PicSecWorth=0;
			}
		}
	}
	return(result);
}

/* ------------------------------------------------------------------------ */

void SetOurPri(void)			/* setzt die Taskpriorität je nach gew. */
{							/* Option */
	static BYTE normPri,normDiskPri;
	static struct Task *OurTask;
	
	if (!OurTask)		/* erster Aufruf ? */
	{
		OurTask=FindTask(0);
		normPri=SetTaskPri(OurTask,0);
		normDiskPri=SetTaskPri((struct Task *)DiskTaskPtr,0);
	}
	
	SetTaskPri(OurTask,(Options&OPT_LowPri)?-50:normPri);
	SetTaskPri((struct Task *)DiskTaskPtr,(Options&OPT_LowPri)?-50:normDiskPri);
}

/* ------------------------------------------------------------------------ */

void GetAPSFilename(void)		/* versucht einen APS-Filenamen zu bilden */
{							/* und schaltet das Fenster ggf. in den */
	static char Buf[600];	/* Vordergrund */
	FILE *fp;
	
	if (Options&OPT_APS)
	{		
		Forbid();
		if (!_Filename || !_Filename[0])
		{
			Permit();
			Error(l[65]); Error(l[100]); 
			Options&=~OPT_APS;
		}
		else
		{
			Permit();
			strncpy(APS_Filename,_Filename,554);
			strmfe(APS_Filename,APS_Filename,"brk");
			if (!(fp=fopen(APS_Filename,"ab+")))
			{
				Error(l[101]);
				Options&=~OPT_APS;
			}			
			else fclose(fp);
		}
	}

	if (!(Options&OPT_APS))	/* Fenster entspr. löschen/setzen */
	{
		WindowToBack(ApsW);
		APS_Filename[0]=0;
	}
	else
	{
		strcpy(Buf,l[101]);
		if (strlen(Buf)+strlen(APS_Filename)>ApsW->Width/W(8)-2)
		{
			strcat(Buf,"...");
			strcat(Buf,&APS_Filename[strlen(APS_Filename)-ApsW->Width/8+strlen(Buf)+2]);
		}
		else strcat(Buf,APS_Filename);
		
		SetWindowTitles(ApsW,Buf,OurS->Title);
		if (mode==MODE_DRAW) WindowToFront(ApsW);
	}
	GT_SetGadgetAttrs(_OPT_APS_G,Opt1W,0,GTCB_Checked,(Options&OPT_APS)?TRUE:FALSE,TAG_END);
}

/* ------------------------------------------------------------------------ */

void RefreshLightGadgets(void)		/* frischt das Erscheinungsbild der */
{									/* Lichtquellengadgets auf */
	BOOL dis;

	GT_SetGadgetAttrs(_OxLightG,Opt2W,0,GTST_String,ftoa(LightVect[SLS].x),TAG_END);
	GT_SetGadgetAttrs(_OyLightG,Opt2W,0,GTST_String,ftoa(LightVect[SLS].y),TAG_END);
	GT_SetGadgetAttrs(_OzLightG,Opt2W,0,GTST_String,ftoa(LightVect[SLS].z),TAG_END);
	GT_SetGadgetAttrs(_OLIntensG,Opt2W,0,GTSL_Level,(short)(LightIntens[SLS]*20.0),TAG_END);
	
	GT_SetGadgetAttrs(_OGLightStateG,Opt2W,0,GTCY_Active,(LightsOn&(1<<SLS))?1:0,TAG_END);
				
	dis=((DrawMode&(DMF_COL|DMF_LCOL))&&(LightsOn&(1<<SLS)))?FALSE:TRUE;
	
	GadDisable(_OLIntensG,Opt2W,dis);	
	GadDisable(_OxLightG,Opt2W,dis);	
	GadDisable(_OyLightG,Opt2W,dis);	
	GadDisable(_OzLightG,Opt2W,dis);	
}

/* ------------------------------------------------------------------------ */

extern unsigned long _IterationBrk;

short CompileFunction(code)	/* wandelt den Funktionsstring in fnctraw um */
fnctcode **code;						/* RETURN: entspricht strtofnctraw() */
{													/* Das Varfeld und das fnctcode Feld  werden */
													/* vorher intitalisiert */
	ClearFunctionCode(code);
	PrepNewFunction();

	if (!(*code=allocfnctcode(&Vars,&_IterationBrk))) Error(l[68]);
	return(makefnctcode(FunctionString,*code));
}

/* ------------------------------------------------------------------------ */

void ClearFunctionCode(code)			/* initialisiert das Array, löscht */
fnctcode **code;									/* und befreit ggf. vorhandene Einträge */
{																/* Uservariablen werden nicht befreit */
	freefnctcode(*code); *code=0;
}

/* ------------------------------------------------------------------------ */

void SetDrawGadget(short mode)	/* Ändert das Aussehen des Draw-Gadgets */
{                             	/* nach mode: 0: Draw / 1: Stop */
	                          	/* -1: Gadget löschen */
	static struct Gadget *_GDraw=0;
	static int _GDraw_cnt=0;
	
	struct Gadget *gad;
	
	if (_GDraw && _GDraw_cnt)
	{
		RemoveGList(OurW,_GDraw,_GDraw_cnt);
		FreeGadgetList(_GDraw,_GDraw_cnt);
		_GDraw=0;
		_GDraw_cnt=0;
	}
	if (mode>=0 && ConsoleState==CS_MAIN)
	{
		CreateContext(&gad);
		GAct.ng_VisualInfo=VisualInfo;
		GAct.ng_GadgetID=20;
		GAct.ng_LeftEdge=(maxx-W(214+340-12)-GAct.ng_Width)/2+W(340-12);
		GAct.ng_TopEdge=maxy+H(23);
		GAct.ng_GadgetText=mode?l[148]:l[35];
		_GDraw=gad=CreateGadget(BUTTON_KIND,gad,&GAct,TAG_END);
		_GDraw_cnt=GadgetCount(_GDraw);
		if (_GDraw)
		{
			AddGList(OurW,_GDraw,-1,-1,0);
			GT_RefreshWindow(OurW,0);
			RefreshGadgets(_GDraw,OurW,0);
		}	
	}
}
