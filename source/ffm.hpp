#pragma once


#include <cstdint>
#include <array>
#include <cmath>

namespace ffm
{

constexpr uint32_t INVZ_STEPS = 8;
constexpr uint32_t INVZ_MAX = 256;
constexpr size_t   INVZ_N     = INVZ_STEPS * (INVZ_MAX + 1);

class fixed16
{
public:
    static constexpr int32_t const FIX_SHIFT = 8;
    static constexpr int32_t const FIX_SCALE = 256;
    static constexpr double const FIX_SCALEF = 256.0;

    int16_t data{0};

     constexpr fixed16() = default;

     constexpr explicit fixed16(int8_t const &that)
        : data(that << FIX_SHIFT)
    {}

     constexpr explicit fixed16(double const &that)
        : data(static_cast<int16_t>(that * FIX_SCALEF))
    {}

     constexpr auto operator=(int8_t const that) -> fixed16 &
    {
        data = that << FIX_SHIFT;
        return (*this);
    }

     constexpr auto operator=(double const that) -> fixed16 &
    {
        data = static_cast<int16_t>(that * FIX_SCALEF);
        return (*this);
    }


     constexpr explicit operator int8_t() const { return data >> FIX_SHIFT; }

     consteval explicit operator double() const { return data / FIX_SCALEF; }




};

class fixed32
{
public:
    static constexpr int32_t const FIX_SHIFT = 16;
    static constexpr int32_t const FIX_SCALE = 65536;
    static constexpr double const FIX_SCALEF = 65536.0;

    int32_t data{0};

     constexpr fixed32() = default;

     constexpr explicit fixed32(int8_t const &that)
        : data(that << FIX_SHIFT)
    {}

     constexpr explicit fixed32(int16_t const &that)
        : data(that << FIX_SHIFT)
    {}

    constexpr explicit fixed32(int32_t const &that)
        : data(that << FIX_SHIFT)
    {}

    constexpr explicit fixed32(double const &that)
        : data(static_cast<int32_t>(that * FIX_SCALEF))
    {}

    constexpr auto operator=(int8_t const that) -> fixed32 &
    {
        data = that << FIX_SHIFT;
        return (*this);
    }

    constexpr auto operator=(int16_t const that) -> fixed32 &
    {
        data = that << FIX_SHIFT;
        return (*this);
    }

    constexpr auto operator=(int32_t const that) -> fixed32 &
    {
        data = that << FIX_SHIFT;
        return (*this);
    }


     constexpr auto operator=(double const that) -> fixed32 &
    {
        data = static_cast<int32_t>(that * FIX_SCALEF);
        return (*this);
    }

    constexpr auto operator+=(fixed32 const that) -> fixed32&
    {
        data += that.data;
        return (*this);
    }

    constexpr auto operator-=(fixed32 const that) -> fixed32&
    {
        data -= that.data;
        return (*this);
    }

     constexpr explicit operator int8_t() const { return data >> FIX_SHIFT; }

     constexpr operator int16_t() const { return data >> FIX_SHIFT; }

     constexpr explicit operator int32_t() const { return data >> FIX_SHIFT; }

     consteval explicit operator double() const { return data / FIX_SCALEF; }

     constexpr explicit operator fixed16() const
    {
        fixed16 r;
        r.data = static_cast<int16_t>((data + 128) >> 8);
        return r;
    }

     constexpr auto operator+(fixed32 const that) const -> fixed32
    {
        fixed32 r;
        r.data = data + that.data;
        return r;
    }

     constexpr auto operator-(fixed32 const that) const -> fixed32
    {
        fixed32 r;
        r.data = data - that.data;
        return r;
    }

     constexpr auto operator*(fixed32 const that) const -> fixed32
    {
        fixed32 r;
        r.data = (int64_t(data) * that.data) >> FIX_SHIFT;
        return r;
    }

     constexpr auto operator|(fixed32 const that) const -> fixed32
    {
        fixed32 r;
        r.data = (int64_t(data) * FIX_SCALE) / (that.data);
        return r;
    }

     constexpr auto operator-() const -> fixed32
    {
        fixed32 r;
        r.data = this->data * -1;
        return r;
    }

