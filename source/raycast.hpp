#ifndef RAYCAST_HPP
#define RAYCAST_HPP

#include "ffm.hpp"
#include "util.hpp"
#include "level.hpp"

using namespace ffm;

#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdint>

consteval auto calcdXdY(double const theta) -> vec2
{
    return {};
}

consteval auto makeDeltaTable() -> std::array<vec2, 256>
{
    return {};
}


//dx/dy and 1/dx 1/dy LUTs?

auto dda_cells(fixed32 camx, fixed32 camy, fixed32 yaw,
                fixed32 dx, fixed32 dy, ILevel const & level, int32_t drawDistance) -> std::vector<std::pair<int,int>>
{
    std::vector<std::pair<int,int>> cells;

    if (dx == 0.0_fx && dy == 0.0_fx)
        return cells;

    camx = std::nextafterf(camx, camx + dx);
    camy = std::nextafterf(camy, camy + dy);

    auto mapX = static_cast<int>(camx);
    auto mapY = static_cast<int>(camy);

    int stepX = (dx > 0.0_fx) ? 1 : -1;
    int stepY = (dy > 0.0_fx) ? 1 : -1;

    fixed32 tDeltaX, tMaxX, tDeltaY, tMaxY;

    if (dx != 0.0_fx)
    {
        fixed32 invDx = 1.0_fx / dx;
        tDeltaX = ffm::abs(invDx);
        tMaxX   = (dx > 0) ? (mapX + 1 - camx) * invDx
                         : (mapX - camy)       * invDx;
    }
    else
    {
        tDeltaX = 0.0_fx;
        tMaxX   = std::numeric_limits<fixed32>::infinity();
    }

    if (dy != 0.0_fx)
    {
        fixed32 invDy = 1.0_fx / dy;
        tDeltaY = ffm::abs(invDy);
        tMaxY   = (dy > 0) ? (mapY + 1 - camy) * invDy
                         : (mapY - camy)       * invDy;
    }
    else
    {
        tDeltaY = 0.0_fx;
        tMaxY   = std::numeric_limits<fixed32>::infinity();
    }

    int visited = 0;

    if (mapX >= 0 && mapX < level.getWidth() && mapY >= 0 && mapY < level.getLength())
    {
        visited++;
        if (level.getCell(static_cast<uint16_t>(mapX), static_cast<uint16_t>(mapY)).collision != Cell::Collision::Empty)
        {
            cells.emplace_back(mapX, mapY);
        }
    }

    while (visited < drawDistance)
    {
        if (tMaxX < tMaxY)
        {
            tMaxX += tDeltaX;
            mapX  += stepX;
        }
        else
        {
            tMaxY += tDeltaY;
            mapY  += stepY;
        }
        if (mapX < 0 || mapX >= level.getWidth() || mapY < 0 || mapY >= level.getLength())
        {
            break;
        }
        visited++;
        if (level.getCell(static_cast<uint16_t>(mapX),
                          static_cast<uint16_t>(mapY)).collision != Cell::Collision::Empty)
            cells.emplace_back(mapX, mapY);
    }

    //std::reverse(cells.begin(), cells.end());
    return cells;
}

#endif // RAYCAST_HPP
