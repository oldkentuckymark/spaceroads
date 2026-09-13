#ifndef CELL_HPP
#define CELL_HPP

#include "color.hpp"
#include <cstdint>

class Cell
{
public:

    enum class Collision : uint8_t
    {
        Empty = 0,

        PlaneLow,
        BlockLow,
        TunnelLow,
        TunnelPlaneLow,
        TunnelBlockLow,

        PlaneMid,
        BlockMid,
        TunnelMid,
        TunnelPlaneMid,
        TunnelBlockMid,

        PlaneHigh,
        BlockHigh,
        TunnelHigh,
        TunnelPlaneHigh,
        TunnelBlockHigh,

        NUM_COLLISIONS
    };

    enum class Type : uint8_t
    {
        Normal,
        Oxygen,
        Boost,
        Sticky,
        Slippery,
        Kill,
        End
    };

    constexpr auto height() const -> int16_t
    {
        int16_t cc = static_cast<int16_t>(collision);
        if(cc >= 0 && cc <= 5)
        {
            return 0;
        }
        else if(cc >= 6 && cc <= 10)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }


    constexpr static auto isTunnel(Collision c) -> bool
    {
        return (c == Collision::TunnelLow || c == Collision::TunnelMid || c == Collision::TunnelHigh ||
                c == Collision::TunnelPlaneLow || c == Collision::TunnelPlaneMid || c == Collision::TunnelPlaneHigh ||
                c == Collision::TunnelBlockLow || c == Collision::TunnelBlockMid || c == Collision::TunnelBlockHigh);
    }

    constexpr static auto isTunnelFloor(Collision c) -> bool
    {
        return (c == Collision::TunnelPlaneLow || c == Collision::TunnelPlaneMid || c == Collision::TunnelPlaneHigh ||
                c == Collision::TunnelBlockLow || c == Collision::TunnelBlockMid || c == Collision::TunnelBlockHigh);
    }

    constexpr Cell() = default;

    constexpr Cell(Collision const c, Type const t, Color const tc, Color const sc) :
        collision(c), type(t), topColor(tc), sideColor(sc)
    {

    }

    ~Cell() = default;

    Collision collision{Collision::Empty};
    Type type{Type::Normal};
    Color topColor{0};
    Color sideColor{0};

private:



};

#endif // CELL_HPP
