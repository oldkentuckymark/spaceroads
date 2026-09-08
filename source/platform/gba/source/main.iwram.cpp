#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>


#include <cstdint>

#include "ffr.hpp"
#include "mesh.hpp"
#include "level.hpp"
#include "renderer.hpp"
#include "game.hpp"

#define KEY_A        0x0001
#define KEY_B        0x0002
#define KEY_SELECT   0x0004
#define KEY_START    0x0008
#define KEY_RIGHT    0x0010
#define KEY_LEFT     0x0020
#define KEY_UP       0x0040
#define KEY_DOWN     0x0080
#define KEY_R        0x0100
#define KEY_L        0x0200

#define KEY_MASK     0xFC00


class FFT
{
public:

     auto operator()(ffm::vec3& in) -> void
    {

        using namespace ffm;


        in = in + modelPos - camPos;

    }

    ffm::vec3 camPos{0.0_fx,0.0_fx,0_fx};
    ffm::vec3 modelPos{0.0_fx,0.0_fx,0.0_fx};

};
\


class Context final : public ffr::BaseContext<Context,FFT>
{
public:

    constexpr static uint32_t RENDER_WIDTH = 160;
    constexpr static uint32_t RENDER_HEIGHT = 128;

    Context()
    {
        // Enable Vblank Interrupt to allow VblankIntrWait
        irqEnable(IRQ_VBLANK);
        SetMode( MODE_5 | BG2_ON );

        // 2. Set the top-left pivot point to 0.
        // Shifted into 20.8 format (0 << 8 is still 0).
        REG_BG2X = 0;
        REG_BG2Y = 0;

        // 3. Calculate 8.8 matrix coefficients.
        // We multiply the ratio by 256 to convert a standard decimal to 8.8 fixed-point.
        constexpr int16_t scale_x = static_cast<int16_t>((RENDER_WIDTH * 256) / SCREEN_WIDTH); // 160/240 * 256 = 170 (0x00AA)
        constexpr int16_t scale_y = static_cast<int16_t>((RENDER_HEIGHT * 256) / SCREEN_HEIGHT); // 128/160 * 256 = 204 (0x00CC)

        // 4. Load values into the GBA transform engine registers.
        REG_BG2PA = scale_x; // Horizontal scaling step
        REG_BG2PB = 0;       // Horizontal shearing (none)
        REG_BG2PC = 0;       // Vertical shearing (none)
        REG_BG2PD = scale_y; // Vertical scaling step

        setViewPort(160,128);
        setNearZ(1.0_fx);
    }


     inline void clear()
    {
        for(volatile uint16_t* p = vram;p < vram+(width*height);++p)
        {
            *p = 0;
        }
    }

     inline void present()
    {
        flipPage();
    }

     inline void plot(int16_t x, int16_t y, uint16_t c)
    {
        vram[(y*width)+x] = c;
    }

     inline void lineHorizontal(int16_t x0, int16_t y0, int16_t x1, uint16_t color)
    {
        for(uint16_t* p = (uint16_t*)&vram[y0*width+x0]; p <= &vram[y0*width+x1]; ++p)
        {
            *p = color;
        }
    }

/*
    auto lineHorizontal(int16_t x0, int16_t y0, int16_t x1, uint16_t color) -> void
    {
        using namespace gba;

        // 1. Determine active page frame buffer (Bit 4 of REG_DISPCNT controls backbuffer)
        uint16_t* vram_base = (uint16_t*)0x06000000;
        if (REG_DISPCNT & (1 << 4)) {
            vram_base += (VRAM_PAGE_SIZE / 2); // Shift pointer to Page 1
        }

        // Ensure proper left-to-right alignment
        if (x0 > x1) std::swap(x0, x1);

        // Compute starting memory location for this scanline row
        uint16_t* dest = &vram_base[y0 * MODE5_WIDTH + x0];
        uint32_t pixel_count = x1 - x0 + 1;

        // 2. Fall back to direct CPU write if the line is too short for DMA setup overhead
        if (pixel_count < 6) {
            while (pixel_count--) {
                *dest++ = color;
            }
            return;
        }

        // 3. Align destination address to a 32-bit boundary if it starts on an odd pixel
        if (reinterpret_cast<uintptr_t>(dest) & 2) {
            *dest++ = color;
            pixel_count--;
        }

        // 4. Duplicate the 16-bit BGR555 color across a full 32-bit word (2 pixels)
        // This allows us to double the transfer speed across the system bus.
        volatile uint32_t color32 = (static_cast<uint32_t>(color) << 16) | color;

        // Calculate how many 32-bit blocks (2 pixels each) we can safely transfer
        uint32_t words_to_transfer = pixel_count >> 1;
        uint32_t leftover_pixels   = pixel_count & 1;

        if (words_to_transfer > 0) {
            // Set up the DMA 3 registers
            REG_DMA3SAD = reinterpret_cast<uint32_t>(&color32);
            REG_DMA3DAD = reinterpret_cast<uint32_t>(dest);

            // Execute the DMA transfer immediately
            REG_DMA3CNT = words_to_transfer |
                          DMA_ENABLE |
                          DMA_TIMING_IMMED |
                          DMA_SRC_FIXED |
                          DMA_DST_INC |
                          DMA_32; // [1]

            // Advance our destination pointer past the DMA'd memory block
            dest += (words_to_transfer << 1);
        }

        // 5. Clean up any trailing leftover odd pixel
        if (leftover_pixels) {
            *dest = color;
        }
    }
*/

private:

