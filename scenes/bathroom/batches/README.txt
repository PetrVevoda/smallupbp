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

Batches expect SmallUPB.exe in ..\..\..\Build\SmallUPBP\x64\Release (Build folder
on the same level as embree-2.0, OpenEXR etc.) and scene files
in ..\ (one level up). If you don't have the exe file, please build the project
first. If you moved the files or changed folder structure, you would need to
modify paths in the batches appropriately.

Provided batches:
- generate_report ... 12 renderings (different modes and algorithms), each 1 hour long
- run_reference   ... reference image without purely specular paths, outputs one image
                      per 100 iterations
- run_specular    ... purely specular paths for the reference image, outputs one image
                      per 100 iterations
- run_techniques  ... renders contribution and weights of all techniques, outputs
                      images per 100 iterations
- run_temp        ... auxiliary file called by generate_report, should not be run
                      directly
                    
If you need to make rendering faster, try lowering down the resolution of images.