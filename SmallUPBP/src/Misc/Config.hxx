/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/), 
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/), 
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#ifndef __CONFIG_HXX__
#define __CONFIG_HXX__

#include <vector>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <omp.h>
#include <string>
#include <set>
#include <sstream>

#include "..\Renderers\EyeLight.hxx"
#include "..\Renderers\PathTracer.hxx"
#include "..\Renderers\VertexCM.hxx"
#include "..\Renderers\VolBidirPT.hxx"
#include "..\Renderers\VolLightTracer.hxx"
#include "..\Renderers\VolPathTracer.hxx"
#include "..\Renderers\UPBP.hxx"

/**
 * @brief	Renderer configuration, holds algorithm, scene, and all other settings.
 */
struct Config
{
    /**
     * @brief	All available algorithms.
     */
    enum Algorithm
    {
        kEyeLight,                
        kProgressivePhotonMapping,
        kBidirectionalPhotonMapping,
        kBidirectionalPathTracing,
        kVertexConnectionMerging,
		kPathTracing,
		kVolumetricPathTracingDirect,
		kVolumetricPathTracingSpecOnly,
		kVolumetricPathTracingLight,
		kVolumetricPathTracingMIS,
		kLightTracing,
		kVolumetricLightTracing,
		kPointBeam2D,
		kBeamBeam1D,
		kVolumetricLightTracingFromVBPT,
		kVolumetricPathTracingDirectFromVBPT,
		kVolumetricPathTracingLightFromVBPT,
		kVolumetricPathTracingMISFromVBPT,
		kVolumetricBidirPathTracing,
		kVolumetricLightTracingFromUPBP,
		kVolumetricPathTracingDirectFromUPBP,
		kVolumetricPathTracingLightFromUPBP,
		kVolumetricPathTracingMISFromUPBP,
		kVolumetricBidirPathTracingFromUPBP,
		kProgressivePhotonMappingFromUPBP,
		kBidirectionalPhotonMappingFromUPBP,
		kVolumetricVertexConnectionMergingFromUPBP,
		kUPBPCustom,
		kUPBPAll,
        kAlgorithmMax // keep this one the last
    };

    /**
     * @brief	Gets a name of the given algorithm.
     * 			
     * 			The name is the human readable name of an algorithm displayed in the help.
     *
     * @param	aAlgorithm	The algorithm.
     *
     * @return	"unknown algorithm" if the value specified is not from \c Algorithm enum, otherwise
     * 			its name.
     */
    static const char* GetName(Algorithm aAlgorithm)
    {
        static const char* algorithmNames[kAlgorithmMax] =
        {
            "eye light",           
            "progressive photon mapping",
            "bidirectional photon mapping",
            "bidirectional path tracing",
			"vertex connection and merging",
			"path tracing",
            "direct volumetric path tracing",
			"direct volumetric path tracing (specular only)",
			"volumetric path tracing with light sampling",
			"volumetric path tracing MIS",
			"light tracing",
			"volumetric light tracing",
			"beam radiance estimate (pb2d on primary rays)",
			"photon beams (bb1d on primary rays)",
			"volumetric light tracing from vbpt",
			"direct volumetric path tracing from vbpt",
			"volumetric path tracing with light sampling from vbpt",
			"volumetric path tracing MIS from vbpt",
			"volumetric bidirectional path tracing",
			"volumetric light tracing from upbp",
			"direct volumetric path tracing from upbp",
			"volumetric path tracing with light sampling from upbp",
			"volumetric path tracing MIS from upbp",
			"volumetric bidirectional path tracing from upbp",
			"progressive photon mapping from upbp",
			"bidirectional photon mapping from upbp",
			"volumetric vertex connection and merging from upbp",
			"custom selection of techniques from upbp (tech=bpt|surf|pp3d|pb2d|bb1d)",
			"all available techniques of upbp"
        };

        if(aAlgorithm < 0 || aAlgorithm >= kAlgorithmMax)
            return "unknown algorithm";

        return algorithmNames[aAlgorithm];
    }

    /**
     * @brief	Gets an acronym of the given algorithm.
     *
     * @param	aAlgorithm	The algorithm.
     * 						
     * 						The acronym is the short code for an algorithm displayed in the help and
     * 						used as an argument of -a option.
     *
     * @return	"unknown" if the value specified is not from \c Algorithm enum, otherwise its acronym.
     */
    static const char* GetAcronym(Algorithm aAlgorithm)
    {
        static const char* algorithmNames[kAlgorithmMax] = {
			"el", "ppm", "bpm", "bpt", "vcm", "pt", "vptd", "vpts", "vptls", "vptmis", "lt", "vlt", "pb2d", "bb1d", "vbpt_lt", "vbpt_ptd", "vbpt_ptls", "vbpt_ptmis", "vbpt", "upbp_lt", "upbp_ptd", "upbp_ptls", "upbp_ptmis", "upbp_bpt", "upbp_ppm", "upbp_bpm", "upbp_vcm", "upbp_<tech>[+<tech>]*", "upbp_all" };

        if(aAlgorithm < 0 || aAlgorithm >= kAlgorithmMax)
            return "unknown";

        return algorithmNames[aAlgorithm];
    }

	// Settings:

    const Scene *mScene; //!< Scene to render.
    
	Algorithm   mAlgorithm;      //!< Algorithm used for rendering.
	uint        mAlgorithmFlags; //!< Flags of the algorithm used for rendering. Can contains \c EstimatorTechnique and \c OtherSettings values. Controls information display, the name of the output image file and settings of \c UPBP renderer.
    
	int         mIterations; //!< Number of rendering iterations.
    float       mMaxTime;    //!< Maximum time the rendering can take.
	int         mNumThreads; //!< Number of threads used for rendering.
	int         mBaseSeed;   //!< Base seed that is used to compute seeds for random number generators.
	std::string mOutputName; //!< Name of the output image file.
	Vec2i       mResolution; //!< Resolution of the rendered image.

	Framebuffer *mFramebuffer; //!< Framebuffer that accumulates result of rendering iterations.
	
	uint mMaxPathLength; //!< Maximum length of constructed paths.
	uint mMinPathLength; //!< Minimum length of constructed paths.
	
	uint mGridResolution; //!< Resolution of photon beams grid in the maximum extent of AABB.
	uint mMaxBeamsInCell; //!< Maximum allowed number of photon beams in a single grid cell. 0 means no restriction.
	uint mReductionType;  //!< Type of the reduction of photon beams in grid cells.
	
	mutable BeamDensity  mBeamDensity;  //!< Accumulated statistics of number of beams in grid cells.
	BeamDensity::ImgType mBeamDensType; //!< Type of accumulated statistics in \c mBeamDensity.
	float                mBeamDensMax;  //!< Maximum of accumulated statistics in \c mBeamDensity.
	
	float mRefPathCountPerIter; //!< Reference number of paths traced from lights per iteration. 
	float mPathCountPerIter;    //!< Number of paths traced from lights per iteration.

	BeamType mQueryBeamType;  //!< Used type of query beams.
	BeamType mPhotonBeamType; //!< Used type of photon beams.	

	float mSurfRadiusInitial; //!< Initial radius for surface photon mapping.
	float mSurfRadiusAlpha;   //!< Radius reduction factor for surface photon mapping.

    float mPP3DRadiusInitial; //!< Initial radius for PP3D.
    float mPP3DRadiusAlpha;   //!< Radius reduction factor for PP3D.
    
	float		      mPB2DRadiusInitial;     //!< Initial radius for PB2D.
	float             mPB2DRadiusAlpha;       //!< Radius reduction factor for PB2D.
	RadiusCalculation mPB2DRadiusCalculation; //!< Type of photon radius calculation.
	int			      mPB2DRadiusKNN;		  //!< Value x means that x-th closest photon will be used for calculation of radius of the current photon. 

	float		      mBB1DRadiusInitial;         //!< Initial radius for BB1D.
	float             mBB1DRadiusAlpha;	          //!< Radius reduction factor for BB1D.
	RadiusCalculation mBB1DRadiusCalculation;     //!< Type of photon beam radius calculation.	
	int			      mBB1DRadiusKNN;	          //!< Value x means that x-th closest beam vertex will be used for calculation of cone radius at the current beam vertex. 
	float             mBB1DUsedLightSubPathCount; //!< Number of light paths used to generate photon beams.
	float             mBB1DBeamStorageFactor;     //!< Factor used for computation of minimum MFP of media where photon beams are used. The lower it is the denser media will use photon beams.	

	std::string         mAdditionalArgs;	 //!< String appended to the name of the output image file that contains additional arguments specified on the command line.
	mutable float       mCameraTracingTime;  //!< Duration of tracing paths from camera.
	int					mContinuousOutput;   //!< Value x > 0 means generating one image per x iterations.
	mutable DebugImages mDebugImages;        //!< For creating debug images.
	std::string         mEnvMapFilePath;	 //!< Full pathname of the environment map file specified on the command line using the -em option.
	size_t				mMaxMemoryPerThread; //!< Maximum memory for light vertices in thread.
	float               mMinDistToMed;       //!< Minimum distance from camera at which scattering events in media can occur.
	bool                mShowTime;           //!< Whether to append duration of the rendering to the name of the output image file.	
	
	bool				mIgnoreFullySpecPaths; //!< Flag for upbp only, its sets it to ignore fully specular paths from camera.	
};

/**
 * @brief	Utility function, essentially a renderer factory.
 * 			
 * 			Creates a renderer according to \c aConfig.mAlgorithm with all relevant settings
 * 			stored in \c aConfig.
 *
 * @param	aConfig  	The configuration for the renderer.
 * @param	aSeed	 	Thread specific seed. Used by all renderers but \c UPBP.
 * @param	aBaseSeed	Global seed. Used by \c UPBP only.
 *
 * @return	New renderer that corresponds to settings in the given \c aConfig.
 */
