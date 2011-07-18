/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2009 Soeren Sonnenburg
 * Copyright (C) 2009 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include <shogun/features/DotFeatures.h>
#include <shogun/io/SGIO.h>
#include <shogun/lib/Signal.h>
#include <shogun/base/Parallel.h>
#include <shogun/base/Parameter.h>

#ifndef WIN32
#include <pthread.h>
#endif

using namespace shogun;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct DF_THREAD_PARAM
{
	CDotFeatures* df;
	int32_t* sub_index;
	float64_t* output;
	int32_t start;
	int32_t stop;
	float64_t* alphas;
	float64_t* vec;
	int32_t dim;
	float64_t bias;
	bool progress;
};
#endif // DOXYGEN_SHOULD_SKIP_THIS


CDotFeatures::CDotFeatures(int32_t size)
	:CFeatures(size), combined_weight(1.0)
{
	init();
	set_property(FP_DOT);
}


CDotFeatures::CDotFeatures(const CDotFeatures & orig)
	:CFeatures(orig), combined_weight(orig.combined_weight)
{
	init();
}


CDotFeatures::CDotFeatures(CFile* loader)
	:CFeatures(loader)
{
	init();
}

void
CDotFeatures::init(void)
{
	m_parameters->add(&combined_weight, "combined_weight",
					  "Feature weighting in combined dot features.");
}

void CDotFeatures::dense_dot_range(float64_t* output, int32_t start, int32_t stop, float64_t* alphas, float64_t* vec, int32_t dim, float64_t b)
{
	ASSERT(output);
	// write access is internally between output[start..stop] so the following
	// line is necessary to write to output[0...(stop-start-1)]
	output-=start; 
	ASSERT(start>=0);
	ASSERT(start<stop);
	ASSERT(stop<=get_num_vectors());

	int32_t num_vectors=stop-start;
	ASSERT(num_vectors>0);

	int32_t num_threads=parallel->get_num_threads();
	ASSERT(num_threads>0);

	CSignal::clear_cancel();

#ifndef WIN32
	if (num_threads < 2)
	{
#endif
		DF_THREAD_PARAM params;
		params.df=this;
		params.sub_index=NULL;
		params.output=output;
		params.start=start;
		params.stop=stop;
		params.alphas=alphas;
		params.vec=vec;
		params.dim=dim;
		params.bias=b;
		params.progress=false; //true;
		dense_dot_range_helper((void*) &params);
#ifndef WIN32
	}
	else
	{
		pthread_t* threads = new pthread_t[num_threads-1];
		DF_THREAD_PARAM* params = new DF_THREAD_PARAM[num_threads];
		int32_t step= num_vectors/num_threads;

		int32_t t;

		for (t=0; t<num_threads-1; t++)
		{
			params[t].df = this;
			params[t].sub_index=NULL;
			params[t].output = output;
			params[t].start = start+t*step;
			params[t].stop = start+(t+1)*step;
			params[t].alphas=alphas;
			params[t].vec=vec;
			params[t].dim=dim;
			params[t].bias=b;
			params[t].progress = false;
			pthread_create(&threads[t], NULL,
					CDotFeatures::dense_dot_range_helper, (void*)&params[t]);
		}

		params[t].df = this;
		params[t].output = output;
		params[t].sub_index=NULL;
		params[t].start = start+t*step;
		params[t].stop = stop;
		params[t].alphas=alphas;
		params[t].vec=vec;
		params[t].dim=dim;
		params[t].bias=b;
		params[t].progress = false; //true;
		dense_dot_range_helper((void*) &params[t]);

		for (t=0; t<num_threads-1; t++)
			pthread_join(threads[t], NULL);

		delete[] params;
		delete[] threads;
	}
#endif

#ifndef WIN32
		if ( CSignal::cancel_computations() )
			SG_INFO( "prematurely stopped.           \n");
#endif
}

