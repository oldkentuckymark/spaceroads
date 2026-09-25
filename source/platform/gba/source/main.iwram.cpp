#include <gba.h>

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
    ffm::vec3 modelPos{0.0_fx, 0.0_fx, 0.0_fx};
    ffm::fixed32 camYawSin, camYawCos;
    ffm::fixed32 modelYawSin, modelYawCos;
};


class Context final : public ffr::BaseContext<Context, VertexFunction>
{
public:

    constexpr static uint32_t RENDER_WIDTH = 160;
    constexpr static uint32_t RENDER_HEIGHT = 128;

    Context()
    {
        // Enable Vblank Interrupt to allow VblankIntrWait
        irqEnable(IRQ_VBLANK);
        SetMode( MODE_5 | BG2_ON );

        REG_BG2CNT |= BG_WRAP;

        // Set the top-left pivot point to 0
        REG_BG2X = 0;
        REG_BG2Y = 0;

        // Calculate 8.8 matrix coefficients
        constexpr int16_t scale_x = static_cast<int16_t>((RENDER_WIDTH * 256) / SCREEN_WIDTH);   // 160/240 * 256 = 170 (0x00AA)
        constexpr int16_t scale_y = static_cast<int16_t>((RENDER_HEIGHT * 256) / SCREEN_HEIGHT); // 128/160 * 256 = 204 (0x00CC)

        // Load values into the GBA transform engine registers
        REG_BG2PA = scale_x; // Horizontal scaling step
        REG_BG2PB = 0;       // Horizontal shearing (none)
        REG_BG2PC = 0;       // Vertical shearing (none)
        REG_BG2PD = scale_y; // Vertical scaling step

        setViewPort(RENDER_WIDTH, RENDER_HEIGHT);
        setNearZ(1.0_fx);
    }

    inline void clear(uint16_t color = 0)
    {
        // Duplicate the 16-bit color across a 32-bit word
        volatile uint32_t fill_color32 = (static_cast<uint32_t>(color) << 16) | color;

        // Mode 5 total transfer count: 160 * 128 = 20,480 pixels = 10,240 32-bit words
        constexpr uint16_t WORD_COUNT = (RENDER_WIDTH * RENDER_HEIGHT) / 2;

        // Safe uintptr_t conversion compatible with modern C++ compilers
        REG_DMA3SAD = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(const_cast<uint32_t*>(&fill_color32)));
        REG_DMA3DAD = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(const_cast<uint16_t*>(vram)));

        // DMA32 is the native libgba constant for 32-bit transfer mode
        REG_DMA3CNT = WORD_COUNT | DMA_ENABLE | DMA32 | DMA_SRC_FIXED | DMA_DST_INC;
    }

    inline void present()
    {
        VBlankIntrWait();
        flipPage();
    }

    inline void plot(int16_t x, int16_t y, uint16_t c)
    {
        vram[(y * RENDER_WIDTH) + x] = c;
    }

    inline void lineHorizontal(int16_t x0, int16_t y0, int16_t x1, uint16_t color)
    {
        if(y0 < 0) { return; }
        if(y0 >= viewport_height_) { return; }
        if(x0 > x1) std::swap(x0, x1);


        if(x0 < 0) { x0 = 0; }
        if(x0 >= viewport_width_) { x0 = viewport_width_ - 1; }
        if(x1 < 0) { x1 = 0; }
        if(x1 >= viewport_width_) { x1 = viewport_width_ - 1; }

        uint16_t* p_16 = const_cast<uint16_t*>(&vram[y0 * RENDER_WIDTH + x0]);
        int32_t count = x1 - x0 + 1;

        // 1. Align pointer to 32-bit boundary (4 bytes) if starting on an odd halfword
        if (reinterpret_cast<uintptr_t>(p_16) & 2)
        {
            *p_16++ = color;
            --count;
        }

        // 2. Perform 32-bit writes for pairs of pixels
        if (count >= 2)
        {
            uint32_t const color32 = (static_cast<uint32_t>(color) << 16) | color;
            uint32_t* p_32 = reinterpret_cast<uint32_t*>(p_16);

            int32_t words = count >> 1;
            count &= 1; // Remaining odd pixel

            while (words--)
            {
                *p_32++ = color32;
            }

            p_16 = reinterpret_cast<uint16_t*>(p_32);
        }

        // 3. Handle trailing leftover pixel
        if (count > 0)
        {
            *p_16 = color;
        }
    }

