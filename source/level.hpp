#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <flat_set>
#include <cstdint>
#include <meta>
#include "color.hpp"
#include "util.hpp"
#include "cell.hpp"
#include "mesh.hpp"

struct LevelParseResult
{
    uint16_t length;
    std::array< std::flat_set<std::pair<Color,Color>>, static_cast<size_t>(Cell::Collision::NUM_COLLISIONS) > seenColors;
    std::vector<Cell> cells;
};


class ILevel
{
public:
    constexpr static uint16_t LEVEL_WIDTH{7};
    constexpr static uint16_t LEVEL_MAX_LENGTH{512};

    [[nodiscard]] constexpr virtual auto getCell(uint16_t w, uint16_t l) const -> Cell const &;

    [[nodiscard]] constexpr virtual auto getCellColorBufferPtr(uint16_t w, uint16_t l) const -> Color const*;

    [[nodiscard]] constexpr auto getWidth() const -> int16_t { return LEVEL_WIDTH; }

    [[nodiscard]] constexpr virtual auto getLength() const -> int16_t;

    [[nodiscard]] constexpr virtual auto getOxygen() const -> int16_t;

    [[nodiscard]] constexpr virtual auto getSunVector() const -> ffm::vec3;

    [[nodiscard]] consteval virtual auto getcolorBufferPointer(Cell::Collision col, uint16_t const idx) const -> Color const*;
};


template<util::ConstexprString SL>
class Level final : public ILevel
{

public:



