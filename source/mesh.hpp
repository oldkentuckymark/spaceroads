#ifndef MESH_HPP
#define MESH_HPP

#include "ffm.hpp"
#include "csv.hpp"
#include <utility>
#include <cstdint>
#include <vector>
#include <ranges>
#include <meta>
#include <span>
#include <initializer_list>
#include "util.hpp"
#include "cell.hpp"


class Vertex
{
public:
    ffm::vec3 position;
    uint16_t color;
};

namespace Mesh
{
    enum class Piece : uint8_t
    {
        SHIP = 0,
        BACKHIGH,
        BACKLOW,
        BACKMID,
        BOTTOMHIGH,
        BOTTOMLOW,
        BOTTOMMID,
        FRONTHIGH,
        FRONTLOW,
        FRONTMID,
        LEFTHIGH,
        LEFTLOW,
        LEFTMID,
        RIGHTHIGH,
        RIGHTLOW,
        RIGHTMID,
        TOPHIGH,
        TOPLOW,
        TOPMID,
        TUNNELHIGH,
        TUNNELLOW,
        TUNNELMID,

        NUM_PIECES
    };

    [[nodiscard]] consteval static auto makeMeshPiece(Mesh::Piece const m, uint16_t const c = 0) -> std::vector<Vertex>
    {

        constexpr char shipcsv[] =
            {
#embed "../data/meshes/ship.csv" suffix(, 0)
            };
        constexpr char backhighcsv[] =
            {
#embed "../data/meshes/backhigh.csv" suffix(, 0)
            };
        constexpr char backlowcsv[] =
            {
#embed "../data/meshes/backlow.csv" suffix(, 0)
            };
        constexpr char backmidcsv[] =
            {
#embed "../data/meshes/backmid.csv" suffix(, 0)
            };
        constexpr char bottomhighcsv[] =
            {
#embed "../data/meshes/bottomhigh.csv" suffix(, 0)
            };
        constexpr char bottomlowcsv[] =
            {
#embed "../data/meshes/bottomlow.csv" suffix(, 0)
            };
        constexpr char bottommidcsv[] =
            {
#embed "../data/meshes/bottommid.csv" suffix(, 0)
            };
        constexpr char fronthighcsv[] =
            {
#embed "../data/meshes/fronthigh.csv" suffix(, 0)
            };
        constexpr char frontlowcsv[] =
            {
#embed "../data/meshes/frontlow.csv" suffix(, 0)
            };
        constexpr char frontmidcsv[] =
            {
#embed "../data/meshes/frontmid.csv" suffix(, 0)
            };
        constexpr char lefthighcsv[] =
            {
#embed "../data/meshes/lefthigh.csv" suffix(, 0)
            };
        constexpr char leftlowcsv[] =
            {
#embed "../data/meshes/leftlow.csv" suffix(, 0)
            };
        constexpr char leftmidcsv[] =
            {
#embed "../data/meshes/leftmid.csv" suffix(, 0)
            };
        constexpr char righthighcsv[] =
            {
#embed "../data/meshes/righthigh.csv" suffix(, 0)
            };
        constexpr char rightlowcsv[] =
            {
#embed "../data/meshes/rightlow.csv" suffix(, 0)
            };
        constexpr char rightmidcsv[] =
            {
#embed "../data/meshes/rightmid.csv" suffix(, 0)
            };
        constexpr char tophighcsv[] =
            {
#embed "../data/meshes/tophigh.csv" suffix(, 0)
            };
        constexpr char toplowcsv[] =
            {
#embed "../data/meshes/toplow.csv" suffix(, 0)
            };
        constexpr char topmidcsv[] =
            {
#embed "../data/meshes/topmid.csv" suffix(, 0)
            };
        constexpr char tunnelhighcsv[] =
            {
#embed "../data/meshes/tunnelhigh.csv" suffix(, 0)
            };
        constexpr char tunnellowcsv[] =
            {
#embed "../data/meshes/tunnellow.csv" suffix(, 0)
            };
        constexpr char tunnelmidcsv[] =
            {
#embed "../data/meshes/tunnelmid.csv" suffix(, 0)
            };



        char const* csvp{shipcsv};
        switch(m)
        {
        case Mesh::Piece::SHIP:
            csvp = shipcsv;
            break;
        case Mesh::Piece::BACKHIGH:
            csvp = backhighcsv;
            break;
        case Mesh::Piece::BACKLOW:
            csvp = backlowcsv;
            break;
        case Mesh::Piece::BACKMID:
            csvp = backmidcsv;
            break;
        case Mesh::Piece::BOTTOMHIGH:
            csvp = bottomhighcsv;
            break;
        case Mesh::Piece::BOTTOMLOW:
            csvp = bottomlowcsv;
            break;
        case Mesh::Piece::BOTTOMMID:
            csvp = bottommidcsv;
            break;
        case Mesh::Piece::FRONTHIGH:
            csvp = fronthighcsv;
            break;
        case Mesh::Piece::FRONTLOW:
            csvp = frontlowcsv;
            break;
        case Mesh::Piece::FRONTMID:
            csvp = frontmidcsv;
            break;
        case Mesh::Piece::LEFTHIGH:
            csvp = lefthighcsv;
            break;
        case Mesh::Piece::LEFTLOW:
            csvp = leftlowcsv;
            break;
        case Mesh::Piece::LEFTMID:
            csvp = leftmidcsv;
            break;
        case Mesh::Piece::RIGHTHIGH:
            csvp = righthighcsv;
            break;
        case Mesh::Piece::RIGHTLOW:
            csvp = rightlowcsv;
            break;
        case Mesh::Piece::RIGHTMID:
            csvp = rightmidcsv;
            break;
        case Mesh::Piece::TOPHIGH:
            csvp = tophighcsv;
            break;
        case Mesh::Piece::TOPLOW:
            csvp = toplowcsv;
            break;
        case Mesh::Piece::TOPMID:
            csvp = topmidcsv;
            break;
        case Mesh::Piece::TUNNELHIGH:
            csvp = tunnelhighcsv;
            break;
        case Mesh::Piece::TUNNELLOW:
            csvp = tunnellowcsv;
            break;
        case Mesh::Piece::TUNNELMID:
            csvp = tunnelmidcsv;
            break;
        }


        std::vector<double> dv = parse_csv(csvp);

        std::vector<Vertex> verts;


        for(std::size_t i = 0; i < dv.size(); i = i + 7)
        {
            ffm::fixed32 const x =  static_cast<ffm::fixed32>(dv[i+0]);
            ffm::fixed32 const y =  static_cast<ffm::fixed32>(dv[i+1]);
            ffm::fixed32 const z =  static_cast<ffm::fixed32>(dv[i+2]);

            auto const r = static_cast<uint8_t>(dv[i+3] * 255.0);
            auto const g = static_cast<uint8_t>(dv[i+4] * 255.0);
            auto const b = static_cast<uint8_t>(dv[i+5] * 255.0);

            Vertex v;
            v.position.x = x;
            v.position.y = y;
            v.position.z = z;
            if(c == 0)
            {
                v.color = util::Convert888to555(r,g,b);
            }
            else
            {
                v.color = c;
            }

            verts.push_back(v);
            //cvec.push_back((((r >> 3) & 31) | (((g >> 3) & 31) << 5) | (((b >> 3) & 31) << 10) ));

        }


        return verts;
    }


