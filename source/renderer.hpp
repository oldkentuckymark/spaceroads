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
    Renderer() = default;
    ~Renderer() = default;

    auto setPlayer(Player* p) -> void { current_player_ = p; }
    auto setPlayerMesh(std::span<Vertex const> m) -> void { current_ship_mesh_ = m; }
    auto setLevel(ILevel const* level) -> void { current_level_ = level; }

    auto draw() -> void
    {
        if (!current_level_ || !current_player_) return;

        ctx.setFaceCulling(ffr::FaceCullMode::Back);
        ctx.clear();

        auto& campos = ctx.getVertexFunction().camPos;
        auto& camyaw = ctx.getVertexFunction().camRot.y;
        campos = current_player_->position + ffm::vec3{0.0_fx, 2.0_fx, -2.8_fx};

        // 1. Raycast & process grid blocks
        auto rayField = castRayField(static_cast<float>(campos.x),
                                     static_cast<float>(campos.y),
                                     static_cast<float>(campos.z),
                                     static_cast<float>(camyaw),
                                     current_level_);

        auto sortedHits = processRayField(rayField);

        // 2. Render visible level blocks (Furthest -> Closest)
        for (RayHit const& bh : sortedHits)
        {
            auto w = bh.w;
            auto l = bh.l;
            auto const& cell = current_level_->getCell(w, l);
            auto const* cp   = current_level_->getCellColorBufferPtr(w, l);

            ctx.getVertexFunction().modelPos = ffm::vec3(
                ffm::fixed32(static_cast<int16_t>(w)),
                0.0_fx,
                ffm::fixed32(static_cast<int16_t>(l * 2)) // Cell length = 2.0
                );

            ctx.setColorPointer(0, cp);
            ctx.setVertexPointer(3, sizeof(Vertex), Mesh::CELL_MESHES[static_cast<size_t>(cell.collision)].data());
            ctx.drawArray(ffr::DrawType::Quads, 0, Mesh::CELL_MESHES[static_cast<size_t>(cell.collision)].size());
        }

        // 3. Render player ship
        ctx.getVertexFunction().modelPos = current_player_->position;
        ctx.setColorPointer(sizeof(Vertex), &Mesh::SHIP_MESH[0].color);
        ctx.setVertexPointer(3, sizeof(Vertex), &Mesh::SHIP_MESH[0].position);
        ctx.drawArray(ffr::DrawType::Quads, 0, Mesh::SHIP_MESH.size());

        ctx.present();
    }

    Context ctx;

