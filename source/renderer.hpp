#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "mesh.hpp"
#include "cell.hpp"
#include "player.hpp"
#include "level.hpp"
#include "ffr.hpp"
#include "camera.hpp"

#include <span>
#include <meta>
#include <ranges>

template <class Context>
class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    auto setPlayer(Player* p) -> void { current_player_ = p; }
    auto setPlayerMesh(std::span<Vertex const> m) -> void { current_ship_mesh_ = m; }
    auto setLevel(ILevel const* level) -> void { current_level_ = level; }
    auto setCamera(Camera * camera) { current_camera_ = camera; }

    auto draw() -> void
    {
        //if (!current_level_ || !current_player_) [[unlikely]] return;

        ctx.setFaceCulling(ffr::FaceCullMode::Back);
        ctx.clear();

        // 1. Update camera position and view angle
        auto& vfn = ctx.getVertexFunction();

        vfn.camPos = current_camera_->position;
        vfn.camYawSin = ffm::singd(current_camera_->yaw);
        vfn.camYawCos = ffm::cosgd(current_camera_->yaw);

        // 2. Populate member buffer in-place to avoid stack allocation
        castAndProcessRayField(current_camera_->position.x, current_camera_->position.z, current_camera_->yaw);

        // 3. Render visible cells with state caching
        void const* last_vertex_ptr = nullptr;

        for (RayHit const hit : hits_buffer_)
        {
            Cell const& cell = current_level_->getCell(hit.w, hit.l);
            auto const mesh_idx = static_cast<size_t>(cell.collision);
            auto const& mesh = Mesh::CELL_MESHES[mesh_idx];

            // Direct Q16.16 shift (Cell width = 1.0_fx -> shift 16, Cell length = 2.0_fx -> shift 17)
            vfn.modelPos = ffm::vec3{
                ffm::fixed32::fromRaw(static_cast<int32_t>(hit.w) << 16),
                0.0_fx,
                ffm::fixed32::fromRaw(static_cast<int32_t>(hit.l) << 17)
            };

            ctx.setColorPointer(0, current_level_->getCellColorBufferPtr(hit.w, hit.l));
            ctx.setVertexPointer(3, sizeof(Vertex), &mesh[0].position);
            ctx.drawArray(ffr::DrawType::Quads, 0, mesh.size());
        }

        vfn.modelPos = current_player_->position;
        ctx.setColorPointer(sizeof(Vertex), &current_ship_mesh_[0].color);
        ctx.setVertexPointer(3, sizeof(Vertex), &current_ship_mesh_[0].position);
        ctx.drawArray(ffr::DrawType::Triangles, 0, current_ship_mesh_.size());

        ctx.present();
    }

    Context ctx;