     constexpr auto operator==(fixed32 const &that) const -> bool
    {
        return this->data == that.data;
    }

     constexpr auto operator<(fixed32 const &that) const -> bool
    {
        return this->data < that.data;
    }

     constexpr auto operator>(fixed32 const &that) const -> bool
    {
        return this->data > that.data;
    }

     constexpr auto operator<=(fixed32 const &that) const -> bool
    {
        return this->data <= that.data;
    }

     constexpr auto operator>=(fixed32 const &that) const -> bool
    {
        return this->data >= that.data;
    }

private:
    consteval static auto makeinvzTable() -> std::array<fixed32, INVZ_N>
    {
        std::array<fixed32, INVZ_N> r;
        double s = 1.0 / INVZ_STEPS;
        double x = 0.0;

        x = x + s;
        for (auto i = 1ul; i < INVZ_N; ++i)
        {
            r[i] = fixed32(1.0 / x);
            x = x + s;
        }

        return r;
    }

     [[nodiscard]] constexpr static auto invZ(fixed32 const z) -> fixed32
    {
        constexpr static std::array<fixed32, INVZ_N> invzlut{makeinvzTable()};

        constexpr auto log2Pow2 = [](uint32_t v) consteval -> int32_t
        {
            int32_t r = 0;
            while (v >>= 1) ++r;
            return r;
        };

        constexpr int32_t SHIFT = fixed32::FIX_SHIFT - log2Pow2(INVZ_STEPS);

        fixed32 v = z;
        if(z.data < 0) { v.data = -v.data;}

        auto const idx = static_cast<std::size_t>(v.data >> SHIFT);

        fixed32 r = invzlut[idx];
        if(z.data < 0) {r.data = -r.data;}
        return r;
    }

public:

    constexpr auto operator/(fixed32 const &that) const -> fixed32
    {
        fixed32 r;
        r = (*this) * invZ(that);
        return r;
    }

    constexpr auto doubled() const -> fixed32
    {
        fixed32 r;
        r.data = this->data << 1;
        return r;
    }

    constexpr auto halved() const -> fixed32
    {
        fixed32 r;
        r.data = this->data >> 1;
        return r;
    }

    constexpr static auto tiny() -> fixed32
    {
        fixed32 r;
        r.data = 1;
        return r;
    }

    constexpr static auto max() -> fixed32
    {
        fixed32 r;
        r.data = 0x7FFFFFFF;
        return r;
    }

};


}

consteval auto operator""_fx(long double f) -> ffm::fixed32
{
    ffm::fixed32 r(static_cast<double>(f));
    return r;
}

consteval auto operator""_fx(unsigned long long f) -> ffm::fixed32
{
    ffm::fixed32 const r(static_cast<int16_t>(f));
    return r;
}

namespace ffm
{


constexpr double TAUF = 6.28318530;
constexpr fixed32 TAU = 6.28318530_fx;
constexpr size_t GAMDEG_IN_CIRCLE = 256; //must be power of 2
constexpr fixed32 RAD_TO_GAMDEG = fixed32(GAMDEG_IN_CIRCLE / TAUF);
constexpr fixed32 GAMDEG_TO_RAD = static_cast<fixed32>(TAUF / GAMDEG_IN_CIRCLE);
constexpr uint16_t QUADRANT_GAMDEG{GAMDEG_IN_CIRCLE / 4};

 [[nodiscard]] constexpr auto factorial(auto const n) -> decltype(n)
{
    decltype(n) r = 1;
    for (decltype(n) i = n; i > 1; --i)
    {
        r = r * i;
    }
    return r;
}

