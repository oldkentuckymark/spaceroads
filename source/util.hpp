#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <array>
#include <vector>
#include "ffm.hpp"

namespace util
{



[[nodiscard]] constexpr auto Convert555to888(uint16_t color) -> std::array<uint8_t, 3>
{
    uint8_t const red = (color & 31) << 3;
    uint8_t const green = ((color >> 5) & 31) << 3;
    uint8_t const blue = ((color >> 10) & 31) << 3;
    return {red,green,blue};
}

[[nodiscard]] constexpr auto Convert555tovec3(uint16_t color) -> ffm::vec3
{
    uint8_t const red = (color & 31) << 3;
    uint8_t const green = ((color >> 5) & 31) << 3;
    uint8_t const blue = ((color >> 10) & 31) << 3;

    ffm::fixed32 const x(static_cast<int16_t>(red));
    ffm::fixed32 const y(static_cast<int16_t>(green));
    ffm::fixed32 const z(static_cast<int16_t>(blue));

    return {x / 255.0_fx,y / 255.0_fx,z / 255.0_fx};
}


[[nodiscard]] constexpr auto Convert888to555(uint8_t const r, uint8_t const g, uint8_t const b) -> uint16_t
{
    return (((r >> 3) & 31) |
            (((g >> 3) & 31) << 5) |
            (((b >> 3) & 31) << 10) );

}

[[nodiscard]] constexpr auto Convert888to555(std::array<uint8_t, 3> const & rgb) -> uint16_t
{
    return (((rgb[0] >> 3) & 31) |
            (((rgb[1] >> 3) & 31) << 5) |
            (((rgb[2] >> 3) & 31) << 10) );

}

[[nodiscard]] constexpr auto brightenColor(uint16_t color, int8_t shades) -> uint16_t
{
    // Extract 5-bit channels (RGB555 layout: bit 15 unused, 14-10 R, 9-5 G, 4-0 B)
    uint8_t r = (color >> 10) & 0x1F;
    uint8_t g = (color >> 5)  & 0x1F;
    uint8_t b = color & 0x1F;

    // Apply shade adjustment
    int16_t nr = r + (shades);
    int16_t ng = g + (shades);
    int16_t nb = b + (shades);

    // Clamp each channel to the valid 5-bit range [0, 31]
    nr = (nr < 0) ? 0 : (nr > 31) ? 31 : nr;
    ng = (ng < 0) ? 0 : (ng > 31) ? 31 : ng;
    nb = (nb < 0) ? 0 : (nb > 31) ? 31 : nb;

    // Reconstruct the RGB555 value (MSB remains 0/unused)
    return static_cast<uint16_t>((nr << 10) | (ng << 5) | nb);
}

[[nodiscard]] constexpr auto CreateEGAPalette() -> std::array<uint16_t, 16>
{
    return
    {
        Convert888to555(0X00,0X00,0X00),
        Convert888to555(0X00,0X00,0XAA),
        Convert888to555(0X00,0XAA,0X00),
        Convert888to555(0X00,0XAA,0XAA),

        Convert888to555(0XAA,0X00,0X00),
        Convert888to555(0XAA,0X00,0XAA),
        Convert888to555(0XAA,0X55,0X00),
        Convert888to555(0XAA,0XAA,0XAA),

        Convert888to555(0X55,0X55,0X55),
        Convert888to555(0X55,0X55,0XFF),
        Convert888to555(0X55,0XFF,0X55),
        Convert888to555(0X55,0XFF,0XFF),

        Convert888to555(0XFF,0X55,0X55),
        Convert888to555(0XFF,0X55,0XFF),
        Convert888to555(0XFF,0XFF,0X55),
        Convert888to555(0XFF,0XFF,0XFF),
    };
}

template<class T, std::size_t N>
 [[nodiscard]] consteval auto make_array(std::vector<T> const & vec) -> std::array<T, N>
{
    std::array<T, N> arr{};
    for (std::size_t i = 0; i < N; ++i)
    {
        arr[i] = vec[i];
    }
    return arr;
}

[[nodiscard]] constexpr auto triangleNormal(ffm::vec3 const & v0, ffm::vec3 const & v1, ffm::vec3 const & v2) -> ffm::vec3
{
    // Create two edge vectors from the vertices
    ffm::vec3 const edge1 = v1 - v0;
    ffm::vec3 const edge2 = v2 - v0;

    // Compute the cross product to get the unnormalized normal
    ffm::vec3 const normal = ffm::vec3::cross(edge1, edge2);

    // Normalize the result to get a unit vector
    return normal / normal.length();
}

[[nodiscard]] constexpr auto calculateLight(ffm::vec3 const & normal,
                                            ffm::vec3 const & surfacecolor,
                                            ffm::vec3 const & lightdirection,
                                            ffm::vec3 const & lightcolor = {1.0_fx,1.0_fx,1.0_fx},
                                            ffm::vec3 const & ambientcolor = {0.15_fx,0.15_fx,0.15_fx}) -> uint16_t
{
    ffm::vec3 lightdirectionnorm = lightdirection.normalize();
    auto d = ffm::max(ffm::vec3::dot(normal, lightdirectionnorm),0.0_fx);
    if(d < 0.0_fx) { d = -d;}
    ffm::fixed32 const factor = d;

    ffm::vec3 diffuse = surfacecolor * lightcolor * factor;
    ffm::vec3 ambient = surfacecolor * ambientcolor;
    ffm::vec3 newcolor = ambient + diffuse;
    if(newcolor.x > 1.0_fx) {newcolor.x = 1.0_fx;}
    if(newcolor.y > 1.0_fx) {newcolor.y = 1.0_fx;}
    if(newcolor.z > 1.0_fx) {newcolor.z = 1.0_fx;}


    newcolor = (newcolor * 255.0_fx) + 0.5_fx;
    return util::Convert888to555(static_cast<int16_t>(newcolor.x),
                                 static_cast<int16_t>(newcolor.y),
                                 static_cast<int16_t>(newcolor.z));
}

constexpr auto combine(uint16_t const high, uint16_t const low) -> uint32_t
{
    return (static_cast<uint32_t>(high) << 16) | static_cast<uint32_t>(low);
}

constexpr auto swap(auto& a, decltype(a)& b) -> void
{
    auto t = a;
    a = b;
    b = t;
}

constexpr auto sort(auto& a, decltype(a)& b) -> void
{
    if(a > b) {swap(a,b);}
}

template<size_t N>
struct ConstexprString
{
    consteval ConstexprString(char const (&str)[N])
    {
        std::ranges::copy_n(str, N, value);
    }

    char value[N];

    consteval auto operator<=>(const ConstexprString&) const = default;
};


}



#endif // UTIL_HPP


