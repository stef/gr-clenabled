/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(clPolyphaseChannelizer.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(8a37af55c647e7cc074fcd8b870be742)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <clenabled/clPolyphaseChannelizer.h>
// pydoc.h is automatically generated in the build directory
#include <clPolyphaseChannelizer_pydoc.h>

void bind_clPolyphaseChannelizer(py::module& m)
{

    using clPolyphaseChannelizer    = ::gr::clenabled::clPolyphaseChannelizer;


    py::class_<clPolyphaseChannelizer, gr::block, gr::basic_block,
        std::shared_ptr<clPolyphaseChannelizer>>(m, "clPolyphaseChannelizer", D(clPolyphaseChannelizer))

        .def(py::init(&clPolyphaseChannelizer::make),
           py::arg("openCLPlatformType"),
           py::arg("devSelector"),
           py::arg("platformId"),
           py::arg("devId"),
           py::arg("taps"),
           py::arg("buf_items"),
           py::arg("num_channels"),
           py::arg("ninputs_per_iter"),
           py::arg("ch_map"),
           py::arg("setDebug") = 0,
           D(clPolyphaseChannelizer,make)
        )
        



        ;




}







