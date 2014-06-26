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
 Jaroslav Køivánek, Iliyan Georgiev, Toshiya Hachisuka, Petr Vévoda, Martin Šik,
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
by Tomáš Davidoviè (http://www.davidovic.cz) and Iliyan Georgiev
(http://www.iliyan.com/). SmallUPBP heavily modifies it to add support for
participating media, point and beam-based volumetric estimators, and loading scenes in OBJ. Its
main authors are Petr Vévoda, Martin Šik (http://cgg.mff.cuni.cz/~sik/)
and Jaroslav Køivánek (http://cgg.mff.cuni.cz/~jaroslav/).

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

If you have any questions or ideas for improvement, feel free to contact Petr Vévoda
<petrvevoda@seznam.cz> or Jaroslav Krivanek <jaroslav.krivanek@mff.cuni.cz>.

================================================================================
 Contents
================================================================================

The archive you have downloaded contains following folders and files:
embree-2.0 - source code of Embree
OpenEXR    - header files and precompiled libraries of OpenEXR
scenes     - scene and batch files used to render images in the UPBP paper
SmallUPBP  - source code of this renderer
Tools      - a few scripts for displaying and comparing rendered images
README.txt - this text

================================================================================
 Compilation and run
================================================================================

The easiest way to compile SmallUPBP is to use the provided Visual Studio 2013
solution SmallUPBP\SmallUPBP.sln (should work in Visual Studio 2010 or later). 
Simply build it and the SmallUPBP.exe file will be created in Build\SmallUPBP\x64\Release
(or Build\SmallUPBP\x64\Debug dependig on what configuration you have selected). 
This file is completely independent of any other files, you can move it and run 
wherever you want (however, the batch files in the scenes folder expect it in the original location).

To give you an idea: the solution contains three projects - embree, sys and
SmallUPBP. The first two are parts of Embree and need to be compiled as static
libraries and linked along with OpenEXR\OpenEXR.lib with the main project SmallUPBP.
We recommend to compile with C++11 support, but the program will work even without it, 
though a worse random number generator will be used.

As mentioned earlier, the resulting executable file is a command line program.
Type SmallUPBP.exe -h or SmallUPBP.exe -hf to see a short and full help respectively.
The short help contains only basic arguments sufficient to render your first images
while the full version lists all arguments the program knows.