AbstractRenderer* CreateRenderer(
    const Config& aConfig,
    const int     aSeed,
	const int     aBaseSeed)
{
    const Scene& scene = *aConfig.mScene;

    switch(aConfig.mAlgorithm)
    {
    case Config::kEyeLight:
        return new EyeLight(scene, aSeed);
    case Config::kPathTracing:
        return new PathTracer(scene, aSeed);
    case Config::kLightTracing:
        return new VertexCM(scene, VertexCM::kLightTrace,
            aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aSeed);
    case Config::kProgressivePhotonMapping:
        return new VertexCM(scene, VertexCM::kPpm,
            aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aSeed);
    case Config::kBidirectionalPhotonMapping:
        return new VertexCM(scene, VertexCM::kBpm,
            aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aSeed);
    case Config::kBidirectionalPathTracing:
        return new VertexCM(scene, VertexCM::kBpt,
            aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aSeed);
    case Config::kVertexConnectionMerging:
        return new VertexCM(scene, VertexCM::kVcm,
            aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aSeed);
	case Config::kVolumetricPathTracingDirect:
		return new VolPathTracer(scene, aSeed, VolPathTracer::kDirect);
	case Config::kVolumetricPathTracingLight:
		return new VolPathTracer(scene, aSeed, VolPathTracer::kLight);
	case Config::kVolumetricPathTracingMIS:
		return new VolPathTracer(scene, aSeed, VolPathTracer::kMIS);
	case Config::kVolumetricPathTracingSpecOnly:
		return new VolPathTracer(scene, aSeed, VolPathTracer::kSpecOnly);
	case Config::kVolumetricLightTracing:
		return new VolLightTracer(scene, aSeed, VolLightTracer::kLightTracer, aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mRefPathCountPerIter);
	case Config::kPointBeam2D:
		return new VolLightTracer(scene, aSeed, VolLightTracer::kPointBeam2D, aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mRefPathCountPerIter);
	case Config::kBeamBeam1D:
		return new VolLightTracer(scene, aSeed, VolLightTracer::kBeamBeam1D, aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mRefPathCountPerIter);
	case Config::kVolumetricLightTracingFromVBPT:
		return new VolBidirPT(scene, VolBidirPT::kLT, aSeed);
	case Config::kVolumetricPathTracingDirectFromVBPT:
		return new VolBidirPT(scene, VolBidirPT::kPTdir, aSeed);
	case Config::kVolumetricPathTracingLightFromVBPT:
		return new VolBidirPT(scene, VolBidirPT::kPTls, aSeed);
	case Config::kVolumetricPathTracingMISFromVBPT:
		return new VolBidirPT(scene, VolBidirPT::kPTmis, aSeed);
	case Config::kVolumetricBidirPathTracing:
		return new VolBidirPT(scene, VolBidirPT::kBPT, aSeed);
	case Config::kVolumetricLightTracingFromUPBP:
		return new UPBP(scene, UPBP::kLT, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kVolumetricPathTracingDirectFromUPBP:
		return new UPBP(scene, UPBP::kPTdir, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kVolumetricPathTracingLightFromUPBP:
		return new UPBP(scene, UPBP::kPTls, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kVolumetricPathTracingMISFromUPBP:
		return new UPBP(scene, UPBP::kPTmis, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kVolumetricBidirPathTracingFromUPBP:
		return new UPBP(scene, UPBP::kBPT, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kProgressivePhotonMappingFromUPBP:
		return new UPBP(scene, UPBP::kPPM, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kBidirectionalPhotonMappingFromUPBP:
		return new UPBP(scene, UPBP::kBPM, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kVolumetricVertexConnectionMergingFromUPBP:
		return new UPBP(scene, UPBP::kVCM, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	case Config::kUPBPCustom:
	case Config::kUPBPAll:
		return new UPBP(scene, UPBP::kCustom, aConfig.mAlgorithmFlags, aConfig.mSurfRadiusInitial, aConfig.mSurfRadiusAlpha, aConfig.mPP3DRadiusInitial, aConfig.mPP3DRadiusAlpha,
			aConfig.mPB2DRadiusInitial, aConfig.mPB2DRadiusAlpha, aConfig.mPB2DRadiusCalculation, aConfig.mPB2DRadiusKNN, aConfig.mQueryBeamType,
			aConfig.mBB1DRadiusInitial, aConfig.mBB1DRadiusAlpha, aConfig.mBB1DRadiusCalculation, aConfig.mBB1DRadiusKNN, aConfig.mPhotonBeamType, aConfig.mBB1DUsedLightSubPathCount, aConfig.mBB1DBeamStorageFactor, aConfig.mRefPathCountPerIter, aConfig.mPathCountPerIter,
			aConfig.mMinDistToMed, aConfig.mMaxMemoryPerThread, aSeed, aBaseSeed, aConfig.mIgnoreFullySpecPaths);
	default:
        std::cerr << "Error: unknown algorithm" << std::endl;
        exit(2);
    }
}

/**
 * @brief	Gets a description of the given configuration.
 * 			
 * 			Description is a multi line string that summarizes all settings in the given \c Config
 * 			relevant to the current rendering. It is displayed at the beginning of the rendering.
 *
 * @param	aConfig		  	The configuration to describe.
 * @param	aLeadingSpaces	Leading spaces to indent the description.
 *
 * @return	The description of the given configuration.
 */
std::string GetDescription(const Config& aConfig, const std::string& aLeadingSpaces)
{
	switch (aConfig.mAlgorithm)
	{
	case Config::kVolumetricBidirPathTracingFromUPBP:
		{
			std::ostringstream oss;
			oss << aConfig.GetName(aConfig.mAlgorithm) << '\n'
				<< aLeadingSpaces << "resolution:        " << aConfig.mResolution.x << "x" << aConfig.mResolution.y << '\n'
				<< aLeadingSpaces << "max path length:   " << aConfig.mMaxPathLength << '\n'
				<< aLeadingSpaces << "ref. paths/iter:   " << aConfig.mRefPathCountPerIter << '\n'
				<< aLeadingSpaces << "     paths/iter:   " << aConfig.mPathCountPerIter << '\n'
				;

			if (aConfig.mAlgorithmFlags & SPECULAR_ONLY)
					oss << aLeadingSpaces << "specular only" << '\n';

			return oss.str();
		}
	case Config::kProgressivePhotonMapping:
	case Config::kBidirectionalPhotonMapping:
	case Config::kVertexConnectionMerging:
		{
			std::ostringstream oss;
			oss << aConfig.GetName(aConfig.mAlgorithm) << '\n'
				<< aLeadingSpaces << "resolution:        " << aConfig.mResolution.x << "x" << aConfig.mResolution.y << '\n'
				<< aLeadingSpaces << "max path length:   " << aConfig.mMaxPathLength << '\n'
				<< aLeadingSpaces << "surf radius init:  " << aConfig.mSurfRadiusInitial << '\n'
				<< aLeadingSpaces << "     radius alpha: " << aConfig.mSurfRadiusAlpha << '\n'
				<< aLeadingSpaces << "ref. paths/iter:   " << aConfig.mRefPathCountPerIter << '\n'
				<< aLeadingSpaces << "     paths/iter:   " << aConfig.mPathCountPerIter << '\n'
				;
			return oss.str();
		}
	case Config::kProgressivePhotonMappingFromUPBP:
	case Config::kBidirectionalPhotonMappingFromUPBP:
	case Config::kVolumetricVertexConnectionMergingFromUPBP:
		{
			std::ostringstream oss;
			oss << aConfig.GetName(aConfig.mAlgorithm) << '\n'
				<< aLeadingSpaces << "resolution:        " << aConfig.mResolution.x << "x" << aConfig.mResolution.y << '\n'
				<< aLeadingSpaces << "max path length:   " << aConfig.mMaxPathLength << '\n'
				<< aLeadingSpaces << "surf radius init:  " << aConfig.mSurfRadiusInitial << '\n'
				<< aLeadingSpaces << "     radius alpha: " << aConfig.mSurfRadiusAlpha << '\n'
				<< aLeadingSpaces << "ref. paths/iter:   " << aConfig.mRefPathCountPerIter << '\n'
				<< aLeadingSpaces << "     paths/iter:   " << aConfig.mPathCountPerIter << '\n'
				<< aLeadingSpaces << "min dist to med:   " << aConfig.mMinDistToMed << '\n'
				;

			if (aConfig.mAlgorithmFlags & COMPATIBLE)
					oss << aLeadingSpaces << "compatible mode" << '\n';
			else if (aConfig.mAlgorithmFlags & PREVIOUS)
					oss << aLeadingSpaces << "previous mode" << '\n';

			if (aConfig.mAlgorithmFlags & SPECULAR_ONLY)
					oss << aLeadingSpaces << "specular only" << '\n';

			return oss.str();
		}
	case Config::kPointBeam2D:
		{
			std::ostringstream oss;
			oss << aConfig.GetName(aConfig.mAlgorithm) << '\n'
			    << aLeadingSpaces << "resolution:        " << aConfig.mResolution.x << "x" << aConfig.mResolution.y << '\n'
				<< aLeadingSpaces << "max path length:   " << aConfig.mMaxPathLength << '\n'
				<< aLeadingSpaces << "pb2d radius init:  " << aConfig.mPB2DRadiusInitial << '\n'
				<< aLeadingSpaces << "     radius alpha: " << aConfig.mPB2DRadiusAlpha << '\n';
				
			if (aConfig.mPB2DRadiusCalculation == KNN_RADIUS)
				oss << aLeadingSpaces << "     radius knn:   " << aConfig.mPB2DRadiusKNN << '\n';

			if (aConfig.mQueryBeamType == SHORT_BEAM)
				oss << aLeadingSpaces << "query beam type:   SHORT\n";
			else
				oss << aLeadingSpaces << "query beam type:   LONG\n";

			oss << aLeadingSpaces << "ref. paths/iter:   " << aConfig.mRefPathCountPerIter << '\n';

			return oss.str();
		}
	case Config::kBeamBeam1D:
		{
			std::ostringstream oss;
			oss << aConfig.GetName(aConfig.mAlgorithm) << '\n'
			    << aLeadingSpaces << "resolution:        " << aConfig.mResolution.x << "x" << aConfig.mResolution.y << '\n'
				<< aLeadingSpaces << "max path length:   " << aConfig.mMaxPathLength << '\n'
				<< aLeadingSpaces << "bb1d radius init:  " << aConfig.mBB1DRadiusInitial << '\n'
				<< aLeadingSpaces << "     radius alpha: " << aConfig.mBB1DRadiusAlpha << '\n';
				
			if (aConfig.mBB1DRadiusCalculation == KNN_RADIUS)
				oss << aLeadingSpaces << "     radius knn:   " << aConfig.mBB1DRadiusKNN << '\n';			

			oss << aLeadingSpaces << "     used l.paths: " << aConfig.mBB1DUsedLightSubPathCount << '\n';

			if (aConfig.mQueryBeamType == SHORT_BEAM)
				oss << aLeadingSpaces << "query beam type:   SHORT\n";
			else
				oss << aLeadingSpaces << "query beam type:   LONG\n";

			if (aConfig.mPhotonBeamType == SHORT_BEAM)
				oss << aLeadingSpaces << "photon beam type:  SHORT\n";
			else
				oss << aLeadingSpaces << "photon beam type:  LONG\n";

			if (aConfig.mMaxBeamsInCell > 0)
			{
				oss << aLeadingSpaces << "max beams/cell:    " << aConfig.mMaxBeamsInCell << '\n';
				oss << aLeadingSpaces << "reduction type:    " << aConfig.mReductionType << '\n';
			}

			oss << aLeadingSpaces << "ref. paths/iter:   " << aConfig.mRefPathCountPerIter << '\n';			

			return oss.str();
		}
	case Config::kUPBPCustom:
	case Config::kUPBPAll:
		{
			std::ostringstream oss;

			oss << "techniques from upbp:";
			if (aConfig.mAlgorithmFlags & BPT)
				oss << " bpt";
			if (aConfig.mAlgorithmFlags & SURF)
				oss << " surf";
			if (aConfig.mAlgorithmFlags & PP3D)
				oss << " pp3d";
			if (aConfig.mAlgorithmFlags & PB2D)
				oss << " pb2d";
			if (aConfig.mAlgorithmFlags & BB1D)
				oss << " bb1d";
			oss << '\n';

			oss << aLeadingSpaces << "resolution:        " << aConfig.mResolution.x << "x" << aConfig.mResolution.y << '\n'
				<< aLeadingSpaces << "max path length:   " << aConfig.mMaxPathLength << '\n';

			if (aConfig.mAlgorithmFlags & SURF)
			{
				oss << aLeadingSpaces << "surf radius init:  " << aConfig.mSurfRadiusInitial << '\n'
					<< aLeadingSpaces << "     radius alpha: " << aConfig.mSurfRadiusAlpha << '\n';
			}

			if (aConfig.mAlgorithmFlags & PP3D)
			{
				oss << aLeadingSpaces << "pp3d radius init:  " << aConfig.mPP3DRadiusInitial << '\n'
					<< aLeadingSpaces << "     radius alpha: " << aConfig.mPP3DRadiusAlpha << '\n';
			}

			if (aConfig.mAlgorithmFlags & PB2D)
			{
				oss << aLeadingSpaces << "pb2d radius init:  " << aConfig.mPB2DRadiusInitial << '\n'
					<< aLeadingSpaces << "     radius alpha: " << aConfig.mPB2DRadiusAlpha << '\n';
				
				if (aConfig.mPB2DRadiusCalculation == KNN_RADIUS)
					oss << aLeadingSpaces << "     radius knn:   " << aConfig.mPB2DRadiusKNN << '\n';
			}

			if (aConfig.mAlgorithmFlags & BB1D)
			{
				oss << aLeadingSpaces << "bb1d radius init:  " << aConfig.mBB1DRadiusInitial << '\n'
					<< aLeadingSpaces << "     radius alpha: " << aConfig.mBB1DRadiusAlpha << '\n';

				if (aConfig.mBB1DRadiusCalculation == KNN_RADIUS)
					oss << aLeadingSpaces << "     radius knn:   " << aConfig.mBB1DRadiusKNN << '\n';

				oss << aLeadingSpaces << "     used l.paths: " << aConfig.mBB1DUsedLightSubPathCount << '\n';
				
				if (aConfig.mAlgorithmFlags & NO_SINE_IN_WEIGHTS)
					oss << aLeadingSpaces << "     no sine in weights" << '\n';

				if (aConfig.mAlgorithmFlags & BB1D_PREVIOUS)
					oss << aLeadingSpaces << "     bb1d previous" << '\n';
			}

			if ((aConfig.mAlgorithmFlags & PB2D) || (aConfig.mAlgorithmFlags & BB1D))
			{
				if (aConfig.mQueryBeamType == SHORT_BEAM)
					oss << aLeadingSpaces << "query beam type:   SHORT\n";
				else
					oss << aLeadingSpaces << "query beam type:   LONG\n";
			}

			if (aConfig.mAlgorithmFlags & BB1D)
			{
				if (aConfig.mPhotonBeamType == SHORT_BEAM)
					oss << aLeadingSpaces << "photon beam type:  SHORT\n";
				else
					oss << aLeadingSpaces << "photon beam type:  LONG\n";

				if (aConfig.mMaxBeamsInCell > 0)
				{
					oss << aLeadingSpaces << "max beams/cell:    " << aConfig.mMaxBeamsInCell << '\n';
					oss << aLeadingSpaces << "reduction type:    " << aConfig.mReductionType << '\n';
				}
			}

			oss << aLeadingSpaces << "ref. paths/iter:   " << aConfig.mRefPathCountPerIter << '\n';
			oss << aLeadingSpaces << "     paths/iter:   " << aConfig.mPathCountPerIter << '\n';
			oss << aLeadingSpaces << "min dist to med:   " << aConfig.mMinDistToMed << '\n';

			if (aConfig.mAlgorithmFlags & COMPATIBLE)
					oss << aLeadingSpaces << "compatible mode" << '\n';
			else if (aConfig.mAlgorithmFlags & PREVIOUS)
					oss << aLeadingSpaces << "previous mode" << '\n';

			if (aConfig.mAlgorithmFlags & SPECULAR_ONLY)
					oss << aLeadingSpaces << "specular only" << '\n';

			return oss.str();
		}
	default:
		{
			std::ostringstream oss;
			oss << aConfig.GetName(aConfig.mAlgorithm) << '\n'
				<< aLeadingSpaces << "resolution:      " << aConfig.mResolution.x << "x" << aConfig.mResolution.y << '\n'
				<< aLeadingSpaces << "max path length: " << aConfig.mMaxPathLength << '\n'
				;
			return oss.str();
		}
	}
}

/**
 * @brief	Scene configurations. Just a shortcut to make lines of \c initSceneConfigs() method
 * 			shorter.
 */
typedef Scene::SceneConfig SC;

/**
 * @brief	Initializes scene configurations.
 * 			
 * 			This is where the predefined scenes listed in the help are defined.
 *
 * @return	A vector of scene configurations.
 */
std::vector<SC> initSceneConfigs()
{
	std::vector<SC> configs;
	
	// Preparing a few empty Cornell boxes used in the scenes. 

	std::vector<SC::Element> cornellBoxWithoutFloor;
	cornellBoxWithoutFloor.push_back(SC::Element(SC::Geometry::kLeftWall, SC::Materials::kDiffuseGreen));
	cornellBoxWithoutFloor.push_back(SC::Element(SC::Geometry::kRightWall, SC::Materials::kDiffuseRed));
	cornellBoxWithoutFloor.push_back(SC::Element(SC::Geometry::kBackWall, SC::Materials::kDiffuseBlue));
	cornellBoxWithoutFloor.push_back(SC::Element(SC::Geometry::kCeiling, SC::Materials::kDiffuseWhite));

	std::vector<SC::Element> cornellBoxWithDiffuseFloor(cornellBoxWithoutFloor);
	cornellBoxWithDiffuseFloor.push_back(SC::Element(SC::Geometry::kFloor, SC::Materials::kDiffuseWhite));

	std::vector<SC::Element> cornellBoxWithGlossyFloor(cornellBoxWithoutFloor);
	cornellBoxWithGlossyFloor.push_back(SC::Element(SC::Geometry::kFloor, SC::Materials::kGlossyWhite));

	std::vector<SC::Element> cornellBoxWithWhiteBackWall;
	cornellBoxWithWhiteBackWall.push_back(SC::Element(SC::Geometry::kLeftWall, SC::Materials::kDiffuseGreen));
	cornellBoxWithWhiteBackWall.push_back(SC::Element(SC::Geometry::kRightWall, SC::Materials::kDiffuseRed));
	cornellBoxWithWhiteBackWall.push_back(SC::Element(SC::Geometry::kBackWall, SC::Materials::kDiffuseWhite));
	cornellBoxWithWhiteBackWall.push_back(SC::Element(SC::Geometry::kCeiling, SC::Materials::kDiffuseWhite));
	cornellBoxWithWhiteBackWall.push_back(SC::Element(SC::Geometry::kFloor, SC::Materials::kDiffuseWhite));

	// Each scene is defined by its short and long name, light, global medium and a vector of elements. 
	// Each element consists of a geometry, material (optional) and medium (optional).

	// 0
	SC config("abssph", "large absorbing sphere + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithDiffuseFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Media::kRedAbsorbing));
	configs.push_back(config);

	// 1
	config.Reset("isosph", "large isoscattering sphere + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithDiffuseFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Media::kWhiteIsoScattering));
	configs.push_back(config);

	// 2
	config.Reset("mirsphinfisov", "large mirror sphere + weak isoscattering + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kWeakWhiteIsoScattering);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Materials::kMirror));
	configs.push_back(config);

	// 3
	config.Reset("smswiso", "small media spheres + weak isoscattering + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kWeakWhiteIsoScattering);
	config.AddAllElements(cornellBoxWithDiffuseFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Media::kBlueAbsorbingAndEmitting));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Media::kYellowGreenAbsorbingEmittingAndScattering));
	configs.push_back(config);

	// 4 
	config.Reset("smswani", "small media spheres + weak anisoscattering + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kWeakWhiteAnisoScattering);
	config.AddAllElements(cornellBoxWithDiffuseFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Media::kBlueAbsorbingAndEmitting));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Media::kYellowGreenAbsorbingEmittingAndScattering));
	configs.push_back(config);

	// 5
	config.Reset("ssssun", "specular small spheres + sun", SC::Lights::kLightSun, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Materials::kMirror));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Materials::kGlass));
	configs.push_back(config);

	// 6
	config.Reset("ssspoint", "specular small spheres + point light", SC::Lights::kLightCeilingPoint, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Materials::kMirror));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Materials::kGlass));
	configs.push_back(config);

	// 7
	config.Reset("sssback", "specular small spheres + background light", SC::Lights::kLightBackground, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Materials::kMirror));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Materials::kGlass));
	configs.push_back(config);

	// 8
	config.Reset("mirsph", "large mirror sphere + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Materials::kMirror));
	configs.push_back(config);

	// 9
	config.Reset("emisph", "large emitting sphere + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithDiffuseFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Media::kYellowEmitting));
	configs.push_back(config);

	// 10
	config.Reset("gemisph", "large glass emitting sphere", SC::Lights::kLightNone, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithDiffuseFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Materials::kGlass, SC::Media::kYellowEmitting));
	configs.push_back(config);

	// 11
	config.Reset("nogeoisobig", "no geometry + isoscattering + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kWhiteIsoScattering);
	configs.push_back(config);

	// 12
	config.Reset("nogeoisosmall", "no geometry + isoscattering + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kWhiteIsoScattering);
	configs.push_back(config);

	// 13
	config.Reset("nogeoisopt", "no geometry + isoscattering + point light", SC::Lights::kLightCeilingPoint, SC::Media::kWhiteIsoScattering);
	configs.push_back(config);

	// 14
	config.Reset("nogeoanibig", "no geometry + anisoscattering + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kWhiteAnisoScattering);
	configs.push_back(config);

	// 15
	config.Reset("nogeoanismall", "no geometry + anisoscattering + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kWhiteAnisoScattering);
	configs.push_back(config);

	// 16
	config.Reset("nogeoanipt", "no geometry + anisoscattering + point light", SC::Lights::kLightCeilingPoint, SC::Media::kWhiteAnisoScattering);
	configs.push_back(config);	

	// 17
	config.Reset("faceisosmall", "no geometry + isoscattering + facing small area light", SC::Lights::kLightFacingAreaSmall, SC::Media::kWhiteIsoScattering);
	configs.push_back(config);

	// 18
	config.Reset("faceisopt", "no geometry + isoscattering + facing point light", SC::Lights::kLightFacingPoint, SC::Media::kWhiteIsoScattering);
	configs.push_back(config);

	// 19
	config.Reset("faceanismall", "no geometry + anisoscattering + facing small area light", SC::Lights::kLightFacingAreaSmall, SC::Media::kWhiteAnisoScattering);
	configs.push_back(config);

	// 20
	config.Reset("faceanipt", "no geometry + anisoscattering + facing point light", SC::Lights::kLightFacingPoint, SC::Media::kWhiteAnisoScattering);
	configs.push_back(config);

	// 21
	config.Reset("overgwi", "three overlapping spheres (glass, water, ice) + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kIce));
	configs.push_back(config);

	// 22
	config.Reset("rovergwi", "three overlapping spheres (glass, water, ice) + one big red sphere + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kIce));
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Media::kRedAbsorbing));
	configs.push_back(config);

	// 23
	config.Reset("wovergwi", "three overlapping spheres (glass, water, ice) + one big water sphere + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kIce));
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	configs.push_back(config);

	// 24
	config.Reset("iovergwi", "three overlapping spheres (glass, water, ice) + one big ice sphere + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kIce));
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Materials::kIce));
	configs.push_back(config);

	// 25
	config.Reset("govergwi", "three overlapping spheres (glass, water, ice) + one big glass sphere + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kIce));
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Materials::kGlass));
	configs.push_back(config);

	// 26
	config.Reset("movergwi", "three overlapping spheres (glass, water, ice) + one big mirror sphere + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kIce));
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Materials::kMirror));
	configs.push_back(config);

	// 27
	config.Reset("aniovergwi", "three overlapping spheres (glass, water, ice) + weak anisoscattering + small area light", SC::Lights::kLightCeilingAreaSmall, SC::Media::kWeakWhiteAnisoScattering);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kWaterMaterial, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kIce));
	configs.push_back(config);

	// 28
	config.Reset("overiwr", "three overlapping glass spheres (iso, water, red absorb) + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Materials::kGlass, SC::Media::kWhiteIsoScattering));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Materials::kGlass, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Materials::kGlass, SC::Media::kRedAbsorbing));
	configs.push_back(config);

	// 29
	config.Reset("isooveriwr", "three overlapping spheres (iso, water, red absorb) + weak isoscattering + big area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kWeakWhiteIsoScattering);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Media::kWhiteIsoScattering));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Media::kRedAbsorbing));
	configs.push_back(config);

	// 30
	config.Reset("woveriwr", "three overlapping spheres (iso, water, red absorb) + one big water sphere + background light", SC::Lights::kLightBackground, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithWhiteBackWall);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomLeft, SC::Media::kWhiteIsoScattering));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereBottomRight, SC::Media::kWaterMedium));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereTop, SC::Media::kRedAbsorbing));
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Media::kWaterMedium));
	configs.push_back(config);

	// 31
	config.Reset("backg", "only background light and global medium", SC::Lights::kLightBackground, SC::Media::kLightReddishMedium);
	configs.push_back(config);
	
	// 32
	config.Reset("backas", "only background light and absorbing sphere", SC::Lights::kLightBackground, SC::Media::kClear);
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Media::kRedAbsorbing));
	configs.push_back(config);

	// 33
	config.Reset("backss", "only background light and scattering sphere", SC::Lights::kLightBackground, SC::Media::kClear);
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Media::kWhiteIsoScattering));
	configs.push_back(config);

	// 34
	config.Reset("bigss", "only big area light and scattering sphere", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeSphere, SC::Media::kWhiteIsoScattering));
	configs.push_back(config);

	// 35
	config.Reset("smallsb", "only small area light and scattering box", SC::Lights::kLightCeilingAreaSmallDistant, SC::Media::kClear);
	config.AddElement(SC::Element(SC::Geometry::kVeryLargeBox, SC::Media::kWeakYellowIsoScattering));
	configs.push_back(config);

	// 36
	config.Reset("ssssuninfisov", "specular small spheres + weak anisoscattering + sun", SC::Lights::kLightSun, SC::Media::kWeakWhiteIsoScattering);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Materials::kMirror));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Materials::kGlass));
	configs.push_back(config);

	// 37
	config.Reset("ssspointinfisov", "specular small spheres + weak anisoscattering + point light", SC::Lights::kLightCeilingPoint, SC::Media::kWeakWhiteIsoScattering);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Materials::kMirror));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Materials::kGlass));
	configs.push_back(config);

	// 38
	config.Reset("sssbackinfisov", "specular small spheres + weak anisoscattering + background light", SC::Lights::kLightBackground, SC::Media::kWeakWhiteIsoScattering);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereLeft, SC::Materials::kMirror));
	config.AddElement(SC::Element(SC::Geometry::kSmallSphereRight, SC::Materials::kGlass));
	configs.push_back(config);

	// 39
	config.Reset("glasssphbck", "large glass medium sphere + background light", SC::Lights::kLightBackground, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Materials::kGlass, SC::Media::kAbsorbingAnisoScattering));
	configs.push_back(config);

	// 40
	config.Reset("glasssphbck", "large glass medium sphere + large area light", SC::Lights::kLightCeilingAreaBig, SC::Media::kClear);
	config.AddAllElements(cornellBoxWithGlossyFloor);
	config.AddElement(SC::Element(SC::Geometry::kLargeSphereMiddle, SC::Materials::kGlass, SC::Media::kAbsorbingAnisoScattering));
	configs.push_back(config);

	return configs;
}

