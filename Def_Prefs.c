/* ------------------------------------------------------------------------- *
 * 3dPlot - DefaultPrefs.c (PlotPreferences) - 25-6-95 - ®SSi
 * ------------------------------------------------------------------------- */

#include "Plot.h"
#include <graphics/displayinfo.h>


struct PlotPrefs _defaultPrefs =
{
	PP_IDSTRING,0,0,0,0,0,
	0,
	45.0,6.0,26.0,19.5,
	0.0,0.0,
	0,0,
	0 /* DEFAULT_MONITOR_ID|HIRES_KEY */ ,3,
	0,0,
	"XEN.font",9,0, // "topaz.font",8,0,
	0,
		0,0xaaaaaaaa,0x99999999,0x88888888,
		1,0xeeeeeeee,0xeeeeeeee,0xcccccccc,
		2,0x77777777,0x66666666,0x55555555,
		3,0,0,0,
		0,
		".125+.6875m",
		".25+.6875m",
		".40625(1+m)",
		0,0x99999999,0x99999999,0x99999999,	
		1,0xcccccccc,0xcccccccc,0xcccccccc,
		2,0x66666666,0x66666666,0x66666666,
		3,0,0,0,
		PPCF_RG3d,
		".25+.75a-.25(1-a)b;a=(.2+.8l)?l;b=(.2+.8r)?r",
		".25+.6b-.25(1-b)a;a=(.2+.8l)?l;b=(.2+.8r)?r",
		".25-.125(a+b);a=(.2+.8l)?l;b=(.2+.8r)?r"
};
