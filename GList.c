/* ---------------------------------------------------------------------- */
/* Plot - 16-10-94 SSi    -    GADGETLISTE                                */
/* ---------------------------------------------------------------------- */

#include "Plot.h"
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/gadtools.h>
#include <intuition/gadgetclass.h>

/* modulinterne Prototypen ---------------------------------------------- */

void AdjustGadgetsToFont(void);
void PrepareNewGadgetStructs(void);

/* Prototypen der Gadgethandler ---------------------------------------- */

void GDH4to9(struct Gadget *,USHORT);

void GUH1(struct Gadget *,USHORT);
void GUH2(struct Gadget *,USHORT);
void GUH3(struct Gadget *,USHORT);
void GUH4to9(struct Gadget *,USHORT);
void GUH10(struct Gadget *,USHORT);
void GUH11(struct Gadget *,USHORT);
void GUH12(struct Gadget *,USHORT);
void GUH13(struct Gadget *,USHORT);
void GUH14(struct Gadget *,USHORT);
void GUH15(struct Gadget *,USHORT);
void GUH16(struct Gadget *,USHORT);
void GUH17(struct Gadget *,USHORT);
void GUH18(struct Gadget *,USHORT);
void GUH19(struct Gadget *,USHORT);
void GUH20(struct Gadget *,USHORT);
void GUH21(struct Gadget *,USHORT);
void GUH22(struct Gadget *,USHORT);
void GUH25(struct Gadget *,USHORT);
void GUH28(struct Gadget *,USHORT);
void GUH29(struct Gadget *,USHORT);
void GUH30(struct Gadget *,USHORT);
void GUH31(struct Gadget *,USHORT);
void GUH33(struct Gadget *,USHORT);
void GUH34(struct Gadget *,USHORT);
void GUH35(struct Gadget *,USHORT);
void GUH36(struct Gadget *,USHORT);
void GUH37(struct Gadget *,USHORT);
void GUH38(struct Gadget *,USHORT);
void GUH39(struct Gadget *,USHORT);
/* void GUH40(struct Gadget *,USHORT); Vom Disktask behandelt */
/* void GUH41(struct Gadget *,USHORT); Vom Disktask behandelt */
/* void GUH42(struct Gadget *,USHORT); Vom Disktask behandelt */
void GUH43(struct Gadget *,USHORT);
void GUH44(struct Gadget *,USHORT);
void GUH45(struct Gadget *,USHORT);
void GUH46(struct Gadget *,USHORT);
void GUH47(struct Gadget *,USHORT);
void GUH48(struct Gadget *,USHORT);
void GUH49(struct Gadget *,USHORT);
void GUH50(struct Gadget *,USHORT);
void GUH51(struct Gadget *,USHORT);
void GUH52(struct Gadget *,USHORT);
void GUH53(struct Gadget *,USHORT);
void GUH54(struct Gadget *,USHORT);
void GUH55(struct Gadget *,USHORT);
void GUH56(struct Gadget *,USHORT);
void GUH57(struct Gadget *,USHORT);
void GUH58(struct Gadget *,USHORT);
void GUH59(struct Gadget *,USHORT);
void GUH60(struct Gadget *,USHORT);
void GUH61(struct Gadget *,USHORT);
void GUH62(struct Gadget *,USHORT);
void GUH63(struct Gadget *,USHORT);
void GUH64(struct Gadget *,USHORT);
void GUH65(struct Gadget *,USHORT);
void GUH66(struct Gadget *,USHORT);
void GUH67(struct Gadget *,USHORT);
void GUH68(struct Gadget *,USHORT);
void GUH69(struct Gadget *,USHORT);
void GUH70(struct Gadget *,USHORT);
void GUH72(struct Gadget *,USHORT);
void GUH73(struct Gadget *,USHORT);
void GUH74(struct Gadget *,USHORT);
void GUH75(struct Gadget *,USHORT);
void GUH76(struct Gadget *,USHORT);
void GUH77(struct Gadget *,USHORT);
void GUH78(struct Gadget *,USHORT);
void GUH79(struct Gadget *,USHORT);
void GUH80(struct Gadget *,USHORT);
/* void GUH81(struct Gadget *,USHORT); Vom Disktask behandelt */
void GUH82(struct Gadget *,USHORT);
void GUH83(struct Gadget *,USHORT);
void GUH84(struct Gadget *,USHORT);
void GUH85(struct Gadget *,USHORT);
void GUH86(struct Gadget *,USHORT);
void GUH87(struct Gadget *,USHORT);
void GUH88(struct Gadget *,USHORT);
void GUH89(struct Gadget *,USHORT);
void GUH90(struct Gadget *,USHORT);
void GUH91(struct Gadget *,USHORT);
void GUH92(struct Gadget *,USHORT);
void GUH93(struct Gadget *,USHORT);