 [[nodiscard]] constexpr auto pow(auto const b, auto const e) -> decltype(b)
{
    decltype(b) r = 1;
    for (decltype(b) i = 0; i < e; ++i) {
        r *= b;
    }
    return r;
}

namespace
{
using LUT = std::array<fixed32, GAMDEG_IN_CIRCLE>;

consteval auto makeSinTable() -> LUT
{
    uint16_t const quadrantSize = GAMDEG_IN_CIRCLE/4;
    LUT r{};

    uint16_t k = (quadrantSize*2);
    for (uint16_t i = 0; i <= quadrantSize; ++i)
    {
        double x = (TAUF / GAMDEG_IN_CIRCLE) * i;
        r[i] = std::sin(x);
        r[k] = r[i];
        --k;
    }

    k = quadrantSize*2;
    for(uint16_t  i = 0; i < quadrantSize*2; ++i)
    {
        r[k] = -r[i];
        ++k;
    }

    r[quadrantSize * 0] = 0.0_fx;
    r[quadrantSize * 1] = 1.0_fx;
    r[quadrantSize * 2] = 0.0_fx;
    r[quadrantSize * 3] = -1.0_fx;

    return r;
}

[[nodiscard]] constexpr auto clampGamdeg(int16_t gamdeg) -> int16_t
{
    return static_cast<int16_t>(static_cast<uint16_t>(gamdeg) & (GAMDEG_IN_CIRCLE - 1));
}

consteval auto makeInvsqrtTable() -> LUT
{
    LUT r{};

    r[0] = 0.0_fx;
    for(auto i = 1ul; i < r.size(); ++i)
    {
        double const x = 1.0 / std::sqrt(static_cast<double>(i));
        fixed32 const y{x};
        r[i] = y;
    }

    return r;
}

constexpr auto log2_pow2(size_t n) -> int16_t
{
    int16_t shift = 0;
    while (n > 1)
    {
        n >>= 1;
        ++shift;
    }
    return shift;
}


static constexpr LUT SINTABLE{makeSinTable()};


} // namespace


[[nodiscard]] constexpr auto radiansToGamdegs(fixed32 const a) -> int16_t
{
    // 1. Single fixed-point multiply
    fixed32 const gamdegs = a * RAD_TO_GAMDEG;

    // 0.5 in Q16.16 format is 0x8000 (1 << 15)
    constexpr int32_t HALF_Q16 = 1 << (fixed32::FIX_SHIFT - 1);

    // 2. Branchless symmetric rounding on raw data
    // Positive: add 0x8000 | Negative: subtract 0x8000
    int32_t raw = gamdegs.data;
    raw += (raw >= 0) ? HALF_Q16 : -HALF_Q16;

    // 3. Extract integer portion via 16-bit arithmetic right shift
    return static_cast<int16_t>(raw >> fixed32::FIX_SHIFT);
}

[[nodiscard]] constexpr auto gamDegsToRadians(int16_t const a) -> fixed32
{
    constexpr int SHIFT = log2_pow2(GAMDEG_IN_CIRCLE);

    // Multiply int16_t 'a' directly by TAU's raw Q16.16 value.
    // Result is in Q16.16 format shifted UP by 16 bits relative to 'a'.
    // TOTAL_SHIFT combines Q16.16 realignment (16) + GAMDEG division (SHIFT).
    constexpr int TOTAL_SHIFT = fixed32::FIX_SHIFT + SHIFT;

    int64_t const product = static_cast<int64_t>(a) * TAU.data;

    fixed32 r;
    r.data = static_cast<int32_t>(product >> TOTAL_SHIFT);
    return r;
}

[[nodiscard]] constexpr auto singd(fixed32 const a) -> fixed32
{
    uint16_t const raw = static_cast<uint16_t>(static_cast<int16_t>(a));
    return SINTABLE[raw & (GAMDEG_IN_CIRCLE - 1)];
}

[[nodiscard]] constexpr auto cosgd(fixed32 const a) -> fixed32
{
    uint16_t const raw = static_cast<uint16_t>(static_cast<int16_t>(a)) + QUADRANT_GAMDEG;
    return SINTABLE[raw & (GAMDEG_IN_CIRCLE - 1)];
}

[[nodiscard]] constexpr auto sin(fixed32 const a) -> fixed32
{
    return singd(a * RAD_TO_GAMDEG);
}

[[nodiscard]] constexpr auto cos(fixed32 const a) -> fixed32
{
    return cosgd(a * RAD_TO_GAMDEG);
}

 [[nodiscard]] constexpr auto tan(fixed32 const n) -> fixed32
{
    return ffm::sin(n) / ffm::cos(n);
}

 [[nodiscard]] constexpr auto cot(fixed32 const n) -> fixed32
{
    return ffm::cos(n) / ffm::sin(n);
}

