#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "mesh.hpp"
#include "cell.hpp"
#include "player.hpp"
#include "level.hpp"
#include "ffr.hpp"

#include <span>
#include <meta>
#include <ranges>

template <class Context>
class Renderer
{
public:

    constexpr static uint32_t DRAW_DISTANCE{8};

    Renderer()
    {


    }

    ~Renderer()
    {

    }

    auto setPlayer(Player* p) -> void
    {
        current_player_ = p;
    }

    auto setPlayerMesh(std::span<Vertex const> m) -> void
    {
        current_ship_mesh_ = m;
    }

    auto setLevel(ILevel const * level) -> void
    {
        current_level_ = level;
    }

    auto draw() -> void
    {

        ctx.setFaceCulling(ffr::FaceCullMode::Back);
        ctx.clear();
        auto& campos = ctx.getVertexFunction().camPos;
        auto& camyaw = ctx.getVertexFunction().camRot.y;
        campos = current_player_->position + ffm::vec3{0.0_fx,2.0_fx,-2.8_fx};

        int16_t const pz = static_cast<int16_t>(current_player_->position.z) / 2;
        int16_t maxz = pz + DRAW_DISTANCE;
        if(maxz >= current_level_->getLength()) { maxz = current_level_->getLength() - 1; }
        int16_t minz = pz;
        if(minz < 0) { minz = 0; }
        int16_t px = current_player_->position.x;
        if (px < 0) { px = 0; }
        if (px > 8) { px = 8; }


        auto rf = castRayField({campos.x,campos.y}, camyaw, current_level_);
        auto cl = getVisibleMeshesBackToFront(rf);

        for(VisitedMeshCell const& vmc : cl)
        {
            auto w = std::get<0>(vmc);
            auto l = std::get<1>(vmc);

            auto const cell = current_level_->getCell(w,l);
            auto const colptr = current_level_->getCellColorBufferPtr(w,l);

            ctx.getVertexFunction().modelPos = ffm::vec3{ffm::fixed32(w),0.0_fx,ffm::fixed32(l*2)};
            ctx.setColorPointer(0,colptr);
            ctx.setVertexPointer(3,sizeof(Vertex), Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision) ].data());
            ctx.drawArray(ffr::DrawType::Quads, 0, Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision) ].size());

        }




        ctx.getVertexFunction().modelPos = current_player_->position;
        ctx.setColorPointer(sizeof(Vertex),&Mesh::SHIP_MESH[0].color);
        ctx.setVertexPointer(3,sizeof(Vertex),&Mesh::SHIP_MESH[0].position);
        ctx.drawArray(ffr::DrawType::Quads,0,Mesh::SHIP_MESH.size());



        ctx.present();
    }

