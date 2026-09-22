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
        using namespace ffm;

        // ==========================================
        // 1. MODEL TRANSFORMATION (Model Space -> World Space)
        // ==========================================
        fixed32 const mcx = cosgd(modelRotation.x);
        fixed32 const msx = singd(modelRotation.x);

        fixed32 const mcy = cosgd(modelRotation.y);
        fixed32 const msy = singd(modelRotation.y);

        fixed32 const mcz = cosgd(modelRotation.z);
        fixed32 const msz = singd(modelRotation.z);

        // --- Model Rotate X ---
        vec3 rx;
        rx.x = in.x;
        rx.y = in.y * mcx - in.z * msx;
        rx.z = in.y * msx + in.z * mcx;

        // --- Model Rotate Y ---
        vec3 ry;
        ry.x = rx.x * mcy + rx.z * msy;
        ry.y = rx.y;
        ry.z = -rx.x * msy + rx.z * mcy;

        // --- Model Rotate Z ---
        vec3 rz;
        rz.x = ry.x * mcz - ry.y * msz;
        rz.y = ry.x * msz + ry.y * mcz;
        rz.z = ry.z;

        // Translate to Camera Relative Position in World Space
        vec3 worldPos = rz + modelPos - camPos;

        // ==========================================
        // 2. CAMERA VIEW TRANSFORMATION (World Space -> View Space)
        // Strict Order: Pitch (X) -> Yaw (Y). Roll (Z) is strictly omitted.
        // ==========================================
        fixed32 const ccx = cosgd(camRot.x); // Pitch cos
        fixed32 const csx = singd(camRot.x); // Pitch sin

        fixed32 const ccy = cosgd(camRot.y); // Yaw cos
        fixed32 const csy = singd(camRot.y); // Yaw sin

        // --- Step 1: Camera Inverse Pitch (-camRot.x) ---
        vec3 crx;
        crx.x = worldPos.x;
        crx.y = worldPos.y * ccx + worldPos.z * csx;
        crx.z = -worldPos.y * csx + worldPos.z * ccx;

        // --- Step 2: Camera Inverse Yaw (-camRot.y) ---
        // Matches standard CCW Cartesian table convention (X = cos, Z = sin)
        vec3 cry;
        cry.x =  crx.x * ccy + crx.z * csy;
        cry.y =  crx.y;
        cry.z = -crx.x * csy + crx.z * ccy;

        // Output final View Space vertex
        in = cry;
    }

    ffm::vec3 camPos{0.0_fx, 0.0_fx, 0.0_fx};
    ffm::vec3 camRot{0.0_fx, 0.0_fx, 0.0_fx}; // x = pitch, y = yaw (z is ignored)
    ffm::vec3 modelPos{0.0_fx, 0.0_fx, 0.0_fx};
    ffm::vec3 modelRotation{0.0_fx, 0.0_fx, 0.0_fx};
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
    constexpr ffm::fixed32 dt = ffm::fixed32(1.0/60.0);
    auto const * const lp = &level0;


    auto c1 = std::chrono::steady_clock::now();
    auto c2 = c1;

    Game game;
    Renderer<Context> renderer;

    renderer.setPlayer(&game.player());
    renderer.setPlayerMesh(Mesh::SHIP_MESH);
    renderer.setLevel(&level0);



    bool running = true;
    while (running)
    {
        std::array<bool,10> inputs{};
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

            renderer.ctx.getVertexFunction().camRot.y = renderer.ctx.getVertexFunction().camRot.y - 0.0001_fx;
        }
        if (keys[SDL_SCANCODE_A])
        {
            inputs[9] = true;

            renderer.ctx.getVertexFunction().camRot.y = renderer.ctx.getVertexFunction().camRot.y + 0.0001_fx;
        }
        }


        c2 = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>( c2.time_since_epoch()-c1.time_since_epoch()).count() >= 16)
        {

            game.processInputs(inputs);
            game.update(dt);
            renderer.draw();

            auto dt = static_cast<uint16_t>(std::chrono::duration_cast<std::chrono::milliseconds>(c2-c1).count());
            c1 = std::chrono::steady_clock::now();
        }




    }

    return 0;
}
