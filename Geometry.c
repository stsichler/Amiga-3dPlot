/* ---------------------------------------------------------------------- *
 * Geometry.c - 21-8-94 SSi     -    geometrische Routinen                *
 * ---------------------------------------------------------------------- */

#include "Plot.h"

/* private Prototypen --------------------------------------------------- */

short ccw(point,point,point);
	
/* ---------------------------------------------------------------------- */

int IsPointInTri(point p,point ep[])	/* Testet ob p im Viereck ep liegt */
{									/* RETURN: 0 - nein / !=0 - ja */
	short a;
	short pos;	/* Bits: 0 l / 1 r / 2 o / 3 u */

	/* testet, ob der Punkt überhaupt im Dreieck liegen kann (Grenztest) */
	
	for (a=0,pos=0;a<3;a++)
	{
		if (ep[a].bx<p.bx) pos|=(1<<0);
		else pos|=(1<<1);
		if (ep[a].by<p.by) pos|=(1<<2);
		else pos|=(1<<3);
	}
	if (pos!=0xf) return(0);
	
	/* genauer Test */
	
	pos=ccw(ep[0],ep[1],p)+ccw(ep[1],ep[2],p)+ccw(ep[2],ep[0],p);
	
	if (pos==3 || pos==-3) return(-1);
	
	return(0);
}

/* ---------------------------------------------------------------------- */

/* aus Algorithmen in C (S.403) */

int DoLinesIntersect(line *l1,line *l2)/* testet, ob sich die Lin. schneiden */
{									/* RETURN: 0 - nein / !=0 - ja */
	return(((ccw(l1->p1,l1->p2,l2->p1)
	        *ccw(l1->p1,l1->p2,l2->p2))<=0)
	    && ((ccw(l2->p1,l2->p2,l1->p1)
		    *ccw(l2->p1,l2->p2,l1->p2))<=0));
}

/* ---------------------------------------------------------------------- */

/* aus Algorithmen in C (S.402) */

short ccw(point p0,point p1, point p2)
{
	short dx1,dx2,dy1,dy2;
	  
	dx1=p1.bx-p0.bx; dy1=p1.by-p0.by;
	dx2=p2.bx-p0.bx; dy2=p2.by-p0.by;
	if (dx1*dy2>dy1*dx2) return(1);
	if (dx1*dy2<dy1*dx2) return(-1);
	if ((dx1*dx2<0)||(dy1*dy2<0)) return(-1);
	if ((dx1*dx1+dy1*dy1)<(dx2*dx2+dy2*dy2)) return(1);
	return(0);
}
