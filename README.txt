// ============================================================================ //
// Copyright (C) 2014,                                                          //
// Petr Vevoda,                                                                 //
// Martin Sik (http://cgg.mff.cuni.cz/~sik/),                                   //
// Tomas Davidovic (http://www.davidovic.cz),                                   //
// Iliyan Georgiev (http://www.iliyan.com/),                                    //
// Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)                        //
//                                                                              //
// Permission is hereby granted, free of charge, to any person obtaining        //
// a copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation    //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,     //
// and/or sell copies of the Software, and to permit persons to whom            //
// the Software is furnished to do so, subject to the following conditions:     //
//                                                                              //
// The above copyright notice and this permission notice shall be included      //
// in all copies or substantial portions of the Software.                       //
//                                                                              //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,              //
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF           //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.       //
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,                //
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE   //
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                //
//                                                                              //
// (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)      //
// ============================================================================ //

================================================================================
 Introduction
================================================================================

SmallUPBP is a small physically based renderer that implements the unified
points, beams and paths algorithm described in the paper

"Unifying points, beams, and paths in volumetric light transport simulation"
 Jaroslav Křivánek, Iliyan Georgiev, Toshiya Hachisuka, Petr Vévoda, Martin Šik,
 Derek Nowrouzezahrai, and Wojciech Jarosz 
 ACM Transactions on Graphics 33(4) (SIGGRAPH 2014)

as well as a number of other algorithms, notably including progressive photon
mapping, (progressive) bidirectional photon mapping, bidirectional path
tracing, vertex connection and merging, beam radiance estimate, and photon 
beams.

The code compiles to a command line program that can render images of a
number of predefined scenes using the provided algorithms as well as of
custom scenes supplied by a user.

It is based on the SmallVCM renderer (http://www.smallvcm.com/) developed in 2012 
by Tomáš Davidovič (http://www.davidovic.cz) and Iliyan Georgiev
(http://www.iliyan.com/). SmallUPBP heavily modifies it to add support for
participating media, point and beam-based volumetric estimators, and loading scenes in OBJ. Its
main authors are Petr Vévoda, Martin Šik (http://cgg.mff.cuni.cz/~sik/)
and Jaroslav Křivánek (http://cgg.mff.cuni.cz/~jaroslav/).

Two third-party libraries are incorporated. Namely Embree (http://embree.github.io/)
for ray tracing acceleration and OpenEXR (http://www.openexr.com/) for reading
and writing OpenEXR images.

SmallUPBP is released under the following license:
 - MIT license (http://en.wikipedia.org/wiki/MIT_License)
   most of the supplied code, scenes and associated files
 - Apache 2.0 license (http://www.apache.org/licenses/LICENSE-2.0)
   Embree
 - modified BSD licence (http://www.openexr.com/license.html)
   OpenEXR

This code was used to render the images in the aforementioned paper and all scenes
and batch files necessary for reproducing the presented results are provided. 
It is released for educational and research purposes and neither the code nor
its documentation claims to be complete and bug-free. We apologize for any errors
and confusion.

Thorough description of the renderer, including its usage and implementation details 
as well as its theoretical background, is given by Petr Vévoda in his thesis 

"Robust light transport simulation in participating media"
 Petr Vévoda
 Master's thesis. Charles University in Prague, Prague, Czech Republic, February 2015.
 
Here we present its slightly modified version with minor fixes and improvements.
Note that the thesis originally refers to a DVD. Content of the DVD is almost the same 
as of the archive you have downloaded.

If you have any questions or ideas for improvement, feel free to contact Petr Vévoda
<petrvevoda@seznam.cz> or Jaroslav Křivanek <jaroslav.krivanek@mff.cuni.cz>.

================================================================================
 Contents
================================================================================

The archive you have downloaded contains following folders and files:
embree-2.0 - source code of Embree
OpenEXR    - header files and source code of OpenEXR
scenes     - scene and batch files used to render images in the UPBP paper
SmallUPBP  - source code of this renderer
Tools      - a few scripts for displaying and comparing rendered images
README.txt - this text

================================================================================
 Compilation and run
================================================================================

The source code is divided into two Microsoft Visual Studio 2013 solutions. The first one,
OpenEXR.sln in the OpenEXR\src folder, contains single project of the same name with
the OpenEXR library. The second one, SmallUPBP.sln in the SmallUPBP folder, contains 
three projects: embree, sys and SmallUPBP. The first two are parts of Embree and need 
to be compiled with the OpenEXR project as static libraries and linked with the main 
SmallUPBP project. The easiest way to do this is to use the provided solutions. Firstly, 
build the OpenEXR.sln solution. It creates the static library OpenEXR.lib (or OpenEXR-dbg.lib
depending on the selected configuration) in OpenEXR. Then built SmallUPBP.sln. It creates 
the remaining embree.lib and sys.lib static libraries as well as the executable SmallUPBP.exe 
file in Build\SmallUPBP\x64\Release (or in Build\SmallUPBP\x64\Debug depending on the selected
configuration). Note that the build configurations of both solutions must match. 
The SmallUPBP.exe file is completely independent of any other files and can be moved and run
freely (however, the batch files in the <code>scenes</code> folder expect it in the original
location).

As mentioned earlier, the resulting executable file is a command line program.
Type SmallUPBP.exe -h or SmallUPBP.exe -hf to see a short and full help respectively.
The short help contains only basic arguments sufficient to render your first images
while the full version lists all arguments the program knows. For more information
about the program please see the Petr Vévoda's thesis.