std::vector<SC> g_SceneConfigs = initSceneConfigs();	//!< Configurations of the predefined scenes.

/**
 * @brief	Creates a default name of the output image file that contains no scene identifier.
 * 			
 * 			If user does not specify his own name of the output image file, default one is
 * 			created based on parameters used for rendering. This one lacks any information about
 * 			rendered scene and is used by \c DefaultFilename() methods that add proper scene
 * 			identifier based on whether predefined or user scene is rendered.
 *
 * @param	aConfig	The configuration.
 *
 * @return	A default filename without scene.
 */
std::string DefaultFilenameWithoutScene(
    const Config &aConfig)
{
    std::string filename("");
	std::ostringstream convert;

	// We add iterations count.
	if (aConfig.mIterations > 0)
	{
		filename += "_i";
		convert.str("");
		convert << std::setfill('0') << std::setw(2) << aConfig.mIterations;
		filename += convert.str();
	}

	// We add maximum path length.
    filename += "_l";
	convert.str("");
	convert << std::setfill('0') << std::setw(2) << aConfig.mMaxPathLength;
	filename += convert.str();

	// We add acronym of the used algorithm.
    filename += "_a";
	convert.str("");
	convert << std::setfill('0') << std::setw(2) << aConfig.mAlgorithm;
	filename += convert.str();
	if (aConfig.mAlgorithm != Config::kUPBPCustom)		
		filename += Config::GetAcronym(aConfig.mAlgorithm);
	else
	{
		convert.str("");
		if (!(aConfig.mAlgorithmFlags & BPT) || !(aConfig.mAlgorithmFlags & SURF))
		{
			if (aConfig.mAlgorithmFlags & BPT)
				convert << "+bpt";
			else if (aConfig.mAlgorithmFlags & SURF)
				convert << "+surf";
		}
		if (aConfig.mAlgorithmFlags & PP3D)
			convert << "+pp3d";
		if (aConfig.mAlgorithmFlags & PB2D)
			convert << "+pb2d";
		if (aConfig.mAlgorithmFlags & BB1D)
			convert << "+bb1d";

		if ((aConfig.mAlgorithmFlags & BPT) && (aConfig.mAlgorithmFlags & SURF))
			filename += "upbp" + convert.str();
		else
			filename += "upbp_" + convert.str().substr(1);
	}

	// We add query beam type (if relevant).
	if ((aConfig.mAlgorithmFlags & PB2D) || (aConfig.mAlgorithmFlags & BB1D))
	{
		filename += "_qbt";
		if (aConfig.mQueryBeamType == SHORT_BEAM)
			filename += "S";
		else
			filename += "L";
	}

	// We add photon beam type (if relevant).
	if (aConfig.mAlgorithmFlags & BB1D)
	{
		filename += "_pbt";
		if (aConfig.mPhotonBeamType == SHORT_BEAM)
			filename += "S";
		else
			filename += "L";
	}

	// We add other args specified on command line.
	filename += aConfig.mAdditionalArgs;

    // And it will be written as exr.
    filename += ".exr";

    return filename;
}

