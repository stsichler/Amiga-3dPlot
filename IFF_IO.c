/* --------------------------------------------------------------------- */
/* Plot V2.1 - 8-1-94 SSi --- IFF-Lade(V3.0)/Speicherroutinen ---------- */
/* --------------------------------------------------------------------- */

#include "Plot.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <intuition/intuition.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <libraries/iffparse.h>
#include <strings.h>
#include <math.h>

#define MINTERM 0xcc		/* Term für Quelle B ohne Invertierung */
 
/* Übergabeparameter --------------------------------------------------- */

extern struct Screen *OurS;
extern struct Window *OurW;

extern short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. für die Funktion */
								/* auf dem Bildschirm */

extern char Filename[];

extern short Options;

extern struct RGColSupp *RGColSupp;
extern struct PlotPrefs *activePrefs;

/* Definition der IFF-Strukturen --------------------------------------- */

#define ID_ILBM MAKE_ID('I','L','B','M')

#define ID_BMHD MAKE_ID('B','M','H','D')
struct IFF_BMHD								/* Profibuch s. 944 */
{
	UWORD	w,h;
	WORD	x,y;
	UBYTE	nPlanes;
	UBYTE	masking;
	UBYTE 	compression;
	UBYTE 	pad1;
	UWORD	transparentColor;
	UBYTE 	xAspect,yAspect;
	WORD	pageWidth,pageHeight;
};

#define ID_CMAP MAKE_ID('C','M','A','P')
struct IFF_CMAP
{
	UBYTE r,g,b;
};

#define ID_CAMG MAKE_ID('C','A','M','G')
struct IFF_CAMG
{
	ULONG ViewModes;
};

#define ID_GRAB MAKE_ID('G','R','A','B')
struct IFF_GRAB
{
	WORD x,y;
};

#define ID_BODY MAKE_ID('B','O','D','Y')

/* Prototypen ---------------------------------------------------------- */

extern int __asm far WriteIFFBody(register __d0 struct IFFHandle *, 
								register __a0 struct BitMap *);

extern int __asm far ReadIFFBody(register __d0 struct IFFHandle *,
								register __a0 struct BitMap *,
								register __a1 struct RastPort *);

/* --------------------------------------------------------------------- */

int SavePicture(void)	/* RETURN: Fehler */
{
	int err;
	BPTR File;
	
	if (!(File=Open(Filename,MODE_NEWFILE)))
	{
		err=IoErr();
		Error(l[70]);
	}
	else
	{
		err=_SavePicture(File);
		Close(File);
		CreateIcon(Filename);
	}
	return(err);
}

/*************************************************************************/
/* Speicherungsroutine ------------------------------------------------- */
/*************************************************************************/

#define SAVEROWS 8		/* Anzahl der Zeilen, die auf einmal gespeichert */
						/* werden */

int _SavePicture(BPTR File)
{
	int error=0;
	short n;
	
	/* Zwischenspeicher */
	
	struct IFF_BMHD bmhd;
	struct IFF_CAMG camg;
	struct IFF_GRAB grab;

	static struct RastPort BufRPort;
	static struct BitMap bm;
	
	struct IFFHandle *iff;


	if (!File) return(-1);
	
	if (!(iff=AllocIFF())) { Error(l[68]); return(-1); }

	iff->iff_Stream=File;
	InitIFFasDOS(iff);
	if (error=OpenIFF(iff,IFFF_WRITE)) { FreeIFF(iff); return(-1); }
	
	/* ---- Pufferrastport initialisiern ------ */
	
	InitBitMap(&bm,activePrefs->DisplayDepth,DrawX-Drawx+1,SAVEROWS);
	for (n=0;n<8;n++) bm.Planes[n]=0;
	for (n=0;n<bm.Depth;n++) if (!(bm.Planes[n]=AllocRaster(DrawX-Drawx+1,SAVEROWS))) { Error(l[68]); goto ERR; }
	
	InitRastPort(&BufRPort);	/* RPort initialisieren */
	BufRPort.BitMap=&bm;

	/* IFF Fileausgabe beginnen ----------- */
	
	if (error=PushChunk(iff,ID_ILBM,ID_FORM,IFFSIZE_UNKNOWN)) goto ERR;
	
	if (error=PushChunk(iff,ID_ILBM,ID_BMHD,sizeof(bmhd))) goto ERR;
	bmhd.w=DrawX-Drawx+1;
	bmhd.h=DrawY-Drawy+1;
	bmhd.x=Drawx;
	bmhd.y=Drawy;
	bmhd.nPlanes=activePrefs->DisplayDepth;
	bmhd.masking=2;
	bmhd.compression=1;
	bmhd.pad1=0;
	bmhd.transparentColor=(Options&OPT_RG3d)?RGColSupp->BGCol:3;
	bmhd.xAspect=bmhd.yAspect=1;
	bmhd.pageWidth=OurS->Width;
	bmhd.pageHeight=OurS->Height;
	if ((error=WriteChunkBytes(iff,&bmhd,sizeof(bmhd))<0)) goto ERR;
	if (error=PopChunk(iff)) goto ERR;
	
	if (error=PushChunk(iff,ID_ILBM,ID_CMAP,IFFSIZE_UNKNOWN)) goto ERR;
	for (n=0;n<(1<<activePrefs->DisplayDepth);n++)
	{
		ULONG buf[3];
		struct IFF_CMAP cmap;
		
		GetRGB(OurS->ViewPort.ColorMap,(ULONG)n,buf);
		
		cmap.r=buf[0]>>24;	/* auf 8 Bit kürzen */
		cmap.g=buf[1]>>24;
		cmap.b=buf[2]>>24;

		if ((error=WriteChunkBytes(iff,&cmap,sizeof(cmap))<0)) goto ERR;
	}
	if (error=PopChunk(iff)) goto ERR;
	
	if (error=PushChunk(iff,ID_ILBM,ID_CAMG,sizeof(camg))) goto ERR;
	camg.ViewModes=activePrefs->DisplayID;
	if ((error=WriteChunkBytes(iff,&camg,sizeof(camg))<0)) goto ERR;
	if (error=PopChunk(iff)) goto ERR;
		
	if (error=PushChunk(iff,ID_ILBM,ID_GRAB,sizeof(grab))) goto ERR;
	grab.x=(DrawX-Drawx+1)/2;
	grab.y=(DrawY-Drawy+1)/2;
	if ((error=WriteChunkBytes(iff,&grab,sizeof(grab))<0)) goto ERR;
	if (error=PopChunk(iff)) goto ERR;
	
	if (error=PushChunk(iff,ID_ILBM,ID_BODY,IFFSIZE_UNKNOWN)) goto ERR;
		
	/* BODY-CHUNK "in Raten" schreiben */
	
	{
		short CurY,MaxRows;
		
		MaxRows=BufRPort.BitMap->Rows;
		
		for (CurY=Drawy; CurY<(DrawY+1); CurY+=MaxRows)
		{
			BufRPort.BitMap->Rows=min(MaxRows,DrawY-CurY+1);
			
			ClipBlit(OurW->RPort,Drawx,CurY,&BufRPort,0,0,DrawX-Drawx+1,
						BufRPort.BitMap->Rows,MINTERM);
	
			if (error=WriteIFFBody(iff,BufRPort.BitMap)) goto ERR; /* BODY-CHUNK */
		}
		BufRPort.BitMap->Rows=MaxRows;
	}
	
	if (error=PopChunk(iff)) goto ERR;
	
	if (error=PopChunk(iff)) goto ERR;
	
ERR:
	/* Speicher freigeben */

	for (n=0;n<bm.Depth;n++) if (bm.Planes[n]) FreeRaster(bm.Planes[n],DrawX-Drawx+1,SAVEROWS);

	CloseIFF(iff);
	FreeIFF(iff);
	return(error);
}

