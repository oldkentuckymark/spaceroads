#pragma once

#include "ffm.hpp"
#include "color.hpp"

class Player
{
public:

    ffm::vec3 position, velocity;
    ffm::fixed32 acceleration{0.1_fx};
    ffm::fixed32 xSpeed{0.1_fx};
private:


};