/**
 * @brief	Creates a default name of the output image file for a predefined scene.
 * 			
 * 			If user does not specify his own name of the output image file, default one is
 * 			created based on parameters used for rendering. This one contains identifier of a
 * 			predefined scene.
 *
 * @param	aSceneID	Identifier of the predefined scene.
 * @param	aConfig 	The configuration.
 *
 * @return	A default filename.
 */
std::string DefaultFilename(
    const int    aSceneID,
    const Config &aConfig)
{
    std::string filename;

	// Name starts with a scene number.
	filename = "s";
	std::ostringstream convert;
	convert << std::setfill('0') << std::setw(2) << aSceneID;
	filename += convert.str();
	
	// We add acronym for the used scene.
    filename += "_";   
    filename += (aConfig.mScene)->mSceneAcronym;   

	// We add the rest.
    filename += DefaultFilenameWithoutScene(aConfig);

    return filename;
}

/**
 * @brief	Creates a default name of the output image file for a user scene.
 * 			
 * 			If user does not specify his own name of the output image file, default one is
 * 			created based on parameters used for rendering. This one contains filename of a user
 * 			scene.
 *
 * @param	aSceneFilePath	Full pathname of the user scene file.
 * @param	aConfig		  	The configuration.
 *
 * @return	A default filename.
 */
