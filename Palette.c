/* ------------------------------------------------------------------------- *
 * 3dPlot - Palette.c (PlotPreferences) - 26-6-95 - ®SSi
 * ------------------------------------------------------------------------- */

#include "Plot.h"
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>

/* globale Daten ----------------------------------------------------------- */

extern short mode,submode,ConsoleState;

extern struct PlotPrefs *editPrefs;

extern short Options;

struct Window *PalW=0;
struct Gadget *FirstPalGad;
short PalWx,PalWy;

extern struct Screen *OurS;
extern struct Window *OurW;
extern void *VisualInfo;
extern struct MsgPort *MainTask_MsgPort;
extern short COLm,COLcnt;	/* min. Farbe und Farbzahl für die Funktion */

extern short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY; /* (fest) */

extern struct TextFont *textfont;

extern struct RGColSupp *RGColSupp;

/* private Daten ----------------------------------------------------------- */

short PalGadgets_attached=0;

short NewGfxChips=0;

short EditedColor=0;
struct PP_ColorFile *EditedCf;	/* zeigt entweder auf EditCf1 oder 2 */
struct PP_ColorFile EditCf1,EditCf2;	/* Kopie zum Editieren */

/* modulinterne Prototypen  ------------------------------------------------ */

/* speziell importierte Prototypen ----------------------------------------- */

void __interrupt CloseWindowSafely(struct Window *);

/* Gadgetprototypen -------------------------------------------------------- */

struct NewGadget PaGChoose,PaGSlider,PaGPalette,PaGInput,PaGAct;

/* Gadgetzeiger ------------------------------------------------------------ */

struct Gadget *_PaGRSlider,*_PaGGSlider,*_PaGBSlider,*_PaGRText,*_PaGGText,
	*_PaGBText;
	
/* ------------------------------------------------------------------------- */