/* Gadgethandlerlisten -------------------------------------------------- */

const short GADGETNUM = 93;

void (*GadgetDownHandle[])(struct Gadget *,USHORT) =
{
	0,0,0,0,&GDH4to9,	/* 0-4 */
	&GDH4to9,&GDH4to9,&GDH4to9,&GDH4to9,&GDH4to9,	/* 5-9 */
	0,0,0,0,0,	/* 10-14 */
	0,0,0,0,0,	/* 15-19 */
	0,0,0,&GDH4to9,&GDH4to9,	/* 20-24 */
	0,&GDH4to9,&GDH4to9,0,0,	/* 25-29 */
	0,0,0,0,0,	/* 30-34 */
	0,0,0,0,0,	/* 35-39 */
	0,0,0,0,0,	/* 40-44 */
	0,0,0,0,0,	/* 45-49 */ 
	0,0,0,0,0,	/* 50-54 */
	0,0,0,0,0,	/* 55-59 */
	0,0,0,0,0,	/* 60-64 */
	0,0,0,0,0,	/* 65-69 */
	0,0,0,0,0,	/* 70-74 */
	0,0,0,0,0,	/* 75-79 */
	0,0,0,0,0,	/* 80-84 */
	0,0,0,0,0,	/* 85-89 */
	0,0,0,0,0	/* 90-94 */
};

void (*GadgetUpHandle[])(struct Gadget *,USHORT) =
{
	0,&GUH1,&GUH2,&GUH3,&GUH4to9,	/* 0-4 */
	&GUH4to9,&GUH4to9,&GUH4to9,&GUH4to9,&GUH4to9,	/* 5-9 */
	&GUH10,&GUH11,&GUH12,&GUH13,&GUH14,	/* 10-14 */
	&GUH15,&GUH16,&GUH17,&GUH18,&GUH19,	/* 15-19 */
	&GUH20,&GUH21,&GUH22,&GUH4to9,&GUH4to9,	/* 20-24 */
	&GUH25,&GUH4to9,&GUH4to9,&GUH28,&GUH29, /* 25-29 */
	&GUH30,&GUH31,0,&GUH33,&GUH34, /* 30-34 */
	&GUH35,&GUH36,&GUH37,&GUH38,&GUH39, /* 35-39 */
	0,0,0,&GUH43,&GUH44,	/* 40-44 */
	&GUH45,&GUH46,&GUH47,&GUH48,&GUH49,	/* 45-49 */
	&GUH50,&GUH51,&GUH52,&GUH53,&GUH54,	/* 50-54 */
	&GUH55,&GUH56,&GUH57,&GUH58,&GUH59,	/* 55-59 */
	&GUH60,&GUH61,&GUH62,&GUH63,&GUH64,	/* 60-64 */
	&GUH65,&GUH66,&GUH67,&GUH68,&GUH69,	/* 65-69 */
	&GUH70,0,&GUH72,&GUH73,&GUH74,	/* 70-74 */
	&GUH75,&GUH76,&GUH77,&GUH78,&GUH79,	/* 75-79 */
	&GUH80,0,&GUH82,&GUH83,&GUH84,	/* 80-84 */
	&GUH85,&GUH86,&GUH87,&GUH88,&GUH89,	/* 85-89 */
	&GUH90,&GUH91,&GUH92,&GUH93,0	/* 90-94 */
};

/* externe Daten ----------------------------------------------------------- */

extern struct TextAttr font;

/* ------------------------------------------------------------------------- *
 * Gadgetinitialisierungsroutinen (f¸r NewGadget Strukts) ------------------ *
 * ------------------------------------------------------------------------- */

/* importierte Gadgetprototypen ----------------------------------------- */

extern struct NewGadget GZFunct,GSKnob,GLText,GChoose,GStep,GRange1,
	GRange2,GRText,GAct,OGChoose,OGText,OGInput,OGSwitch,OGPrefs,
	OGMinIn,OGText2,OGLightSel,OGLightChoose,OGBackup,DGList,DGText,
	DGLoad,DGSave,GHelpScroller,PGMeasure,PGDArea,PGResol,PGAction,
	PGSpecial,PGColNum,RGList,RGAct,PaGChoose,PaGSlider,PaGPalette,
	PaGInput,PaGAct;

/* ---------------------------------------------------------------------- */

