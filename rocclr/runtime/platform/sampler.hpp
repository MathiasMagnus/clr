//
// Copyright (c) 2008 Advanced Micro Devices, Inc. All rights reserved.
//

#ifndef SAMPLER_HPP_
#define SAMPLER_HPP_

#include "top.hpp"
#include "platform/object.hpp"
#include "device/device.hpp"

namespace amd
{

//! Abstraction layer sampler class
class Sampler : public RuntimeObject
{
public:
    typedef std::map<Device const*, device::Sampler*> DeviceSamplers;

    //! \note the sampler states must match the compiler's defines.
    //! See amd_ocl_sys_predef.c
    enum State
    {
        StateNormalizedCoordsFalse  = 0x00,
        StateNormalizedCoordsTrue   = 0x01,
        StateNormalizedCoordsMask   = (StateNormalizedCoordsFalse |
                                       StateNormalizedCoordsTrue),
        StateAddressNone            = 0x00,
        StateAddressRepeat          = 0x02,
        StateAddressClampToEdge     = 0x04,
        StateAddressClamp           = 0x06,
        StateAddressMirroredRepeat  = 0x08,
        StateAddressMask            = (StateAddressNone |
                                       StateAddressRepeat |
                                       StateAddressMirroredRepeat |
                                       StateAddressClampToEdge |
                                       StateAddressClamp),
        StateFilterNearest          = 0x10,
        StateFilterLinear           = 0x20,
        StateFilterMask             = (StateFilterNearest |
                                       StateFilterLinear)
    };

private:
    Context&    context_;   //!< OpenCL context associated with this sampler
    uint32_t    state_;     //!< Sampler state
    DeviceSamplers   deviceSamplers_;   //!< Container for the device samplers

public:
    Sampler(
        Context&    context,        //!< OpenCL context
        bool        norm_coords,    //!< normalized coordinates
        uint        addr_mode,      //!< adressing mode
        uint        filter_mode     //!< filter mode
        )
        : context_(context)
    {   // Packs the sampler state into uint32_t for kernel execution
        state_ = 0;

        // Set normalized state
        if (norm_coords) {
            state_ |= StateNormalizedCoordsTrue;
        }
        else {
            state_ |= StateNormalizedCoordsFalse;
        }

        // Program the sampler filter mode
        if (filter_mode == CL_FILTER_LINEAR) {
            state_ |= StateFilterLinear;
        }
        else {
            state_ |= StateFilterNearest;
        }

        // Program the sampler address mode
        switch (addr_mode) {
        case CL_ADDRESS_CLAMP_TO_EDGE:
            state_ |= StateAddressClampToEdge;
            break;        
        case CL_ADDRESS_REPEAT: 
            state_ |= StateAddressRepeat;
            break;
        case CL_ADDRESS_CLAMP:
            state_ |= StateAddressClamp;
            break;
        case CL_ADDRESS_MIRRORED_REPEAT: 
            state_ |= StateAddressMirroredRepeat;
            break;
        case CL_ADDRESS_NONE:
            state_ |= StateAddressNone;
            break;
        default:
            break;
        }
    }

    virtual ~Sampler()
    {
        for (DeviceSamplers::const_iterator it = deviceSamplers_.begin();
            it != deviceSamplers_.end(); ++it) {
            delete it->second;
        }
    }

    bool create()
    {
        for (uint i = 0; i < context_.devices().size(); ++i) {
            device::Sampler*    sampler = NULL;
            Device* dev = context_.devices()[i];
            if (!dev->createSampler(*this, &sampler)) {
                return false;
            }
            deviceSamplers_[dev] = sampler;
        }
        return true;
    }

    device::Sampler* getDeviceSampler(const Device& dev) const
    {
        DeviceSamplers::const_iterator it = deviceSamplers_.find(&dev);
        if (it != deviceSamplers_.end()) {
            return it->second;
        }
        return NULL;
    }

    //! Accessor functions
    Context& context() const { return context_; }
    uint32_t state() const { return state_; }
    bool normalizedCoords() const
    {
        return (state_ & StateNormalizedCoordsTrue) ? true : false;
    }

    uint addressingMode() const
    {
        uint    adressing = 0;

        // Program the sampler address mode
        switch (state_ & StateAddressMask) {
            case StateAddressRepeat:
                adressing = CL_ADDRESS_REPEAT;
                break;
            case StateAddressClampToEdge:
                adressing = CL_ADDRESS_CLAMP_TO_EDGE;
                break;
            case StateAddressClamp:
                adressing = CL_ADDRESS_CLAMP;
                break;
            case StateAddressMirroredRepeat:
                adressing = CL_ADDRESS_MIRRORED_REPEAT;
                break;
            case StateAddressNone:
                adressing = CL_ADDRESS_NONE;
                break;
            default:
                break;
        } 
        return adressing;
    }

    uint filterMode() const
    {
        return ((state_ & StateFilterMask) == StateFilterNearest) ?
            CL_FILTER_NEAREST : CL_FILTER_LINEAR;
    }
    //! RTTI internal implementation
    virtual ObjectType objectType() const { return ObjectTypeSampler; }
};

} // namespace amd

#endif /*SAMPLER_HPP_*/
