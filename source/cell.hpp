#ifndef CELL_HPP
#define CELL_HPP

#include "color.hpp"
#include <cstdint>

class Cell
{
public:

    enum class Collision : uint8_t
    {
        Empty,
        PlaneLow,
        PlaneMid,
        PlaneHigh,
        BlockLow,
        BlockMid,
        BlockHigh,
        TunnelLow,
        TunnelMid,
        TunnelHigh,
        TunnelPlaneLow,
        TunnelPlaneMid,
        TunnelPlaneHigh,
        TunnelBlockLow,
        TunnelBlockMid,
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


    constexpr static auto isTunnel(Collision c) -> bool
    {
        return (static_cast<std::size_t>(c) >= 7) && (static_cast<std::size_t>(c) <= 15);
    }

    constexpr static auto isTunnelFloor(Collision c) -> bool
    {
        return (static_cast<std::size_t>(c) >= 10) && (static_cast<std::size_t>(c) <= 15);
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
