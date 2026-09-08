#ifndef GAME_HPP
#define GAME_HPP

#include "player.hpp"
#include "level.hpp"

class Game
{
public:
    Game()
    {

    }

    ~Game()
    {

    }


    auto setCurrentLevel(ILevel const * lvl) -> void
    {
        current_level_ = lvl;

    }

    auto player() -> Player&
    {
        return player_;
    }

    auto processInputs(std::array<bool, 10>& inputs) -> void
    {
        constexpr size_t A = 0;
        constexpr size_t B = 1;
        constexpr size_t SELECT = 2;
        constexpr size_t START = 3;
        constexpr size_t RIGHT = 4;
        constexpr size_t LEFT = 5;
        constexpr size_t UP = 6;
        constexpr size_t DOWN = 7;
        constexpr size_t R = 8;
        constexpr size_t L = 9;

        if(inputs[UP])
        {
            player_.velocity.z = player_.velocity.z + player_.acceleration;

        }
        if(inputs[DOWN])
        {
            player_.velocity.z = player_.velocity.z - player_.acceleration;
        }
        if(inputs[RIGHT])
        {
            player_.velocity.x = player_.velocity.x + player_.acceleration;
        }
        if(inputs[LEFT])
        {
            player_.velocity.x = player_.velocity.x - player_.acceleration;
        }

    }


    auto update() -> void
    {
        player_.position = player_.position + player_.velocity;
    }

private:
    Player player_;
    ILevel const* current_level_{nullptr};
};



#endif // GAME_HPP