    // BG2 Affine 2x2 Matrix Scale Registers (8.8 Fixed Point)

    constexpr static uint16_t width = 160;
    constexpr static uint16_t height = 128;

    volatile uint16_t * FB = (uint16_t*)0x6000000;
    volatile uint16_t * BB = (uint16_t*)0x600A000;


    volatile uint16_t * vram = BB;

     inline void flipPage()
    {

        //while(REG_VCOUNT >= 160); // Wait for vertical blank

        if(REG_DISPCNT & 0x10)
        {
            REG_DISPCNT &= ~0x10; // Show Frame 0
            vram = BB;
        }
        else
        {
            REG_DISPCNT |= 0x10;  // Show Frame 1
            vram = FB;
        }
    }

};



/*
class Context final : public ffr::Context<FFT>
{
public:

    Context()
    {
        // Enable Vblank Interrupt to allow VblankIntrWait
        irqEnable(IRQ_VBLANK);
        SetMode( MODE_3 | BG2_ON );
    }

    inline void clear() override
    {
        for(volatile uint16_t* p = vram;p < vram+(width*height);++p)
        {
            *p = 0;
        }
    }

    inline void present() override
    {

    }

    inline void plot(int16_t x, int16_t y, uint16_t c) override
    {
        vram[(y*width)+x] = c;
    }

    inline void lineHorizontal(int16_t x0, int16_t y0, int16_t x1, uint16_t color) override
    {
        if(x0 > x1)
        {
            auto tmp = x0;
            x0 = x1;
            x1 = tmp;
        }

        for(uint16_t* p = (uint16_t*)&vram[(y0*width)+x0]; p <= &vram[(y0*width)+x1]; ++p)
        {
            *p = color;
        }
    }

private:
    constexpr static uint16_t width = 240;
    constexpr static uint16_t height = 160;

    volatile uint16_t * vram = (uint16_t*)0x6000000;

};
*/


uint32_t getKeyState(uint16_t key_code)
{
    return !(key_code & (REG_KEYINPUT | KEY_MASK) );
}

int main(void)
{
    // Set up the interrupt handlers
    irqInit();
    //irqSet( IRQ_VBLANK, VblankInterrupt);

    Game game;
    Renderer<Context> renderer;

    renderer.setPlayer(&game.player());
    renderer.setPlayerMesh(Mesh::SHIP_MESH);
    renderer.setDrawDistance(10);
    renderer.setLevel(&level0);

    std::array<bool,10> inputs{};

    while (true)
    {
        if(bool doinput = true)
        {
            inputs[0] = getKeyState(KEY_A);
            inputs[1] = getKeyState(KEY_B);
            inputs[2] = getKeyState(KEY_SELECT);
            inputs[3] = getKeyState(KEY_START);
            inputs[4] = getKeyState(KEY_RIGHT);
            inputs[5] = getKeyState(KEY_LEFT);
            inputs[6] = getKeyState(KEY_UP);
            inputs[7] = getKeyState(KEY_DOWN);
            inputs[8] = getKeyState(KEY_R);
            inputs[9] = getKeyState(KEY_L);
        }


        game.processInputs(inputs);
        game.update();
        renderer.draw();
    }

    return 0;
}