 [[nodiscard]] constexpr auto abs(auto const n) -> decltype(n)
{
    return (n > decltype(n)(0)) ? n : -n;
}

//[[nodiscard]] constexpr auto abs(fixed32 const n) -> fixed32
//{
//    return (n > 0.0_fx) ? n : -n;
//}

constexpr auto sqrt(fixed32 const x) -> fixed32
{
    fixed32 r{};
    if (x.data <= 0) { return r; }

    // x.data is positive Q16.16 (max ~32767.999, so highest bit set is <= bit 30).
    // We compute sqrt(x.data) in integer domain, yielding result in Q8.8 format.
    // Shifting left by 8 turns Q8.8 into Q16.16 format.
    uint32_t n = static_cast<uint32_t>(x.data);
    uint32_t res = 0;
    uint32_t bit = 1U << 30; // Highest power of 4 fitting in uint32_t

// Unroll or step through fixed 16 iterations (always 16 for 32-bit integer sqrt)
#pragma unroll
    for (int i = 0; i < 16; ++i)
    {
        uint32_t const trial = res + bit;
        if (n >= trial)
        {
            n -= trial;
            res = (res >> 1) + bit;
        }
        else
        {
            res >>= 1;
        }
        bit >>= 2;
    }

    // res is now in Q8.8 format; shift left by 8 bits to convert to Q16.16 (.data)
    r.data = static_cast<int32_t>(res << 8);
    return r;
}

class vec2
{
public:
    fixed32 x, y;

     constexpr auto operator+(vec2 const &that) const -> vec2 { return {x + that.x, y + that.y}; }

     constexpr auto operator+(fixed32 const & that) const -> vec2
    {
        return {this->x + that, this->y + that};
    }

     constexpr auto operator-(vec2 const &that) const -> vec2 { return {x - that.x, y - that.y}; }

     constexpr auto operator-(fixed32 const & that) const -> vec2
    {
        return {this->x - that, this->y - that};
    }

     constexpr auto operator*(fixed32 const &that) const -> vec2 { return {x * that, y * that}; }

     constexpr auto operator/(fixed32 const &that) const -> vec2 { return {x / that, y / that}; }

     constexpr static auto dot(vec2 const & a, vec2 const & b) -> fixed32
    {
        return {(a.x * b.x) + (a.y * b.y)};
    }

     [[nodiscard]] constexpr static auto cross(vec2 const& a, vec2 const& b) -> fixed32
    {
        return (a.x * b.y) - (a.y * b.x);
    }


     [[nodiscard]] constexpr auto length() const -> fixed32
    {
        const auto x2 = x * x;
        const auto y2 = y * y;
        const auto sum = x2 + y2;
        return sqrt(sum);
    }
};

class vec3
{
public:
    fixed32 x,y,z;

     constexpr auto operator+(vec3 const &that) const -> vec3
    {
        return {this->x + that.x, this->y + that.y, this->z + that.z};
    }

     constexpr auto operator+(fixed32 const & that) const -> vec3
    {
        return {this->x + that, this->y + that, this->z + that};
    }

     constexpr auto operator-(vec3 const &that) const -> vec3
    {
        return {this->x - that.x, this->y - that.y, this->z - that.z};
    }

     constexpr auto operator-(fixed32 const &that) const -> vec3
    {
        return {this->x - that, this->y - that, this->z - that};
    }

     constexpr auto operator*(fixed32 const &that) const -> vec3
    {
        return {this->x * that, this->y * that, this->z * that};
    }

     constexpr auto operator/(fixed32 const &that) const -> vec3
    {
        return {this->x / that, this->y / that, this->z / that};
    }

     constexpr auto operator*(vec3 const &that) const -> vec3
    {
        return {(this->x * that.x), (this->y * that.y), (this->z * that.z)};
    }

