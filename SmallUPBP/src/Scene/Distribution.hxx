/*
    pbrt source code Copyright(c) 1998-2012 Matt Pharr and Greg Humphreys.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#ifndef __DISTRIBUTION_HXX__
#define __DISTRIBUTION_HXX__

#include <algorithm>
#include <vector>

#include "..\Misc\Utils2.hxx"

struct Distribution1D 
{
public:
    Distribution1D(const float *f, int n) 
	{
        count = n;
        func = new float[n];
        memcpy(func, f, n*sizeof(float));
        cdf = new float[n+1];
        
		// Compute integral of step function at $x_i$
        cdf[0] = 0.;
        for (int i = 1; i < count+1; ++i)
            cdf[i] = cdf[i-1] + func[i-1] / n;

        // Transform step function integral into CDF
        funcInt = cdf[count];
        if (funcInt == 0.f) 
		{
            for (int i = 1; i < n+1; ++i)
                cdf[i] = float(i) / float(n);
        }
        else 
		{
            for (int i = 1; i < n+1; ++i)
                cdf[i] /= funcInt;
        }
    }

    ~Distribution1D()
	{
        delete[] func;
        delete[] cdf;
    }

    float SampleContinuous(float u, float *pdf, int *off = NULL) const 
	{
        // Find surrounding CDF segments and _offset_
        float *ptr = std::upper_bound(cdf, cdf+count+1, u);
        int offset = Utils::clamp<int>(int(ptr-cdf-1), 0, count - 1);
        if (off) *off = offset;
        UPBP_ASSERT(offset < count);
        UPBP_ASSERT(u >= cdf[offset] && (u < cdf[offset+1] || u == 1));

		// Fix the case when func ends with zeros
		if (cdf[offset] == cdf[offset + 1])
		{
			UPBP_ASSERT(u == 1.0f);

			do { offset--; }
			while (cdf[offset] == cdf[offset + 1] && offset > 0);

			UPBP_ASSERT(cdf[offset] != cdf[offset + 1]);
		}

        // Compute offset along CDF segment
        float du = (u - cdf[offset]) / (cdf[offset+1] - cdf[offset]);
        UPBP_ASSERT(!Float::isNan(du));

        // Compute PDF for sampled offset
        if (pdf) *pdf = func[offset] / funcInt;
		UPBP_ASSERT(func[offset] > 0);

        // Return $x\in{}[0,1]$ corresponding to sample
        return (offset + du) / count;
    }

    int SampleDiscrete(float u, float *pdf) const
	{
        // Find surrounding CDF segments and _offset_
        float *ptr = std::upper_bound(cdf, cdf+count+1, u);
        int offset = std::max(0, int(ptr-cdf-1));
        UPBP_ASSERT(offset < count);
        UPBP_ASSERT(u >= cdf[offset] && u < cdf[offset+1]);
        if (pdf) *pdf = func[offset] / (funcInt * count);
        return offset;
    }

private:
    friend struct Distribution2D;
    float *func, *cdf;
    float funcInt;
    int count;
};

struct Distribution2D
{
public:
	Distribution2D(const float *func, int nu, int nv)
	{
		pConditionalV.reserve(nv);
		for (int v = 0; v < nv; ++v) 
		{
			// Compute conditional sampling distribution for $\tilde{v}$
			pConditionalV.push_back(new Distribution1D(&func[v*nu], nu));
		}

		// Compute marginal sampling distribution $p[\tilde{v}]$
		std::vector<float> marginalFunc;
		marginalFunc.reserve(nv);
		for (int v = 0; v < nv; ++v)
			marginalFunc.push_back(pConditionalV[v]->funcInt);
		pMarginal = new Distribution1D(&marginalFunc[0], nv);
	}

	~Distribution2D()
	{
		delete pMarginal;
		for (uint32_t i = 0; i < pConditionalV.size(); ++i)
			delete pConditionalV[i];
	}

	void SampleContinuous(float u0, float u1, float uv[2], float *pdf) const
	{
		float pdfs[2];
		int v;
		uv[1] = pMarginal->SampleContinuous(u1, &pdfs[1], &v);
		uv[0] = pConditionalV[v]->SampleContinuous(u0, &pdfs[0]);
		*pdf = pdfs[0] * pdfs[1];
	}

	float Pdf(float u, float v) const
	{
		int iu = Utils::clamp<int>((int)(u * pConditionalV[0]->count), 0,
			pConditionalV[0]->count - 1);
		int iv = Utils::clamp<int>((int)(v * pMarginal->count), 0,
			pMarginal->count - 1);
		if (pConditionalV[iv]->funcInt * pMarginal->funcInt == 0.f) return 0.f;
		return (pConditionalV[iv]->func[iu] * pMarginal->func[iv]) /
			(pConditionalV[iv]->funcInt * pMarginal->funcInt);
	}

private:
	std::vector<Distribution1D *> pConditionalV;
	Distribution1D *pMarginal;
};

#endif //__DISTRIBUTION_HXX__