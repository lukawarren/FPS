#pragma once
#include "common.h"

namespace Layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NO_COLLISION = 2;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer one, JPH::ObjectLayer two) const override
    {
        if (one == Layers::NO_COLLISION || two == Layers::NO_COLLISION)
            return false;

        switch (one)
        {
            case Layers::NON_MOVING:
                return two == Layers::MOVING;

            case Layers::MOVING:
                return true;

            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::BroadPhaseLayer NO_COLLISION(2);
    static constexpr u32 NUM_LAYERS(3);
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        mObjectToBroadPhase[Layers::NO_COLLISION] = BroadPhaseLayers::NO_COLLISION;
    }

    virtual u32 GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	    return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		    return "MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NO_COLLISION:    return "NO_COLLISION";
            default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer one, JPH::BroadPhaseLayer two) const override
    {
        if (one == Layers::NO_COLLISION || two == BroadPhaseLayers::NO_COLLISION)
            return false;

        switch (one)
        {
            case Layers::NON_MOVING:
                return two == BroadPhaseLayers::MOVING;

            case Layers::MOVING:
                return true;

            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

class IgnoreLayerFilter : public JPH::ObjectLayerFilter
{
public:
    explicit IgnoreLayerFilter(JPH::ObjectLayer inLayerToIgnore)
        : mLayerToIgnore(inLayerToIgnore) {}

    virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        return inLayer != mLayerToIgnore;
    }

private:
    JPH::ObjectLayer mLayerToIgnore;
};
