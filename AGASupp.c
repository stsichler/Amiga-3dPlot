/* ------------------------------------------------------------------------- *
 * 3dPlot - AGASupp.c (AGA-Support) - 30-8-94 - ÆSSi
 * ------------------------------------------------------------------------- */

#include "Plot.h"
#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <clib/alib_stdio_protos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <stdlib.h>
#include <strings.h>
#include <exec/lists.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <math.h>

/* globale Daten ----------------------------------------------------------- */

UWORD OSVERSION;	/* von Main.c initialisiert */

struct RGColSupp *RGColSupp;	/* bei OPT_RG3d ^entspr. RGColSupp-Struktur */

extern struct Screen *OurS;
extern struct DimensionInfo OurS_DIM;

extern struct PlotPrefs *activePrefs;

extern short Options;

struct List *DIList;	/* Liste der Bildschirmmodi (f¸r Prefs) */

/* private Prototypen ----------------------------------------------------- */

void EnqueueByName(struct List *,struct Node *);
ULONG ToLong(double);

/* ------------------------------------------------------------------------ */
/* RGColSupp - Strukturen ------------------------------------------------- */
/* ------------------------------------------------------------------------ */

UBYTE RGColors_3[] = { 7 };
UBYTE RGColors_4[] = { 12,15 };
UBYTE RGColors_5[] = { 26,21,31 };
UBYTE RGColors_6[] = { 48,12,60,51,15,63 };
UBYTE RGColors_7[] = { 100,82,118,73,109,91,127 };
UBYTE RGColors_8[] = { 192,48,240,204,60,252,195,51,243,207,63,255 };

struct RGColSupp RGColSupp_3 = { 5,6,4,1,RGColors_3 };
struct RGColSupp RGColSupp_4 = { 5,10,3,2,RGColors_4 };
struct RGColSupp RGColSupp_5 = { 19,28,16,3,RGColors_5 };
struct RGColSupp RGColSupp_6 = { 21,42,3,6,RGColors_6 };
struct RGColSupp RGColSupp_7 = { 71,120,64,7,RGColors_7 };
struct RGColSupp RGColSupp_8 = { 85,170,3,12,RGColors_8 };

/* Systemprototypen (V39+) ------------------------------------------------- */

extern void SetRGB32CM(struct ColorMap *cm,unsigned long n,unsigned long r,unsigned long g,unsigned long b);
extern void LoadRGB32(struct ViewPort *vp,ULONG *table);
extern void SetRGB32(struct ViewPort *vp,unsigned long n,unsigned long r,unsigned long g,unsigned long b);
extern void GetRGB32(struct ColorMap *cm,unsigned long FirstCol,unsigned long nColors, unsigned long *table);
#pragma libcall GfxBase SetRGB32CM 3e4 3210805
#pragma libcall GfxBase LoadRGB32 372 9802
#pragma libcall GfxBase SetRGB32 354 3210805
#pragma libcall GfxBase GetRGB32 384 910804

/* #define SA_Colors32 (SA_Dummy+0x0023) in Open.c */

/* private Prototypen ------------------------------------------------------ */

ULONG RHBits(ULONG val, UBYTE bitcnt);

/* ------------------------------------------------------------------------- */

void SetRGBCM(struct ColorMap *cm,unsigned long n,unsigned long r,unsigned long g,unsigned long b)
{
	/* ACHTUNG: OS-Routine hat einen Bug beim Blauwert */
	if (OSVERSION>=39) SetRGB32CM(cm,n,r,g,b);
	else SetRGB4CM(cm,n,RHBits(r,4),RHBits(g,4),RHBits(b,4));
	return;
}

void GetRGB(struct ColorMap *cm,unsigned long n,ULONG *table)
{
	/* table zeigt auf 3 LW: R,G,B */
	
	if (OSVERSION>=39) GetRGB32(cm,n,1,table);
	else
	{
		short buf,i;
		
		buf=GetRGB4(cm,n);
		
		table[0]=(buf&0x0f00)>>8;
		table[1]=(buf&0x00f0)>>4;
		table[2]=(buf&0x000f);
		
		for (i=0;i<3;i++)
		{
			table[i]|=table[i]<<4;
			table[i]|=table[i]<<8;
			table[i]|=table[i]<<16;
		}
	}
	return;
}

void SetRGB(struct ViewPort *vp,unsigned long n,unsigned long r,unsigned long g,unsigned long b)
{
	if (OSVERSION>=39) SetRGB32(vp,n,r,g,b);
	else SetRGB4(vp,n,RHBits(r,4),RHBits(g,4),RHBits(b,4));
	return;
}

/* ------------------------------------------------------------------------ */

extern unsigned long _IterationBrk;

