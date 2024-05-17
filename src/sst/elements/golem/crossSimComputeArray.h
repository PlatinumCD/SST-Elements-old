// Copyright 2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _CrossSimComputeARRAY_H
#define _CrossSimComputeARRAY_H

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include "numpy/arrayobject.h"

#include <sst/core/component.h>
#include "computeArray.h"

#include <vector>
#include <string>

namespace SST {
namespace Golem {

class CrossSimComputeArray : public SST::Golem::ComputeArray {
public:
	
    SST_ELI_REGISTER_SUBCOMPONENT(CrossSimComputeArray, "golem", "CrossSimComputeArray",
                                      SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                      "Implements a Compute array using CrossSim",
                                      SST::Golem::ComputeArray)

    SST_ELI_DOCUMENT_PARAMS(
        {"arrayLatency",       "Latency of array compution, include all data conversion (ADC, DAC) latencies", "100ns" },
        { "verbose", "Set the verbosity of output for the RoCC", "0" }, 
        { "max_instructions", "Set the maximum number of RoCC instructions permitted in the queue", "8" },
        {"clock",              "Clock frequency for component TimeConverter. MMIOTile is Unclocked but subcomponents use the TimeConverter", "1Ghz"},
        {"mmioAddr" ,          "Address MMIO interface"},
        {"numArrays",          "Number of distinct arrays in the the tile.", "1"},
        {"arrayInputSize",     "Length of input vector. Implies array rows."},
        {"arrayOutputSize",    "Length of output vector. Implies array columns."},
        {"inputOperandSize",   "Size of input operand in bytes.", "4"},
        {"outputOperandSize",  "Size of output operand in bytes.", "4"},
        {"CrossSimJSON",       "Path to CrossSim JSON."}
    )

    CrossSimComputeArray(ComponentId_t id, Params& params, 
        TimeConverter * tc,
        Event::HandlerBase * handler,
        std::vector<std::vector<int64_t>>* ins,
        std::vector<std::vector<int64_t>>* outs,
        std::vector<std::vector<int64_t>>* mats)
    : ComputeArray(id, params, tc, handler, ins, outs, mats) {
        
        //All operations have the same latency so just set it here
        //Because of the fixed latency just reset the TimeBase here from the TimeBase of parent component in genericArray
        arrayLatency = params.find<UnitAlgebra>("arrayLatency", "100ns");
        latencyTC = getTimeConverter(arrayLatency);
        selfLink->setDefaultTimeBase(latencyTC);

        Py_Initialize();
        import_array1();
        size = params.find<uint64_t>("arrayInputSize");
        numArrays = params.find<uint64_t>("numArrays", 1);
        inputOperandSize = params.find<uint64_t>("inputOperandSize", 4);
        outputOperandSize = params.find<uint64_t>("outputOperandSize", 4);
        CrossSimJSON = params.find<std::string>("CrossSimJSON", std::string());
    }

    virtual void init(unsigned int phase) override {
        // Matrix Dimensions
        npy_intp matrixDims[2] {size,size};
        int matrixNumDims = 2;

        // Input Array Dimensions
        npy_intp arrayInDim [1] {size};
        int arrayInNumDims = 1;

        // Create Numpy matrices/arrays from mats, ins
        pyMatrix = new PyObject*[numArrays];
        npMatrix = new PyArrayObject*[numArrays];

        pyArrayIn = new PyObject*[numArrays];
        npArrayIn = new PyArrayObject*[numArrays];


        int npy_type;
        switch (inputOperandSize) {
            case 1: npy_type = NPY_INT8; break;
            case 2: npy_type = NPY_INT16; break;
            case 4: npy_type = NPY_INT32; break;
            case 8: npy_type = NPY_INT64; break;
            default: throw std::invalid_argument("Unsupported input operand size");
        }

        for (int i = 0; i < numArrays; i++) {
            auto& cMatrix = (*matrices)[i];
            auto& cArrayIn = (*inVecs)[i];

            pyMatrix[i] = PyArray_SimpleNewFromData(matrixNumDims, matrixDims, npy_type, reinterpret_cast<void*>(cMatrix.data()));
            npMatrix[i] = reinterpret_cast<PyArrayObject*>(pyMatrix[i]);
            pyArrayIn[i] = PyArray_SimpleNewFromData(arrayInNumDims, arrayInDim, npy_type, reinterpret_cast<void*>(cArrayIn.data()));
            npArrayIn[i] = reinterpret_cast<PyArrayObject*>(pyArrayIn[i]);

        }

        //import CrossSim
        crossSim = PyImport_ImportModule("simulator");
        if (!crossSim) {
            std::cout << "Import CrossSim failed" << std::endl;
            PyErr_Print();
        }

        paramsConstructor = PyObject_GetAttrString(crossSim, "CrossSimParameters"); //no args
        if (!paramsConstructor) {
            std::cout << "Set Parameters constructor failed" << std::endl;
            PyErr_Print();
        }

        AnalogCoreConstructor = PyObject_GetAttrString(crossSim, "AnalogCore"); //args:params
        if (!AnalogCoreConstructor) {
            std::cout << "Set cross_sim.MakeCore function failed" << std::endl;
            PyErr_Print();
        }

        if (CrossSimJSON.empty()) {
            crossSim_params = PyObject_CallFunction(paramsConstructor, NULL);
        } else {
            PyObject* paramsConstructorJSON = PyObject_GetAttrString(paramsConstructor, "from_json");
            PyObject* jsonArgs = Py_BuildValue("(s)", CrossSimJSON.c_str());
            crossSim_params = PyObject_CallObject(paramsConstructorJSON, jsonArgs);
        }

        if (!crossSim_params) {
            std::cout << "Call to Parameters constructor failed" << std::endl;
            PyErr_Print();
        }

        // Create the cores
        cores = new PyObject*[numArrays];
        setMatrixFunction = new PyObject*[numArrays];
        runMVM = new PyObject*[numArrays];
        for (int i = 0; i < numArrays; i++) {
            cores[i] = PyObject_CallFunctionObjArgs(AnalogCoreConstructor, npMatrix[i], crossSim_params, NULL);
            if (!cores[i]) {
                std::cout << "Call to AnalogCore failed" << std::endl;
                PyErr_Print();
            }

            setMatrixFunction[i] = PyObject_GetAttrString(cores[i], "set_matrix"); //args:matrix[2d fp64]
            if (!setMatrixFunction[i]) {
                std::cout << "Set core.set_matrix failed" << std::endl;
                PyErr_Print();
            }

            runMVM[i] = PyObject_GetAttrString(cores[i], "matvec"); //args: vector[1d fp64]
            if (!runMVM[i]) {
                std::cout << "Set core.run_xbar_mvm failed" << std::endl;
                PyErr_Print();
            }
        }
    }