private:

    struct RayHit
    {
        int16_t w;
        int16_t l;
        ffm::fixed32 distance;
    };

    struct RayStepLUT
    {
        ffm::fixed32 dx;
        ffm::fixed32 dz;
        ffm::fixed32 invdx;
        ffm::fixed32 invdz;
    };


    ILevel const* current_level_{nullptr};
    Player* current_player_{nullptr};
    std::span<Vertex const> current_ship_mesh_{};
    Camera* current_camera_{nullptr};

    static constexpr size_t DRAW_DISTANCE  = 8;
    static constexpr size_t NUM_RAYS       = 19;
    static constexpr size_t MAX_FIELD_HITS = NUM_RAYS * DRAW_DISTANCE;
    using DeduplicatedRayHits = std::inplace_vector<RayHit, MAX_FIELD_HITS>;
    DeduplicatedRayHits hits_buffer_{};


    static consteval auto makeRayStepTable() -> std::array<RayStepLUT, ffm::GAMDEG_IN_CIRCLE>
    {
        std::array<RayStepLUT, ffm::GAMDEG_IN_CIRCLE> lut{};
        for (std::size_t i = 0; i < ffm::GAMDEG_IN_CIRCLE; ++i)
        {
            double const theta = ((2.0 * std::numbers::pi) / static_cast<double>(ffm::GAMDEG_IN_CIRCLE)) * (static_cast<double>(i) + 0.5);
            double const dx = std::sin(theta);
            double const dz = std::cos(theta);

            lut[i] = RayStepLUT{
                .dx    = ffm::fixed32(dx),
                .dz    = ffm::fixed32(dz),
                .invdx = ffm::fixed32(1.0 / dx),
                .invdz = ffm::fixed32(1.0 / dz)
            };
        }
        return lut;
    }

    alignas(4) static constexpr auto rayStepTable = makeRayStepTable();

    auto castAndProcessRayField(ffm::fixed32 camX, ffm::fixed32 camZ, ffm::fixed32 yaw) -> void
    {
        hits_buffer_.clear();

        int16_t const width = current_level_->getWidth();
        int16_t const length = current_level_->getLength();

        auto const levelMaxX = ffm::fixed32::fromRaw(static_cast<int32_t>(width) << 16);
        auto const levelMaxZ = ffm::fixed32::fromRaw(static_cast<int32_t>(length) << 17);

        uint32_t visitedBitset[128] = {0};

        auto const startAngle = yaw - 36.0_fx;
        constexpr ffm::fixed32 stepAngle = 4.0_fx;

        for (uint32_t r = 0; r < NUM_RAYS; ++r)
        {
            if (hits_buffer_.size() >= MAX_FIELD_HITS) [[unlikely]] break;

            auto const y = startAngle + ffm::fixed32::fromRaw(r * stepAngle.data);
            uint8_t const angleIdx = static_cast<uint8_t>(y.data >> 16);

            RayStepLUT const rs = rayStepTable[angleIdx];
            auto const dx = rs.dx;
            auto const dz = rs.dz;
            auto const invdx = rs.invdx;
            auto const invdz = rs.invdz;

            ffm::fixed32 tEnter = ffm::fixed32::fromRaw(0);

            if (camX.data >= 0 && camX < levelMaxX && camZ.data >= 0 && camZ < levelMaxZ) [[likely]]
            {
                // Hot path: camera inside bounds
            }
            else [[unlikely]]
            {
                ffm::fixed32 t1 = (-camX) * invdx;
                ffm::fixed32 t2 = (levelMaxX - camX) * invdx;
                if (dx.data < 0) std::swap(t1, t2);

                ffm::fixed32 t3 = (-camZ) * invdz;
                ffm::fixed32 t4 = (levelMaxZ - camZ) * invdz;
                if (dz.data < 0) std::swap(t3, t4);

                ffm::fixed32 const tMin = ffm::max(t1, t3);
                ffm::fixed32 const tMax = ffm::min(t2, t4);

                if (tMax.data < 0 || tMin > tMax) continue;
                tEnter = ffm::max(ffm::fixed32::fromRaw(0), tMin);
            }

            auto const startX = camX + tEnter * dx;
            auto const startZ = camZ + tEnter * dz;

            int16_t mapX = static_cast<int16_t>(startX.data >> 16);
            int16_t mapZ = static_cast<int16_t>(startZ.data >> 17);

            if (mapX >= width)  mapX = width - 1;
            if (mapZ >= length) mapZ = length - 1;
            if (mapX < 0) mapX = 0;
            if (mapZ < 0) mapZ = 0;

            auto const absInvDx = ffm::abs(invdx);
            auto const absInvDz = ffm::abs(invdz);
            auto const deltaDistX = absInvDx;
            auto const deltaDistZ = absInvDz.doubled();

            int16_t const stepX = (dx.data > 0) ? 1 : -1;
            int16_t const stepZ = (dz.data > 0) ? 1 : -1;

            ffm::fixed32 sideDistX = (stepX > 0)
                                         ? (ffm::fixed32::fromRaw((static_cast<int32_t>(mapX + 1) << 16) - startX.data)) * absInvDx
                                         : (ffm::fixed32::fromRaw(startX.data - (static_cast<int32_t>(mapX) << 16))) * absInvDx;

            ffm::fixed32 sideDistZ = (stepZ > 0)
                                         ? (ffm::fixed32::fromRaw((static_cast<int32_t>(mapZ + 1) << 17) - startZ.data)) * absInvDz
                                         : (ffm::fixed32::fromRaw(startZ.data - (static_cast<int32_t>(mapZ) << 17))) * absInvDz;

            int32_t remainingSteps = DRAW_DISTANCE;

            uint32_t const startCellIdx = static_cast<uint32_t>(mapX + mapZ * width);
            uint32_t const startWordIdx = startCellIdx >> 5;
            uint32_t const startBitMask = 1u << (startCellIdx & 31);

            if (current_level_->getCell(mapX, mapZ).collision != Cell::Collision::Empty)
            {
                if (hits_buffer_.size() < MAX_FIELD_HITS) [[likely]]
                {
                    visitedBitset[startWordIdx] |= startBitMask;
                    hits_buffer_.emplace_back(RayHit{mapX, mapZ, tEnter});
                }
            }

            --remainingSteps;

            while (remainingSteps > 0 && hits_buffer_.size() < MAX_FIELD_HITS)
            {
                ffm::fixed32 traveled;

                if (sideDistX < sideDistZ)
                {
                    traveled = sideDistX;
                    sideDistX += deltaDistX;
                    mapX += stepX;
                }
                else
                {
                    traveled = sideDistZ;
                    sideDistZ += deltaDistZ;
                    mapZ += stepZ;
                }

                --remainingSteps;

                if (static_cast<uint16_t>(mapX) >= static_cast<uint16_t>(width) ||
                    static_cast<uint16_t>(mapZ) >= static_cast<uint16_t>(length)) [[unlikely]]
                {
                    break;
                }

                uint32_t const cellIdx = static_cast<uint32_t>(mapX + mapZ * width);
                uint32_t const wordIdx = cellIdx >> 5;
                uint32_t const bitMask = 1u << (cellIdx & 31);

                if (!(visitedBitset[wordIdx] & bitMask)) [[likely]]
                {
                    if (current_level_->getCell(mapX, mapZ).collision != Cell::Collision::Empty)
                    {
                        visitedBitset[wordIdx] |= bitMask;
                        hits_buffer_.emplace_back(RayHit{mapX, mapZ, tEnter + traveled});
                    }
                }
            }
        }

        util::sort(hits_buffer_.begin(), hits_buffer_.end(), [](RayHit const& a, RayHit const& b) {
            return a.distance > b.distance;
        });
    }
};


#endif // RENDERER_HPP
