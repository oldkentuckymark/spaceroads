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
    Renderer()
    {


    }

    ~Renderer()
    {

    }

    auto setDrawDistance(uint16_t const blocks) -> void
    {
        draw_distance_ = blocks;
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
        campos = current_player_->position + ffm::vec3{0.0_fx,2.0_fx,-2.1_fx};
        //campos.x = 3.5_fx;

        int16_t const pz = static_cast<int16_t>(current_player_->position.z) / 2;
        int16_t maxz = pz + draw_distance_;
        if(maxz >= current_level_->getLength()) { maxz = current_level_->getLength() - 1; }
        int16_t minz = pz;
        if(minz < 0) { minz = 0; }
        int16_t px = current_player_->position.x;
        if (px < 0) { px = 0; }
        if (px > 8) { px = 8; }

        //draw level back to front, each row outside in, low pieces to high
        for(int16_t z = maxz; z >= minz; --z)
        {

            auto cxs = std::array<size_t, 7>{0,6,1,5,2,4,3};

            for(auto i = 0ul; i < 7; ++i)
            {
                auto& cell = current_level_->getCell(cxs[i],z);
                if(cell.collision != Cell::Collision::Empty)
                {
                    ctx.getVertexFunction().modelPos = {ffm::fixed32(static_cast<int16_t>(cxs[i])),0.0_fx,ffm::fixed32(static_cast<int16_t>(z*2))};
                    auto const colptr{current_level_->getCellColorBufferPtr(cxs[i],z)};
                    ctx.setColorPointer(0, colptr);
                    ctx.setVertexPointer(3,sizeof(Vertex), Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision) ].data());
                    ctx.drawArray(ffr::DrawType::Quads, 0, Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision) ].size());
                }
            }

        }





        ctx.present();
    }

private:
    Context ctx;

    int16_t draw_distance_{0};

    ILevel const * current_level_{nullptr};

    Player* current_player_{nullptr};
    std::span<Vertex const> current_ship_mesh_{};


};


#endif // RENDERER_HPP
