#pragma once

#include <cassert>
#include <cstdint>
#include <inplace_vector>
#include <span>
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
    QuadsWireFra
};

enum class FaceCullMode : uint32_t
{
    Back,
    None,
    Front,
    All
};

enum class ClipResult : uint32_t
{
    Accept,
    Reject,
    Partial
};

enum class PolygonClipMode : uint32_t
{
    None,
    Trivial,
    Accurate
};



template<class Derived, class VERTEX_FUNCTION>
class BaseContext
{

    static constexpr size_t MAX_VERTS{256};

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

    auto line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, Color color) -> void
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

    auto lineHorizontal(int32_t x0, int32_t y0, int32_t x1, Color color) -> void
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
         if constexpr (HasLineVertical<Derived>) {
             derived().lineVertical(x0, y0, y1, color);
         } else {
             // Default vertical plotting loop

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

    auto setVertexPointer(uint32_t const size, uint32_t const stride, void const* vp) -> void
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

    auto setAspectRatio(fixed32 const ar)
    {
        aspect_ratio_ = 1.0_fx / ar;
    }

    [[nodiscard]] auto getVertexFunction() -> VERTEX_FUNCTION&
    {
        return vf_;
    }

    auto setFaceCulling(FaceCullMode const mode) -> void
    {
        cull_ = mode;
    }

    auto setClipMode(PolygonClipMode const pcm) -> void
    {
        clip_ = pcm;
    }

    auto setNearZ(fixed32 const z) -> void
    {
        near_z_ = z;
    }


    template<uint32_t VERTEX_SIZE>
    auto readVertex(std::byte const * const p) -> vec3
    {
        vec3 r;
        fixed32 const * f32p = reinterpret_cast<fixed32 const * const>(p);
        if constexpr(VERTEX_SIZE == 2)
        {
            r.x = f32p[0];
            r.y = f32p[1];
            r.z = 0.0_fx;
        }
        else if constexpr (VERTEX_SIZE == 3)
        {
            r.x = f32p[0];
            r.y = f32p[1];
            r.z = f32p[2];
        }
        return r;
    }

    template<DrawType DT, uint32_t VERTEX_SIZE>
    auto drawArray_impl_(uint32_t first, uint32_t count) -> void
    {
        working_vertex_buffer_.clear();
        working_color_buffer_.clear();

        std::byte const * vp = reinterpret_cast<std::byte const *>(vertex_pointer_);
        std::byte const * cp = reinterpret_cast<std::byte const *>(color_pointer_);
        auto vs = vertex_stride_;
        auto cs = color_stride_;

        uint16_t wc;
        vec3 wv0;
        vec3 wv1;
        vec3 wv2;
        vec3 wv4;


        if constexpr(VERTEX_SIZE == 2)
        {

        }
        else if constexpr (VERTEX_SIZE == 3)
        {

        }

    }


    auto drawArray2(DrawType const drawType, uint32_t const first, uint32_t const count) -> void
    {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^DrawType)))
        {
            if (drawType == [:e:])
            {
                if (vertex_size_ == 2) {drawArray_impl_<([:e:]), 2>(first, count); }
                else { drawArray_impl_<([:e:]), 3>(first, count); }
                return;
            }
        }
    }

    auto drawArray(DrawType const dt, uint32_t const first, uint32_t const count) -> void
    {
        current_draw_type_ = dt;
        working_vertex_buffer_.clear();
        working_color_buffer_.clear();

        std::byte const * vp = reinterpret_cast<std::byte const *>(vertex_pointer_);
        std::byte const * cp = reinterpret_cast<std::byte const *>(color_pointer_);
        auto vs = vertex_stride_;
        auto cs = color_stride_;

        if(vertex_size_ == 2)
        {
            if(vs == 0) {vs = sizeof(vec2);}
            for(auto const * p = vp + (first*vs); p < vp + ((first+count)*vs); p = p + vs)
            {
                working_vertex_buffer_.push_back( {reinterpret_cast<vec2 const*>(p)->x,reinterpret_cast<vec2 const*>(p)->y,0.0_fx} );
            }

        }
        else if(vertex_size_ == 3)
        {
            if(vs == 0) {vs = sizeof(vec3);}
            for(auto const * p = vp + (first*vs); p < vp + ((first+count)*vs); p = p + vs)
            {
                working_vertex_buffer_.push_back( {reinterpret_cast<vec3 const*>(p)[0]} );
            }

        }

        //gather colors into working buffer
        if(color_pointer_ == nullptr)
        {
            for(auto i = 0; i < working_vertex_buffer_.size(); ++i)
            {
                working_color_buffer_.push_back(color_stride_);
            }
        }
        else
        {
            if(color_stride_ == 0) {cs = sizeof(uint16_t);}
            for(auto const * p = cp + (first*cs); p < cp + ((first+count)*cs); p = p + cs)
            {
                working_color_buffer_.push_back(reinterpret_cast<uint16_t const*>(p)[0]);
            }

        }


        //vertex_pipeline_();

        //run vertex function
        for(uint32_t i = 0; i < working_vertex_buffer_.size(); ++i)
        {
            vf_(working_vertex_buffer_[i]);
        }






    }




