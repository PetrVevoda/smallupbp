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

#pragma warning(disable: 4482)

#include "Bre\EmbreeAcc.hxx"
#include "Misc\Config.hxx"

// Output image in continuous outputting
void continuousOutput(const Config &aConfig, int iter, Framebuffer & accumFrameBuffer, Framebuffer & outputFrameBuffer, AbstractRenderer* renderer, const std::string & name, const std::string & ext, char * filename)
{
	if (aConfig.mContinuousOutput > 0)
	{
		accumFrameBuffer.Add(renderer->GetFramebufferUnscaled());
		renderer->GetFramebufferUnscaled().Clear();
		if (iter % aConfig.mContinuousOutput == 0)
		{
			outputFrameBuffer.Clear();
			outputFrameBuffer.AddScaled(accumFrameBuffer, 1.0f / iter);

			
			sprintf_s(filename,1024,"%s-%d.%s", name.c_str(), iter, ext.c_str());
			//// Saves the image
			outputFrameBuffer.Save(filename);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// The main rendering function, renders what is in aConfig

float render(
    const Config &aConfig,
    int *oUsedIterations = NULL)
{
	// Don't use more threads than iterations (in case of rendering limited by number of iterations not time)
	int usedThreads = aConfig.mNumThreads;
	if (aConfig.mMaxTime <= 0) usedThreads = std::min(usedThreads, aConfig.mIterations); 
	
	// Set number of used threads
    omp_set_num_threads(usedThreads);

    // Create 1 renderer per thread
    typedef AbstractRenderer* AbstractRendererPtr;
    AbstractRendererPtr *renderers;
    renderers = new AbstractRendererPtr[usedThreads];

    for(int i=0; i<usedThreads; i++)
    {
        renderers[i] = CreateRenderer(aConfig, aConfig.mBaseSeed + i, aConfig.mBaseSeed);

        renderers[i]->mMaxPathLength = aConfig.mMaxPathLength;
        renderers[i]->mMinPathLength = aConfig.mMinPathLength;
		renderers[i]->SetupDebugImages(aConfig.mDebugImages);
		renderers[i]->SetupBeamDensity(aConfig.mBeamDensType, aConfig.mScene->mCamera.mResolution, aConfig.mBeamDensMax);
    }

    clock_t startT = clock();
    int iter = 0;

	Framebuffer accumFrameBuffer, outputFrameBuffer;
	accumFrameBuffer.Setup(aConfig.mResolution);
	outputFrameBuffer.Setup(aConfig.mResolution);
	std::string name = aConfig.mOutputName.substr(0, aConfig.mOutputName.length() - 4);
	std::string ext = aConfig.mOutputName.substr(aConfig.mOutputName.length() - 3, 3);
	char filename[1024]; // Must be shared, otherwise critical section fails
    
	// Rendering loop, when we have any time limit, use time-based loop,
    // otherwise go with required iterations
    if(aConfig.mMaxTime > 0)
    {
        // Time based loop
#pragma omp parallel shared(iter,accumFrameBuffer,outputFrameBuffer,name,ext,filename)
        while(clock() < startT + aConfig.mMaxTime*CLOCKS_PER_SEC)
        {
            int threadId = omp_get_thread_num();
			renderers[threadId]->RunIteration(iter);

#pragma omp critical
			{
				iter++; // counts number of iterations
				continuousOutput(aConfig, iter, accumFrameBuffer, outputFrameBuffer, renderers[threadId], name, ext, filename);
			}
        }
    }
    else
    {
        // Iterations based loop
		int cnt = 0, p = -1;
#pragma omp parallel for shared(cnt,p,accumFrameBuffer,outputFrameBuffer,name,ext,filename)
        for(iter=0; iter < aConfig.mIterations; iter++)
        {
            int threadId = omp_get_thread_num();
			renderers[threadId]->RunIteration(iter);
#pragma omp critical
			{
				++cnt;
				int percent = (int)(((float)cnt / aConfig.mIterations)*100.0f);
				if (percent != p)
				{
					p = percent;
					std::cout << percent << "%" << std::endl;
				}
				continuousOutput(aConfig, cnt, accumFrameBuffer, outputFrameBuffer, renderers[threadId], name, ext,filename);
			}
        }
		iter = aConfig.mIterations;
    }

    clock_t endT = clock();

    if(oUsedIterations)
        *oUsedIterations = iter+1;

	int usedRenderers = 0;

	aConfig.mCameraTracingTime = 0;
	aConfig.mBeamDensity.Setup(aConfig.mBeamDensType, aConfig.mScene->mCamera.mResolution, aConfig.mBeamDensMax);

    // Not all created renderers had to have been used.
    // Those must not participate in accumulation.
    for(int i=0; i<usedThreads; i++)
    {
        if(!renderers[i]->WasUsed())
            continue;

		if (aConfig.mContinuousOutput <= 0)
		{
			if (usedRenderers == 0)
			{
				renderers[i]->GetFramebuffer(*aConfig.mFramebuffer);
			}
			else
			{
				Framebuffer tmp;
				renderers[i]->GetFramebuffer(tmp);
				aConfig.mFramebuffer->Add(tmp);
			}
		}

		renderers[i]->AccumulateDebugImages(aConfig.mDebugImages);
		renderers[i]->AccumulateBeamDensity(aConfig.mBeamDensity);

		aConfig.mCameraTracingTime += renderers[i]->mCameraTracingTime;

        usedRenderers++;
    }
	
	if (aConfig.mContinuousOutput <= 0)
	{
		// Scale framebuffer by the number of used renderers
		aConfig.mFramebuffer->Scale(1.f / usedRenderers);
	}
	else
	{
		*aConfig.mFramebuffer = accumFrameBuffer;
		aConfig.mFramebuffer->Scale(1.f / iter);
	}

	aConfig.mCameraTracingTime /= iter;

    // Clean up renderers
    for(int i=0; i<usedThreads; i++)
        delete renderers[i];

    delete [] renderers;

    return float(endT - startT) / CLOCKS_PER_SEC;
}

//////////////////////////////////////////////////////////////////////////
// Main

int main(int argc, const char *argv[])
{
	try
	{
		// Warns when not using C++11 Mersenne Twister
		PrintRngWarning();

		EmbreeAcc::initLib();
		
		// Setups config based on command line
		Config config;
		ParseCommandline(argc, argv, config);

		// If number of threads is invalid, set 1 thread per processor
		if (config.mNumThreads <= 0)
			config.mNumThreads = std::max(1, omp_get_num_procs());

		// When some error has been encountered, exits
		if (config.mScene == NULL)
			return 1;

		// Sets up framebuffer
		Framebuffer fbuffer;
		config.mFramebuffer = &fbuffer;

		// Prints what we are doing
		printf("Scene:    %s\n", config.mScene->mSceneName.c_str());
		if (config.mMaxTime > 0)
			printf("Target:   %g seconds render time\n", config.mMaxTime);
		else
			printf("Target:   %d iteration(s)\n", config.mIterations);

		// Renders the image
		std::string desc = GetDescription(config, "            ");
		printf("Running:  %s", desc.c_str());
		fflush(stdout);
		int iterations;
		float time = render(config, &iterations);
		EmbreeAcc::cleanupLib();
		printf("done in %.2f s (%i iterations)\n", time, (iterations - 1));
		if (config.mCameraTracingTime) printf("avg camera time %.2f s\n", config.mCameraTracingTime);

		std::string extension = config.mOutputName.substr(config.mOutputName.length() - 3, 3);

		if (config.mShowTime || config.mIterations <= 0)
		{
			std::ostringstream modifiedOutputName;

			modifiedOutputName << config.mOutputName.substr(0, config.mOutputName.length() - 4);
			if (config.mIterations <= 0)
			{
				modifiedOutputName << "_i" << iterations - 1;
			}
			if (config.mShowTime)
			{
				modifiedOutputName.precision(2);
				modifiedOutputName << "_time" << std::fixed << time;
			}
			modifiedOutputName << "." << extension;

			config.mOutputName = modifiedOutputName.str();
		}

		// Saves the image
		fbuffer.Save(config.mOutputName, 2.2f /*gamma*/);

		std::string name = config.mOutputName.substr(0, config.mOutputName.length() - 4);
		config.mDebugImages.Output(name, extension);
		config.mBeamDensity.Output(name, extension);

		// Scene cleanup
		delete config.mScene;

		return 0;
	}
	catch (...)
	{
		std::cerr << "Error: unknown error" << std::endl;
		exit(2);
	}
}