    [[nodiscard]] consteval static auto applyLightingColors(size_t const n, std::span<Vertex const> const & verts,
                                                            ffm::vec3 const & lightdirection, ffm::vec3 lightColor) -> std::vector<Vertex>
    {
        std::vector<Vertex> r; r.append_range(verts);
        for(auto i = 0ul; i < verts.size(); i = i + n)
        {
            auto const colv3 = util::Convert555tovec3(r[i].color);

            auto const norm = util::triangleNormal(r[i+0].position, r[i+1].position, r[i+2].position);
            auto newcolor = util::calculateLight(norm, colv3, lightdirection, lightColor);
            r[i+0].color = newcolor; r[i+1].color = newcolor; r[i+2].color = newcolor;
            if(n == 4) { r[i+3].color = newcolor; }

        }
        return r;
    }

    [[nodiscard]] consteval static auto makeMesh(std::vector<Mesh::Piece> const & pieces,
                                                        uint16_t tc = 0,
                                                        uint16_t sc = 0) -> std::vector<Vertex>
    {
        constexpr auto hasTunnel = [](std::vector<Mesh::Piece> const & pi) consteval
        {
            for(auto i : pi)
            {
                if(i == Mesh::Piece::TUNNELLOW || i == Mesh::Piece::TUNNELMID || i == Mesh::Piece::TUNNELHIGH)
                {
                    return true;
                }
            }
            return false;
        };

        std::vector<Vertex> verts;

        for(auto p : pieces)
        {

            uint16_t c = sc;

            if( p == Mesh::Piece::TUNNELLOW || p == Mesh::Piece::TUNNELMID || p == Mesh::Piece::TUNNELHIGH)
            {
                c = tc;
            }
            if(!hasTunnel(pieces) && (p == Mesh::Piece::TOPLOW || p == Mesh::Piece::TOPMID || p == Mesh::Piece::TOPHIGH))
            {
                c = tc;
            }
            verts.append_range(makeMeshPiece(p,c));

        }


        return verts;
    }