void OpenPaletteWindow(void)	/* öffnet das Palette-Window im entspr. Modus */
{
	struct Gadget *gad;

	if (Options&OPT_ACR) ReduceColors(1);

	if (GfxBase->ChipRevBits0 & GFXF_HR_DENISE) NewGfxChips=1;

	if (!PalW)
	{
		memcpy(&EditCf1,&editPrefs->StdCols,sizeof(struct PP_ColorFile));
		memcpy(&EditCf2,&editPrefs->RG3dCols,sizeof(struct PP_ColorFile));	

		/* Window öffnen */
	
		if (PalW=OpenWindowTags(0,WA_Left,PalWx==-1?(PalWx=((short)BorderDrawx+(short)BorderDrawX-W(400))/2):PalWx,
			WA_Top,PalWy==-1?(PalWy=((short)BorderDrawy+(short)BorderDrawY-H(139))/2):PalWy,
			WA_Width,W(400),WA_Height,H(139),
			WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR,
			WA_CustomScreen,(ULONG)OurS,
			WA_Title,l[145],
			TAG_DONE))
		{
			PalGadgets_attached=0;
															/* IDCMP-Port installieren */
			PalW->UserPort=MainTask_MsgPort;
			ModifyIDCMP(PalW, SLIDERIDCMP|IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_CHANGEWINDOW);
			GT_RefreshWindow(PalW,0);
			
			/* Farbreihe unten zeichnen */
			
			{
				short col,maxwidth;
				short xa,x=0;
				
				maxwidth=W(386)-2;
			
				for (col=0;col<COLcnt;col++)
				{
					xa=x;
					x=(short)((double)(col+1)*(double)maxwidth/(double)COLcnt);
					SetAPen(PalW->RPort,col+COLm);
					RectFill(PalW->RPort,W(7)+xa,H(110),W(7)+x,H(119));
				}
				DrawBevelBox(PalW->RPort,W(7),H(110),maxwidth+2,H(10),GT_VisualInfo,VisualInfo,GTBB_Recessed,TRUE,TAG_END);
			}
		}
		else Error(l[68]);
	}
	
	if (PalW && !FirstPalGad)			/* Gadgets initialisieren */
	{
		EditedColor=0;
		EditedCf=(Options&OPT_RG3d)?&EditCf2:&EditCf1;

		SetColors(EditedCf);
				
		gad=CreateContext(&FirstPalGad);

		PaGChoose.ng_VisualInfo=VisualInfo;
		PaGChoose.ng_GadgetText=(EditedCf->Flags&PPCF_RG3d)?l[147]:l[146];
		gad=CreateGadget(BUTTON_KIND,gad,&PaGChoose,TAG_END);

		PaGSlider.ng_VisualInfo=VisualInfo;
		PaGSlider.ng_TopEdge=H(33);
		PaGSlider.ng_GadgetID=82;		
		PaGSlider.ng_GadgetText="R:    ";
		_PaGRSlider=gad=CreateGadget(SLIDER_KIND,gad,&PaGSlider,GTSL_Min,0,GTSL_Max,NewGfxChips?255:15,GTSL_LevelFormat,"%3ld",GTSL_MaxLevelLen,3,GTSL_LevelPlace,PLACETEXT_LEFT,
			GTSL_Level,(EditedCf->StdCol[EditedColor].red)>>(NewGfxChips?24:28),GA_RelVerify,TRUE,TAG_END);
		PaGSlider.ng_TopEdge+=10;
		PaGSlider.ng_GadgetID++;
		PaGSlider.ng_GadgetText="G:    ";
		_PaGGSlider=gad=CreateGadget(SLIDER_KIND,gad,&PaGSlider,GTSL_Min,0,GTSL_Max,NewGfxChips?255:15,GTSL_LevelFormat,"%3ld",GTSL_MaxLevelLen,3,GTSL_LevelPlace,PLACETEXT_LEFT,
			GTSL_Level,(EditedCf->StdCol[EditedColor].green)>>(NewGfxChips?24:28),GA_RelVerify,TRUE,TAG_END);
		PaGSlider.ng_TopEdge+=10;
		PaGSlider.ng_GadgetID++;
		PaGSlider.ng_GadgetText="B:    ";
		_PaGBSlider=gad=CreateGadget(SLIDER_KIND,gad,&PaGSlider,GTSL_Min,0,GTSL_Max,NewGfxChips?255:15,GTSL_LevelFormat,"%3ld",GTSL_MaxLevelLen,3,GTSL_LevelPlace,PLACETEXT_LEFT,
			GTSL_Level,(EditedCf->StdCol[EditedColor].blue)>>(NewGfxChips?24:28),GA_RelVerify,TRUE,TAG_END);

		PaGPalette.ng_VisualInfo=VisualInfo;
		gad=CreateGadget(PALETTE_KIND,gad,&PaGPalette,GTPA_Depth,2,GTPA_Color,EditedColor,GTPA_IndicatorHeight,10,TAG_END);	
		
		PaGInput.ng_VisualInfo=VisualInfo;
		PaGInput.ng_GadgetID=86;
		PaGInput.ng_TopEdge=H(67);
		PaGInput.ng_Width=(EditedCf->Flags&PPCF_RG3d)?W(234):W(312);
		PaGInput.ng_GadgetText=(EditedCf->Flags&PPCF_RG3d)?"rf(l,r)=":"rf(m)=";
		_PaGRText=gad=CreateGadget(STRING_KIND,gad,&PaGInput,GTST_String,EditedCf->Red,GTST_MaxChars,79,STRINGA_ExitHelp,TRUE,GA_TabCycle,TRUE,TAG_END);		
		PaGInput.ng_GadgetID++;
		PaGInput.ng_TopEdge+=H(14);
		PaGInput.ng_GadgetText=(EditedCf->Flags&PPCF_RG3d)?"gf(l,r)=":"gf(m)=";
		_PaGGText=gad=CreateGadget(STRING_KIND,gad,&PaGInput,GTST_String,EditedCf->Green,GTST_MaxChars,79,STRINGA_ExitHelp,TRUE,GA_TabCycle,TRUE,TAG_END);
		PaGInput.ng_GadgetID++;
		PaGInput.ng_TopEdge+=H(14);
		PaGInput.ng_GadgetText=(EditedCf->Flags&PPCF_RG3d)?"bf(l,r)=":"bf(m)=";
		_PaGBText=gad=CreateGadget(STRING_KIND,gad,&PaGInput,GTST_String,EditedCf->Blue,GTST_MaxChars,79,STRINGA_ExitHelp,TRUE,GA_TabCycle,TRUE,TAG_END);

		PaGAct.ng_VisualInfo=VisualInfo;
		PaGAct.ng_LeftEdge=W(7);
		PaGAct.ng_GadgetID=90;
		PaGAct.ng_GadgetText=l[137];
		gad=CreateGadget(BUTTON_KIND,gad,&PaGAct,TAG_END);

		PaGAct.ng_LeftEdge=W(303);
		PaGAct.ng_GadgetID=91;
		PaGAct.ng_GadgetText=l[129];
		gad=CreateGadget(BUTTON_KIND,gad,&PaGAct,TAG_END);

		if (!gad) { Error(l[80]); ClosePaletteWindow(-1); return; }
		
		if (EditedCf->Flags&PPCF_RG3d)
		{														/* Farbenquadrat rechts zeichnen */
			short xdim,ydim,xd,yd;
			double a,xa,ya,maxcols;
			short xc,yc;	/* x/y  Color count */
			
			xa=(double)W(70)/activePrefs->XAspect;
			ya=(double)H(40)/activePrefs->YAspect;
			a=min(xa,ya);
			
			xdim=(short)(activePrefs->XAspect*a)-2;	/* dimensionieren */
			ydim=(short)(activePrefs->YAspect*a)-2;
			xd=(W(70)-xdim)/2+W(323);
			yd=(H(40)-ydim)/2+H(67);
			
			maxcols=(double)(RGColSupp->ColNum+1);
			
			for (xc=0;xc<=RGColSupp->ColNum;xc++)
			{
				struct Rectangle box;
				UBYTE color;
				
				box.MinX=xd+(short)((double)xc*(double)xdim/maxcols);
				box.MaxX=xd+(short)((double)(xc+1)*(double)xdim/maxcols);
				for (yc=0;yc<=RGColSupp->ColNum;yc++)
				{
					box.MinY=yd+(short)((double)yc*(double)ydim/maxcols);
					box.MaxY=yd+(short)((double)(yc+1)*(double)ydim/maxcols);
					
					color=(xc?RGColSupp->Colors[xc-1]:RGColSupp->BGCol)&RGColSupp->RedMask;
					color|=(yc?RGColSupp->Colors[yc-1]:RGColSupp->BGCol)&RGColSupp->GreenMask;
					SetAPen(PalW->RPort,color);
					RectFill(PalW->RPort,box.MinX,box.MinY,box.MaxX,box.MaxY);
				}
			}
			DrawBevelBox(PalW->RPort,xd,yd,xdim+2,ydim+2,GT_VisualInfo,VisualInfo,GTBB_Recessed,TRUE,TAG_END);		
		}
	}		
	
	if(!PalGadgets_attached && PalW)
	{
		AddGList(PalW,FirstPalGad,-1,-1,0);
		GT_RefreshWindow(PalW,0);
		RefreshGadgets(FirstPalGad,PalW,0);
		PalGadgets_attached=1;
	}

	if (Options&OPT_ACR) ReduceColors(0);
	
	if (PalW) WindowToFront(PalW);
}

/* ------------------------------------------------------------------------- */

void ClosePaletteWindow(short mode) /* siehe bei CloseResolutionWindow() */
{                                   /* in PPrefs.c; außer mode=3:        */
	static short requested=0;       /*    nur Gadgets löschen            */

	if (mode==1 || mode==-1) requested=-1;
	if (requested && mode==2 || mode==-1)
	{
		CloseWindowSafely(PalW);
		PalW=0;
		if (FirstPalGad) FreeGadgets(FirstPalGad);
		FirstPalGad=0;
		requested=0;
		PalGadgets_attached=0;
	}
	if (mode==3)
	{
		if (PalGadgets_attached) RemoveGList(PalW,FirstPalGad,-1);
		if (FirstPalGad)
		{
			FreeGadgets(FirstPalGad);
			FirstPalGad=0;
		}
		PalGadgets_attached=0;
		SetAPen(PalW->RPort,0);
		RectFill(PalW->RPort,W(6),H(65),W(394),H(108));
	}
}
