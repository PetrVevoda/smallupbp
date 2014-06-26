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
                

Each scene is defined by three files:

OBJ
Geometry of the scene with references to materials. Can be obtained via export
from 3ds Max (to Wavefront OBJ). 

MTL
Materials of surfaces in the scene, must contain all materials referenced in
the OBJ file. Can be obtained via export from 3ds Max (while exporting to
Wavefront OBJ). Only Ka, Kd, Ke, Ks and Ns properties are taken into account.

AUX
Camera, media and lights. Camera definition can be obtained via export from
3ds Max (to ASCII), the rest is edited manually. To define a medium, type 
medium <mediumID> followed by its properties, each property on a new line. 
To place a medium in the scene, either make it the global medium, i.e. type 
globalMediumID <mediumID> anywhere bellow the definition of the desired medium 
(there can be only one global medium in the scene), or attach it to a material. 
To accomplish this, type material <name_in_MTL> and add a new line with 
mediumID <mediumID>. Crossing boundary of such material will then mean entering 
the specified medium. This way also other material properties can be defined or
redefined.
There are two types of materials: real and imaginary. Geometry with a real
material acts as expected, depending on its properties incident ray gets reflected
or refracted. Geometry with an imaginary material is only a container for medium.
Ray does not interact with its boundary at all. Type of a material is specified
by geometryType property. Another important material property is priority. It is
necessary in case of overlapping geometry with different materials for decision
which of them is in effect. Following rules must be obeyed:
- no imaginary material can have greater priority than a real material
- no object with an imaginary material can intersect an object with a real material
  (but it can include it completely)
- materials of intersecting objects cannot have same priorities
If the camera or an area light source is not in the global medium (there is always
a global medium, clear by default), it is enclosed by a geometry with a material.
This material must be explicitly specified by CAMERA_MATERIAL and enclosingMatId 
property, respectively.
Schematic example of the AUX file is given bellow. For details please see actual
scene files and ObjReader.hxx and ObjReader.cxx source files.  

Example:

TM_ROW0 <float>	<float> <float>
TM_ROW1 <float>	<float> <float>
TM_ROW2 <float>	<float> <float>
TM_ROW3 <float>	<float> <float>
CAMERA_FOV <float>                  ... horizontal FOV
CAMERA_TDIST <float>                ... focal distance
CAMERA_MATERIAL <materialID>

medium Medium1
absorption <float> <float> <float>
emission <float> <float> <float>
scattering <float> <float> <float>
g <float>                           ... mean cosine
continuation_probability <float>

globalMediumID <mediumID>

material Material1
mirror <float> <float> <float>
ior <float>
mediumID <mediumID>
priority <int>
geometryType <real|imaginary>

material Material2
Ke <float> <float> <float>          ... emission for area light sources
enclosingMatId <materialID>

light_background_em <float> <foat> <filename> ... adds background light source 
                                                  defined by an environment map 
                                                  stored in the specified file 
                                                  scaled and rotated by the 
                                                  specified factors (in this order)

Besides the described three files you will find a folder called batches for each
scene. It contains batch files that were used to render images in the UPBP paper.
You can directly run them or use as a guide for rendering these scenes with suitable
command line arguments.