std::string DefaultFilename(
	const std::string &aSceneFilePath,
	const Config      &aConfig)
{
	std::string filename;

	// Name starts with scene file name
	filename = "s-";
	size_t lastSlashPos = aSceneFilePath.find_last_of("\\/");
	size_t lastDotPos = aSceneFilePath.find_last_of('.');
	filename += aSceneFilePath.substr(lastSlashPos + 1, lastDotPos - lastSlashPos - 1);   

	// We add the rest
    filename += DefaultFilenameWithoutScene(aConfig);

    return filename;
}

/**
 * @brief	Prints a warning if a legacy random number generator has to be used.
 */
void PrintRngWarning()
{
#if defined(LEGACY_RNG)
    printf("The code was not compiled for C++11.\n");
    printf("It will be using Tiny Encryption Algorithm-based"
        "random number generator.\n");
    printf("This is worse than the Mersenne Twister from C++11.\n");
    printf("Consider setting up for C++11.\n");
    printf("Visual Studio 2010, and g++ 4.6.3 and later work.\n\n");
#endif
}

/**
 * @brief	Prints a full help.
 * 			
 * 			Full help contains all available options and lists all algorithms and predefined
 * 			scenes.
 *
 * @param	argv	Arguments passed on the command line. Only a name of the exe file is used.
 */
void PrintHelp(const char *argv[])
{
    printf("\n");
    printf("Usage: %s <options>\n\n", argv[0]);
	printf("    Basic options:\n\n");
    
	printf("    -s  <scene_id> Selects the scene (default 0):\n");
	printf("        -1    user must supply additional argument which contains path to an obj scene file  \n");
	for(int i = 0; i < g_SceneConfigs.size(); i++)
		printf("        %-2d    %s\n", i, g_SceneConfigs[i].mLongName.c_str());

    printf("\n");

	printf("    -a  <algorithm> Selects the rendering algorithm (default upbp_all):\n");

    for(int i = 0; i < (int)Config::kAlgorithmMax; i++)
		printf("        %-10s  %s\n",
            Config::GetAcronym(Config::Algorithm(i)),
            Config::GetName(Config::Algorithm(i)));
	printf("\n");

    printf("    -l <length>    Maximum length of traced paths (default 10).\n");
	printf("    -t <sec>       Number of seconds to run the algorithm.\n");
	printf("    -i <iter>      Number of iterations to run the algorithm (default 1).\n");
	printf("    -o <name>      User specified output name, with extension .bmp or .exr (default .exr). The name can be prefixed with relative or absolute path but the path must exists.\n");
	printf("    -r <res>       Image resolution in format WIDTHxHEIGHT (default 256x256).\n");    
	printf("    -seed <seed>   Sets base seed (default 1234).\n");
	printf("\n    Note: Time (-t) takes precedence over iterations (-i) if both are defined.\n"); 

	printf("\n    Performance options:\n\n");
	printf("    -th <threads>                     Number of threads (default 0 means #threads = #cores).\n");
	printf("    -maxMemPerThread <memory>         Sets max memory in MB for light vertex array per each thread (default 500). Works only for upbp algorithms.\n");

	printf("\n    Radius options:\n\n");
	printf("    -r_alpha <alpha>       Sets same radius reduction parameter for techniques surf, pp3d, pb2d and bb1d.\n");
	printf("    -r_alpha_surf <alpha>  Sets radius reduction parameter for technique surf (default 0.75, value of 1 implies no radius reduction).\n");
	printf("    -r_alpha_pp3d <alpha>  Sets radius reduction parameter for technique pp3d (default 1, value of 1 implies no radius reduction).\n");
	printf("    -r_alpha_pb2d <alpha>  Sets radius reduction parameter for technique pb2d (default 1, value of 1 implies no radius reduction).\n");
	printf("    -r_alpha_bb1d <alpha>  Sets radius reduction parameter for technique bb1d (default 1, value of 1 implies no radius reduction).\n");
	printf("    -r_initial <initial>       Sets same initial radius for techniques surf, pp3d, pb2d and bb1d (if positive, absolute, if negative, relative to scene size).\n");
	printf("    -r_initial_surf <initial>  Sets initial radius for technique surf (default -0.0015, if positive, absolute, if negative, relative to scene size).\n");
	printf("    -r_initial_pp3d <initial>  Sets initial radius for technique pp3d (default -0.001,  if positive, absolute, if negative, relative to scene size).\n");
	printf("    -r_initial_pb2d <initial>  Sets initial radius for technique pb2d (default -0.001,  if positive, absolute, if negative, relative to scene size).\n");
	printf("    -r_initial_bb1d <initial>  Sets initial radius for technique bb1d (default -0.001,  if positive, absolute, if negative, relative to scene size).\n");
	printf("    -r_initial_pb2d_knn <const> <knn>  Sets photon A radius to distance between photon A and <knn>th closest photon multiplied by <const>.\n");	
	printf("    -r_initial_bb1d_knn <const> <knn>  Makes photon beam's conic with radius at each end computed from distance between beam end and <knn>th closest beam vertex multiplied by <const>.\n");
	printf("\n    Note: If both r_alpha and r_alpha_<tech> are specified, r_alpha_<tech> takes precedence (but for techniques other than <tech> parameter r_alpha applies). \n");
	printf("          Same applies for r_initial.\n");

	printf("\n    Light transport options (works only for upbp algorithms):\n\n");
	printf("    -previous            Simulates \"previous work\" (paths ES*M(S|D|M)*L with camera paths stopping at the first M).\n");
	printf("    -previous_bb1d       Simulates \"previous work\" for bb1d, other techniques not affected (paths ES*M(S|D|M)*L with camera paths stopping at the first M).\n");	
	printf("    -compatible          Restricts traced paths to be comparable with the \"previous work\" (paths ES*M(S|D|M)*L).\n");
	printf("    -speconly            Traces only purely specular paths.\n");
	printf("    -ignorespec <option> Sets whether upbp will ignore fully specular paths from camera (0=no(default),1=yes).\n");	

	printf("\n    Beams options:\n\n");
	printf("    -gridres <res>          Sets photon beams grid resolution in dimension of a maximum extent of grid AABB, resolution in other dim. is set to give cube sized grid cells (default 256).\n");
	printf("    -gridmax <max>          Sets maximum number of beams in one grid cell (default 0 means unlimited). Works only for bb1d algorithm (not upbp).\n");
	printf("    -gridred <red>          Sets type of reduction of tested beams in one grid cell (0=presample (default), 1=offset, 2=resample_fixed, 3=resample). Works only for bb1d algorithm (not upbp).\n");
	printf("    -beamdens <type> <max>  Outputs image(s) of statistics of hit beams and cells (0=none(default), 1=abs, 2=avg, 3=cells, 4=overfull, 5=all) normalized to the given max (-1=max in data(default), positive=given max). Works only for bb1d algorithm (not upbp).\n");
	printf("    -beamstore <factor>     Sets multiple of bb1d radius used for decision whether to store beams or not (0=stores all beams (default)). Works only for bb1d algorithm (not upbp).\n");
	printf("    -qbt <type>             Sets query beam type: S = uses short query beams,  L = uses long query beams (default).\n");
	printf("    -pbt <type>             Sets photon beam type: S = uses short photon beams (default), L = uses long photon beams.\n");
	printf("    -pbc <count>            First <count> traced light paths will generate photon beams (default -1, if positive, absolute, if negative, relative to total number of pixels).\n");
	printf("    -nosin                  Use sine only to compute bb1d contribution not weights.\n");

	printf("\n    Debug options:\n\n");
	printf("    -debugimg_option <option>          Sets debug images output options, possibilities are none(default), simple_pyramid (Veach-like), per_technique (one image per technique), pyramid_per_technique.\n");
	printf("    -debugimg_multbyweight <option>    Sets whether output debug images should be multiplied by MIS weights or not (no, yes(default), output_both).\n");
	printf("    -debugimg_output_weights <option>  Sets whether MIS weights per each technique should be output (0=no(default), 1=yes).\n");

	printf("\n    Other options:\n\n");
	printf("    -continuous_output <iter_count>  Sets whether we should continuously output images (<iter_count> > 0 says output image once per <iter_count> iterations, 0(default) no cont. output).\n");
	printf("    -em <filepath>                   Sets environment map in scenes with background light (expects absolute path to OpenEXR file with latitude-longitude mapping).\n");
	printf("    -min_dist2med <distance>         Sets minimum distance from camera for medium contribution (positive=absolute, negative=relative to scene size, zero=no effect (default)). Works only for upbp algorithms.\n");	
	printf("    -rpcpi <path_count>              Reference light path count per iteration (default -1, if positive, absolute, if negative, relative to total number of pixels). Works only for vlt, pb2d, bb1d and upbp algorithms.\n");
	printf("    -pcpi <path_count>               Light path count per iteration (default -1, if positive, absolute, if negative, relative to total number of traced light paths). Works only for vlt, pb2d, bb1d and upbp algorithms.\n");
	printf("    -sn <option>                     Whether to use shading normals: 0 = does not use shading normals, 1 = uses shading normals (default).\n");
	printf("    -time                            If present algorithm run duration is appended to the name of the output file.\n");	
}

/**
 * @brief	Prints a short help.
 * 			
 * 			Short help lists only basic options, scenes and algorithms.
 *
 * @param	argv	Arguments passed on the command line. Only a name of the exe file is used.
 */
void PrintShortHelp(const char *argv[])
{
    printf("\n");
    printf("Usage: %s [ -s <scene_id> | -a <algorithm> | -l <path length> |\n", argv[0]);
    printf("           -t <time> | -i <iteration> | -o <output_name> ]\n\n");
    
	printf("    -s  Selects the scene (default 0):\n");
	printf("          -1    user must supply additional argument which contains path to an obj scene file  \n");
	for(int i = 0; i < 5; i++)
		printf("          %-2d    %s\n", i, g_SceneConfigs[i].mLongName.c_str());
	printf("          5..%d other predefined simple scenes, for complete list, please see full help (-hf)\n", g_SceneConfigs.size() - 1);
    
	printf("    -a  Selects the rendering algorithm (default upbp_all):\n");
    for(int i = (int)Config::kVolumetricLightTracingFromUPBP; i < (int)Config::kAlgorithmMax; i++)
        printf("          %-10s  %s\n",
            Config::GetAcronym(Config::Algorithm(i)),
            Config::GetName(Config::Algorithm(i)));
	printf("          for complete list, please see full help (-hf)\n", g_SceneConfigs.size());

    printf("    -l  Maximum length of traced paths (default 10).\n");
	printf("    -t  Number of seconds to run the algorithm.\n");
    printf("    -i  Number of iterations to run the algorithm (default 1).\n");
    printf("    -o  User specified output name, with extension .bmp or .exr (default .exr). The name can be prefixed with relative or absolute path but the path must exists.\n");	
	printf("\n    Note: Time (-t) takes precedence over iterations (-i) if both are defined.\n"); 
	printf("\n    For more options, please see full help (-hf)\n");
}