    virtual void setup() override {}
    virtual void finish() override {}
    virtual void emergencyShutdown() override {}

    virtual void setMatrix(void* data, uint32_t arrayID, uint32_t num_rows, uint32_t num_cols) override {
        auto& matrix = (*matrices)[arrayID];

        // Resize the matrix to fit the data
        matrix.resize(num_rows * num_cols);

        auto assign_data = [&](auto ptr) {
            for (auto i = 0; i < num_rows * num_cols; i++) {
                matrix[i] = static_cast<int64_t>(ptr[i]);
            }
        };

        switch (inputOperandSize) {
            case 1: assign_data(static_cast<int8_t*>(data)); break;
            case 2: assign_data(static_cast<int16_t*>(data)); break;
            case 4: assign_data(static_cast<int32_t*>(data)); break;
            case 8: assign_data(static_cast<int64_t*>(data)); break;
            default: /* handle error */ break;
        }

        std::cout << "Matrix for array " << arrayID << ":" << std::endl;
        for (auto i = 0; i < num_rows; i++) {
            for (auto j = 0; j < num_cols; j++) {
                std::cout << std::setw(3) << matrix[i * num_cols + j] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;

        status = PyObject_CallFunctionObjArgs(setMatrixFunction[arrayID], npMatrix[arrayID], NULL); //no return, just for error checking
        if (!status) {
            std::cout << "Call to core.set_matrix failed" << std::endl;
            PyErr_Print();
        }
    }

    virtual void setInputVector(void* data, uint32_t arrayID, uint32_t num_cols) override {
        auto& inVec = (*inVecs)[arrayID];

        auto assign_data = [&](auto ptr) {
            for(size_t i = 0; i < num_cols; ++i) {
                inVec[i] = static_cast<int64_t>(ptr[i]);
            }
        };

        switch (inputOperandSize) {
            case 1: assign_data(static_cast<int8_t*>(data)); break;
            case 2: assign_data(static_cast<int16_t*>(data)); break;
            case 4: assign_data(static_cast<int32_t*>(data)); break;
            case 8: assign_data(static_cast<int64_t*>(data)); break;
            default: /* handle error */ break;
        }

        std::cout << "Loaded array " << arrayID << ":" << std::endl;
        for (int i = 0; i < num_cols; i++) {
            std::cout << std::setw(3) << inVec[i] << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }

    virtual void compute(uint32_t arrayID) override {
        auto& outVec = (*outVecs)[arrayID];

        // Call MVM
        pyArrayOut = PyObject_CallFunctionObjArgs(runMVM[arrayID], npArrayIn[arrayID], NULL);
        if (nullptr == pyArrayOut) {
            std::cout << "Run MVM Call Failed" << std::endl;
            PyErr_Print();
        }
        npArrayOut = reinterpret_cast<PyArrayObject*> (pyArrayOut);
        cArrayOut = reinterpret_cast<int64_t*> (PyArray_DATA(npArrayOut));

        int len = PyArray_SIZE(npArrayOut);
        std::copy(cArrayOut, cArrayOut + len, outVec.begin());
        std::cout << "CrossSim MVM on array " << arrayID << ":" << std::endl;
        for (int i = 0; i < size; i++) {
            std::cout << outVec[i] << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }

    //Since we set the timebase in the constructor the latency is just 1 timebase
    virtual SimTime_t getArrayLatency(uint32_t arrayID) {
        return 1;
    }

protected:
    UnitAlgebra arrayLatency; //Latency of array operation including array conversion times
    TimeConverter* latencyTC; //TimeConverter corrosponding to the above

    uint64_t size; // dimensions of matrix and length of input and output vectors
    std::string CrossSimJSON;

    // bunch of C++/Python conversion variables
    PyObject* crossSim;

    PyObject** pyMatrix; // matrix stuff
    PyArrayObject** npMatrix;

    PyObject** pyArrayIn;
    PyArrayObject** npArrayIn;

    int64_t* cArrayOut; // output vector
    PyObject* pyArrayOut;
    PyArrayObject* npArrayOut;
    
    PyObject* paramsConstructor; // crossSim stuff that I copied from Ben's C++/Python example
    PyObject* AnalogCoreConstructor;
    PyObject* crossSim_params;
    PyObject* pysize;
    PyObject** cores;
    PyObject** setMatrixFunction;
    PyObject** runMVM;
    PyObject* status;
};


}
}
#endif /* _CrossSimComputeARRAY_H */
