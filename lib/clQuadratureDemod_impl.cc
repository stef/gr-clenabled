/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "clQuadratureDemod_impl.h"
#include "fast_atan2f.h"

namespace gr {
  namespace clenabled {

    clQuadratureDemod::sptr
    clQuadratureDemod::make(float gain, int openCLPlatformType,int devSelector,int platformId, int devId,int setDebug)
    {
      if (setDebug == 1)
		  return gnuradio::get_initial_sptr
			(new clQuadratureDemod_impl(gain, openCLPlatformType,devSelector,platformId,devId,true));
      else
		  return gnuradio::get_initial_sptr
			(new clQuadratureDemod_impl(gain, openCLPlatformType,devSelector,platformId,devId,false));
    }

    /*
     * The private constructor
     */
    clQuadratureDemod_impl::clQuadratureDemod_impl(float gain, int openCLPlatformType,int devSelector,int platformId, int devId, bool setDebug)
      : gr::block("clQuadratureDemod",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(float))),
	  	  	  GRCLBase(DTYPE_COMPLEX, sizeof(gr_complex),openCLPlatformType,devSelector,platformId,devId,setDebug)
   {
    	d_gain = gain;

    	// Now we set up our OpenCL kernel
        std::string srcStdStr="";
        std::string fnName = "quadDemod";

        srcStdStr += "#define GAIN " + std::to_string(d_gain) + "\n";
    	srcStdStr += "struct ComplexStruct {\n";
    	srcStdStr += "float real;\n";
    	srcStdStr += "float imag; };\n";
    	srcStdStr += "typedef struct ComplexStruct SComplex;\n";
    	srcStdStr += "__kernel void quadDemod(__constant SComplex * a, __global float * restrict c) {\n";
    	srcStdStr += "    size_t index =  get_global_id(0);\n";
    	srcStdStr += "    float a_r=a[index].real;\n";
    	srcStdStr += "    float a_i=a[index].imag;\n";
    	srcStdStr += "    float b_r=a[index].real;\n";
    	srcStdStr += "    float b_i=-1.0 * a[index].imag;\n";
    	srcStdStr += "    SComplex multCC;\n";
    	srcStdStr += "    multCC.real = a_r * b_r - (a_i*b_i);\n";
    	srcStdStr += "    multCC.imag = a_r * b_i + a_i * b_r;\n";
    	srcStdStr += "    c[index] = GAIN * atan2(multCC.imag,multCC.real);\n";
    	srcStdStr += "}\n";

    	int imaxItems=gr::block::max_noutput_items();
    	if (imaxItems==0)
    		imaxItems=8192;

    	maxConstItems = (int)((float)maxConstMemSize / ((float)sizeof(gr_complex)));

    	if (maxConstItems < imaxItems || imaxItems == 0) {
    		gr::block::set_max_noutput_items(maxConstItems);
    		imaxItems = maxConstItems;

    		if (debugMode)
    			std::cout << "OpenCL INFO: Quadrature demod adjusting output buffer for " << maxConstItems << " due to OpenCL constant memory restrictions" << std::endl;
		}
		else {
			if (debugMode)
				std::cout << "OpenCL INFO: Quadrature demod using default output buffer of " << imaxItems << "..." << std::endl;
		}

        GRCLBase::CompileKernel((const char *)srcStdStr.c_str(),(const char *)fnName.c_str());

        setBufferLength(imaxItems);
        // And finally optimize the data we get based on the preferred workgroup size.
        // Note: We can't do this until the kernel is compiled and since it's in the block class
        // it has to be done here.
        // Note: for CPU's adjusting the workgroup size away from 1 seems to decrease performance.
        // For GPU's setting it to the preferred size seems to have the best performance.
        if (contextType != CL_DEVICE_TYPE_CPU) {
        	gr::block::set_output_multiple(preferredWorkGroupSizeMultiple);
        }
}

    void clQuadratureDemod_impl::setBufferLength(int numItems) {
    	if (aBuffer)
    		delete aBuffer;

    	if (cBuffer)
    		delete cBuffer;

    	aBuffer = new cl::Buffer(
            *context,
            CL_MEM_READ_ONLY,
			numItems * sizeof(gr_complex));

        cBuffer = new cl::Buffer(
            *context,
            CL_MEM_READ_WRITE,
			numItems * sizeof(float));

        curBufferSize=numItems;
    }

    /*
     * Our virtual destructor.
     */
    clQuadratureDemod_impl::~clQuadratureDemod_impl()
    {
    	if (aBuffer)
    		delete aBuffer;

    	if (cBuffer)
    		delete cBuffer;
    }

    int clQuadratureDemod_impl::testCPU(int noutput_items,
            gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items)
    	{
        gr_complex *in = (gr_complex*)input_items[0];
        float *out = (float*)output_items[0];

        std::vector<gr_complex> tmp(noutput_items);
        volk_32fc_x2_multiply_conjugate_32fc(&tmp[0], &in[1], &in[0], noutput_items);
        for(int i = 0; i < noutput_items; i++) {
          out[i] = d_gain * gr::clenabled::fast_atan2f(imag(tmp[i]), real(tmp[i]));
        }

        return noutput_items;
    }

    int clQuadratureDemod_impl::testOpenCL(int noutput_items,
            gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items) {
    	return processOpenCL(noutput_items,ninput_items,input_items, output_items);
    }

    int clQuadratureDemod_impl::processOpenCL(int noutput_items,
            gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items)
    {

		if (kernel == NULL) {
			return 0;
		}

    	if (noutput_items > curBufferSize) {
    		setBufferLength(noutput_items);
    	}

    	int inputSize = noutput_items*sizeof(gr_complex);
        queue->enqueueWriteBuffer(*aBuffer,CL_TRUE,0,inputSize,input_items[0]);

		// Do the work

		// Set kernel args
		kernel->setArg(0, *aBuffer);
		kernel->setArg(1, *cBuffer);

		cl::NDRange localWGSize=cl::NullRange;

		if (contextType!=CL_DEVICE_TYPE_CPU) {
			if (noutput_items % preferredWorkGroupSizeMultiple == 0) {
				localWGSize=cl::NDRange(preferredWorkGroupSizeMultiple);
			}
		}

		// Do the work
		queue->enqueueNDRangeKernel(
			*kernel,
			cl::NullRange,
			cl::NDRange(noutput_items),
			localWGSize);


    // Map cBuffer to host pointer. This enforces a sync with
    // the host

	queue->enqueueReadBuffer(*cBuffer,CL_TRUE,0,noutput_items*sizeof(float),(void *)output_items[0]);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }


    int
    clQuadratureDemod_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        int retVal = processOpenCL(noutput_items,ninput_items,input_items,output_items);

      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return retVal;
    }

    void
    clQuadratureDemod_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }


  } /* namespace clenabled */
} /* namespace gr */