    [[nodiscard]] consteval static auto splitVertexArray(std::span<Vertex const> const & verts) -> std::pair<std::vector<ffm::vec3>, std::vector<uint16_t>>
    {
        std::vector<ffm::vec3> positions;
        std::vector<uint16_t> colors;
        for(auto& v : verts)
        {
            positions.push_back(v.position);
            colors.push_back(v.color);
        }
        return {positions,colors};
    }

    [[nodiscard]] consteval static auto mergeAttributeArrays(std::span<ffm::vec3> pos, std::span<uint16_t> cols) -> std::vector<Vertex>
    {
        std::vector<Vertex> r;
        for(auto i = 0ul; i < pos.size(); ++i)
        {
            r.push_back({pos[i], cols[i]});
        }
        return r;

    }


    consteval static auto getCellPieceLists() -> std::array<std::vector<Mesh::Piece>, static_cast<size_t>(Cell::Collision::NUM_COLLISIONS)>
    {
        return
        {
        std::vector<Mesh::Piece>{},
        {Mesh::Piece::TOPLOW},
        {Mesh::Piece::TOPMID},
        {Mesh::Piece::TOPHIGH},
        {Mesh::Piece::LEFTLOW,Mesh::Piece::RIGHTLOW,Mesh::Piece::FRONTLOW,Mesh::Piece::TOPLOW},
        {Mesh::Piece::LEFTMID,Mesh::Piece::RIGHTMID,Mesh::Piece::FRONTMID,Mesh::Piece::TOPMID},
        {Mesh::Piece::LEFTHIGH,Mesh::Piece::RIGHTHIGH,Mesh::Piece::FRONTHIGH,Mesh::Piece::TOPHIGH},
        {Mesh::Piece::TUNNELLOW},
        {Mesh::Piece::TUNNELMID},
        {Mesh::Piece::TUNNELHIGH},
        {Mesh::Piece::TOPLOW,Mesh::Piece::TUNNELLOW},
        {Mesh::Piece::TOPMID,Mesh::Piece::TUNNELMID},
        {Mesh::Piece::TOPHIGH,Mesh::Piece::TUNNELHIGH},
        {Mesh::Piece::LEFTLOW,Mesh::Piece::RIGHTLOW,Mesh::Piece::FRONTLOW,Mesh::Piece::TOPLOW,Mesh::Piece::TUNNELLOW},
        {Mesh::Piece::LEFTMID,Mesh::Piece::RIGHTMID,Mesh::Piece::FRONTMID,Mesh::Piece::TOPMID,Mesh::Piece::TUNNELMID},
        {Mesh::Piece::LEFTHIGH,Mesh::Piece::RIGHTHIGH,Mesh::Piece::FRONTHIGH,Mesh::Piece::TOPHIGH,Mesh::Piece::TUNNELHIGH}
        };
    }


    constexpr static std::span<Vertex const> SHIP_MESH
    {
        std::define_static_array(makeMeshPiece(Mesh::Piece::SHIP))
    };

    constexpr static std::span<Vertex const> CELL_MESHES[]
    {
        std::define_static_array(makeMesh(getCellPieceLists()[0])),
        std::define_static_array(makeMesh(getCellPieceLists()[1])),
        std::define_static_array(makeMesh(getCellPieceLists()[2])),
        std::define_static_array(makeMesh(getCellPieceLists()[3])),
        std::define_static_array(makeMesh(getCellPieceLists()[4])),
        std::define_static_array(makeMesh(getCellPieceLists()[5])),
        std::define_static_array(makeMesh(getCellPieceLists()[6])),
        std::define_static_array(makeMesh(getCellPieceLists()[7])),
        std::define_static_array(makeMesh(getCellPieceLists()[8])),
        std::define_static_array(makeMesh(getCellPieceLists()[9])),
        std::define_static_array(makeMesh(getCellPieceLists()[10])),
        std::define_static_array(makeMesh(getCellPieceLists()[11])),
        std::define_static_array(makeMesh(getCellPieceLists()[12])),
        std::define_static_array(makeMesh(getCellPieceLists()[13])),
        std::define_static_array(makeMesh(getCellPieceLists()[14])),
        std::define_static_array(makeMesh(getCellPieceLists()[15]))
    };

} // end Mesh


#endif // MESH_HPP
