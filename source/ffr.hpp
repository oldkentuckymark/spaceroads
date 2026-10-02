#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <inplace_vector>
#include <span>
#include <ranges>
#include <meta>

#include "color.hpp"
#include "util.hpp"
#include "ffm.hpp"

namespace ffr
{
using namespace ffm;

template<class Derived, class VERTEX_FUNCTION>
class BaseContext;

template<typename F>
concept IsVertexFunction = requires(F f, vec3& v) {
    { f(v) } -> std::same_as<void>;
};

template<typename T>
concept HasPlot = requires(T t, int32_t x, int32_t y, uint16_t color) {
    { t.plot(x, y, color) } -> std::same_as<void>;
};

template<typename T>
concept HasLine = requires {
    requires std::is_same_v<
        decltype(&T::line),
        auto (T::*)(int32_t, int32_t, int32_t, int32_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasLineHorizontal = requires {
    requires std::is_same_v<
        decltype(&T::lineHorizontal),
        auto (T::*)(int32_t, int32_t, int32_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasLineVertical = requires {
    requires std::is_same_v<
        decltype(&T::lineVertical),
        auto (T::*)(int32_t, int32_t, int32_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasTriangle = requires {
    requires std::is_same_v<
        decltype(&T::triangle),
        auto (T::*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasQuad = requires {
    requires std::is_same_v<
        decltype(&T::quad),
        auto (T::*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasClear = requires {
    requires std::is_same_v<
        decltype(&T::clear),
        auto (T::*)() -> void
        >;
};

template<typename T>
concept HasPresent = requires {
    requires std::is_same_v<
        decltype(&T::present),
        auto (T::*)() -> void
        >;
};





enum class DrawType : uint32_t
{
    Points = 1,
    Lines,
    Triangles,
    TrianglesWireFrame,
    Quads,
    QuadsWireFrame
};

enum class FaceCullMode : uint32_t
{
    Back = 0,
    None = 1,
    Front = 2,
    All = 3
};

enum class PolygonClipMode: uint32_t
{
    None = 0,   //do not run clipping
    Trivial,    //clip whole polygon if any vertex fails
    Full        //accurate, but slower
};

template<class Derived, class VERTEX_FUNCTION>
class BaseContext
{
private:
    constexpr Derived& derived() { return static_cast<Derived&>(*this); }

public:
    BaseContext()
    {
        static_assert(IsVertexFunction<VERTEX_FUNCTION>,
                      "CRITICAL: The provided VERTEX_FUNCTION template parameter must override operator()(vec3&).");
        static_assert(HasPlot<Derived>,
                      "CRITICAL: Your derived platform renderer class must implement void plot(int32_t x, int32_t y, uint32_t color).");
    }
    ~BaseContext() = default;

    BaseContext(BaseContext&) = delete;
    auto operator = (BaseContext&) = delete;
    BaseContext(BaseContext&&) = delete;
    auto operator = (BaseContext&&) = delete;

    auto plot(int32_t x, int32_t y, Color color) -> void
    {
        derived().plot(x, y, color);
    }

    auto line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) -> void
    {
        if constexpr (HasLine<Derived>) {
            derived().line(x0, y0, x1, y1, color);
        }
        else
        {
            // Default Bresenham line algorithm parsing down to plot()

        bool const steep = ffm::abs(y1 - y0) > ffm::abs(x1 - x0);

        if (steep)
        {
            int32_t tmp = x0;
            x0 = y0;
            y0 = tmp;

            tmp = x1;
            x1 = y1;
            y1 = tmp;
        }

        if (x0 > x1)
        {
            int32_t tmp = x0;
            x0 = x1;
            x1 = tmp;

            tmp = y0;
            y0 = y1;
            y1 = tmp;
        }

        int32_t const dx = x1 - x0;
        int32_t const dy = ffm::abs(y1 - y0);
        int32_t error = dx / 2;
        int32_t const ystep = (y0 < y1) ? 1 : -1;
        int32_t y = y0;

        for (int32_t x = x0; x <= x1; ++x)
        {
            if (steep)
            {
                plot(y, x, color);
            }
            else
            {
                plot(x, y, color);
            }
            error -= dy;
            if (error < 0)
            {
                y += ystep;
                error += dx;
            }
        }
        }
    }

    auto lineHorizontal(int32_t x0, int32_t y0, int32_t x1, uint16_t color) -> void
    {
        if constexpr (HasLineHorizontal<Derived>)
        {
            derived().lineHorizontal(x0, y0, x1, color);
        }
        else
        {
            line(x0, y0, x1, y0, color);
        }
    }

     auto lineVertical(int32_t x0, int32_t y0, int32_t y1, Color color) -> void
    {
         if constexpr (HasLineVertical<Derived>)
        {
            derived().lineVertical(x0, y0, y1, color);
        }
         else
         {
            line(x0, y0, x0, y1, color);
        }
    }



    auto triangle(int32_t x0, int32_t y0,
                  int32_t x1, int32_t y1,
                  int32_t x2, int32_t y2,
                  Color color) -> void
    {

        struct EdgeWalker {
            int32_t x, xStep, errStep, err, dy;

            void init(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
                dy      = y1 - y0;   // > 0, guaranteed by caller
                int32_t dx = x1 - x0;
                xStep   = dx / dy;   // the only division -- once per edge
                errStep = dx % dy;   // remainder, same sign as dx
                x   = x0;
                err = 0;
            }

            inline void step() {
                x   += xStep;
                err += errStep;
                if (errStep >= 0) { if (err >= dy)  { err -= dy; ++x; } }
                else              { if (err <= -dy) { err += dy; --x; } }
            }
        };

        if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
        if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
        if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }

        if (y2 <= 0 || y0 >= render_height_) return; // outside viewport, vertically
        if (y0 == y2) return;                                // zero height, zero area

        int32_t xmin = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int32_t xmax = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        if (xmax < 0 || xmin >= render_width_) return; // outside viewport, horizontally


        int32_t cross = (int32_t)(x2 - x0) * (y1 - y0) - (int32_t)(y2 - y0) * (x1 - x0);
        bool longIsLeft = cross < 0;

        EdgeWalker longEdge;
        longEdge.init(x0, y0, x2, y2);

        EdgeWalker shortEdge;
        bool inTopHalf = (y0 != y1);
        if (inTopHalf) shortEdge.init(x0, y0, x1, y1);
        else           shortEdge.init(x1, y1, x2, y2);

        int yEnd = y2 < render_height_ ? y2 : render_height_;

        for (int y = y0; y < yEnd; ++y) {
            if (y >= 0) {
                int32_t xa = longEdge.x, xb = shortEdge.x;
                int32_t xl = longIsLeft ? xa : xb;
                int32_t xr = longIsLeft ? xb : xa;
                if (xl < 0) xl = 0;
                if (xr > render_width_) xr = render_width_;
                if (xl < xr) {
                    lineHorizontal((int32_t)xl, (int32_t)y, (int32_t)(xr - 1), color);
                }
            }

            longEdge.step();
            shortEdge.step();

            if (inTopHalf && (y + 1) == y1) {
                inTopHalf = false;
                if (y1 != y2) shortEdge.init(x1, y1, x2, y2);
            }
        }
    }





    auto quad(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, Color color) -> void
    {
        if constexpr (HasQuad<Derived>)
        {
            derived().quad(x0, y0, x1, y1, x2, y2, x3, y3, color);
        }
        else
        {
            triangle(x0,y0,x1,y1,x2,y2,color);
            triangle(x2,y2,x3,y3,x0,y0,color);
        }
    }

    auto clear() -> void
    {
        if constexpr (HasClear<Derived>) {
            derived().clear();
        }
    }

    auto present() -> void
    {
        if constexpr (HasPresent<Derived>) {
            derived().present();
        }
    }

    auto setVertexPointer(uint32_t size, uint32_t const stride, void const* vp) -> void
    {
        vertex_size_ = size;
        vertex_stride_ = stride;
        vertex_pointer_= vp;
    }

    auto setColorPointer(uint16_t const stride, void const* cp)-> void
    {
        color_stride_ = stride;
        color_pointer_ = cp;
    }

    auto setScreenSize(int32_t const w, int32_t const h) -> void
    {
        render_width_ = w;
        render_height_ = h;
        render_width_fx_ = w;
        render_height_fx_ = h;
    }

     [[nodiscard]] auto getVertexFunction() -> VERTEX_FUNCTION&
    {
        return vf_;
    }

    auto setFaceCulling(FaceCullMode const mode) -> void
    {
        cull_ = mode;
    }

    auto setPolygonClipping(PolygonClipMode const mode) -> void
    {
        clip_ = mode;
    }

    auto setNearZ(fixed32 const z) -> void
    {
        near_z_ = z;
    }

    auto setAspectRatio(fixed32 const ar) -> void
    {
        aspect_ratio_ = ar;
    }


    template<DrawType DT, uint32_t VERTEX_SIZE>
    auto drawArray_impl_(uint32_t first, uint32_t count) -> void
    {

        constexpr uint32_t VERTS_PER_POINT = 1;
        constexpr uint32_t VERTS_PER_LINE = 2;
        constexpr uint32_t VERTS_PER_TRIANGLE = 3;
        constexpr uint32_t VERTS_PER_QUAD = 4;


        ffm::vec3 wv0;
        ffm::vec3 wv1;
        ffm::vec3 wv2;
        ffm::vec3 wv3;

        Color workingColor = 0;

        std::byte const * vp;
        std::byte const * cp;

        auto vertStride = vertex_stride_;
        if(vertStride == 0) { vertStride = VERTEX_SIZE*sizeof(ffm::fixed32); }
        auto colStride = color_stride_;
        if(color_pointer_ == nullptr)
        {
            workingColor = static_cast<Color>(color_stride_);
            cp = reinterpret_cast<std::byte const *>(&workingColor);
            colStride = 0;
        }
        else
        {
            if(color_stride_ == 0)
            {
                colStride = sizeof(Color);
            }
            cp = reinterpret_cast<std::byte const *>(color_pointer_) + (first*colStride);
        }

        vp = reinterpret_cast<std::byte const *>(vertex_pointer_) + (first*vertStride);

        for(auto i = count; i > 0; --i)
        {

            workingColor = reinterpret_cast<Color const *>(cp)[0];


            if constexpr (DT == DrawType::Points)
            {
                read_vertex<VERTEX_SIZE>(vp,reinterpret_cast<ffm::fixed32*>(&wv0));

                vf_(wv0);
                if(clip_point_near(wv0))
                {
                    project_to_ndc(wv0);
                    if(clip_point_ndc(wv0))
                    {
                        to_screen_space(wv0);
                        plot(static_cast<int32_t>(wv0.x),static_cast<int32_t>(wv0.y),workingColor);
                    }
                }
            }
            else if constexpr (DT == DrawType::Lines)
            {

            }
            else if constexpr (DT == DrawType::Triangles)
            {

            }
            else if constexpr (DT == DrawType::TrianglesWireFrame)
            {

            }
            else if constexpr (DT == DrawType::Quads)
            {

            }
            else if constexpr (DT == DrawType::QuadsWireFrame)
            {

            }

            vp = vp + vertStride;
            cp = cp + colStride;

        }
    }


    auto drawArray(DrawType const DT, uint32_t const first, uint32_t const count) -> void
    {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^DrawType)))
        {
            if(DT != [:e:]) { continue; }

            if(vertex_size_ == 2) { drawArray_impl_<([:e:]), 2>(first, count); }
            else                  { drawArray_impl_<([:e:]), 3>(first, count); }
            return;
        }
    }

protected:


    template<size_t VERTEX_SIZE>
    auto read_vertex(std::byte const * src, ffm::fixed32* dst) -> void
    {
        fixed32 const* vp = reinterpret_cast<ffm::fixed32 const*>(src);
        if constexpr (VERTEX_SIZE == 2)
        {
            dst[0] = vp[0];
            dst[1] = vp[1];
            dst[2].data = 0;
        }
        else if constexpr (VERTEX_SIZE == 3)
        {
            dst[0] = vp[0];
            dst[1] = vp[1];
            dst[2] = vp[2];
        }
    }





    auto clip_point_near(vec3 const & p) const -> bool
    {
        return p.z >= near_z_;
    }

    auto clip_point_ndc(vec3 const & p) const -> bool
    {
        return p.x >= -1.0_fx && p.x <= 1.0_fx && p.y >= -1.0_fx && p.y <= 1.0_fx;
    }


    auto clip_line_near(vec3& v0, vec3& v1) const -> bool
    {
        // Trivial pass: Both vertices in front of near plane
        if (v0.z >= near_z_ && v1.z >= near_z_)
        {
            return true;
        }

        // Trivial fail: Both vertices behind near plane
        if (v0.z < near_z_ && v1.z < near_z_)
        {
            return false;
        }

        // Partial clip: One vertex is behind near_z_
        fixed32 const dz = v1.z - v0.z;

        // Avoid division by zero if dz is extremely small
        if (dz == 0.0_fx)
        {
            return false;
        }

        // Calculate interpolation ratio t = (near_z - v0.z) / (v1.z - v0.z)
        fixed32 const t = (near_z_ - v0.z) / dz;

        // Compute intersection point along the segment
        vec3 const intersection{
            v0.x + t * (v1.x - v0.x),
            v0.y + t * (v1.y - v0.y),
            near_z_
        };

        // Replace whichever vertex is behind near_z_
        if (v0.z < near_z_)
        {
            v0 = intersection;
        }
        else
        {
            v1 = intersection;
        }

        return true;
    }

    auto clip_line_ndc(vec3& v0, vec3& v1) const -> bool
    {
        // Trivial accept: both endpoints inside the NDC square
        if (clip_point_ndc(v0) && clip_point_ndc(v1))
        {
            return true;
        }

        // Trivial reject: both endpoints outside the same edge
        if ((v0.x < -1.0_fx && v1.x < -1.0_fx) ||
            (v0.x >  1.0_fx && v1.x >  1.0_fx) ||
            (v0.y < -1.0_fx && v1.y < -1.0_fx) ||
            (v0.y >  1.0_fx && v1.y >  1.0_fx))
        {
            return false;
        }

        fixed32 const dx = v1.x - v0.x;
        fixed32 const dy = v1.y - v0.y;
        fixed32 const dz = v1.z - v0.z;

        // Parametric range of the segment that remains inside: P(t) = v0 + t * d
        fixed32 t_enter = 0.0_fx;
        fixed32 t_exit  = 1.0_fx;

        // p = direction of travel relative to the edge, q = distance from v0 to the edge
        auto const clip_edge = [&](fixed32 p, fixed32 q) -> bool
        {
            if (p == 0.0_fx)
            {
                // Parallel to this edge: inside if q >= 0, otherwise entirely outside
                return q >= 0.0_fx;
            }

            fixed32 const r = q / p;

            if (p < 0.0_fx)
            {
                // Segment is entering the clip region
                if (r > t_exit)  return false;
                if (r > t_enter) t_enter = r;
            }
            else
            {
                // Segment is leaving the clip region
                if (r < t_enter) return false;
                if (r < t_exit)  t_exit = r;
            }
            return true;
        };

        if (!clip_edge(-dx, v0.x + 1.0_fx) ||   // x >= -1
            !clip_edge( dx, 1.0_fx - v0.x) ||   // x <=  1
            !clip_edge(-dy, v0.y + 1.0_fx) ||   // y >= -1
            !clip_edge( dy, 1.0_fx - v0.y))     // y <=  1
        {
            return false;
        }

        // Both new endpoints are computed from the ORIGINAL v0, so build them
        // before overwriting anything.
        vec3 const orig = v0;

        auto const clamp_ndc = [](fixed32 v)
        {
            return v < -1.0_fx ? -1.0_fx : (v > 1.0_fx ? 1.0_fx : v);
        };

        if (t_exit < 1.0_fx)
        {
            v1 = vec3{
                clamp_ndc(orig.x + t_exit * dx),
                clamp_ndc(orig.y + t_exit * dy),
                orig.z + t_exit * dz
            };
        }

        if (t_enter > 0.0_fx)
        {
            v0 = vec3{
                clamp_ndc(orig.x + t_enter * dx),
                clamp_ndc(orig.y + t_enter * dy),
                orig.z + t_enter * dz
            };
        }

        return true;
    }

    [[nodiscard]] auto clip_horizontal_line_screen(int32_t x0, int32_t y0, int32_t x1) -> int32_t
    {
        if (y0 < 0)                { return -1; }   // above screen: skip this row only
        if (y0 >= render_height_) { return 0; }    // below screen: nothing further can be visible
        util::order(x0,x1);
        if (x0 < 0 && x1 < 0)      { return -1; }
        if (x0 >= render_width_ && x1 >= render_width_) { return -1; }
        if(x0 < 0) {x0 = 0;}
        if(x1 >= render_width_) {x1 = render_width_ - 1;}
        return 1;
    }



    auto project_to_ndc(vec3& p) -> void
    {
        p.x = p.x * aspect_ratio_;
        p.x = p.x * invZ(p.z);
        p.y = p.y * invZ(p.z);
    }

    [[nodiscard]] auto is_cull_passing(vec3 const& v0, vec3 const& v1, vec3 const& v2) const -> bool
    {
        if (cull_ == FaceCullMode::None) { return true; }
        if (cull_ == FaceCullMode::All) { return false; }

        const auto ab_x = v1.x - v0.x;
        const auto ab_y = v1.y - v0.y;
        const auto ac_x = v2.x - v0.x;
        const auto ac_y = v2.y - v0.y;
        const auto nz = ab_x * ac_y - ab_y * ac_x;

        if (cull_ == FaceCullMode::Back) [[likely]] { return nz < 0.0_fx; }
        else if (cull_ == FaceCullMode::Front) { return nz > 0.0_fx; }

        return false;
    }

    auto to_screen_space(vec3& p) -> void
    {
        // Map from [-1, +1] → [0, 1]
        fixed32 sx = (p.x + 1.0_fx).halved();
        fixed32 sy = (1.0_fx - p.y).halved();

        // Scale to viewport [0, width - 1] and [0, height - 1]
        p.x = sx * (render_width_fx_ - 1.0_fx);
        p.y = sy * (render_height_fx_ - 1.0_fx);
    }




    [[nodiscard]] auto clip_triangle_trivial(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> int32_t //0:reject,1:pass,2:partial
    {
        bool in0 = (v0.z >= near_z_);
        bool in1 = (v1.z >= near_z_);
        bool in2 = (v2.z >= near_z_);

        if (in0 && in1 && in2) return 1;
        if (!in0 && !in1 && !in2) return 0;
        return 2;
    }

    [[nodiscard]] auto clip_triangle_accurate(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> std::inplace_vector<vec3, 9>
    {
        // Max 5 vertices after clipping a triangle against 1 plane.
        std::inplace_vector<vec3, 5> clipped;

        // Local lambda allows the compiler to fully unroll and inline
        // the edge processing without loop overhead.
        auto process_edge = [&](vec3 const& curr, vec3 const& next) {
            bool currIn = (curr.z >= near_z_);
            bool nextIn = (next.z >= near_z_);

            if (currIn) {
                clipped.emplace_back(curr);
            }
            if (currIn != nextIn) {
                fixed32 t = (near_z_ - curr.z) / (next.z - curr.z);
                clipped.emplace_back(
                    curr.x + (next.x - curr.x) * t,
                    curr.y + (next.y - curr.y) * t,
                    near_z_ // GBA OPTIMIZATION: Z is exactly the near plane. Saves MUL + ADD.
                    );
            }
        };

        // Explicitly unrolled edges for maximum ARM7TDMI speed
        process_edge(v0, v1);
        process_edge(v1, v2);
        process_edge(v2, v0);

        // Fan triangulation: Safely converts 3, 4, or 5 vertex convex polygons into triangles.
        // Max output: 5 vertices = 3 triangles = 9 vertices.
        std::inplace_vector<vec3, 9> output;
        size_t m = clipped.size();
        for (size_t i = 1; i + 1 < m; ++i) {
            output.emplace_back(clipped[0]);
            output.emplace_back(clipped[i]);
            output.emplace_back(clipped[i + 1]);
        }

        return output;
    }

    [[nodiscard]] auto clip_quad_trivial(vec3 const& v0, vec3 const& v1, vec3 const& v2, vec3 const& v3) -> int32_t
    {
        bool in0 = (v0.z >= near_z_);
        bool in1 = (v1.z >= near_z_);
        bool in2 = (v2.z >= near_z_);
        bool in3 = (v3.z >= near_z_);

        if (in0 && in1 && in2 && in3) return 1;
        if (!in0 && !in1 && !in2 && !in3) return 0;
        return 2;
    }

    [[nodiscard]] auto clip_quad_accurate(vec3 const& v0, vec3 const& v1, vec3 const& v2, vec3 const& v3) -> std::inplace_vector<vec3, 9>
    {
        // Max 5 vertices after clipping a quad against 1 plane.
        std::inplace_vector<vec3, 5> clipped;

        auto process_edge = [&](vec3 const& curr, vec3 const& next) {
            bool currIn = (curr.z >= near_z_);
            bool nextIn = (next.z >= near_z_);

            if (currIn) {
                clipped.emplace_back(curr);
            }
            if (currIn != nextIn) {
                fixed32 t = (near_z_ - curr.z) / (next.z - curr.z);
                clipped.emplace_back(
                    curr.x + (next.x - curr.x) * t,
                    curr.y + (next.y - curr.y) * t,
                    near_z_ // GBA OPTIMIZATION: Z is exactly the near plane.
                    );
            }
        };

        // Explicitly unrolled edges
        process_edge(v0, v1);
        process_edge(v1, v2);
        process_edge(v2, v3);
        process_edge(v3, v0);

        // Fan triangulation: Converts the clipped quad (now 3, 4, or 5 vertices) into triangles.
        std::inplace_vector<vec3, 9> output;
        size_t m = clipped.size();
        for (size_t i = 1; i + 1 < m; ++i) {
            output.emplace_back(clipped[0]);
            output.emplace_back(clipped[i]);
            output.emplace_back(clipped[i + 1]);
        }

        return output;
    }


    [[no_unique_address]] VERTEX_FUNCTION vf_;

    void const* vertex_pointer_{nullptr};
    uint32_t vertex_size_{0};
    uint32_t vertex_stride_{0};
    void const* color_pointer_{nullptr};
    uint16_t color_stride_{0};


    int32_t render_width_{0};
    int32_t render_height_{0};
    fixed32 render_width_fx_{0.0_fx};
    fixed32 render_height_fx_{0.0_fx};

    fixed32 aspect_ratio_{0.0_fx};
    fixed32 near_z_{0.0_fx};

    DrawType current_draw_type_{DrawType::Points};
    FaceCullMode cull_{FaceCullMode::All};
    PolygonClipMode clip_{PolygonClipMode::None};




};

}




