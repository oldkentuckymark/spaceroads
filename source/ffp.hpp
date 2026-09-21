#pragma once

#include "ffm.hpp"

namespace ffp
{

class AABB
{
public:
    ffm::vec3 min, max;
};

[[nodiscard]] constexpr auto intersect(AABB const & a, AABB const & b) -> ffm::vec3
{
    ffm::fixed32 dx = ffm::min(a.max.x, b.max.x) - ffm::max(a.min.x, b.min.x);
    if (dx < 0.0_fx) { return {}; } // No overlap in X

    ffm::fixed32 dy = ffm::min(a.max.y, b.max.y) - ffm::max(a.min.y, b.min.y);
    if (dy < 0.0_fx) { return {}; } // No overlap in Y

    ffm::fixed32 dz = ffm::min(a.max.z, b.max.z) - ffm::max(a.min.z, b.min.z);
    if (dz < 0.0_fx) { return {}; } // No overlap in Z

    return ffm::vec3(dx, dy, dz);
}



}
