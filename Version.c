/* ---------------------------------------------------------------------- */
/* Plot - 25-12-93 SSi    -    VERSIONSANGABE                             */
/* ---------------------------------------------------------------------- */

#include "Plot.h"

/* Version von 3dPlot ------------------------------------------------------ */

char _versionstring[]="\0$VER: 3dPlot 5.03";

short PLOTVERSION=5;
short PLOTSUBVERSION=3;

char *PROGTYPE=
#ifdef _M68881				/* Konfigurationsabhängige Anzeigen */
	"AA/FPU";
#else
	#ifdef _FFP
		"AA/FFP";
	#else
		"AA";
	#endif
#endif

char *COMPILATIONDATE=__DATE__;

/* ------------------------------------------------------------------------ */