    constexpr static auto dot(vec3 const & a, vec3 const & b) -> fixed32
    {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

     constexpr static auto cross(vec3 const & a, vec3 const & b) -> vec3
    {
        return {a.y * b.z - a.z * b.y,a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

     [[nodiscard]] constexpr auto length() const -> fixed32
    {
        const auto x2 = x * x;
        const auto y2 = y * y;
        const auto z2 = z * z;
        const auto sum = x2 + y2 + z2;
        return sqrt(sum);
    }

     [[nodiscard]] constexpr auto normalize() const -> vec3
    {
        auto const l = this->length();
        return {this->x / l, this->y / l, this->z / l};
    }


};

class vec4
{
public:
    fixed32 x,y,z,w = 1.0_fx;

     constexpr auto operator+(vec4 const &that) const -> vec4
    {
        return {this->x + that.x, this->y + that.y, z + that.z, w + that.w};
    }

     constexpr auto operator+(fixed32 const & that) const -> vec4
    {
        return {this->x + that, this->y + that, this->z + that, this->w + that};
    }

     constexpr auto operator-(vec4 const &that) const -> vec4
    {
        return {this->x - that.x, this->y - that.y, this->z - that.z, this->w - that.w};
    }

     constexpr auto operator-(fixed32 const & that) const -> vec4
    {
        return {this->x - that, this->y - that, this->z - that, this->w - that};
    }

     constexpr auto operator*(fixed32 const &that) const -> vec4
    {
        return {this->x * that, this->y * that, this->z * that, this->w * that};
    }

     constexpr auto operator/(fixed32 const &that) const -> vec4
    {
        return {this->x / that, this->y / that, this->z / that, this->w / that};
    }

     constexpr auto operator*(vec4 const &that) const -> vec4
    {
        return {(this->x * that.x), (this->y * that.y), (this->z * that.z), (this->w * that.w)};
    }

     [[nodiscard]] constexpr static auto dot(vec4 const & a, vec4 const & b) -> fixed32
    {
        return {(a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w)};
    }

     [[nodiscard]] constexpr auto length() const -> fixed32
    {
        const auto x2 = x * x;
        const auto y2 = y * y;
        const auto z2 = z * z;
        const auto w2 = w * w;
        const auto sum = x2 + y2 + z2 + w2;
        return sqrt(sum);
    }
};

class mat4
{
public:
    fixed32 m[4][4];

     constexpr mat4()
    {
        m[0][0] = 1.0_fx;
        m[0][1] = 0.0_fx;
        m[0][2] = 0.0_fx;
        m[0][3] = 0.0_fx;

        m[1][0] = 0.0_fx;
        m[1][1] = 1.0_fx;
        m[1][2] = 0.0_fx;
        m[1][3] = 0.0_fx;

        m[2][0] = 0.0_fx;
        m[2][1] = 0.0_fx;
        m[2][2] = 1.0_fx;
        m[2][3] = 0.0_fx;

        m[3][0] = 0.0_fx;
        m[3][1] = 0.0_fx;
        m[3][2] = 0.0_fx;
        m[3][3] = 1.0_fx;
    }

     constexpr auto operator+(mat4 const &that) -> mat4
    {
        mat4 n;

        for (uint8_t c = 0; c < 4; ++c) {
            for (uint8_t r = 0; r < 4; ++r) {
                n.m[c][r] = this->m[c][r] + that.m[c][r];
            }
        }

        return n;
    }

     constexpr auto operator-(mat4 const &that) -> mat4
    {
        mat4 n;

        for (uint8_t c = 0; c < 4; ++c) {
            for (uint8_t r = 0; r < 4; ++r) {
                n.m[c][r] = this->m[c][r] - that.m[c][r];
            }
        }

        return n;
    }

     constexpr auto operator*(fixed32 const &that) -> mat4
    {
        mat4 n;

        for (uint8_t c = 0; c < 4; ++c) {
            for (uint8_t r = 0; r < 4; ++r) {
                n.m[c][r] = this->m[c][r] * that;
            }
        }

        return n;
    }

     constexpr auto operator/(fixed32 const &that) -> mat4
    {
        mat4 n;

        for (uint8_t c = 0; c < 4; ++c) {
            for (uint8_t r = 0; r < 4; ++r) {
                n.m[c][r] = this->m[c][r] / that;
            }
        }

        return n;
    }

     constexpr auto operator*(mat4 const &that) -> mat4
    {
        mat4 n;

        for (uint8_t c = 0; c < 4; ++c) {
            for (uint8_t r = 0; r < 4; ++r) {
                n.m[c][r] = (this->m[0][r] * that.m[c][0]) + (this->m[1][r] * that.m[c][1])
                            + (this->m[2][r] * that.m[c][2]) + (this->m[3][r] * that.m[c][3]);
            }
        }

        return n;
    }

     constexpr auto operator*(vec4 const &that) -> vec4
    {
        vec4 n;

        n.x = (this->m[0][0] * that.x) + (this->m[1][0] * that.y) + (this->m[2][0] * that.z)
              + (this->m[3][0] * that.w);

        n.y = (this->m[0][1] * that.x) + (this->m[1][1] * that.y) + (this->m[2][1] * that.z)
              + (this->m[3][1] * that.w);

        n.z = (this->m[0][2] * that.x) + (this->m[1][2] * that.y) + (this->m[2][2] * that.z)
              + (this->m[3][2] * that.w);

        n.w = (this->m[0][3] * that.x) + (this->m[1][3] * that.y) + (this->m[2][3] * that.z)
              + (this->m[3][3] * that.w);

        return n;
    }

     static constexpr auto perspective(fixed32 const fovy,
                                      fixed32 const aspect,
                                      fixed32 const zNear,
                                      fixed32 const zFar)
    {
        mat4 n;
        fixed32 const fovR = fovy * (TAU / 360.0_fx);
        fixed32 const f = cot(fovR * 0.5_fx);

        n.m[0][0] = f / aspect;
        n.m[1][1] = f;
        n.m[2][2] = (zFar + zNear) / (zNear - zFar);
        n.m[3][2] = (2.0_fx * zFar * zNear) / (zNear - zFar);
        n.m[2][3] = -1.0_fx;
        n.m[3][3] = 0.0_fx;

        return n;
    }

     static constexpr auto perspective90DegSquare(fixed32 const zNear, fixed32 const zFar)
    {
        mat4 n;

        n.m[0][0] = 1.0_fx;
        n.m[1][1] = 1.0_fx;
        n.m[2][2] = (zFar + zNear) / (zNear - zFar);
        n.m[3][2] = (2.0_fx * zFar * zNear) / (zNear - zFar);
        n.m[2][3] = -1.0_fx;
        n.m[3][3] = 0.0_fx;

        return n;
    }

     static constexpr auto translation(vec3 const &v) -> mat4
    {
        mat4 n;

        n.m[3][0] = v.x;
        n.m[3][1] = v.y;
        n.m[3][2] = v.z;
        n.m[3][3] = 1.0_fx;

        return n;
    }

     static constexpr auto translation(vec4 const &v) -> mat4
    {
        mat4 n;

        n.m[3][0] = v.x;
        n.m[3][1] = v.y;
        n.m[3][2] = v.z;
        n.m[3][3] = v.w;

        return n;
    }

     static constexpr auto rotationX(fixed32 const &radians) -> mat4
    {
        mat4 r;

        r.m[1][1] = cos(radians);
        r.m[1][2] = sin(radians);
        r.m[2][1] = -sin(radians);
        r.m[2][2] = cos(radians);

        return r;
    }

     static constexpr auto rotationY(fixed32 const &radians) -> mat4
    {
        mat4 r;

        r.m[0][0] = cos(radians);
        r.m[0][2] = -sin(radians);
        r.m[2][0] = sin(radians);
        r.m[2][2] = cos(radians);

        return r;
    }

     static constexpr auto rotationZ(fixed32 const &radians) -> mat4
    {
        mat4 r;

        r.m[0][0] = cos(radians);
        r.m[0][1] = sin(radians);
        r.m[1][0] = -sin(radians);
        r.m[1][1] = cos(radians);

        return r;
    }
};


 [[nodiscard]] constexpr auto mix(auto x, auto y, auto a) -> auto
{
    return x * (1.0_fx - a) + y * a;
}

 [[nodiscard]] constexpr auto min(auto x, auto y) -> auto
{
    if(x < y) {return x;}
    return y;
}

 [[nodiscard]] constexpr auto max(auto x, auto y) -> auto
{
    if(x > y) {return x;}
    return y;
}


} // namespace ffm

