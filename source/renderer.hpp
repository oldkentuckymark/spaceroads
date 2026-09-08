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
        //ctx.setViewPort(160,128);
        //ctx.setNearZ(0.6_fx);
        ctx.getVertexFunction().camPos = {0.5_fx,2_fx,-2.0_fx};

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
        campos = current_player_->position + ffm::vec3{0.0_fx,2.0_fx,-2.0_fx};
        campos.x = 0.5_fx;

        int16_t const pz = static_cast<int16_t>(current_player_->position.z) / 2;

        for(int16_t z = pz + draw_distance_; z >= pz; --z)
        {
            if(z < 0) {break;}
            if(z >= current_level_->getLength()) {continue;}


            for(int16_t x = 0; x < ILevel::LEVEL_WIDTH; ++x)
            {
                auto const & cell {current_level_->getCell(x,z)};
                if( cell.collision != Cell::Collision::Empty )
                {
                    ctx.getVertexFunction().modelPos = {ffm::fixed32(static_cast<int16_t>(x-3)),0.0_fx,ffm::fixed32(static_cast<int16_t>(z*2))};
                    auto const colptr{current_level_->getCellColorBufferPtr(x,z)};
                    ctx.setColorPointer(0, colptr);
                    ctx.setVertexPointer(3,sizeof(Vertex), Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision) ].data());
                    if(Cell::isTunnel(cell.collision))
                    {
                        if(cell.collision == Cell::Collision::TunnelPlaneLow ||
                            cell.collision == Cell::Collision::TunnelPlaneMid ||
                            cell.collision == Cell::Collision::TunnelPlaneHigh)
                        {
                            ctx.drawArray(ffr::DrawType::Quads,0,4);
                        }
                        else if(cell.collision == Cell::Collision::TunnelBlockLow ||
                                cell.collision == Cell::Collision::TunnelBlockMid ||
                                cell.collision == Cell::Collision::TunnelBlockHigh)
                        {
                            ctx.drawArray(ffr::DrawType::Quads,0,16);
                        }
                    }
                    else
                    {

                        ctx.drawArray(ffr::DrawType::Quads,0,Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision)].size());
                    }
                }


            }
        }

        if(true || current_player_->position.y < 1.0_fx)
        {
            ctx.getVertexFunction().modelPos = current_player_->position;
            ctx.setColorPointer(sizeof(Vertex), &Mesh::SHIP_MESH.data()->color);
            ctx.setVertexPointer(3,sizeof(Vertex),Mesh::SHIP_MESH.data());
            ctx.drawArray(ffr::DrawType::Quads,0,Mesh::SHIP_MESH.size());
        }

        for(int16_t z = pz + draw_distance_; z >= pz; --z)
        {
            if(z < 0) {break;}
            if(z >= current_level_->getLength()) {continue;}


            for(int16_t x = 0; x < ILevel::LEVEL_WIDTH; ++x)
            {
                auto const & cell {current_level_->getCell(x,z)};
                if( cell.collision != Cell::Collision::Empty )
                {
                    ctx.getVertexFunction().modelPos = {ffm::fixed32(static_cast<int16_t>(x-3)),0.0_fx,ffm::fixed32(static_cast<int16_t>(z*2))};
                    auto const colptr{current_level_->getCellColorBufferPtr(x,z)};
                    ctx.setColorPointer(0, colptr);
                    ctx.setVertexPointer(3,sizeof(Vertex), Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision) ].data());
                    if(Cell::isTunnel(cell.collision))
                    {
                        if(Cell::isTunnel(cell.collision))
                        {
                            if(cell.collision == Cell::Collision::TunnelPlaneLow ||
                                cell.collision == Cell::Collision::TunnelPlaneMid ||
                                cell.collision == Cell::Collision::TunnelPlaneHigh)
                            {
                                ctx.drawArray(ffr::DrawType::Quads,4,Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision)].size() - 4);
                            }
                            else if(cell.collision == Cell::Collision::TunnelBlockLow ||
                                     cell.collision == Cell::Collision::TunnelBlockMid ||
                                     cell.collision == Cell::Collision::TunnelBlockHigh)
                            {
                                ctx.drawArray(ffr::DrawType::Quads,16,Mesh::CELL_MESHES[ static_cast<size_t>(cell.collision)].size() - 16);
                            }
                        }
                    }

                }

            }
        }





        //ctx.quad(20,50,50,0,100,100,20,100, 65535);

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
