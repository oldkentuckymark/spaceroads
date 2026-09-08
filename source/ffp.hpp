#pragma once

#include "ffm.hpp"

namespace ffp
{

using namespace ffm;

class AABB
{
public:
    vec3 min, max;
};

class Ray
{
public:
    vec3 origin, direction;
};

class Sphere
{
    vec3 center;
    fixed32 radius;
};

[[nodiscard]] constexpr auto intersect(AABB const & a, AABB const & b) -> vec3
{
    fixed32 const dx = min(a.max.x, b.max.x) - max(a.min.x, b.min.x);
    fixed32 const dy = min(a.max.y, b.max.y) - max(a.min.y, b.min.y);
    fixed32 const dz = min(a.max.z, b.max.z) - max(a.min.z, b.min.z);

    if(dx < 0.0_fx || dy < 0.0_fx || dz < 0.0_fx)
    {
        return {};
    };

    return { dx, dy, dz };
}



}
