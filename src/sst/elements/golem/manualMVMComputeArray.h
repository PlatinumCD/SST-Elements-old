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

#ifndef _ManualMVMComputeARRAY_H
#define _ManualMVMComputeARRAY_H

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <sst/core/component.h>
#include "computeArray.h"

#include <bitset>


#include <vector>

namespace SST {
namespace Golem {

class ManualMVMComputeArray : public SST::Golem::ComputeArray {
public:
	
    SST_ELI_REGISTER_SUBCOMPONENT(ManualMVMComputeArray, "golem", "ManualMVMComputeArray",
                                      SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                      "Implements a Compute array using manual MVM (basically for loops)",
                                      SST::Golem::ComputeArray)

    SST_ELI_DOCUMENT_PARAMS(
        {"arrayLatency",       "Latency of array compution, include all data conversion (ADC, DAC) latencies", "100ns" },
        {"verbose", "Set the verbosity of output for the RoCC", "0" }, 
        {"max_instructions", "Set the maximum number of RoCC instructions permitted in the queue", "8" },
        {"clock",              "Clock frequency for component TimeConverter. MMIOTile is Unclocked but subcomponents use the TimeConverter", "1Ghz"},
        {"mmioAddr",          "Address MMIO interface"},
        {"numArrays",          "Number of distinct arrays in the the tile.", "1"},
        {"arrayInputSize",     "Length of input vector. Implies array rows."},
        {"arrayOutputSize",    "Length of output vector. Implies array columns."},
        {"inputOperandSize",   "Size of input operand in bytes.", "4"},
        {"outputOperandSize",  "Size of output operand in bytes.", "4"}
    )

    ManualMVMComputeArray(ComponentId_t id, Params& params, 
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

        numArrays = params.find<uint64_t>("numArrays", 1);
        arrayInSize = params.find<uint64_t>("arrayInputSize");
        arrayOutSize = params.find<uint64_t>("arrayOutputSize");
        inputOperandSize = params.find<uint64_t>("inputOperandSize", 4);
        outputOperandSize = params.find<uint64_t>("outputOperandSize", 4);

        out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);
    }

    virtual void init(unsigned int phase) override {}
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
                std::cout << matrix[i * num_cols + j] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
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
            std::cout << inVec[i] << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }


    virtual void compute(uint32_t arrayID) override {
        auto& outVec = (*outVecs)[arrayID];
        auto& inVec = (*inVecs)[arrayID];
        auto& matrix = (*matrices)[arrayID];
    
        for (int i = 0; i < arrayOutSize; i++) {
            outVec[i] = 0.0;
        }
    
        std::cout << "Manual MVM on array " << arrayID << ":" << std::endl;
        for (auto row = 0; row < arrayOutSize; row++) {
            for (auto col = 0; col < arrayInSize; col++) {
                outVec[row] += matrix[row * arrayInSize + col] * inVec[col];
            }
            std::cout << outVec[row] << " ";
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
    TimeConverter* latencyTC; //TimeConveter corrosponding to the above
};


}
}
#endif /* _ManualMVMComputeARRAY_H */
