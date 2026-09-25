#ifndef GAME_HPP
#define GAME_HPP

#include "player.hpp"
#include "level.hpp"
#include "camera.hpp"

class Game
{
public:
    Game()
    {
        player_.position.x = 3.5_fx;
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

    auto getCamera() -> Camera*
    {
        return &camera_;
    }

    auto processInputs(std::array<bool, 10> const & inputs) -> void
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
            player_.velocity.x = player_.xSpeed;
        }
        if(inputs[LEFT])
        {
            player_.velocity.x = -player_.xSpeed;
        }
        if(!inputs[LEFT] && !inputs[RIGHT])
        {
            player_.velocity.x = 0.0_fx;
        }
        if(inputs[L])
        {
            camera_.yaw = camera_.yaw - 0.5_fx;
        }
        if(inputs[R])
        {
            camera_.yaw = camera_.yaw + 0.5_fx;
        }


    }

    auto checkCollision() -> void
    {




    }

    auto update(ffm::fixed32 dt) -> void
    {
        player_.position = player_.position + (player_.velocity * dt);
        camera_.position = player_.position + ffm::vec3{0.0_fx, 1.0_fx, -1.5_fx};
    }

private:
    Player player_;
    ILevel const* current_level_{nullptr};

    Camera camera_;
};



#endif // GAME_HPP