private:
    Context ctx;

    ILevel const * current_level_{nullptr};

    Player* current_player_{nullptr};
    std::span<Vertex const> current_ship_mesh_{};

    constexpr static uint32_t NUM_RAYS{33};
    constexpr static uint32_t MAX_DEDUP_BLOCKS = NUM_RAYS * DRAW_DISTANCE;

    using VisitedCell = std::tuple<int16_t, int16_t, ffm::fixed32>;
    using RayResult   = std::inplace_vector<VisitedCell, DRAW_DISTANCE>;
    using VisitedMeshCell = std::tuple<int16_t, int16_t, ffm::fixed32>;

    template<size_t N = 64>
    static consteval auto makedXdYTable() -> std::array<ffm::vec2, N>
    {
        std::array<ffm::vec2, N> r{};
        constexpr double step = (std::numbers::pi * 2.0) / static_cast<double>(N);

        for (size_t i = 0; i < N; ++i)
        {
            double theta = step * static_cast<double>(i);
            r[i] = ffm::vec2(
                ffm::fixed32(std::sin(theta)), // X component (+X is Right, -X is Left)
                ffm::fixed32(std::cos(theta))  // Y/Z component (+Z is Forward)
                );
        }
        return r;
    }

    template<size_t N = 64>
    static consteval auto makeInvdXdYTable() -> std::array<ffm::vec2, N>
    {
        std::array<ffm::vec2, N> r{};
        constexpr double step = (std::numbers::pi * 2.0) / static_cast<double>(N);

        // Maximum safe float representation that fits inside a signed Q16.16 fixed32
        // INT32_MAX (2147483647) / 65536.0 ≈ 32767.9999
        constexpr double maxFixedVal = 32767.0;

        for (size_t i = 0; i < N; ++i)
        {
            double theta = step * static_cast<double>(i);
            double dx = std::sin(theta);
            double dy = std::cos(theta);

            // Guard against division by zero and cap to max fixed32 capacity
            double invX = (std::abs(dx) > 1e-9) ? (1.0 / dx) : ((dx < 0) ? -maxFixedVal : maxFixedVal);
            double invY = (std::abs(dy) > 1e-9) ? (1.0 / dy) : ((dy < 0) ? -maxFixedVal : maxFixedVal);

            // Clamp extreme values if reciprocal slightly exceeds maxFixedVal range
            invX = std::clamp(invX, -maxFixedVal, maxFixedVal);
            invY = std::clamp(invY, -maxFixedVal, maxFixedVal);

            r[i] = ffm::vec2(
                ffm::fixed32(invX),
                ffm::fixed32(invY)
                );
        }
        return r;
    }

    auto static constexpr dxDyTable = makedXdYTable();
    auto static constexpr invDxDyTable = makeInvdXdYTable();

    template<size_t N = 64>
    static constexpr auto getdXdYfromYaw(ffm::fixed32 const yawGamDegs) -> ffm::vec2
    {
        static constexpr auto dXdYTable = makedXdYTable<N>();

        // Direct power-of-two bitmask indexing for efficient lookup
        size_t index = static_cast<size_t>(yawGamDegs) & (N - 1);
        return dXdYTable[index];
    }

    template<size_t N = 64>
    static constexpr auto getInvdXdYfromYaw(ffm::fixed32 const yawGamDegs) -> ffm::vec2
    {
        static constexpr auto invdXdYTable = makeInvdXdYTable<N>();

        size_t index = static_cast<size_t>(yawGamDegs) & (N - 1);
        return invdXdYTable[index];
    }

    [[nodiscard]] constexpr static auto gamDegsToLutIndex(ffm::fixed32 const yawGamDegs) -> size_t
    {
        // Extract integer GAMDEGS from Q16.16
        int32_t const rawGamDegs = yawGamDegs.data >> 16;

        // Proper positive modulo for 64-entry LUT (handles negative numbers correctly)
        return static_cast<size_t>((rawGamDegs % 64 + 64) % 64);
    }

    [[nodiscard]] auto raycastDDA(
        ffm::vec2 const cameraPos,
        ffm::fixed32 const yawInGamDegs,
        ILevel const* level
        ) -> std::inplace_vector<VisitedCell, DRAW_DISTANCE>
    {
        std::inplace_vector<VisitedCell, DRAW_DISTANCE> encounteredBlocks;
        uint32_t const levelLength = static_cast<uint32_t>(level->getLength());

        // Positive modulo 0..63 table indexing
        int32_t const rawGamDegs = yawInGamDegs.data >> 16;
        size_t const angleIdx = static_cast<size_t>((rawGamDegs % 64 + 64) % 64);

        ffm::vec2 const dir = dxDyTable[angleIdx];
        ffm::vec2 const invDir = invDxDyTable[angleIdx];

        if (dir.x.data == 0 && dir.y.data == 0) [[unlikely]] {
            return encounteredBlocks;
        }

        // Grid coordinates
        int mapX = cameraPos.x.data >> 16;
        int mapZ = cameraPos.y.data >> 17; // Cell height = 2.0 (128k units)

        // Delta Distances
        ffm::fixed32 deltaDistX;
        ffm::fixed32 deltaDistZ;
        deltaDistX.data = (invDir.x.data != 0) ? std::abs(invDir.x.data) : ffm::fixed32::max().data;

        int32_t const absInvY = std::abs(invDir.y.data);
        deltaDistZ.data = (absInvY != 0) ? (absInvY << 1) : ffm::fixed32::max().data;

        // Fractional position inside current 1.0 x 2.0 cell
        uint32_t const rawFracX = static_cast<uint32_t>(cameraPos.x.data) & 0xFFFF;
        uint32_t const rawFracZ = static_cast<uint32_t>(cameraPos.y.data) & 0x1FFFF;

        int stepX = 0;
        int stepZ = 0;
        ffm::fixed32 sideDistX;
        ffm::fixed32 sideDistZ;

        // --- SYMMETRIC X-AXIS INITIALIZATION ---
        if (dir.x.data < 0) {
            stepX = -1;
            uint32_t const distToLeft = (rawFracX == 0) ? 0x10000 : rawFracX;
            sideDistX.data = static_cast<int32_t>((static_cast<int64_t>(distToLeft) * deltaDistX.data) >> 16);
        } else {
            stepX = 1;
            uint32_t const distToRight = 0x10000 - rawFracX;
            sideDistX.data = static_cast<int32_t>((static_cast<int64_t>(distToRight) * deltaDistX.data) >> 16);
        }

        // --- SYMMETRIC Z-AXIS INITIALIZATION ---
        if (dir.y.data < 0) {
            stepZ = -1;
            uint32_t const distToBottom = (rawFracZ == 0) ? 0x20000 : rawFracZ;
            sideDistZ.data = static_cast<int32_t>((static_cast<int64_t>(distToBottom) * deltaDistZ.data) >> 17);
        } else {
            stepZ = 1;
            uint32_t const distToTop = 0x20000 - rawFracZ;
            sideDistZ.data = static_cast<int32_t>((static_cast<int64_t>(distToTop) * deltaDistZ.data) >> 17);
        }

        // DDA Step Loop
        for (uint32_t step = 0; step < DRAW_DISTANCE; ++step)
        {
            if (sideDistX.data < sideDistZ.data) {
                sideDistX.data += deltaDistX.data;
                mapX += stepX;
            } else {
                sideDistZ.data += deltaDistZ.data;
                mapZ += stepZ;
            }

            if (static_cast<uint32_t>(mapX) >= static_cast<uint32_t>(ILevel::LEVEL_WIDTH) ||
                static_cast<uint32_t>(mapZ) >= levelLength) [[unlikely]]
            {
                break;
            }

            if (level->getCell(mapX, mapZ).collision != Cell::Collision::Empty)
            {
                ffm::fixed32 blockWorldX;
                ffm::fixed32 blockWorldZ;
                blockWorldX.data = (mapX << 16) + 0x8000;
                blockWorldZ.data = (mapZ << 17) + 0x10000;

                ffm::fixed32 const dx = blockWorldX - cameraPos.x;
                ffm::fixed32 const dz = blockWorldZ - cameraPos.y;

                encounteredBlocks.emplace_back(
                    static_cast<int16_t>(mapX),
                    static_cast<int16_t>(mapZ),
                    (dx * dx) + (dz * dz)
                    );

                if (encounteredBlocks.size() == DRAW_DISTANCE) [[unlikely]] {
                    break;
                }
            }
        }

        return encounteredBlocks;
    }

    [[nodiscard]] auto castRayField(
        ffm::vec2 const cameraPos,
        ffm::fixed32 const centerYawInGamDegs,
        ILevel const* level
        ) -> std::inplace_vector<RayResult, NUM_RAYS>
    {
        std::inplace_vector<RayResult, NUM_RAYS> rayField;

        // Ray 0:   centerYaw - 8.0 GAMDEGS (-45.0°)
        // Ray 16:  centerYaw + 0.0 GAMDEGS ( 0.0° - Center)
        // Ray 32:  centerYaw + 8.0 GAMDEGS (+45.0°)
        ffm::fixed32 currentYaw = centerYawInGamDegs - 8_fx;

        for (uint32_t i = 0; i < NUM_RAYS; ++i)
        {
            rayField.unchecked_emplace_back(
                raycastDDA(cameraPos, currentYaw, level)
                );

            // 0.5 GAMDEG step = 0x8000 in Q16.16
            currentYaw.data += 0x8000;
        }

        return rayField;
    }

    [[nodiscard]] auto getVisibleMeshesBackToFront(
        std::inplace_vector<RayResult, NUM_RAYS> const& rayField
        ) -> std::inplace_vector<VisitedMeshCell, MAX_DEDUP_BLOCKS>
    {
        std::inplace_vector<VisitedMeshCell, MAX_DEDUP_BLOCKS> visibleMeshes;

        for (auto const& ray : rayField)
        {
            for (auto const& [gridX, gridZ, distSq] : ray)
            {
                int32_t const newDist = distSq.data;

                // 1. Deduplication Check
                // Scan existing entries to see if this grid cell was already added
                bool alreadyExists = false;
                for (size_t i = 0; i < visibleMeshes.size(); ++i)
                {
                    auto& [x, z, d] = visibleMeshes[i];
                    if (x == gridX && z == gridZ)
                    {
                        alreadyExists = true;
                        // If this ray found a closer distance to the block, update its distance metric
                        if (newDist < d.data)
                        {
                            d.data = newDist;

                            // Re-sort in-place: bubble forward if the updated distance is now smaller
                            size_t curr = i;
                            while (curr + 1 < visibleMeshes.size() &&
                                   std::get<2>(visibleMeshes[curr]).data < std::get<2>(visibleMeshes[curr + 1]).data)
                            {
                                std::swap(visibleMeshes[curr], visibleMeshes[curr + 1]);
                                ++curr;
                            }
                        }
                        break;
                    }
                }

                if (alreadyExists) continue;

                // 2. Ordered Insertion (Back-to-Front / Descending Distance)
                // Find insertion index: keep array sorted descending by distSq.data
                size_t insertPos = visibleMeshes.size();
                while (insertPos > 0 && std::get<2>(visibleMeshes[insertPos - 1]).data < newDist)
                {
                    --insertPos;
                }

                // Insert element into correct sorted position directly
                visibleMeshes.insert(visibleMeshes.begin() + insertPos, VisitedMeshCell{gridX, gridZ, distSq});
            }
        }

        return visibleMeshes;
    }

};


#endif // RENDERER_HPP