void CDotFeatures::dense_dot_range_subset(int32_t* sub_index, int32_t num, float64_t* output, float64_t* alphas, float64_t* vec, int32_t dim, float64_t b)
{
	ASSERT(sub_index);
	ASSERT(output);

	int32_t num_threads=parallel->get_num_threads();
	ASSERT(num_threads>0);

	CSignal::clear_cancel();

#ifndef WIN32
	if (num_threads < 2)
	{
#endif
		DF_THREAD_PARAM params;
		params.df=this;
		params.sub_index=sub_index;
		params.output=output;
		params.start=0;
		params.stop=num;
		params.alphas=alphas;
		params.vec=vec;
		params.dim=dim;
		params.bias=b;
		params.progress=false; //true;
		dense_dot_range_helper((void*) &params);
#ifndef WIN32
	}
	else
	{
		pthread_t* threads = new pthread_t[num_threads-1];
		DF_THREAD_PARAM* params = new DF_THREAD_PARAM[num_threads];
		int32_t step= num/num_threads;

		int32_t t;

		for (t=0; t<num_threads-1; t++)
		{
			params[t].df = this;
			params[t].sub_index=sub_index;
			params[t].output = output;
			params[t].start = t*step;
			params[t].stop = (t+1)*step;
			params[t].alphas=alphas;
			params[t].vec=vec;
			params[t].dim=dim;
			params[t].bias=b;
			params[t].progress = false;
			pthread_create(&threads[t], NULL,
					CDotFeatures::dense_dot_range_helper, (void*)&params[t]);
		}

		params[t].df = this;
		params[t].sub_index=sub_index;
		params[t].output = output;
		params[t].start = t*step;
		params[t].stop = num;
		params[t].alphas=alphas;
		params[t].vec=vec;
		params[t].dim=dim;
		params[t].bias=b;
		params[t].progress = false; //true;
		dense_dot_range_helper((void*) &params[t]);

		for (t=0; t<num_threads-1; t++)
			pthread_join(threads[t], NULL);

		delete[] params;
		delete[] threads;
	}
#endif

#ifndef WIN32
		if ( CSignal::cancel_computations() )
			SG_INFO( "prematurely stopped.           \n");
#endif
}

void* CDotFeatures::dense_dot_range_helper(void* p)
{
	DF_THREAD_PARAM* par=(DF_THREAD_PARAM*) p;
	CDotFeatures* df=par->df;
	int32_t* sub_index=par->sub_index;
	float64_t* output=par->output;
	int32_t start=par->start;
	int32_t stop=par->stop;
	float64_t* alphas=par->alphas;
	float64_t* vec=par->vec;
	int32_t dim=par->dim;
	float64_t bias=par->bias;
	bool progress=par->progress;

	if (sub_index)
	{
#ifdef WIN32
		for (int32_t i=start; i<stop i++)
#else
		for (int32_t i=start; i<stop &&
				!CSignal::cancel_computations(); i++)
#endif
		{
			if (alphas)
				output[i]=alphas[sub_index[i]]*df->dense_dot(sub_index[i], vec, dim)+bias;
			else
				output[i]=df->dense_dot(sub_index[i], vec, dim)+bias;
			if (progress)
				df->display_progress(start, stop, i);
		}

	}
	else
	{
#ifdef WIN32
		for (int32_t i=start; i<stop i++)
#else
		for (int32_t i=start; i<stop &&
				!CSignal::cancel_computations(); i++)
#endif
		{
			if (alphas)
				output[i]=alphas[i]*df->dense_dot(i, vec, dim)+bias;
			else
				output[i]=df->dense_dot(i, vec, dim)+bias;
			if (progress)
				df->display_progress(start, stop, i);
		}
	}

	return NULL;
}

SGMatrix<float64_t> CDotFeatures::get_computed_dot_feature_matrix()
{
	SGMatrix<float64_t> m;
	
    int64_t offs=0;
	int32_t num=get_num_vectors();
    int32_t dim=get_dim_feature_space();
    ASSERT(num>0);
    ASSERT(dim>0);

    int64_t sz=((uint64_t) num)* dim;

	m.do_free=true;
    m.num_cols=dim;
    m.num_rows=num;
    m.matrix=new float64_t[sz];
    memset(m.matrix, 0, sz*sizeof(float64_t));

    for (int32_t i=0; i<num; i++)
    {
		add_to_dense_vec(1.0, i, &(m.matrix[offs]), dim);
        offs+=dim;
    }

	return m;
}

SGVector<float64_t> CDotFeatures::get_computed_dot_feature_vector(int32_t num)
{
	SGVector<float64_t> v;

    int32_t dim=get_dim_feature_space();
    ASSERT(num>=0 && num<=num);
    ASSERT(dim>0);

	v.do_free=true;
    v.vlen=dim;
    v.vector=new float64_t[dim];
    memset(v.vector, 0, dim*sizeof(float64_t));

    add_to_dense_vec(1.0, num, v.vector, dim);
	return v;
}