void PrepareNewGadgetStructs(void)
{
	GZFunct.ng_TextAttr=
	GSKnob.ng_TextAttr=
	GLText.ng_TextAttr=
	GChoose.ng_TextAttr=
	GStep.ng_TextAttr=
	GRange1.ng_TextAttr=
	GRange2.ng_TextAttr=
	GRText.ng_TextAttr=
	GAct.ng_TextAttr=
	OGChoose.ng_TextAttr=
	OGText.ng_TextAttr=
	OGInput.ng_TextAttr=
	OGSwitch.ng_TextAttr=
	OGPrefs.ng_TextAttr=
	OGMinIn.ng_TextAttr=
	OGText2.ng_TextAttr=
	OGLightSel.ng_TextAttr=
	OGLightChoose.ng_TextAttr=
	OGBackup.ng_TextAttr=
	DGList.ng_TextAttr=
	DGText.ng_TextAttr=
	DGLoad.ng_TextAttr=
	DGSave.ng_TextAttr=
	GHelpScroller.ng_TextAttr=
	PGMeasure.ng_TextAttr=
	PGSpecial.ng_TextAttr=
	PGAction.ng_TextAttr=
	PGDArea.ng_TextAttr=
	PGResol.ng_TextAttr=
	PGColNum.ng_TextAttr=
	RGList.ng_TextAttr=
	RGAct.ng_TextAttr=
	PaGChoose.ng_TextAttr=
	PaGSlider.ng_TextAttr=
	PaGPalette.ng_TextAttr=
	PaGInput.ng_TextAttr=
	PaGAct.ng_TextAttr=&font;
	
	GZFunct.ng_Flags=
	GLText.ng_Flags=
	GStep.ng_Flags=
	GRText.ng_Flags=
	OGChoose.ng_Flags=
	OGText.ng_Flags=
	OGInput.ng_Flags=
	OGMinIn.ng_Flags=
	DGText.ng_Flags=
	PGMeasure.ng_Flags=
	PGDArea.ng_Flags=
	OGLightSel.ng_Flags=
	RGList.ng_Flags=
	PaGSlider.ng_Flags=
	PaGInput.ng_Flags=PLACETEXT_LEFT;
	
	OGSwitch.ng_Flags=
	OGText2.ng_Flags=PLACETEXT_RIGHT;
	
	GHelpScroller.ng_Width=20;
	
	GZFunct.ng_GadgetID=10;
	GSKnob.ng_GadgetID=1;
	GChoose.ng_GadgetID=48;
	GStep.ng_GadgetID=12;
	GRange1.ng_GadgetID=14;
	GRange2.ng_GadgetID=15;
	GAct.ng_GadgetID=20;
	OGChoose.ng_GadgetID=28;
	OGInput.ng_GadgetID=29;
	OGPrefs.ng_GadgetID=50;
	OGBackup.ng_GadgetID=62;
	DGList.ng_GadgetID=40;
	DGText.ng_GadgetID=41;
	DGLoad.ng_GadgetID=43;
	DGSave.ng_GadgetID=44;
	GHelpScroller.ng_GadgetID=63;
	RGList.ng_GadgetID=77;
	PaGPalette.ng_GadgetID=85;
	PaGChoose.ng_GadgetID=89;
	
	{
		static char str1[]="z(x,y)=",
								str2[]="x ",
								str3[]=";";
		GZFunct.ng_GadgetText=str1;
		GStep.ng_GadgetText=str2;
		GRText.ng_GadgetText=str3;
	}
}