private:

    volatile uint16_t * FB = (uint16_t*)0x6000000;
    volatile uint16_t * BB = (uint16_t*)0x600A000;

    volatile uint16_t * vram = BB;

    inline void flipPage()
    {
        if (REG_DISPCNT & 0x10)
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




uint32_t getKeyState(uint16_t key_code)
{
    return !(key_code & (REG_KEYINPUT | KEY_MASK) );
}



class Timer {
public:
    // Q16.16 fixed-point type (public data member as requested)
    struct fixed32 {
        int32_t data; // Q16.16 value: integer part in high 16 bits, fractional in low 16 bits
    };

    Timer() noexcept
    {
        stop();
        *TM0CNT_L = 0;
        *TM0CNT_H = 0;
        m_overflow = 0;
        m_prevLow = 0;
        m_lastTicks = 0;
    }

    // Start Timer0 free-running with prescaler = 1 (tick = 1 / CPU_HZ)
    void start() noexcept
    {
        *TM0CNT_L = 0;
        m_overflow = 0;
        m_prevLow = 0;
        m_lastTicks = 0;
        *TM0CNT_H = static_cast<std::uint16_t>(TIMER_PRESCALE_1 | TIMER_ENABLE);
    }

    // Stop Timer0
    void stop() noexcept
    {
        std::uint16_t ctrl = *TM0CNT_H;
        ctrl &= static_cast<std::uint16_t>(~TIMER_ENABLE);
        *TM0CNT_H = ctrl;
    }

    // Return elapsed time since last getdt() as Q16.16 fixed-point (fixed32).
    // The returned fixed32.data holds seconds in Q16.16 format.
    fixed32 getdt() noexcept
    {
        const std::uint32_t ticks = readTicks32();
        const std::uint32_t prev = m_lastTicks;
        const std::uint32_t delta = ticks - prev; // unsigned wrap handles natural wrap-around
        m_lastTicks = ticks;

        // Convert delta ticks to Q16.16 seconds.
        // On GBA CPU_HZ = 16,777,216 = 2^24. So:
        // fixed = (delta * 2^16) / 2^24 = delta / 2^8 = delta >> 8
        // This avoids any division or floating point.
        fixed32 out;
        out.data = static_cast<int32_t>(delta >> SHIFT_TICKS_TO_Q16); // delta >> 8
        return out;
    }

    // Optional: total seconds since start as Q16.16 (does not modify last-tick marker)
    fixed32 totalSecondsFixed() noexcept
    {
        const std::uint32_t ticks = readTicks32();
        fixed32 out;
        out.data = static_cast<int32_t>(ticks >> SHIFT_TICKS_TO_Q16);
        return out;
    }

private:
    // Read a consistent 32-bit tick count by polling the 16-bit hardware counter
    // and updating a software overflow counter when the 16-bit counter wraps.
    // Note: if the 16-bit counter wraps multiple times between reads, extra wraps
    // will not be detected. Call getdt() frequently enough (at least once per 65536 ticks).
    std::uint32_t readTicks32() noexcept
    {
        const std::uint16_t low = *TM0CNT_L;

        // Detect wrap relative to previous low value
        if (low < m_prevLow) {
            ++m_overflow;
        }

        m_prevLow = low;

        return (static_cast<std::uint32_t>(m_overflow) << 16) | static_cast<std::uint32_t>(low);
    }

private:
    // Hardware registers (memory-mapped IO)
    static inline volatile std::uint16_t* const TM0CNT_L =
        reinterpret_cast<volatile std::uint16_t*>(0x04000100);
    static inline volatile std::uint16_t* const TM0CNT_H =
        reinterpret_cast<volatile std::uint16_t*>(0x04000102);

    // All constants inside the class
    static constexpr std::uint16_t TIMER_ENABLE       = static_cast<std::uint16_t>(1u << 7);
    static constexpr std::uint16_t TIMER_PRESCALE_1   = static_cast<std::uint16_t>(0u << 0);

    // GBA CPU frequency: 16,777,216 Hz = 2^24
    // SHIFT_TICKS_TO_Q16 = 24 - 16 = 8, so delta >> 8 yields Q16.16 seconds
    static constexpr std::uint32_t CPU_HZ = 16'777'216u;
    static constexpr int SHIFT_TICKS_TO_Q16 = 8;

    // Software state (no atomics; single-threaded / non-interrupt use)
    std::uint32_t m_overflow{0};   // number of times the 16-bit timer wrapped
    std::uint16_t m_prevLow{0};    // previous low counter value for wrap detection
    std::uint32_t m_lastTicks{0};  // last tick count returned by getdt()
};


int main(void)
{
    // Set up the interrupt handlers
    irqInit();
    //irqSet( IRQ_VBLANK, VblankInterrupt);

    Game game;
    Renderer<Context> renderer;
    //FixedPointTimer timer(0);
    //timer.start(FixedPointTimer::PRESCALER_DIVS[1]);

    renderer.setPlayer(&game.player());
    renderer.setPlayerMesh(Mesh::SHIP_MESH);
    //renderer.setDrawDistance(10);
    renderer.setLevel(&level0);
    renderer.setCamera(game.getCamera());

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
        game.update(1.0_fx);
        renderer.draw();
    }

    return 0;
}

