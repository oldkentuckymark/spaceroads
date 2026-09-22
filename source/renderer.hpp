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

    constexpr static uint32_t DRAW_DISTANCE{10};

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


        auto rf = castRayField({campos.x,campos.z}, camyaw, current_level_);
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

        Context ctx;
private:


    ILevel const * current_level_{nullptr};

    Player* current_player_{nullptr};
    std::span<Vertex const> current_ship_mesh_{};

    constexpr static uint32_t NUM_RAYS{17};
    constexpr static uint32_t MAX_DEDUP_BLOCKS = NUM_RAYS * DRAW_DISTANCE;

    using VisitedCell = std::tuple<int16_t, int16_t, ffm::fixed32>;
    using RayResult   = std::inplace_vector<VisitedCell, DRAW_DISTANCE>;
    using VisitedMeshCell = std::tuple<int16_t, int16_t, ffm::fixed32>;

    static consteval auto makedXdYTable() -> std::array<ffm::vec2, ffm::GAMDEG_IN_CIRCLE>
    {
        std::array<ffm::vec2, ffm::GAMDEG_IN_CIRCLE> r{};
        constexpr double step = (std::numbers::pi * 2.0) / static_cast<double>(ffm::GAMDEG_IN_CIRCLE);
        constexpr double offset = step * 0.1;

        for (size_t i = 0; i < ffm::GAMDEG_IN_CIRCLE; ++i)
        {
            double const theta = (step * static_cast<double>(i)) + offset;

            r[i] = ffm::vec2(
                ffm::fixed32(std::sin(theta)),
                ffm::fixed32(std::cos(theta))
                );
        }
        return r;
    }

    static consteval auto makeInvdXdYTable() -> std::array<ffm::vec2, ffm::GAMDEG_IN_CIRCLE>
    {
        std::array<ffm::vec2, ffm::GAMDEG_IN_CIRCLE> r{};
        constexpr double step = (std::numbers::pi * 2.0) / static_cast<double>(ffm::GAMDEG_IN_CIRCLE);
        constexpr double offset = step * 0.1;

        constexpr double MAX_SAFE_INV = 30000.0;

        for (size_t i = 0; i < ffm::GAMDEG_IN_CIRCLE; ++i)
        {
            double const theta = (step * static_cast<double>(i)) + offset;
            double const dx = std::sin(theta);
            double const dz = std::cos(theta);

            // Clamp inverse values to prevent Q16.16 overflow near 0
            double const invdx = std::clamp(1.0 / dx, -MAX_SAFE_INV, MAX_SAFE_INV);
            double const invdz = std::clamp(1.0 / dz, -MAX_SAFE_INV, MAX_SAFE_INV);

            r[i] = ffm::vec2(
                ffm::fixed32(1.0 / invdx),
                ffm::fixed32(1.0 / invdz)
                );
        }
        return r;
    }

    auto static constexpr dxDyTable = makedXdYTable();
    auto static constexpr invDxDyTable = makeInvdXdYTable();


    [[nodiscard]] auto castRayField(
        ffm::vec2 const cameraPos,
        ffm::fixed32 const yawInGamDegs,
        ILevel const* level
        ) -> std::array<std::inplace_vector<VisitedCell, DRAW_DISTANCE>, NUM_RAYS>
    {
        std::array<std::inplace_vector<VisitedCell, DRAW_DISTANCE>, NUM_RAYS> rayField;

        //ffm::fixed32 ggg{.data = 1024};

        int const levelLength = static_cast<int>(level->getLength());
        constexpr int LEVEL_WIDTH = static_cast<int>(ILevel::LEVEL_WIDTH);

        constexpr int32_t CELL_SIZE_X = 65536;   // 1.0_fx
        constexpr int32_t CELL_SIZE_Z = 131072;  // 2.0_fx

        auto const floorDiv = [](int32_t val, int32_t div) -> int {
            if (val >= 0) return val / div;
            return (val - div + 1) / div;
        };

        auto const posMod = [](int32_t val, int32_t div) -> uint32_t {
            int32_t const r = val % div;
            return static_cast<uint32_t>(r < 0 ? r + div : r);
        };

        int32_t const camX = static_cast<int32_t>(cameraPos.x.data);
        int32_t const camZ = static_cast<int32_t>(cameraPos.y.data);

        int const baseMapX = floorDiv(camX, CELL_SIZE_X);
        int const baseMapZ = floorDiv(camZ, CELL_SIZE_Z);

        uint32_t const rawFracX = posMod(camX, CELL_SIZE_X);
        uint32_t const rawFracZ = posMod(camZ, CELL_SIZE_Z);

        // Normalize input yaw to a safe positive Q16.16 range [0, 256 << 16)
        constexpr int32_t FULL_CIRCLE_RAW = 256 << 16;
        int32_t normalizedYaw = yawInGamDegs.data % FULL_CIRCLE_RAW;
        if (normalizedYaw < 0) normalizedYaw += FULL_CIRCLE_RAW;

        // 90-degree FOV span = 64 GamDegs out of 256 (Half-span = 32 in Q16.16)
        constexpr int32_t HALF_FOV_RAW = 32 << 16;
        int32_t const leftAngleRaw = normalizedYaw - HALF_FOV_RAW;

        int32_t const fovStepRaw = (NUM_RAYS > 1)
                                       ? ((64 << 16) / static_cast<int32_t>(NUM_RAYS - 1))
                                       : 0;

        for (uint32_t rayIdx = 0; rayIdx < NUM_RAYS; ++rayIdx)
        {
            auto& encounteredBlocks = rayField[rayIdx];

            int32_t rayAngleRaw = leftAngleRaw + (fovStepRaw * static_cast<int32_t>(rayIdx));
            rayAngleRaw %= FULL_CIRCLE_RAW;
            if (rayAngleRaw < 0) rayAngleRaw += FULL_CIRCLE_RAW;

            int32_t const rawGamDegs = rayAngleRaw >> 16;
            size_t const angleIdx = static_cast<size_t>((rawGamDegs >> 2) % 64);

            ffm::vec2 const dir = dxDyTable[angleIdx];
            ffm::vec2 const invDir = invDxDyTable[angleIdx];

            // Fixed-point delta distances (scaled for anisotropic CELL_SIZE_Z = 2x CELL_SIZE_X)
            int64_t const fixedDeltaX = static_cast<int64_t>(std::abs(invDir.x.data));
            int64_t const fixedDeltaZ = static_cast<int64_t>(std::abs(invDir.y.data)) * 2;

            int stepX = 0;
            int stepZ = 0;
            int64_t sideDistX = 0;
            int64_t sideDistZ = 0;

            if (dir.x.data < 0) {
                stepX = -1;
                uint32_t const distToBoundary = rawFracX;
                sideDistX = (static_cast<uint64_t>(distToBoundary) * static_cast<uint64_t>(fixedDeltaX)) >> 16;
            } else {
                stepX = 1;
                uint32_t const distToBoundary = CELL_SIZE_X - rawFracX;
                sideDistX = (static_cast<uint64_t>(distToBoundary) * static_cast<uint64_t>(fixedDeltaX)) >> 16;
            }

            if (dir.y.data < 0) {
                stepZ = -1;
                uint32_t const distToBoundary = rawFracZ;
                sideDistZ = (static_cast<uint64_t>(distToBoundary) * static_cast<uint64_t>(fixedDeltaZ)) >> 16;
            } else {
                stepZ = 1;
                uint32_t const distToBoundary = CELL_SIZE_Z - rawFracZ;
                sideDistZ = (static_cast<uint64_t>(distToBoundary) * static_cast<uint64_t>(fixedDeltaZ)) >> 16;
            }

            int mapX = baseMapX;
            int mapZ = baseMapZ;

            auto const inspectCell = [&](int x, int z) {
                if (x >= 0 && x < LEVEL_WIDTH && z >= 0 && z < levelLength) {
                    if (level->getCell(x, z).collision != Cell::Collision::Empty) {
                        if (encounteredBlocks.size() < DRAW_DISTANCE) {
                            ffm::fixed32 blockWorldX;
                            ffm::fixed32 blockWorldZ;

                            blockWorldX.data = (x * CELL_SIZE_X) + (CELL_SIZE_X / 2);
                            blockWorldZ.data = (z * CELL_SIZE_Z) + (CELL_SIZE_Z / 2);

                            ffm::fixed32 const dx = blockWorldX - cameraPos.x;
                            ffm::fixed32 const dz = blockWorldZ - cameraPos.y;

                            encounteredBlocks.emplace_back(
                                static_cast<int16_t>(x),
                                static_cast<int16_t>(z),
                                (dx * dx) + (dz * dz)
                                );
                        }
                    }
                }
            };

            inspectCell(mapX, mapZ);

            for (uint32_t step = 0; step < DRAW_DISTANCE; ++step)
            {
                // Use 64-bit comparison and accumulation to prevent int32 overflow near cardinal axes
                if (sideDistX < sideDistZ) {
                    sideDistX += fixedDeltaX;
                    mapX += stepX;
                } else {
                    sideDistZ += fixedDeltaZ;
                    mapZ += stepZ;
                }

                if (mapX < -1 || mapX > LEVEL_WIDTH + 1 || mapZ < -1 || mapZ > levelLength + 1) [[unlikely]] {
                    break;
                }

                inspectCell(mapX, mapZ);
            }
        }

        return rayField;
    }

    [[nodiscard]] auto getVisibleMeshesBackToFront(
        std::array<RayResult, NUM_RAYS> const& rayField
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