    consteval Level(int16_t const oxygen, int16_t const gravity, ffm::vec3 const sunVector = {0.0_fx,-1.0_fx,0.0_fx}, ffm::vec3 const sunColor = {1.0_fx,1.0_fx,1.0_fx}) :
        oxygen_(oxygen), gravity_(gravity), sun_vector_(sunVector), sun_color_(sunColor)
    {

        auto getIndex = [](std::flat_set<std::pair<Color,Color>> set, std::pair<Color,Color> cp) -> size_t
        {
            auto it = set.begin();
            for(auto i = 0ul; i < set.size(); ++i)
            {
                if(*it == cp)
                {
                    return i;
                }
                std::advance(it, 1);
            }
            //throw(-1);
        };


        auto const result = parseCsvLevel(SL);


        //make color buffers
        auto colorSet = result.seenColors[0];

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::PlaneLow)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            PlaneLowBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::PlaneLow)].size()>
            (
                Mesh::splitVertexArray
                (
                    Mesh::applyLightingColors
                    (
                        4,
                        Mesh::makeMesh
                        (
                            Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::PlaneLow)],cp.first,cp.second
                        ), sun_vector_, sun_color_
                    )
                ).second
            );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::PlaneMid)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            PlaneMidBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::PlaneMid)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::PlaneMid)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::PlaneHigh)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            PlaneHighBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::PlaneHigh)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::PlaneHigh)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::BlockLow)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            BlockLowBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::BlockLow)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::BlockLow)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::BlockMid)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            BlockMidBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::BlockMid)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::BlockMid)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::BlockHigh)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            BlockHighBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::BlockHigh)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::BlockHigh)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelLow)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            TunnelLowBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelLow)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelLow)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelMid)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            TunnelMidBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelMid)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelMid)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelPlaneLow)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            TunnelPlaneLowBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelPlaneLow)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelPlaneLow)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelPlaneMid)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            TunnelPlaneMidBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelPlaneMid)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelPlaneMid)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelPlaneHigh)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            TunnelPlaneHighBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelPlaneHigh)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelPlaneHigh)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelBlockLow)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
         TunnelBlockLowBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelBlockLow)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelBlockLow)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelBlockMid)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            TunnelBlockMidBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelBlockMid)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelBlockMid)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }

        colorSet = result.seenColors[static_cast<size_t>(Cell::Collision::TunnelBlockHigh)];
        for(auto cp : colorSet)
        {
            auto idx = getIndex(colorSet, cp);
            TunnelBlockHighBuffers[idx] = util::make_array<Color, Mesh::CELL_MESHES[static_cast<size_t>(Cell::Collision::TunnelBlockHigh)].size()>
                (
                    Mesh::splitVertexArray
                    (
                        Mesh::applyLightingColors
                        (
                            4,
                            Mesh::makeMesh
                            (
                                Mesh::getCellPieceLists()[static_cast<size_t>(Cell::Collision::TunnelBlockHigh)],cp.first,cp.second
                                ), sun_vector_, sun_color_
                            )
                        ).second
                    );
        }


        //assign each colorbufferindex for each cell
        for(auto i = 0ul; i < result.cells.size(); ++i)
        {
            Cell const& cell{cells[i]};
            colorSet = result.seenColors[static_cast<size_t>(cell.collision)];
            std::pair<Color,Color> cp = {cell.topColor,cell.sideColor};
            auto idx = getIndex(colorSet,cp);
            Color const* colPtr{ getcolorBufferPointer(cell.collision, idx) };
            cellColorBufferPtrs[i] = colPtr;
        }







    }


    [[nodiscard]] constexpr auto getCell(uint16_t w, uint16_t l) const -> Cell const & override { return cells[w*length_ + l]; }

    [[nodiscard]] constexpr auto getCellColorBufferPtr(uint16_t w, uint16_t l) const -> Color const* override
    {
        return cellColorBufferPtrs[w*length_ + l];
    }

    [[nodiscard]] constexpr auto getLength() const -> int16_t override { return length_; }

    [[nodiscard]] constexpr auto getOxygen() const -> int16_t override { return oxygen_; }

    [[nodiscard]] constexpr auto getSunVector() const -> ffm::vec3 override {return sun_vector_;}

    [[nodiscard]] consteval auto getcolorBufferPointer(Cell::Collision col, uint16_t const idx) const -> Color const* override
    {
        Color const* cp;
        if(col == Cell::Collision::Empty)
        {
            cp = EmptyBuffers[idx].data();
        }
        if(col == Cell::Collision::PlaneLow)
        {
            cp = PlaneLowBuffers[idx].data();
        }
        if(col == Cell::Collision::PlaneMid)
        {
            cp = PlaneMidBuffers[idx].data();
        }
        if(col == Cell::Collision::PlaneHigh)
        {
            cp = PlaneHighBuffers[idx].data();
        }
        if(col == Cell::Collision::BlockLow)
        {
            cp = BlockLowBuffers[idx].data();
        }
        if(col == Cell::Collision::BlockMid)
        {
            cp = BlockMidBuffers[idx].data();
        }
        if(col == Cell::Collision::BlockHigh)
        {
            cp = BlockHighBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelLow)
        {
            cp = TunnelLowBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelMid)
        {
            cp = TunnelMidBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelHigh)
        {
            cp = TunnelHighBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelPlaneLow)
        {
            cp = TunnelPlaneLowBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelPlaneMid)
        {
            cp = TunnelPlaneMidBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelPlaneHigh)
        {
            cp = TunnelPlaneHighBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelBlockLow)
        {
            cp = TunnelBlockLowBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelBlockMid)
        {
            cp = TunnelBlockMidBuffers[idx].data();
        }
        if(col == Cell::Collision::TunnelBlockHigh)
        {
            cp = TunnelBlockHighBuffers[idx].data();
        }
        return cp;
    }


