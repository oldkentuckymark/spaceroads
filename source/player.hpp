#pragma once

#include "ffm.hpp"
#include "color.hpp"

class Player
{
public:

    ffm::vec3 position, velocity;
    ffm::fixed32 acceleration{0.01_fx};
private:


};