int SetColors(struct PP_ColorFile *cf)	/* setzt alle Farben */
{
	ULONG a;			/* RETURN: 16 Bit MSB Feldnummer des Fehlers, */
	fnctcode *redCode,*greenCode,*blueCode; /* 16 Bit LSB Fehlernummer */
	struct VarList ColVars;                 /* von makefnctcode() */
	struct Var *lv,*rv;                /* oder -1 bei glob. Fehler */
	short err=0,errfield=0;
		
	if (!cf) return(-1);
	
	memset(&ColVars,0,sizeof(ColVars));
		
	switch(OurS->BitMap.Depth)	/* Hilfsinfo f¸r RG-3d festlegen */
	{
		case 3: RGColSupp=&RGColSupp_3; break;
		case 4: RGColSupp=&RGColSupp_4; break;
		case 5: RGColSupp=&RGColSupp_5; break;
		case 6: RGColSupp=&RGColSupp_6; break;
		case 7: RGColSupp=&RGColSupp_7; break;
		case 8: RGColSupp=&RGColSupp_8; break;
		default:RGColSupp=0; Options&=~OPT_RG3d; cf->Flags&=~PPCF_RG3d;
	}
	redCode=allocfnctcode(&ColVars,&_IterationBrk);
	greenCode=allocfnctcode(&ColVars,&_IterationBrk);
	blueCode=allocfnctcode(&ColVars,&_IterationBrk);
	
	SetVariable(GetVariable(&ColVars,"pi",VAR_SYS),asin(1.0)*2.0,(double)0.0);
	SetVariable(GetVariable(&ColVars,"e",VAR_SYS),exp(1.0),(double)0.0);	
	SetVariable(GetVariable(&ColVars,"i",VAR_SYS),(double)0.0,(double)1.0);
	
	if (cf->Flags&PPCF_RG3d)
	{
		lv=GetVariable(&ColVars,"l",VAR_SYS);
		rv=GetVariable(&ColVars,"r",VAR_SYS);
	}
	else lv=rv=GetVariable(&ColVars,"m",VAR_SYS);
	
	if (lv && rv && redCode && greenCode && blueCode)
	{
		if (err=makefnctcode(cf->Red,redCode)) errfield=1;
		else if (err=makefnctcode(cf->Green,greenCode)) errfield=2;
		else if (err=makefnctcode(cf->Blue,blueCode)) errfield=3;
		
		if (!err)
		{
			/* Farben berechnen */
			
			if (cf->Flags&PPCF_RG3d)
			{
				short maxcolnum,nl,nr;	/* n links / n rechts */

				maxcolnum=(1<<OurS->BitMap.Depth)-4;				
				for (nl=0;nl<maxcolnum;nl++) SetRGB(&OurS->ViewPort,nl+4,0,0,0);
				
				maxcolnum=RGColSupp->ColNum+1;
				
				for (nl=0; nl<maxcolnum; nl++)
				{
					for (nr=0; nr<maxcolnum; nr++)
					{
						complex buffer;
						double rval,gval,bval;
						short colnum;

						SetVariable(lv,(double)nl/(double)(maxcolnum-1),(double)0.0);
						SetVariable(rv,(double)nr/(double)(maxcolnum-1),(double)0.0);
						evalfnctcode(&buffer,redCode); rval=buffer.re;
						if (_FPERR) rval=0.0;
						evalfnctcode(&buffer,greenCode); gval=buffer.re;
						if (_FPERR) gval=0.0;
						evalfnctcode(&buffer,blueCode); bval=buffer.re;
						if (_FPERR) bval=0.0;
						
						colnum=(nl?RGColSupp->Colors[nl-1]:RGColSupp->BGCol)&RGColSupp->RedMask;
						colnum|=(nr?RGColSupp->Colors[nr-1]:RGColSupp->BGCol)&RGColSupp->GreenMask;
						
						SetRGB(&OurS->ViewPort,colnum,ToLong(rval),ToLong(gval),ToLong(bval));
					}
				}
			}
			else
			{
				short maxcolnum,n;
				
				maxcolnum=(1<<OurS->BitMap.Depth)-4;
				
				for (n=0;n<maxcolnum;n++)
				{
					complex buffer;
					double rval,gval,bval;
					
					SetVariable(lv,(double)n/(double)(maxcolnum-1),(double)0.0);
					evalfnctcode(&buffer,redCode); rval=buffer.re;
					if (_FPERR) rval=0.0;
					evalfnctcode(&buffer,greenCode); gval=buffer.re;
					if (_FPERR) gval=0.0;
					evalfnctcode(&buffer,blueCode); bval=buffer.re;
					if (_FPERR) bval=0.0;
					
					SetRGB(&OurS->ViewPort,n+4,ToLong(rval),ToLong(gval),ToLong(bval));
				}
			}
			cf->Flags&=~PPCF_ERR;
		}
		else
		{
			Error(l[60]);
			cf->Flags|=PPCF_ERR;
		}
	} else { Error(l[68]); err=-1; }
	
	for (a=0; a<4; a++)
	{
		if ((cf->Flags&PPCF_RG3d) && RGColSupp->BGCol==a) continue;
		SetRGB(&OurS->ViewPort,cf->StdCol[a].num,cf->StdCol[a].red,
			cf->StdCol[a].green,cf->StdCol[a].blue);
	}
	
	freefnctcode(redCode);
	freefnctcode(greenCode);
	freefnctcode(blueCode);
	DelAllVars(&ColVars);

	MakeScreen(OurS);
	RethinkDisplay();

	return(err|(errfield<<16));
}

