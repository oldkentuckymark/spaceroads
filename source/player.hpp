#pragma once

#include "ffm.hpp"
#include "color.hpp"

class Player
{
public:

    ffm::vec3 position, velocity;
    ffm::fixed32 acceleration{1.0_fx};
    ffm::fixed32 xSpeed{1.0_fx};
private:


};
