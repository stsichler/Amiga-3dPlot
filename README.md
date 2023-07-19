# Amiga-3dPlot
**Mathematical Function Plotter for Amiga, programmed in the 90s**

![Screenshot](/Screenshot.png)

3dPlot renders two-dimensional functions `f(x,y)` using the real part of the function's result value as z coordinate.
The view can be rotated and positions of light sources can be configured to your needs. Anaglyph red/green stereoscopic mode is also supported.

The binary also features localization. Currently available languages are English and German.

For execution of the included original binary, a 68060 CPU is required, but 3dPlot can be recompiled (using SAS/C 6.58) for 68020/40. **An FPU ist required.** 

3dPlot's custom function parser supports the following mathematical functions, all featuring full support of complex numbers:

  `sin`, `cos`, `tan`, `cot`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `ln`, `exp`, `log`, `abs`, `sqrt`, `sgn`, `sin²`, `cos²`, `tan²`

See the list of all supported operators in [Help.txt](/Help.txt) or [Hilfe.txt](/Hilfe.txt).

In addition, an `ìterate(<result>, <expression1>, <expression2>, ...)` function is supported for rendering of functions requiring iterative calculations, like the *Mandelbrot set*.