/* ------------------------------------------------------------------------ */

ULONG RHBits(ULONG val, UBYTE bitcnt)	/* schneidet bitcnt Bits */
{												/* oben ab, verschiebt sie auf Nullpos. und */
	ULONG vv,mask;				/* rundet sie entsprechend der unteren Bits */
	
	if (bitcnt>=32) return(val);
	
	mask=(1<<bitcnt)-1;
	vv=(val>>((UBYTE)32-bitcnt))&mask;
	
	if (vv!=mask && val&(1<<((UBYTE)31-bitcnt))) vv+=1;
	
	return(vv);
}

/* ------------------------------------------------------------------------ */

#define ForbiddenProperties (DIPF_IS_DUALPF|DIPF_IS_PF2PRI|DIPF_IS_HAM|DIPF_IS_EXTRAHALFBRITE)

UWORD MakeDIList(ULONG ActDisplayID) /* baut die Liste w‰hlbarer Bildschirmmodi auf */
{								 /* -> DIList / RETURN: akt. Screenmode (Listenpos.) */
	UWORD act;			/* ~0 heiﬂt nicht gefunden */
	ULONG CurrentID;
	struct DisplayInfo di;
	struct NameInfo ni;
	struct DimensionInfo dimi;
	char buffer[50];

	FreeDIList();
	
	act=(UWORD)0xffff;
	
	if (DIList=malloc(sizeof(*DIList)))
	{
		int i=0;
		CurrentID=INVALID_ID;
		NewList(DIList);

		while (((CurrentID=NextDisplayInfo(CurrentID)) != INVALID_ID) && i<200)
		{
			if (GetDisplayInfoData(0,(UBYTE *)&di,sizeof(di),DTAG_DISP,CurrentID))
			{
				if (!(di.PropertyFlags&ForbiddenProperties) && GetDisplayInfoData(0,(UBYTE *)&ni,sizeof(ni),DTAG_NAME,CurrentID) && GetDisplayInfoData(0,(UBYTE *)&dimi,sizeof(dimi),DTAG_DIMS,CurrentID))
				{
					if (!ModeNotAvailable(CurrentID))
					{
						struct DINode *din;
						
						if (din=malloc(sizeof(*din)))
						{
							sprintf(buffer,"%.32s [%dx%d]",ni.Name,dimi.TxtOScan.MaxX-dimi.TxtOScan.MinX+1,dimi.TxtOScan.MaxY-dimi.TxtOScan.MinY+1);
							din->DisplayID=CurrentID;
							din->node.ln_Name=strdup(buffer);
													
							EnqueueByName(DIList,(struct Node *)din);
							i++;
						}
					}
				}
			}
		}
	}
	/* Herausfinden, welcher Modus gerade aktiv ist */
	{
		struct DINode *n=(struct DINode *)DIList->lh_Head;
		short num=0;
		
		while (n->node.ln_Succ->ln_Succ && n->DisplayID!=ActDisplayID) // strcmp(n->node.ln_Name,ni.Name))
		{
			n=(struct DINode *)n->node.ln_Succ;
			num++;
		}
		if (n->node.ln_Succ) act=num;
	}	

	return(act);
}

/* ------------------------------------------------------------------------ */

void FreeDIList(void)
{
	struct DINode *n,*next;
	
	n=(struct DINode *)DIList->lh_Head;
	
	while (next=(struct DINode *)n->node.ln_Succ)
	{
		Remove((struct Node *)n);
		free(n->node.ln_Name);
		free(n);
		n=next;
	}
	free(DIList);
	DIList=0;
}


void EnqueueByName(struct List *list,struct Node *node)
{
	struct Node *nd;	/* Node nach Namen einsortieren */
	int compare=1;
	
	for (nd=(struct Node *)list->lh_Head;nd->ln_Succ;nd=nd->ln_Succ) 
		if ((compare=stricmp(nd->ln_Name,node->ln_Name))>=0) break;
	if (compare) Insert(list,node,nd->ln_Pred);
}

/* ------------------------------------------------------------------------ */

ULONG ToLong(double val)
{
	ULONG res;
	
	if (val<0.0) val=0.0;
	if (val>1.0) val=1.0;
	res=(ULONG)((double)0xffff*val);
	if (res>0xffff) res=0xffff;
	res|=res<<16;
/*
	res=(ULONG)((double)0xff*val);
	if (res>0xff) res=0xff;
	res|=res<<8;
	res|=res<<16;
*/	
	return(res);
}