/**
 * @brief	Splitting string by a given character.
 * 			
 * 			Based on http://stackoverflow.com/a/236803.
 *
 * @param	stringToSplit	The string to split.
 * @param	charToSplitBy	The character to split by.
 *
 * @return	A vector of parts of the given string.
 */
std::vector<std::string> splitByChar(const std::string &stringToSplit, char charToSplitBy)
{
	std::vector<std::string> elems;
	std::stringstream ss(stringToSplit);
	std::string item;
	while (std::getline(ss, item, charToSplitBy)) {
		elems.push_back(item);
	}
	return elems;
}

/**
 * @brief	Reports parsing error.
 * 			
 * 			Prints the given error message to standard error stream and exits.
 *
 * @param	message	The error message.
 */
void ReportParsingError(std::string message)
{
	std::cerr << "Error: " << message << std::endl;
	exit(2);
}

/**
 * @brief	Parses command line and sets up the \c Config according to it.
 *
 * @param	argc		   	Number of command line arguments.
 * @param	argv		   	The command line arguments.
 * @param [in,out]	oConfig	The configuration to set.
 */
void ParseCommandline(int argc, const char *argv[], Config &oConfig)
{
	// Setting defaults.

    oConfig.mScene = NULL;
    
	oConfig.mAlgorithm      = Config::kAlgorithmMax;
	oConfig.mAlgorithmFlags = 0;
    
	oConfig.mIterations = 1;
    oConfig.mMaxTime    = -1.f;
    oConfig.mOutputName = "";
    oConfig.mNumThreads = 0;
    oConfig.mBaseSeed   = 1234;
	oConfig.mResolution = Vec2i(256, 256);

    oConfig.mMaxPathLength  = 10;
    oConfig.mMinPathLength  = 0;	

	oConfig.mGridResolution = 256;
	oConfig.mMaxBeamsInCell = 0;
	oConfig.mReductionType  = 0;
	
	oConfig.mBeamDensType = BeamDensity::NONE;
	oConfig.mBeamDensMax  = 0;
	
	oConfig.mRefPathCountPerIter = -1;
	oConfig.mPathCountPerIter    = -1;	
    
	oConfig.mQueryBeamType	= LONG_BEAM;
	oConfig.mPhotonBeamType = SHORT_BEAM;
	
	oConfig.mSurfRadiusInitial = -0.0015f;
	oConfig.mSurfRadiusAlpha   = 0.75f;

	oConfig.mPP3DRadiusInitial = -0.001f;
    oConfig.mPP3DRadiusAlpha   = 1.0f;
	
	oConfig.mPB2DRadiusInitial     = -0.001f;
	oConfig.mPB2DRadiusAlpha       = 1.0f;
	oConfig.mPB2DRadiusCalculation = CONSTANT_RADIUS;
	oConfig.mPB2DRadiusKNN         = 0;	
	
	oConfig.mBB1DRadiusInitial         = -0.001f;
	oConfig.mBB1DRadiusAlpha           = 1.0f;
	oConfig.mBB1DRadiusCalculation     = CONSTANT_RADIUS; 
	oConfig.mBB1DRadiusKNN             = 0;	
	oConfig.mBB1DUsedLightSubPathCount = -1;
	oConfig.mBB1DBeamStorageFactor     = 0;

	oConfig.mContinuousOutput   = 0;
	oConfig.mEnvMapFilePath     = "";
	oConfig.mMaxMemoryPerThread = 500 * 1024 * 1024;
	oConfig.mMinDistToMed       = 0;
	oConfig.mShowTime           = false;

	oConfig.mIgnoreFullySpecPaths = false;

    int sceneID = 0;
	std::string sceneObjFile = "";
	
	std::ostringstream additionalArgs;
	
	// To deal with options priorities.
	bool r_alpha_surf_set = false;
	bool r_alpha_pp3d_set = false;
	bool r_alpha_pb2d_set = false;
	bool r_alpha_bb1d_set = false;
	bool r_init_surf_set = false;
	bool r_init_pp3d_set = false;
	bool r_init_pb2d_set = false;
	bool r_init_bb1d_set = false;
	bool r_init_pb2d_knn_set = false;
	bool r_init_bb1d_knn_set = false;
    
	// Debug images params.
	DebugImages::DebugOptions debugImagesOptions = DebugImages::NONE;
	DebugImages::WeightsOptions debugImagesWeightsOptions = DebugImages::DO_NOT_OUTPUT_WEIGHTS;
	DebugImages::MisWeights debugImagesMisWeights = DebugImages::MULTIPLY_BY_WEIGHTS;

	// Load arguments.
    for(int i=1; i<argc; i++)
    {
        std::string arg(argv[i]);

        // Print help string (at any position).
        if(arg == "-h" || arg == "--help" || arg == "/?")
        {
            PrintShortHelp(argv);
            return;
        }
		else if(arg == "-hf" || arg == "--help_full" || arg == "/??")
        {
            PrintHelp(argv);
            return;
        }

        if(arg[0] != '-') // all our commands start with -
        {
            continue;
        }

		// Basic options:
		
        else if(arg == "-s") // scene to load
        {
            if(++i == argc)	ReportParsingError("missing argument of -s option, please see help (-h)");

            std::istringstream iss(argv[i]);
            iss >> sceneID;

            if(iss.fail() || sceneID >= (int)g_SceneConfigs.size()) ReportParsingError("invalid argument of -s option, please see help (-h)");

			if (sceneID == -1) // try to load obj
			{
				if (++i == argc) ReportParsingError("missing file name argument of -s option, please see help (-h)");
		
				sceneObjFile = argv[i];
			}
        }
        else if(arg == "-a") // algorithm to use
        {
            if(++i == argc) ReportParsingError("missing argument of -a option, please see help (-h)");

            std::string alg(argv[i]);
			for (int i = 0; i < Config::kAlgorithmMax; i++)
			{
				if (alg == Config::GetAcronym(Config::Algorithm(i)))
					oConfig.mAlgorithm = Config::Algorithm(i);
			}

			if (oConfig.mAlgorithm == Config::kUPBPCustom)
				oConfig.mAlgorithm = Config::kAlgorithmMax;
			
			if (oConfig.mAlgorithm == Config::kUPBPAll)
				oConfig.mAlgorithmFlags |= BPT|SURF|PP3D|PB2D|BB1D;

			if (oConfig.mAlgorithm == Config::kAlgorithmMax && alg.size() >= 8 && alg.substr(0, 4) == "upbp" && (alg[4] == '+' || alg[4] == '_'))
			{
				bool valid = true;
				
				oConfig.mAlgorithmFlags = 0;

				if (alg[4] == '+')
					oConfig.mAlgorithmFlags |= BPT | SURF;

				std::vector<std::string> techniques = splitByChar(alg.substr(5), '+');

				for (std::vector<std::string>::const_iterator i = techniques.cbegin(); i != techniques.cend(); ++i)
				{
					if (*i == "bpt")
						oConfig.mAlgorithmFlags |= BPT;
					else if (*i == "surf")
						oConfig.mAlgorithmFlags |= SURF;
					else if (*i == "pp3d")
						oConfig.mAlgorithmFlags |= PP3D;
					else if (*i == "pb2d")
						oConfig.mAlgorithmFlags |= PB2D;
					else if (*i == "bb1d")
						oConfig.mAlgorithmFlags |= BB1D;
					else valid = false;
				}

				if (valid)
					oConfig.mAlgorithm = Config::kUPBPCustom;
			}

			if (oConfig.mAlgorithm == Config::kAlgorithmMax) ReportParsingError("invalid argument of -a option, please see help (-h)");
        }
		else if(arg == "-l") // maximum path length
        {
            if(++i == argc) ReportParsingError("missing argument of -l option, please see help (-h)");

            std::istringstream iss(argv[i]);
			iss >> oConfig.mMaxPathLength;

            if(iss.fail() || oConfig.mMaxPathLength < 1) ReportParsingError("invalid argument of -l option, please see help (-h)");
        }
        else if(arg == "-t") // number of seconds to run
        {
            if(++i == argc) ReportParsingError("missing argument of -t option, please see help (-h)");

            std::istringstream iss(argv[i]);
            iss >> oConfig.mMaxTime;

            if(iss.fail() || oConfig.mMaxTime < 0) ReportParsingError("invalid argument of -t option, please see help (-h)");

			additionalArgs << "_t" << argv[i];
        }
		else if(arg == "-i") // number of iterations to run
        {
            if(++i == argc) ReportParsingError("missing argument of -i option, please see help (-h)");

            std::istringstream iss(argv[i]);
            iss >> oConfig.mIterations;

            if(iss.fail() || oConfig.mIterations < 1) ReportParsingError("invalid argument of -i option, please see help (-h)");
        }
        else if(arg == "-o") // output name
        {
            if(++i == argc) ReportParsingError("missing argument of -o option, please see help (-h)");

            oConfig.mOutputName = argv[i];

            if(oConfig.mOutputName.length() == 0) ReportParsingError("invalid argument of -o option, please see help (-h)");
        }
		else if (arg == "-r") // resolution
		{
			if (++i == argc) ReportParsingError("missing argument of -r option, please see help (-hf)");

			int w = -1, h = -1;
			sscanf_s(argv[i], "%dx%d", &w, &h);
			if (w <= 0 || h <= 0) ReportParsingError("invalid argument of -r option, please see help (-hf)");
			oConfig.mResolution = Vec2i(w, h);

			additionalArgs << "_r" << argv[i];
		}
		else if (arg == "-seed")
		{
			if (++i == argc) ReportParsingError("missing argument of -seed option, please see help (-hf)");

			std::istringstream iss(argv[i]);
			iss >> oConfig.mBaseSeed;

			if (iss.fail() || oConfig.mBaseSeed < 0) ReportParsingError("invalid argument of -seed option, please see help (-hf)");
		}

		// Performance options:

		else if(arg == "-th") // threads count
		{
            if(++i == argc) ReportParsingError("missing argument of -th option, please see help (-hf)");

            std::istringstream iss(argv[i]);
			iss >> oConfig.mNumThreads;

            if(iss.fail() || oConfig.mNumThreads < 0) ReportParsingError("invalid argument of -th option, please see help (-hf)");

			additionalArgs << "_th" << argv[i];
		}
		else if (arg == "-maxMemPerThread") // maximum memory used by a thread
		{
			if (++i == argc) ReportParsingError("missing argument of -maxMemPerThread option, please see help (-hf)");

			std::istringstream iss(argv[i]);
			iss >> oConfig.mMaxMemoryPerThread;
			oConfig.mMaxMemoryPerThread *= 1024 * 1024;
			if (iss.fail() || oConfig.mMaxMemoryPerThread <= 0) ReportParsingError("invalid argument of -maxMemPerThread option, please see help (-hf)");
		}

		// Radius options:
		
		else if (arg == "-r_alpha") // radius reduction factor
		{
			if (++i == argc) ReportParsingError("missing argument of -r_alpha option, please see help (-hf)");

			float alpha;
			sscanf_s(argv[i], "%f", &alpha);
			if (alpha <= 0.0f) ReportParsingError("invalid argument of -r_alpha option, please see help (-hf)");

			if (!r_alpha_surf_set)
				oConfig.mSurfRadiusAlpha = alpha;
			if (!r_alpha_pp3d_set)
				oConfig.mPP3DRadiusAlpha = alpha;
			if (!r_alpha_pb2d_set)
				oConfig.mPB2DRadiusAlpha = alpha;
			if (!r_alpha_bb1d_set)
				oConfig.mBB1DRadiusAlpha = alpha;

			additionalArgs << "_ralpha" << argv[i];
		}
		else if (arg == "-r_alpha_surf") // radius reduction factor for surface photon mapping
		{
			if (++i == argc) ReportParsingError("missing argument of -r_alpha_surf option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mSurfRadiusAlpha);
			if (oConfig.mSurfRadiusAlpha <= 0.0f) ReportParsingError("invalid argument of -r_alpha_surf option, please see help (-hf)");

			r_alpha_surf_set = true;
			additionalArgs << "_ralphasurf" << argv[i];
		}
		else if (arg == "-r_alpha_pp3d") // radius reduction factor for PP3D
		{
			if (++i == argc) ReportParsingError("missing argument of -r_alpha_pp3d option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mPP3DRadiusAlpha);
			if (oConfig.mPP3DRadiusAlpha <= 0.0f) ReportParsingError("invalid argument of -r_alpha_pp3d option, please see help (-hf)");

			r_alpha_pp3d_set = true;
			additionalArgs << "_ralphapp3d" << argv[i];
		}
		else if (arg == "-r_alpha_pb2d") // radius reduction factor for PB2D
		{
			if (++i == argc) ReportParsingError("missing argument of -r_alpha_pb2d option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mPB2DRadiusAlpha);
			if (oConfig.mPB2DRadiusAlpha <= 0.0f) ReportParsingError("invalid argument of -r_alpha_pb2d option, please see help (-hf)");

			r_alpha_pb2d_set = true;
			additionalArgs << "_ralphapb2d" << argv[i];
		}
		else if (arg == "-r_alpha_bb1d") // radius reduction factor for BB1D
		{
			if (++i == argc) ReportParsingError("missing argument of -r_alpha_bb1d option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mBB1DRadiusAlpha);
			if (oConfig.mBB1DRadiusAlpha <= 0.0f) ReportParsingError("invalid argument of -r_alpha_bb1d option, please see help (-hf)");

			r_alpha_bb1d_set = true;
			additionalArgs << "_ralphabb1d" << argv[i];
		}
		else if (arg == "-r_initial") // initial radius 
		{
			if (++i == argc) ReportParsingError("missing argument of -r_initial option, please see help (-hf)");

			float init;
			sscanf_s(argv[i], "%f", &init);
			if (init == 0.0f) ReportParsingError("invalid argument of -r_initial option, please see help (-hf)");

			if (!r_init_surf_set)
				oConfig.mSurfRadiusInitial = init;				
			if (!r_init_pp3d_set)
				oConfig.mPP3DRadiusInitial = init;
			if (!r_init_pb2d_set && !r_init_pb2d_knn_set)
			{
				oConfig.mPB2DRadiusInitial = init;
				oConfig.mPB2DRadiusCalculation = CONSTANT_RADIUS;
			}
			if (!r_init_bb1d_set && !r_init_bb1d_knn_set)
			{
				oConfig.mBB1DRadiusInitial = init;
				oConfig.mBB1DRadiusCalculation = CONSTANT_RADIUS;
			}

			additionalArgs << "_rinit" << argv[i];
		}
		else if (arg == "-r_initial_surf") // initial radius for surface photon mapping
		{
			if (++i == argc) ReportParsingError("missing argument of -r_initial_surf option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mSurfRadiusInitial);
			if (oConfig.mSurfRadiusInitial == 0.0f) ReportParsingError("invalid argument of -r_initial_surf option, please see help (-hf)");

			r_init_surf_set = true;
			additionalArgs << "_rinitsurf" << argv[i];
		}
		else if (arg == "-r_initial_pp3d") // initial radius for PP3D
		{
			if (++i == argc) ReportParsingError("missing argument of -r_initial_pp3d option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mPP3DRadiusInitial);
			if (oConfig.mPP3DRadiusInitial == 0.0f) ReportParsingError("invalid argument of -r_initial_pp3d option, please see help (-hf)");

			r_init_pp3d_set = true;
			additionalArgs << "_rinitpp3d" << argv[i];
		}
		else if (arg == "-r_initial_pb2d") // initial radius for PB2D
		{
			if (++i == argc) ReportParsingError("missing argument of -r_initial_pb2d option, please see help (-hf)");

			float init;
			sscanf_s(argv[i], "%f", &init);
			if (init == 0.0f) ReportParsingError("invalid argument of -r_initial_pb2d option, please see help (-hf)");

			if (!r_init_pb2d_knn_set)
			{
				oConfig.mPB2DRadiusInitial = init;
				oConfig.mPB2DRadiusCalculation = CONSTANT_RADIUS;
				r_init_pb2d_set = true;
			}

			additionalArgs << "_rinitpb2d" << argv[i];
		}
		else if (arg == "-r_initial_bb1d") // initial radius for BB1D
		{
			if (++i == argc) ReportParsingError("missing argument of -r_initial_bb1d option, please see help (-hf)");

			float init;
			sscanf_s(argv[i], "%f", &init);
			if (init == 0.0f) ReportParsingError("invalid argument of -r_initial_bb1d option, please see help (-hf)");

			if (!r_init_bb1d_knn_set)
			{
				oConfig.mBB1DRadiusInitial = init;
				oConfig.mBB1DRadiusCalculation = CONSTANT_RADIUS;
				r_init_bb1d_set = true;
			}

			additionalArgs << "_rinitbb1d" << argv[i];
		}
		else if (arg == "-r_initial_pb2d_knn") // initial radius for PB2D based on k-th nearest photon
		{
			if (++i == argc) ReportParsingError("missing first argument of -r_initial_pb2d_knn option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mPB2DRadiusInitial);
			if (oConfig.mPB2DRadiusInitial <= 0.0f) ReportParsingError("invalid first argument of -r_initial_pb2d_knn option, please see help (-hf)");

			if (++i == argc) ReportParsingError("missing second argument of -r_initial_pb2d_knn option, please see help (-hf)");

			sscanf_s(argv[i], "%d", &oConfig.mPB2DRadiusKNN);
			if (oConfig.mPB2DRadiusKNN <= 0) ReportParsingError("invalid second argument of -r_initial_pb2d_knn option, please see help (-hf)");

			oConfig.mPB2DRadiusCalculation = KNN_RADIUS;

			r_init_pb2d_knn_set = true;
			additionalArgs << "_rinitpb2dknn" << argv[i - 1] << "_" << argv[i];
		}
		else if (arg == "-r_initial_bb1d_knn") // initial radius for BB1D based on k-th nearest beam vertex
		{
			if (++i == argc) ReportParsingError("missing first argument of -r_initial_bb1d_knn option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mBB1DRadiusInitial);
			if (oConfig.mBB1DRadiusInitial <= 0.0f) ReportParsingError("invalid first argument of -r_initial_bb1d_knn option, please see help (-hf)");

			if (++i == argc) ReportParsingError("missing second argument of -r_initial_bb1d_knn option, please see help (-hf)");

			sscanf_s(argv[i], "%d", &oConfig.mBB1DRadiusKNN);
			if (oConfig.mBB1DRadiusKNN <= 0) ReportParsingError("invalid second argument of -r_initial_bb1d_knn option, please see help (-hf)");

			oConfig.mBB1DRadiusCalculation = KNN_RADIUS;

			r_init_bb1d_knn_set = true;
			additionalArgs << "_rinitbb1dknn" << argv[i - 1] << "_" << argv[i];
		}

		// Light transport options:
		
		else if (arg == "-previous") // previous mode
		{
			oConfig.mAlgorithmFlags |= PREVIOUS;

			additionalArgs << "_prev";
		}
		else if (arg == "-previous_bb1d") // previous mode for BB1D
		{
			oConfig.mAlgorithmFlags |= BB1D_PREVIOUS;

			additionalArgs << "_prevbb1d";
		}
		else if (arg == "-compatible") // compatible mode
		{
			oConfig.mAlgorithmFlags |= COMPATIBLE;

			additionalArgs << "_comp";
		}		
		else if (arg == "-speconly") // only specular paths
		{
			oConfig.mAlgorithmFlags |= SPECULAR_ONLY;

			additionalArgs << "_speconly";
		}
		else if (arg == "-ignorespec") // upbp will ignore fully specular paths
		{
			if (++i == argc) ReportParsingError("missing argument of -ignorespec option, please see help (-hf)");

			std::string option(argv[i]);
			if (option == "1")
			{
				oConfig.mIgnoreFullySpecPaths = true;
			}
			else if (option == "0")
			{
				oConfig.mIgnoreFullySpecPaths = false;
			}
			else ReportParsingError("invalid argument of -ignorespec option, please see help (-hf)");
		}

		// Beams options:

		else if (arg == "-gridres") // resolution of grid for photon beams
		{
			if (++i == argc) ReportParsingError("missing argument of -gridres option, please see help (-hf)");

			std::istringstream iss(argv[i]);
			iss >> oConfig.mGridResolution;

			if (iss.fail() || oConfig.mGridResolution < 1) ReportParsingError("invalid argument of -gridres option, please see help (-hf)");
		}
		else if (arg == "-gridmax") // maximum number of beams in a single grid cell
		{
			if (++i == argc) ReportParsingError("missing argument of -gridmax option, please see help (-hf)");

			std::istringstream iss(argv[i]);
			iss >> oConfig.mMaxBeamsInCell;

			if (iss.fail()) ReportParsingError("invalid argument of -gridmax option, please see help (-hf)");

			additionalArgs << "_gridmax" << argv[i];
		}
		else if (arg == "-gridred") // type of reduction of beams in grid
		{
			if (++i == argc) ReportParsingError("missing argument of -gridred option, please see help (-hf)");

			std::istringstream iss(argv[i]);
			iss >> oConfig.mReductionType;

			if (iss.fail() || oConfig.mReductionType > 3) ReportParsingError("invalid argument of -gridred option, please see help (-hf)");

			additionalArgs << "_gridred" << argv[i];
		}
		else if (arg == "-beamdens") // type of accumulated beam statistics
		{
			if (++i == argc) ReportParsingError("missing first argument of -beamdens option, please see help (-hf)");

			uint type;
			std::istringstream iss(argv[i]);
			iss >> type;

			if (iss.fail() || type > 5) ReportParsingError("invalid first argument of -beamdens option, please see help (-hf)");

			oConfig.mBeamDensType = static_cast<BeamDensity::ImgType>(type);
			
			if (++i == argc) ReportParsingError("missing second argument of -beamdens option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mBeamDensMax);

			if (iss.fail()) ReportParsingError("invalid second argument of -beamdens option, please see help (-hf)");

			additionalArgs << "_beamdens" << type << "-" << argv[i];
		}
		else if (arg == "-beamstore") // factor for decision which media will use beams
		{
			if (++i == argc) ReportParsingError("missing argument of -beamstore option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mBB1DBeamStorageFactor);

			if (oConfig.mBB1DBeamStorageFactor < 0) ReportParsingError("invalid argument of -beamstore option, please see help (-hf)");

			additionalArgs << "_beamstore" << argv[i];
		}
		else if (arg == "-qbt") // query beam type
		{
			if (++i == argc) ReportParsingError("missing argument of -qbt option, please see help (-hf)");

			char a[2];
			sscanf_s(argv[i], "%s", &a,2);
			if (a[0] == 'L' || a[0] == 'l')
				oConfig.mQueryBeamType = LONG_BEAM;
			else
				oConfig.mQueryBeamType = SHORT_BEAM;
		}
		else if (arg == "-pbt") // photon beam type
		{
			if (++i == argc) ReportParsingError("missing argument of -pbt option, please see help (-hf)");

			char a[2];
			sscanf_s(argv[i], "%s", &a, 2);
			if (a[0] == 'L' || a[0] == 'l')
				oConfig.mPhotonBeamType = LONG_BEAM;
			else
				oConfig.mPhotonBeamType = SHORT_BEAM;
		}
		else if (arg == "-pbc") // paths with beams count
		{
			if (++i == argc) ReportParsingError("missing argument of -pbc option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mBB1DUsedLightSubPathCount);
			if (oConfig.mBB1DUsedLightSubPathCount == 0.0f) ReportParsingError("invalid argument of -pbc option, please see help (-hf)");

			additionalArgs << "_pbc" << argv[i];
		}
		else if (arg == "-nosin") // no sine in BB1D MIS weights
		{
			oConfig.mAlgorithmFlags |= NO_SINE_IN_WEIGHTS;

			additionalArgs << "_nosin";
		}

		// Debug options:

		else if (arg == "-debugimg_option") // type of debug images
		{
			if (++i == argc) ReportParsingError("missing argument of -debugimg_option option, please see help (-hf)");

			std::string option(argv[i]);
			if (option == "none")
			{
				debugImagesOptions = DebugImages::NONE;
			}
			else if (option == "simple_pyramid")
			{
				debugImagesOptions = DebugImages::SIMPLE_PYRAMID;
			}
			else if (option == "per_technique")
			{
				debugImagesOptions = DebugImages::PER_TECHNIQUE;
			}
			else if (option == "pyramid_per_technique")
			{
				debugImagesOptions = DebugImages::PYRAMID_PER_TECHNIQUE;
			}
			else ReportParsingError("invalid argument of -debugimg_option option, please see help (-hf)");
		}
		else if (arg == "-debugimg_multbyweight") // whether the debug images are weighted
		{
			if (++i == argc) ReportParsingError("missing argument of -debugimg_multbyweight option, please see help (-hf)");

			std::string option(argv[i]);
			if (option == "yes")
			{
				debugImagesMisWeights = DebugImages::MULTIPLY_BY_WEIGHTS;
			}
			else if (option == "no")
			{
				debugImagesMisWeights = DebugImages::IGNORE_WEIGHTS;
			}
			else if (option == "output_both")
			{
				debugImagesMisWeights = DebugImages::OUTPUT_BOTH_VERSIONS;
			}
			else ReportParsingError("invalid argument of -debugimg_multbyweight option, please see help (-hf)");
		}
		else if (arg == "-debugimg_output_weights") // whether to output weights
		{
			if (++i == argc) ReportParsingError("missing argument of -debugimg_output_weights option, please see help (-hf)");

			std::string option(argv[i]);
			if (option == "1")
			{
				debugImagesWeightsOptions = DebugImages::OUTPUT_WEIGHTS;
			}
			else if (option == "0")
			{
				debugImagesWeightsOptions = DebugImages::DO_NOT_OUTPUT_WEIGHTS;
			}
			else ReportParsingError("invalid argument of -debugimg_output_weights option, please see help (-hf)");
		}

		// Other options:

		else if (arg == "-continuous_output") // output image each x iterations
		{
			if (++i == argc) ReportParsingError("missing argument of -continuous_output option, please see help (-hf)");

			std::istringstream iss(argv[i]);
			iss >> oConfig.mContinuousOutput;

			if (iss.fail()) ReportParsingError("invalid argument of -continuous_output option, please see help (-hf)");
		}
		else if(arg == "-em") // path of an environment map
        {
            if(++i == argc) ReportParsingError("missing argument of -em option, please see help (-hf)");

            oConfig.mEnvMapFilePath = argv[i];

            if(oConfig.mEnvMapFilePath.length() == 0) ReportParsingError("invalid argument of -em option, please see help (-hf)");

			size_t lastSlashPos = oConfig.mEnvMapFilePath.find_last_of("\\/");
			size_t lastDotPos = oConfig.mEnvMapFilePath.find_last_of('.');
			additionalArgs << "_em-" << oConfig.mEnvMapFilePath.substr(lastSlashPos + 1, lastDotPos - lastSlashPos - 1);
        }
		else if (arg == "-min_dist2med") // minimum distance from camera to medium
		{
			if (++i == argc) ReportParsingError("missing argument of -min_dist2med option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mMinDistToMed);

			additionalArgs << "_mindist2med" << argv[i];
		}
		else if (arg == "-rpcpi") // reference path count per iteration
		{
			if (++i == argc) ReportParsingError("missing argument of -rpcpi option, please see help (-hf");

			sscanf_s(argv[i], "%f", &oConfig.mRefPathCountPerIter);
			if (std::floor(oConfig.mRefPathCountPerIter) == 0.0f) ReportParsingError("invalid argument of -rpcpi option, please see help (-hf)");

			additionalArgs << "_rpcpi" << argv[i];
		}
		else if (arg == "-pcpi") // path count per iteration
		{
			if (++i == argc) ReportParsingError("missing argument of -pcpi option, please see help (-hf)");

			sscanf_s(argv[i], "%f", &oConfig.mPathCountPerIter);
			if (std::floor(oConfig.mPathCountPerIter) == 0.0f) ReportParsingError("invalid argument of -pcpi option, please see help (-hf)");

			additionalArgs << "_pcpi" << argv[i];
		}
		else if (arg == "-sn") // surface normals
		{
			if (++i == argc) ReportParsingError("missing argument of -sn option, please see help (-hf)");

			int a;
			sscanf_s(argv[i], "%d", &a);
			if (a == 0)
				AbstractGeometry::setUseShadingNormal(false);
			else
				AbstractGeometry::setUseShadingNormal(true);

			additionalArgs << "_sn" << argv[i];
		}
		else if (arg == "-time")
		{
			oConfig.mShowTime = true;
		}
	}

	oConfig.mAdditionalArgs = additionalArgs.str();

	// Computing path counts if specified negative, i.e. relative.
	if (oConfig.mRefPathCountPerIter < 0)
		oConfig.mRefPathCountPerIter = std::floor(-oConfig.mRefPathCountPerIter * oConfig.mResolution.x * oConfig.mResolution.y);
	else
		oConfig.mRefPathCountPerIter = std::floor(oConfig.mRefPathCountPerIter);
	if (oConfig.mPathCountPerIter < 0)
		oConfig.mPathCountPerIter = std::floor(-oConfig.mPathCountPerIter * oConfig.mResolution.x * oConfig.mResolution.y);
	else
		oConfig.mPathCountPerIter = std::floor(oConfig.mPathCountPerIter);

    // Check algorithm was selected.
    if(oConfig.mAlgorithm == Config::kAlgorithmMax)
    {
        oConfig.mAlgorithm = Config::kUPBPAll;
		oConfig.mAlgorithmFlags |= BPT|SURF|PP3D|PB2D|BB1D;
    }

    // Load scene.
    Scene *scene = new Scene;
	if (sceneID > -1)
		scene->LoadCornellBox(oConfig.mResolution, g_SceneConfigs[sceneID]);
	else
		scene->LoadFromObj(sceneObjFile.c_str(), oConfig.mResolution);
    scene->BuildSceneSphere();

	// Set environment map.
	if (oConfig.mEnvMapFilePath.length() > 0 && scene->mBackground)
	{
		delete(scene->mBackground->mEnvMap);
		scene->mBackground->mEnvMap = new EnvMap(oConfig.mEnvMapFilePath, 0.0f, 1);
	}

    oConfig.mScene = scene;

    // If no output name is chosen, create a default one.
    if(oConfig.mOutputName.length() == 0)
    {
		if (sceneID > -1)
			oConfig.mOutputName = DefaultFilename(sceneID, oConfig);
		else
			oConfig.mOutputName = DefaultFilename(sceneObjFile, oConfig);
    }

    // Check if output name has valid extension (.bmp or .exr) and if not add .exr
    std::string extension = "";

    if(oConfig.mOutputName.length() > 4) // must be at least 1 character before .exr
        extension = oConfig.mOutputName.substr(
            oConfig.mOutputName.length() - 4, 4);

    if(extension != ".bmp" && extension != ".exr")
        oConfig.mOutputName += ".exr";

	oConfig.mDebugImages.Setup(oConfig.mMaxPathLength, oConfig.mResolution, debugImagesOptions, debugImagesWeightsOptions, debugImagesMisWeights);
	
	// Grid parameters.
	PhotonBeamsEvaluator::sGridSize = oConfig.mGridResolution;
	PhotonBeamsEvaluator::sMaxBeamsInCell = oConfig.mMaxBeamsInCell;
	PhotonBeamsEvaluator::sReductionType = oConfig.mReductionType;
}

#endif  //__CONFIG_HXX__