void CDotFeatures::benchmark_add_to_dense_vector(int32_t repeats)
{
	int32_t num=get_num_vectors();
	int32_t d=get_dim_feature_space();
	float64_t* w= new float64_t[d];
	CMath::fill_vector(w, d, 0.0);

	CTime t;
	float64_t start_cpu=t.get_runtime();
	float64_t start_wall=t.get_curtime();
	for (int32_t r=0; r<repeats; r++)
	{
		for (int32_t i=0; i<num; i++)
			add_to_dense_vec(1.172343*(r+1), i, w, d);
	}

	SG_PRINT("Time to process %d x num=%d add_to_dense_vector ops: cputime %fs walltime %fs\n",
			repeats, num, (t.get_runtime()-start_cpu)/repeats,
			(t.get_curtime()-start_wall)/repeats);

	delete[] w;
}

void CDotFeatures::benchmark_dense_dot_range(int32_t repeats)
{
	int32_t num=get_num_vectors();
	int32_t d=get_dim_feature_space();
	float64_t* w= new float64_t[d];
	float64_t* out= new float64_t[num];
	float64_t* alphas= new float64_t[num];
	CMath::range_fill_vector(w, d, 17.0);
	CMath::range_fill_vector(alphas, num, 1.2345);
	//CMath::fill_vector(w, d, 17.0);
	//CMath::fill_vector(alphas, num, 1.2345);

	CTime t;
	float64_t start_cpu=t.get_runtime();
	float64_t start_wall=t.get_curtime();

	for (int32_t r=0; r<repeats; r++)
			dense_dot_range(out, 0, num, alphas, w, d, 23);

#ifdef DEBUG_DOTFEATURES
    CMath::display_vector(out, 40, "dense_dot_range");
	float64_t* out2= new float64_t[num];

	for (int32_t r=0; r<repeats; r++)
    {
        CMath::fill_vector(out2, num, 0.0);
        for (int32_t i=0; i<num; i++)
            out2[i]+=dense_dot(i, w, d)*alphas[i]+23;
    }
    CMath::display_vector(out2, 40, "dense_dot");
	for (int32_t i=0; i<num; i++)
		out2[i]-=out[i];
    CMath::display_vector(out2, 40, "diff");
#endif
	SG_PRINT("Time to process %d x num=%d dense_dot_range ops: cputime %fs walltime %fs\n",
			repeats, num, (t.get_runtime()-start_cpu)/repeats,
			(t.get_curtime()-start_wall)/repeats);

	delete[] alphas;
	delete[] out;
	delete[] w;
}

void CDotFeatures::get_mean(float64_t** mean, int32_t* mean_length)
{
	int32_t num=get_num_vectors();
	int32_t dim=get_dim_feature_space();
	ASSERT(num>0);
	ASSERT(dim>0);

	*mean_length = dim;
	*mean = new float64_t[dim];
    memset(*mean, 0, sizeof(float64_t)*dim);

	for (int i = 0; i < num; i++)
		add_to_dense_vec(1, i, *mean, dim);
	for (int j = 0; j < dim; j++)
		(*mean)[j] /= num;
}									

void CDotFeatures::get_cov(float64_t** cov, int32_t* cov_rows, int32_t* cov_cols)
{
	int32_t num=get_num_vectors();
	int32_t dim=get_dim_feature_space();
	ASSERT(num>0);
	ASSERT(dim>0);

	*cov_rows = dim;
	*cov_cols = dim;

	*cov = new float64_t[dim*dim];
    memset(*cov, 0, sizeof(float64_t)*dim*dim);

	float64_t* mean;
	int32_t mean_length;
	get_mean(&mean, &mean_length);

	for (int i = 0; i < num; i++)
	{
		SGVector<float64_t> v = get_computed_dot_feature_vector(i);
		CMath::add<float64_t>(v.vector, 1, v.vector, -1, mean, v.vlen);
		for (int m = 0; m < v.vlen; m++)
		{
			for (int n = 0; n <= m ; n++)
			{
				(*cov)[m*v.vlen+n] += v.vector[m]*v.vector[n];
			}
		}
		v.free_vector();
	}
	for (int m = 0; m < dim; m++)
	{
		for (int n = 0; n <= m ; n++)
		{
			(*cov)[m*dim+n] /= num;
		}
	}
	for (int m = 0; m < dim-1; m++)
	{
		for (int n = m+1; n < dim; n++)
		{
			(*cov)[m*dim+n] = (*cov)[n*dim+m];
		}
	}
	delete[] mean;
}
