#pragma once

#include <cassert>
#include <cstdint>
#include <inplace_vector>
#include <span>

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

enum class FaceCullMode : int32_t
{
    Back = -1,
    None = 0,
    Front = 1,
    All = 2
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

     [[nodiscard]] auto getVertexFunction() -> VERTEX_FUNCTION&
    {
        return vf_;
    }

    auto setFaceCulling(FaceCullMode const mode) -> void
    {
        cull_ = mode;
    }

    auto setNearZ(fixed32 const z) -> void
    {
        near_z_ = z;
    }

    auto setAspectRatio(fixed32 const ar) -> void
    {
        aspect_ratio_ = ar;
    }


     auto drawArray(DrawType const dt, uint32_t const first, uint32_t const count) -> void
    {
        // else if(vertex_size_ == 3)
        // {
        //     if(vs == 0) {vs = sizeof(vec3);}
        //     for(auto const * p = vp + (first*vs); p < vp + ((first+count)*vs); p = p + vs)
        //     {
        //         working_vertex_buffer_.push_back( {reinterpret_cast<vec3 const*>(p)[0]} );
        //     }

        // }

        vec3 workingVerts[4];
        vec3& wv0{workingVerts[0]};
        vec3& wv1{workingVerts[1]};
        vec3& wv2{workingVerts[2]};
        vec3& wv3{workingVerts[3]};

        uint16_t workingColors[4];
        uint16_t& wc0{workingColors[0]};
        uint16_t& wc1{workingColors[1]};
        uint16_t& wc2{workingColors[2]};
        uint16_t& wc3{workingColors[3]};


        std::byte const * vertPtr = reinterpret_cast<std::byte const *>(vertex_pointer_);
        std::byte const * colPtr = reinterpret_cast<std::byte const *>(color_pointer_);

        auto vertStrideBytes = vertex_stride_;
        auto colStrideBytes = color_stride_;
        if(vertStrideBytes == 0) { vertStrideBytes = sizeof(fixed32)*vertex_size_; }
        if(colStrideBytes == 0) { colStrideBytes = sizeof(uint16_t); }

        auto const vertSize = vertex_size_;
        auto const colSize = 1;
        auto const vertSizeBytes = sizeof(fixed32)*vertex_size_;
        auto const colSizeBytes = sizeof(fixed32);

        constexpr uint32_t VERTS_PER_POINT = 1;
        constexpr uint32_t VERTS_PER_LINE = 2;
        constexpr uint32_t VERTS_PER_TRIANGLE = 3;
        constexpr uint32_t VERTS_PER_QUAD = 4;

        if(dt == DrawType::Points)
        {
            for(auto const* vp = vertPtr+(first*vertSizeBytes); vp < vertPtr+((first+count)*vertSizeBytes); vp = vp + vertStrideBytes)
            {
                //copy position data from vertex_pointer_ to working buffer
                fixed32 const* vpf32 = reinterpret_cast<fixed32 const*>(vp);
                fixed32* wvp = reinterpret_cast<fixed32*>(workingVerts);
                for(auto i = 0ul; i < vertSize*VERTS_PER_POINT; ++i)
                {

                    *wvp = *vpf32;
                    ++vpf32;
                    ++wvp;
                }
            }

            //copy color data from color_pointer_ to working buffer
            for(auto const* cp = colPtr+(first+colSizeBytes); cp < colPtr+((first+count)*colSizeBytes); cp = cp + colSizeBytes)
            {
                uint16_t const* cpus16 = reinterpret_cast<uint16_t const*>(cp);
                uint16_t* wcp = workingColors;
                for(auto i = 0ul; i < colSize*VERTS_PER_POINT; ++i)
                {
                    *wcp = *cpus16;
                    ++cpus16;
                    ++wcp;
                }
            }

            //run vertex shader
            vf_(wv0);

            //clip near
            if(clip_point_near(wv0))
            {
                //project to ndc
                project_to_ndc(wv0);

                //map to screen
            }






        }



    }




protected:

    auto clip_point_near(vec3 const & p) const -> bool
    {
        return p.z >= near_z_;
    }

    auto clip_point_ndc(vec3 const & p) -> bool
    {

    }


    auto clip_line_near(vec3& v0, vec3& v1) -> bool
    {
        //trivial pass
            return true;

        //trivial fai
            return false;


        //clamp if partial
        return false;
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
        p.x = sx * (screen_width_fx_ - 1.0_fx);
        p.y = sy * (screen_height_fx_ - 1.0_fx);
    }


    enum class ClipResult
    {
        Accept,
        Reject,
        Partial
    };

    [[nodiscard]] auto clip_triangle_trivial(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> ClipResult
    {
        bool in0 = (v0.z >= near_z_);
        bool in1 = (v1.z >= near_z_);
        bool in2 = (v2.z >= near_z_);

        if (in0 && in1 && in2) return ClipResult::Accept;
        if (!in0 && !in1 && !in2) return ClipResult::Reject;
        return ClipResult::Partial;
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

    [[nodiscard]] auto clip_quad_trivial(vec3 const& v0, vec3 const& v1, vec3 const& v2, vec3 const& v3) -> ClipResult
    {
        bool in0 = (v0.z >= near_z_);
        bool in1 = (v1.z >= near_z_);
        bool in2 = (v2.z >= near_z_);
        bool in3 = (v3.z >= near_z_);

        if (in0 && in1 && in2 && in3) return ClipResult::Accept;
        if (!in0 && !in1 && !in2 && !in3) return ClipResult::Reject;
        return ClipResult::Partial;
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
    DrawType current_draw_type_{DrawType::Points};

    int32_t render_width_{0};
    int32_t render_height_{0};
    fixed32 render_width_fx_{0.0_fx};
    fixed32 render_height_fx_{0.0_fx};

    fixed32 aspect_ratio_{0.0_fx};
    fixed32 near_z_{0.0_fx};

    FaceCullMode cull_{FaceCullMode::All};


};



}