protected:



    [[nodiscard]] auto clip_point_near(vec3 const & p) -> bool
    {
        return p.z >= near_z_;
    }

    [[nodiscard]] auto clip_point_ndc(vec3 const & p) -> bool
    {
        return p.x > -1.0_fx && p.x < 1.0_fx &&
               p.y > -1.0_fx && p.y < 1.0_fx;
    }

    [[nodiscard]] constexpr bool clip_line_near_accurate(vec3& a, vec3& b)
    {

        const bool a_behind = a.z < near_z_;
        const bool b_behind = b.z < near_z_;

        if (!a_behind && !b_behind) {
            return true;
        }

        if (a_behind && b_behind) {
            return false;
        }

        const fixed32 t = (near_z_ - a.z) / (b.z - a.z);
        const vec3 hit
        {
            ffm::mix(a.x, b.x, t),
            ffm::mix(a.y, b.y, t),
            near_z_
        };

        if (a_behind) {
            a = hit;
        } else {
            b = hit;
        }

        return false;
    }

    [[nodiscard]] auto clip_horizontal_line_screen(int32_t& x0, int32_t& y0, int32_t& x1) -> int32_t
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

    [[nodiscard]] auto clip_triangle_near_trivial(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> ClipResult
    {
        bool in0 = (v0.z >= near_z_);
        bool in1 = (v1.z >= near_z_);
        bool in2 = (v2.z >= near_z_);

        if (in0 && in1 && in2) return ClipResult::Accept;
        if (!in0 && !in1 && !in2) return ClipResult::Reject;
        return ClipResult::Partial;
    }

    [[nodiscard]] auto clip_triangle_near_accurate(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> std::inplace_vector<vec3, 9>
    {
        std::inplace_vector<vec3, 5> clipped;

        auto process_edge = [&](vec3 const& curr, vec3 const& next) {
            bool currIn = (curr.z >= near_z_);
            bool nextIn = (next.z >= near_z_);

            if (currIn)
            {
                clipped.emplace_back(curr);
            }
            if (currIn != nextIn)
            {
                fixed32 t = (near_z_ - curr.z) / (next.z - curr.z);
                clipped.emplace_back(curr.x + (next.x - curr.x) * t,curr.y + (next.y - curr.y) * t,near_z_);
            }
        };

        process_edge(v0, v1);
        process_edge(v1, v2);
        process_edge(v2, v0);

        std::inplace_vector<vec3, 9> output;
        size_t m = clipped.size();
        for (size_t i = 1; i + 1 < m; ++i) {
            output.emplace_back(clipped[0]);
            output.emplace_back(clipped[i]);
            output.emplace_back(clipped[i + 1]);
        }

        return output;
    }

    [[nodiscard]] auto clip_quad_near_trivial(vec3 const& v0, vec3 const& v1, vec3 const& v2, vec3 const& v3) -> ClipResult
    {
        bool in0 = (v0.z >= near_z_);
        bool in1 = (v1.z >= near_z_);
        bool in2 = (v2.z >= near_z_);
        bool in3 = (v3.z >= near_z_);

        if (in0 && in1 && in2 && in3) return ClipResult::Accept;
        if (!in0 && !in1 && !in2 && !in3) return ClipResult::Reject;
        return ClipResult::Partial;
    }

    [[nodiscard]] auto clip_quad_near_accurate(vec3 const& v0, vec3 const& v1, vec3 const& v2, vec3 const& v3) -> std::inplace_vector<vec3, 9>
    {
        // Max 5 vertices after clipping a quad against 1 plane.
        std::inplace_vector<vec3, 5> clipped;

        auto process_edge = [&](vec3 const& curr, vec3 const& next)
        {
            bool currIn = (curr.z >= near_z_);
            bool nextIn = (next.z >= near_z_);

            if (currIn)
            {
                clipped.emplace_back(curr);
            }
            if (currIn != nextIn)
            {
                fixed32 t = (near_z_ - curr.z) / (next.z - curr.z);
                clipped.emplace_back(curr.x + (next.x - curr.x) * t,curr.y + (next.y - curr.y) * t,near_z_);
            }
        };

        process_edge(v0, v1);
        process_edge(v1, v2);
        process_edge(v2, v3);
        process_edge(v3, v0);

        // Fan triangulation: Converts the clipped quad (now 3, 4, or 5 vertices) into triangles.
        std::inplace_vector<vec3, 9> output;
        size_t m = clipped.size();
        for (size_t i = 1; i + 1 < m; ++i)
        {
            output.emplace_back(clipped[0]);
            output.emplace_back(clipped[i]);
            output.emplace_back(clipped[i + 1]);
        }

        return output;
    }



    inline auto project_to_ndc(vec3& p) -> void
    {
        p.x = p.x * aspect_ratio_;
        p.x = p.x * invDiv(p.z);
        p.y = p.y * invDiv(p.z);
    }

    [[nodiscard]] auto is_cull_passing(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> bool
    {
        if (cull_ == FaceCullMode::None) { return true; }
        if (cull_ == FaceCullMode::All) { return false; }

        // Edge vectors sharing vertex v0
        const auto ab_x = v1.x - v0.x;
        const auto ab_y = v1.y - v0.y;
        const auto ac_x = v2.x - v0.x;
        const auto ac_y = v2.y - v0.y;

        // 2D cross product determinant (Z-component of AB x AC)
        const auto nz = ab_x * ac_y - ab_y * ac_x;

        if (cull_ == FaceCullMode::Back) [[likely]]
        {
            return nz < 0.0_fx;
        }
        else if (cull_ == FaceCullMode::Front)
        {
            return nz > 0.0_fx;
        }

        return false;
    }

    inline auto to_screen_space(vec3& p) -> void
    {
        // Map from [-1, +1] → [0, 1]
        fixed32 sx = (p.x + 1.0_fx).halved();
        fixed32 sy = (1.0_fx - p.y).halved();

        // Scale to viewport
        p.x = sx * render_width_fx_;
        p.y = sy * render_height_fx_;
    }







    [[no_unique_address]] VERTEX_FUNCTION vf_;

    void const* vertex_pointer_{nullptr};
    uint32_t vertex_size_{0};
    uint32_t vertex_stride_{0};
    void const* color_pointer_{nullptr};
    uint16_t color_stride_{0};
    DrawType current_draw_type_{DrawType::Points};

    int32_t render_width_{0};
    int32_t render_height_{0};
    fixed32 render_width_fx_{0.0_fx};
    fixed32 render_height_fx_{0.0_fx};
    fixed32 aspect_ratio_{0.0_fx};
    fixed32 near_z_{0.0_fx};

    std::inplace_vector<vec3,MAX_VERTS> working_vertex_buffer_;
    std::inplace_vector<uint16_t,MAX_VERTS> working_color_buffer_;
    std::inplace_vector<vec3, 16> post_clip_verts1_;
    std::inplace_vector<vec3, 16> post_clip_verts2_;

    FaceCullMode cull_{FaceCullMode::All};
    PolygonClipMode clip_{PolygonClipMode::None};


};



}