private:
    ILevel const* current_level_{nullptr};
    Player* current_player_{nullptr};
    std::span<Vertex const> current_ship_mesh_{};

    constexpr static size_t DRAW_DISTANCE = 10;
    constexpr static float GAMDEG_TO_RAD = (2.0f * std::numbers::pi_v<float>) / static_cast<float>(ffm::GAMDEG_IN_CIRCLE);



    // 90° FOV half-width
    static constexpr int HALF_FOV_GAMDEG = static_cast<int>(ffm::GAMDEG_IN_CIRCLE / 8); // 32 for 256 GAMDEG

    // Extra rays to cast past screen edges to prevent volumetric culling artifacts
    static constexpr int EXTRA_SIDE_RAYS = 4;

    // Sweep dimensions and capacities
    static constexpr int HALF_SWEEP       = HALF_FOV_GAMDEG + EXTRA_SIDE_RAYS;            // 36
    static constexpr size_t NUM_RAYS      = static_cast<size_t>(HALF_SWEEP * 2) + 1;    // 73
    static constexpr size_t MAX_FIELD_HITS = NUM_RAYS * DRAW_DISTANCE;                   // 730


    struct RayHit
    {
        int16_t w;
        int16_t l;
        float distance;
    };

    using RaycastResult       = std::inplace_vector<RayHit, DRAW_DISTANCE>;
    using RayFieldResult      = std::inplace_vector<RaycastResult, NUM_RAYS>;
    using DeduplicatedRayHits = std::inplace_vector<RayHit, MAX_FIELD_HITS>;

    auto castRay(float camX, float camY, float camZ, float rayYawGamdeg, float offsetGamdeg, ILevel const* const level)
        -> RaycastResult
    {
        RaycastResult hits;

        if (!level) return hits;

        const int16_t width  = static_cast<int16_t>(level->getWidth());
        const int16_t length = static_cast<int16_t>(level->getLength());

        if (width <= 0 || length <= 0) return hits;

        constexpr float CELL_W = 1.0f;
        constexpr float CELL_L = 2.0f;

        const float worldWidth  = static_cast<float>(width) * CELL_W;
        const float worldLength = static_cast<float>(length) * CELL_L;

        const float rad  = rayYawGamdeg * GAMDEG_TO_RAD;
        const float dirW = std::sin(rad); // +W (X axis)
        const float dirL = std::cos(rad); // +L (Z axis)

        // --- 1. Ray-AABB Intersection ---
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::infinity();

        if (dirW != 0.0f) {
            float t1 = (0.0f - camX) / dirW;
            float t2 = (worldWidth - camX) / dirW;
            tMin = std::max(tMin, std::min(t1, t2));
            tMax = std::min(tMax, std::max(t1, t2));
        } else if (camX < 0.0f || camX >= worldWidth) {
            return hits;
        }

        if (dirL != 0.0f) {
            float t1 = (0.0f - camZ) / dirL;
            float t2 = (worldLength - camZ) / dirL;
            tMin = std::max(tMin, std::min(t1, t2));
            tMax = std::min(tMax, std::max(t1, t2));
        } else if (camZ < 0.0f || camZ >= worldLength) {
            return hits;
        }

        if (tMin > tMax) return hits;

        // --- 2. Max Depth Traversal Limit INSIDE Level ---
        const float offsetRad     = offsetGamdeg * GAMDEG_TO_RAD;
        const float maxLevelDepth = (static_cast<float>(DRAW_DISTANCE) / std::cos(offsetRad)) + 2.0f;

        // --- 3. Entry Point & Grid Cell Indexing ---
        constexpr float epsilon = 1e-4f;
        float rayStartX = camX + tMin * dirW;
        float rayStartZ = camZ + tMin * dirL;

        if (tMin > 0.0f) {
            rayStartX += epsilon * dirW;
            rayStartZ += epsilon * dirL;
        }

        int16_t mapW = std::clamp<int16_t>(static_cast<int16_t>(std::floor(rayStartX / CELL_W)), 0, width - 1);
        int16_t mapL = std::clamp<int16_t>(static_cast<int16_t>(std::floor(rayStartZ / CELL_L)), 0, length - 1);

        // --- 4. DDA Setup ---
        const int16_t stepW = (dirW >= 0.0f) ? 1 : -1;
        const int16_t stepL = (dirL >= 0.0f) ? 1 : -1;

        const float deltaDistW = (dirW == 0.0f) ? std::numeric_limits<float>::infinity() : std::abs(CELL_W / dirW);
        const float deltaDistL = (dirL == 0.0f) ? std::numeric_limits<float>::infinity() : std::abs(CELL_L / dirL);

        const float nextBoundaryW = (dirW >= 0.0f) ? (static_cast<float>(mapW + 1) * CELL_W)
                                                   : (static_cast<float>(mapW) * CELL_W);
        const float nextBoundaryL = (dirL >= 0.0f) ? (static_cast<float>(mapL + 1) * CELL_L)
                                                   : (static_cast<float>(mapL) * CELL_L);

        float sideDistW = tMin + ((dirW == 0.0f) ? std::numeric_limits<float>::infinity()
                                                 : std::abs((nextBoundaryW - rayStartX) / dirW));
        float sideDistL = tMin + ((dirL == 0.0f) ? std::numeric_limits<float>::infinity()
                                                 : std::abs((nextBoundaryL - rayStartZ) / dirL));

        float currentDist = tMin;

        // --- 5. Grid Traversal ---
        constexpr size_t MAX_SAFETY_STEPS = 128;

        for (size_t step = 0; step < MAX_SAFETY_STEPS; ++step)
        {
            const float depthInsideLevel = currentDist - tMin;
            if (currentDist > tMax || depthInsideLevel > maxLevelDepth) {
                break;
            }

            if (mapW < 0 || mapW >= width || mapL < 0 || mapL >= length) {
                break;
            }

            if (auto const& cell = level->getCell(mapW, mapL); cell.collision != Cell::Collision::Empty)
            {
                if (hits.size() < DRAW_DISTANCE) {
                    hits.push_back(RayHit{
                        .w = mapW,
                        .l = mapL,
                        .distance = currentDist
                    });
                }

                if (hits.size() == DRAW_DISTANCE) {
                    break;
                }
            }

            if (sideDistW < sideDistL) {
                currentDist = sideDistW;
                sideDistW += deltaDistW;
                mapW += stepW;
            } else {
                currentDist = sideDistL;
                sideDistL += deltaDistL;
                mapL += stepL;
            }
        }

        return hits;
    }

    auto castRayField(float camX, float camY, float camZ, float yawGamdeg, ILevel const* const level)
        -> RayFieldResult
    {
        RayFieldResult field;

        for (int i = -HALF_SWEEP; i <= HALF_SWEEP; ++i)
        {
            const float offsetGamdeg = static_cast<float>(i);
            const float rayYaw       = yawGamdeg + offsetGamdeg;

            field.push_back(castRay(camX, camY, camZ, rayYaw, offsetGamdeg, level));
        }

        return field;
    }

    auto processRayField(RayFieldResult const& field) -> DeduplicatedRayHits
    {
        DeduplicatedRayHits flatHits;

        for (auto const& ray : field) {
            for (auto const& hit : ray) {
                flatHits.push_back(hit);
            }
        }

        // Sort primary by (w, l) and secondary by distance ascending
        std::sort(flatHits.begin(), flatHits.end(), [](RayHit const& a, RayHit const& b) {
            if (a.w != b.w) return a.w < b.w;
            if (a.l != b.l) return a.l < b.l;
            return a.distance < b.distance;
        });

        // Deduplicate by cell coordinate
        auto lastUnique = std::unique(flatHits.begin(), flatHits.end(), [](RayHit const& a, RayHit const& b) {
            return a.w == b.w && a.l == b.l;
        });
        flatHits.erase(lastUnique, flatHits.end());

        // Sort by distance descending (furthest -> closest)
        std::sort(flatHits.begin(), flatHits.end(), [](RayHit const& a, RayHit const& b) {
            return a.distance > b.distance;
        });

        return flatHits;
    }
};


#endif // RENDERER_HPP