//rprivate:
    [[nodiscard]] consteval static auto parseCsvLevel(auto sl) -> LevelParseResult
    {
        constexpr char level0csv[] =
        {
            #embed "../data/levels/level0.txt" suffix(, 0)
        };
        constexpr char level1csv[] =
        {
            #embed "../data/levels/level1.txt" suffix(, 0)
        };

        char const* csvp{level0csv};
        if(sl == "../data/levels/level0.txt")
        {
            csvp = level0csv;
        }
        if(sl == "../data/levels/level1.txt")
        {
            csvp = level1csv;
        }


        auto isSeparator = [](char c)
        {
            return c == ',' || c == '\r' || c == '\n' || c == '\t' || c == '|';
        };

        auto num = [](char const * p)
        {
            uint16_t c = static_cast<uint16_t>(*p);

            if(c == ' ')
            {
                c = '0';
            }
            else if(c >= '0' && c <= '9')
            {
                c = c - '0';
            }
            else if(c >= 'A' && c <= 'F')
            {
                c = (c - 'A') + 10;
            }
            else
            {
                //c = 15;
                throw "ERROR: Invalid data in level file!";
            }

            return c;
        };


        constexpr auto palette_{util::CreateEGAPalette()};

        std::vector<Cell> cells;
        std::array<std::flat_set<std::pair<Color,Color>>, static_cast<size_t>(Cell::Collision::NUM_COLLISIONS)> seenColors;


        char const* p = csvp;
        while(*p != '\0')
        {

            Cell::Collision collision = static_cast<Cell::Collision>(num(p));
            ++p;

            Cell::Type type = static_cast<Cell::Type>(num(p));
            ++p;

            uint16_t topcolor = num(p);
            ++p;

            uint16_t sidecolor = num(p);
            ++p;

            topcolor = palette_[topcolor];
            sidecolor = palette_[sidecolor];

            seenColors[static_cast<size_t>(collision)].insert({topcolor,sidecolor});

            auto& cell = cells.emplace_back(collision, type, topcolor, sidecolor);

            while(isSeparator(*p))
            {
                ++p;
            }

        }


        return {static_cast<uint16_t>(cells.size()/LEVEL_WIDTH),seenColors,cells};
    }


    constexpr static int16_t length_{static_cast<int16_t>(parseCsvLevel(SL).length)};
    constexpr static std::array< Cell, LEVEL_WIDTH * length_> cells{util::make_array<Cell,LEVEL_WIDTH*length_>(parseCsvLevel(SL).cells)};

    int16_t oxygen_;
    int16_t gravity_;
    ffm::vec3 sun_vector_;
    ffm::vec3 sun_color_;



    std::array<Color const*,LEVEL_WIDTH * length_> cellColorBufferPtrs{nullptr};
    std::array<std::array<Color,Mesh::CELL_MESHES[0].size()>,parseCsvLevel(SL).seenColors[0].size()> EmptyBuffers{};

    std::array<std::array<Color,Mesh::CELL_MESHES[1].size()>,parseCsvLevel(SL).seenColors[1].size()> PlaneLowBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[2].size()>,parseCsvLevel(SL).seenColors[2].size()> BlockLowBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[3].size()>,parseCsvLevel(SL).seenColors[3].size()> TunnelLowBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[4].size()>,parseCsvLevel(SL).seenColors[4].size()> TunnelPlaneLowBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[5].size()>,parseCsvLevel(SL).seenColors[5].size()> TunnelBlockLowBuffers{};

    std::array<std::array<Color,Mesh::CELL_MESHES[6].size()>,parseCsvLevel(SL).seenColors[6].size()> PlaneMidBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[7].size()>,parseCsvLevel(SL).seenColors[7].size()> BlockMidBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[8].size()>,parseCsvLevel(SL).seenColors[8].size()> TunnelMidBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[9].size()>,parseCsvLevel(SL).seenColors[9].size()> TunnelPlaneMidBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[10].size()>,parseCsvLevel(SL).seenColors[10].size()> TunnelBlockMidBuffers{};

    std::array<std::array<Color,Mesh::CELL_MESHES[11].size()>,parseCsvLevel(SL).seenColors[11].size()> PlaneHighBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[12].size()>,parseCsvLevel(SL).seenColors[12].size()> BlockHighBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[13].size()>,parseCsvLevel(SL).seenColors[13].size()> TunnelHighBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[14].size()>,parseCsvLevel(SL).seenColors[14].size()> TunnelPlaneHighBuffers{};
    std::array<std::array<Color,Mesh::CELL_MESHES[15].size()>,parseCsvLevel(SL).seenColors[15].size()> TunnelBlockHighBuffers{};


};



constexpr static Level<"../data/levels/level0.txt"> level0(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level1(100,500);
constexpr static Level<"../data/levels/level0.txt"> level2(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level3(100,500);
constexpr static Level<"../data/levels/level0.txt"> level4(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level5(100,500);
constexpr static Level<"../data/levels/level0.txt"> level6(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level7(100,500);
constexpr static Level<"../data/levels/level0.txt"> level8(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level9(100,500);
constexpr static Level<"../data/levels/level0.txt"> level10(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level11(100,500);
constexpr static Level<"../data/levels/level0.txt"> level12(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level13(100,500);
constexpr static Level<"../data/levels/level0.txt"> level14(100,500,ffm::vec3{0.1_fx,-1.0_fx,0.1_fx});
constexpr static Level<"../data/levels/level1.txt"> level15(100,500);


constexpr static std::array<ILevel const *, 16> Levels
{
    &level0,
    &level1,
    &level2,
    &level3,
    &level4,
    &level5,
    &level6,
    &level7,
    &level8,
    &level9,
    &level10,
    &level11,
    &level12,
    &level13,
    &level14,
    &level15,

};


#endif // LEVEL_HPP