void AdjustGadgetsToFont(void)	/* gleicht die NewGadget-Strukt. */
{															/* an die Fontgrˆﬂe an */
	GZFunct.ng_LeftEdge=W(70);
	GZFunct.ng_Height=H(12);		/* Hauptmen¸ und alle Windows */
	GSKnob.ng_Width=W(18);
	GSKnob.ng_Height=H(10);
	GLText.ng_Height=H(10);
	GChoose.ng_Width=W(64);
	GChoose.ng_Height=H(12);
	GStep.ng_LeftEdge=W(104);
	GStep.ng_Width=W(46);
	GStep.ng_Height=H(12);
	GRange1.ng_LeftEdge=W(72);
	GRange1.ng_Width=GRange2.ng_Width=W(36);
	GRange1.ng_Height=GRange2.ng_Height=H(12);
	GRange2.ng_LeftEdge=W(114);
	GRText.ng_LeftEdge=W(123);
	GRText.ng_Height=H(12);
	GAct.ng_Width=W(64);
	GAct.ng_Height=H(12);
	OGChoose.ng_LeftEdge=W(7);
	OGChoose.ng_TopEdge=H(57);
	OGChoose.ng_Width=W(110);
	OGChoose.ng_Height=H(12);
	OGText.ng_LeftEdge=W(188);
	OGText.ng_TopEdge=H(57);
	OGText.ng_Height=H(12);
	OGInput.ng_LeftEdge=W(188);
	OGInput.ng_TopEdge=H(57);
	OGInput.ng_Width=W(38);
	OGInput.ng_Height=H(12);
	OGSwitch.ng_LeftEdge=W(8);
	OGSwitch.ng_Width=W(26);
	OGSwitch.ng_Height=H(11);
	OGPrefs.ng_LeftEdge=W(8);
	OGPrefs.ng_Height=H(12);
	OGMinIn.ng_LeftEdge=W(84);
	OGMinIn.ng_TopEdge=H(24);
	OGMinIn.ng_Width=W(38);
	OGMinIn.ng_Height=H(12);
	OGText2.ng_LeftEdge=W(8);
	OGText2.ng_TopEdge=H(41);
	OGText2.ng_Height=H(12);
	OGLightSel.ng_Width=W(80);
	OGLightSel.ng_Height=H(9);
	OGLightChoose.ng_Width=W(150);
	OGLightChoose.ng_Height=H(12);
	OGLightChoose.ng_LeftEdge=W(7);
	OGLightChoose.ng_TopEdge=H(13);
	OGBackup.ng_Width=W(100);
	OGBackup.ng_Height=H(12);
	DGList.ng_LeftEdge=W(8);
	DGList.ng_TopEdge=H(13);
	DGList.ng_Width=W(284);
	DGList.ng_Height=H(68);
	DGText.ng_LeftEdge=W(64);
	DGText.ng_TopEdge=H(84);
	DGText.ng_Width=W(220);
	DGText.ng_Height=H(12);
	DGLoad.ng_LeftEdge=W(8);
	DGLoad.ng_TopEdge=H(125);
	DGLoad.ng_Width=W(284);
	DGLoad.ng_Height=H(12);
	DGSave.ng_LeftEdge=W(8);
	DGSave.ng_TopEdge=H(143);
	DGSave.ng_Width=W(138);
	DGSave.ng_Height=H(12);

	PGMeasure.ng_Width=W(6*8);						/* "Einstellungen" Men¸ */
	PGMeasure.ng_Height=H(12);
	PGDArea.ng_Width=W(6*8);
	PGDArea.ng_Height=H(12);
	PGResol.ng_Width=W(15*8);
	PGResol.ng_Height=H(12);
	PGColNum.ng_Width=W(10*8);
	PGColNum.ng_Height=H(8);
	PGSpecial.ng_Width=W(90);
	PGSpecial.ng_Height=H(12);
	PGAction.ng_Width=W(90);
	PGAction.ng_Height=H(11);

	RGList.ng_LeftEdge=W(8);							/* Resolution Window */
	RGList.ng_TopEdge=H(13);
	RGList.ng_Width=W(352);
	RGList.ng_Height=H(74);
	RGAct.ng_Width=W(90);
	RGAct.ng_Height=H(11);
	
	PaGChoose.ng_LeftEdge=W(7);						/* Palette Window */
	PaGChoose.ng_TopEdge=H(13);
	PaGChoose.ng_Width=W(385);
	PaGChoose.ng_Height=H(12);
	PaGSlider.ng_LeftEdge=W(80);
	PaGSlider.ng_TopEdge=H(33);
	PaGSlider.ng_Width=W(234);
	PaGSlider.ng_Height=H(8);
	PaGPalette.ng_LeftEdge=W(323);
	PaGPalette.ng_TopEdge=H(33);
	PaGPalette.ng_Width=W(69);
	PaGPalette.ng_Height=H(27);
	PaGInput.ng_LeftEdge=W(80);
	PaGInput.ng_Height=H(12);
	PaGAct.ng_Width=W(90);
	PaGAct.ng_Height=H(11);
	PaGAct.ng_TopEdge=H(124);
}

/* Gadgethilfsroutinen ------------------------------------------------------ */

void __interrupt GadDisable(struct Gadget *gad,struct Window *win,BOOL state)
{
	GT_SetGadgetAttrs(gad,win,0,GA_Disabled,state,TAG_END);
}

int GadgetCount(struct Gadget *first)	/* z‰hlt die Gadgets der Liste ab first */
{
	int num=0;
	
	if (first) do { num++; } while (first=first->NextGadget);
	return(num);
}

void FreeGadgetList(struct Gadget *first, int cnt)
{	/* gibt den Speicher der cnt Gadgets in der Liste frei */
	/* !!! Der cnt-Wert muﬂ genau stimmen (von GadgetCount() vor AddGList() */
	
	int n;
	struct Gadget *gad;
	
	gad=first;
	for (n=0; n<(cnt-1) && gad; n++) gad=gad->NextGadget;
	if (gad)
	{
		gad->NextGadget=0;
		FreeGadgets(gad);
	}
}
