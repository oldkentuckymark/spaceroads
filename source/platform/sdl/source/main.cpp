//TODO: parse CSV/OBJ files at compile time for #embed


#include "ffr.hpp"
#include <chrono>
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_events.h>
#include "mesh.hpp"
#include "renderer.hpp"
#include "level.hpp"
#include "game.hpp"


class VertexFunction
{
public:

    auto operator()(ffm::vec3& in) -> void
    {

        // 1. Model Space -> World Space
        ffm::fixed32 wx = in.x + modelPos.x;
        ffm::fixed32 wy = in.y + modelPos.y;
        ffm::fixed32 wz = in.z + modelPos.z;

        // 2. World Space -> Camera-Relative Space (Translate FIRST)
        // This makes the camera the origin (0,0,0) for the rotation
        ffm::fixed32 dx = wx - camPos.x;
        ffm::fixed32 dy = wy - camPos.y;
        ffm::fixed32 dz = wz - camPos.z;

        // 3. Apply Camera Yaw Rotation around the Camera's position
        ffm::fixed32 rx = dx * camYawCos - dz * camYawSin;
        ffm::fixed32 rz = dx * camYawSin + dz * camYawCos;
        // dy remains unchanged (Yaw only affects X and Z)

        // 4. Output to View Space
        in.x = rx;
        in.y = dy;
        in.z = rz;


        //in = in + modelPos - camPos;
    }

    ffm::vec3 camPos{0.0_fx, 0.0_fx, 0.0_fx};
    ffm::fixed32 camYawSin, camYawCos;
    ffm::fixed32 camPitchSin, camPitchCos;

    ffm::vec3 modelPos{0.0_fx, 0.0_fx, 0.0_fx};

    //for futrue use
    ffm::fixed32 modelYawSin, modelYawCos;
    ffm::fixed32 modelPitchSin, modelPitchCos;
};

class Context final : public ffr::BaseContext<Context,VertexFunction>
{
public:
    Context()
    {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_CreateWindowAndRenderer("spaceroads-sdl",SCREEN_WIDTH*scale,SCREEN_HEIGHT*scale,0,&win,&ren);
        tex = SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RENDER_WIDTH,RENDER_HEIGHT);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderTarget(ren,tex);
        setViewPort(RENDER_WIDTH, RENDER_HEIGHT);
        setNearZ(1.0_fx);
    }

    ~Context()
    {
        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
    }

    auto clear() -> void
    {
        SDL_SetRenderDrawColor(ren,0,0,0,255);
        SDL_RenderClear(ren);
    }

    auto present() -> void
    {

        SDL_SetRenderTarget(ren,nullptr);
        SDL_RenderTexture(ren,tex,nullptr,nullptr);
        SDL_RenderPresent(ren);
        SDL_SetRenderTarget(ren,tex);
    }

    auto lineHorizontal(int16_t x0, int16_t y0, int16_t x1, uint16_t color) -> void
    {
        auto cc = util::Convert555to888(color);
        SDL_SetRenderDrawColor(ren,cc[0],cc[1],cc[2],255);
        SDL_RenderLine(ren, x0,y0,x1,y0);
    }

    auto plot(int16_t x, int16_t y, uint16_t color) -> void
    {
        auto cc = util::Convert555to888(color);
        SDL_SetRenderDrawColor(ren,cc[0],cc[1],cc[2],255);
        SDL_RenderPoint(ren,x,y);
    }

private:
    int scale = 4;
    int SCREEN_WIDTH = 240;
    int SCREEN_HEIGHT = 160;

    int RENDER_WIDTH = 160;
    int RENDER_HEIGHT = 128;

    SDL_Window* win{nullptr};
    SDL_Renderer* ren{nullptr};

    SDL_Texture* tex{nullptr};


};

auto main() -> int
{


    auto const * const lp = &level0;


    auto c1 = std::chrono::steady_clock::now();
    auto c2 = c1;

    Game game;
    Renderer<Context> renderer;

    renderer.setPlayer(&game.player());
    renderer.setPlayerMesh(Mesh::SHIP_MESH);
    renderer.setLevel(&level0);
    renderer.setCamera(game.getCamera());



    bool running = true;
    while (running)
    {
        std::array<bool,10> inputs{};
        c2 = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>( c2.time_since_epoch()-c1.time_since_epoch()).count() >= 16)
        {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if(e.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if(e.type == SDL_EVENT_MOUSE_MOTION)
            {

            }
        }
        if(bool doinput = true)
        {
        auto const * const keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_ESCAPE])
        {
            running = false;
        }
        if (keys[SDL_SCANCODE_X])
        {
            inputs[0] = true;
        }
        if (keys[SDL_SCANCODE_Z])
        {
            inputs[1] = true;
        }
        if (keys[SDL_SCANCODE_BACKSPACE])
        {
            inputs[2] = true;
        }
        if (keys[SDL_SCANCODE_RETURN])
        {
            inputs[3] = true;
        }
        if (keys[SDL_SCANCODE_RIGHT])
        {
            inputs[4] = true;
        }
        if (keys[SDL_SCANCODE_LEFT])
        {
            inputs[5] = true;
        }
        if (keys[SDL_SCANCODE_UP])
        {
            inputs[6] = true;
        }
        if (keys[SDL_SCANCODE_DOWN])
        {
            inputs[7] = true;
        }
        if (keys[SDL_SCANCODE_S])
        {
            inputs[8] = true;
        }
        if (keys[SDL_SCANCODE_A])
        {
            inputs[9] = true;
        }
        }


            game.processInputs(inputs);
            game.update(1.0_fx);
            renderer.draw();

            c1 = std::chrono::steady_clock::now();
        }




    }

    return 0;
}