/* Kompressionsroutine für IFF-Grafikdaten -------------------------------- */

/* im File DiskIOa.a	*/

/****************************************************************************/

#define LOADROWS 8		/* Anzahl der Zeilen, die auf einmal geladen */
						/* werden */

int _ReadIFF_BMHD(BPTR File)	/* Einlesen und kontrolle eines IFF-BMHD */
{							/* Lesezeiger auf 'FORM' */
	short a;
	int error=0;

	struct IFFHandle *iff;
	
	if (!File) return(-1);
	if (!(iff=AllocIFF())) { Error(l[68]); return(-1); }
	iff->iff_Stream=File;
	InitIFFasDOS(iff);
	if (!(error=OpenIFF(iff,IFFF_READ)))
	{
		PropChunk(iff,ID_ILBM,ID_BMHD);
		StopChunk(iff,ID_ILBM,ID_BODY);
	
		if (!(error=ParseIFF(iff,IFFPARSE_SCAN)))
		{
			struct StoredProperty *sp;
			
			/* BMHD überprüfen */
			
			if (sp=FindProp(iff,ID_ILBM,ID_BMHD))
			{
				struct IFF_BMHD *bmhd;
				bmhd=(struct IFF_BMHD *)sp->sp_Data;
					
				/* BitMapHeader kontrollieren */
				
				if (bmhd->w!=DrawX-Drawx+1) error++;
				if (bmhd->h!=DrawY-Drawy+1) error++;
				if (bmhd->nPlanes!=OurW->RPort->BitMap->Depth) error++;
				if (bmhd->compression!=1) error++;
				
				if (!error)
				{
					struct BitMap *cbm;				/* Clipboard Bitmap */
				
					/* Clipboard Bitmap öffnen und BODY einlesen */
					
					if (cbm=AllocMem(sizeof(struct BitMap),MEMF_CLEAR))
					{
						InitBitMap(cbm,OurW->RPort->BitMap->Depth,DrawX-Drawx+1,LOADROWS);
						for (a=0;a<cbm->Depth;a++) if (!(cbm->Planes[a]=AllocRaster(DrawX-Drawx+1,LOADROWS))) error=-1;
						if (!error)
						{
							if (error=ReadIFFBody(iff,cbm,OurW->RPort)) Error(l[74]);
						}
						else Error(l[68]);
						for (a=0;a<cbm->Depth;a++) if (cbm->Planes[a]) FreeRaster(cbm->Planes[a],DrawX-Drawx+1,LOADROWS);
					} else { Error(l[68]); error=-1; }
				} else { Error(l[113]); error=-1; }
			} else { Error(l[112]); error=-1; }
			StopOnExit(iff,ID_ILBM,ID_FORM);
			ParseIFF(iff,IFFPARSE_SCAN);
		} else Error(l[112]);
	
		CloseIFF(iff);
	} else Error(l[74]);
	
	FreeIFF(iff);
	
	return(error);
}
