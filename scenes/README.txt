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


This folder contains all 4 scenes that appeared in the UPBP paper: 
- bathroom:     created by Chaos Group (http://www.chaosgroup.com), 
                modified by Iliyan Georgiev and Martin Sik
- candle:       originally from the bathroom scene,
                modified by Jaroslav Krivanek
- mirror balls: created by Toshiya Hachisuka (http://cs.au.dk/~toshiya/), 
                modified by Martin Sik
- still life:   created by Ondrej Karlik (http://keymaster.cz/), 
                modified by Martin Sik, environment map Newport Loft by Blochi
                taken from http://www.hdrlabs.com/sibl/archive/

Each scene is defined by three files (please see the Petr Vévoda's thesis 
for more information about them):

OBJ
A file with geometry of the scene in the Wavefront OBJ file format. Can
be obtained via export from 3ds max or other 3D modelling software. 

MTL
A file with materials of the scene geometry in the MTL file format. Can
be obtained via export from 3ds max or other 3D modelling software
(while exporting to OBJ).

AUX
A file with definition of the camera, media, additional material
properties and lights. It is a manually created ASCII file in our own format.

Besides the .OBJ, .MTL and .OBJ.AUX files you will find a folder called batches
for each scene. It contains batch files that were used to render images in
the UPBP paper. You can directly run them or use as a guide for rendering these 
scenes with suitable command line arguments.