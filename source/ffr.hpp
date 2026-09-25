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

        if (y2 <= viewport_y_ || y0 >= viewport_height_) return; // outside viewport, vertically
        if (y0 == y2) return;                                // zero height, zero area

        int32_t xmin = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int32_t xmax = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        if (xmax < viewport_x_ || xmin >= viewport_width_) return; // outside viewport, horizontally


        int32_t cross = (int32_t)(x2 - x0) * (y1 - y0) - (int32_t)(y2 - y0) * (x1 - x0);
        bool longIsLeft = cross < 0;

        EdgeWalker longEdge;
        longEdge.init(x0, y0, x2, y2);

        EdgeWalker shortEdge;
        bool inTopHalf = (y0 != y1);
        if (inTopHalf) shortEdge.init(x0, y0, x1, y1);
        else           shortEdge.init(x1, y1, x2, y2);

        int yEnd = y2 < viewport_height_ ? y2 : viewport_height_;

        for (int y = y0; y < yEnd; ++y) {
            if (y >= viewport_y_) {
                int32_t xa = longEdge.x, xb = shortEdge.x;
                int32_t xl = longIsLeft ? xa : xb;
                int32_t xr = longIsLeft ? xb : xa;
                if (xl < viewport_x_) xl = viewport_x_;
                if (xr > viewport_width_) xr = viewport_width_;
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

    auto setViewPort(int32_t const x, int32_t const y, int32_t const w, int32_t const h) -> void
    {
        viewport_x_ = x;
        viewport_y_ = y;
        viewport_width_ = w;
        viewport_height_ = h;
        viewport_x_fx_ = x;
        viewport_y_fx_ = y;
        viewport_width_fx_ = w;
        viewport_height_fx_ = h;
        //aspect_ratio_ = 1.0_fx / (viewport_width_fx_ / viewport_height_fx_);
        aspect_ratio_ = 1.0_fx / (viewport_width_fx_ / viewport_height_fx_);
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


        size_t col = 0;
        for(uint32_t i = 0; i < working_vertex_buffer_.size(); ++i)
        {
            uint16_t const & ccs{working_color_buffer_[col]};

            if(current_draw_type_ == DrawType::Points)
            {
                vec3& cvs = working_vertex_buffer_[i];



                col = col + 1;
            }
            else if(current_draw_type_ == DrawType::Lines)
            {
                vec3& p0{working_vertex_buffer_[i]};
                vec3& p1{working_vertex_buffer_[i+1]};



                i = i + 1;;
                col = col + 2;
            }
            if (current_draw_type_ == DrawType::Triangles)
            {
                vec3& v0 = working_vertex_buffer_[i];
                vec3& v1 = working_vertex_buffer_[i+1];
                vec3& v2 = working_vertex_buffer_[i+2];

                if (auto res = clip_triangle_trivial(v0, v1, v2); res == ClipResult::Accept)
                {
                    project_to_ndc(v0); project_to_ndc(v1); project_to_ndc(v2);
                    to_screen_space(v0); to_screen_space(v1); to_screen_space(v2);
                    if (is_cull_passing(v0, v1, v2))
                    {
                        triangle(fixed32::round(v0.x), fixed32::round(v0.y),
                                 fixed32::round(v1.x), fixed32::round(v1.y),
                                 fixed32::round(v2.x), fixed32::round(v2.y), ccs);
                    }
                }
                else if (false && res == ClipResult::Partial)
                {
                    auto outVerts = clip_triangle_accurate(v0, v1, v2);
                    for (size_t k = 0; k + 2 < outVerts.size(); k += 3)
                    {
                        project_to_ndc(outVerts[k]); project_to_ndc(outVerts[k+1]); project_to_ndc(outVerts[k+2]);
                        to_screen_space(outVerts[k]); to_screen_space(outVerts[k+1]); to_screen_space(outVerts[k+2]);
                        if (is_cull_passing(outVerts[k], outVerts[k+1], outVerts[k+2]))
                        {
                            triangle(fixed32::round(outVerts[k].x), fixed32::round(outVerts[k].y),
                                     fixed32::round(outVerts[k+1].x), fixed32::round(outVerts[k+1].y),
                                     fixed32::round(outVerts[k+2].x), fixed32::round(outVerts[k+2].y), ccs);
                        }
                    }
                }
                i = i + 2;
                col = col + 3;
            }
            else if(current_draw_type_ == DrawType::TrianglesWireFrame)
            {
                vec3& v0{working_vertex_buffer_[i]};
                vec3& v1{working_vertex_buffer_[i+1]};
                vec3& v2{working_vertex_buffer_[i+2]};



                i = i + 2;
                col = col + 3;
            }
            else if (current_draw_type_ == DrawType::Quads)
            {
                vec3& v0 = working_vertex_buffer_[i];
                vec3& v1 = working_vertex_buffer_[i+1];
                vec3& v2 = working_vertex_buffer_[i+2];
                vec3& v3 = working_vertex_buffer_[i+3];

                if (auto res = clip_quad_trivial(v0, v1, v2, v3); res == ClipResult::Accept)
                {
                    project_to_ndc(v0); project_to_ndc(v1); project_to_ndc(v2); project_to_ndc(v3);
                    to_screen_space(v0); to_screen_space(v1); to_screen_space(v2); to_screen_space(v3);

                    if(is_cull_passing(v0,v1,v2))
                    {
                        quad(fixed32::round(v0.x), fixed32::round(v0.y),
                            fixed32::round(v1.x), fixed32::round(v1.y),
                            fixed32::round(v2.x), fixed32::round(v2.y),
                            fixed32::round(v3.x), fixed32::round(v3.y), ccs);
                    }
                }
                else if (false && res == ClipResult::Partial)
                {
                    // NOTE: Accurate quad clipping returns TRIANGLES. We iterate by 3, not 4!
                    auto outVerts = clip_quad_accurate(v0, v1, v2, v3);
                    for (size_t k = 0; k + 2 < outVerts.size(); k += 3)
                    {
                        project_to_ndc(outVerts[k]); project_to_ndc(outVerts[k+1]); project_to_ndc(outVerts[k+2]);
                        to_screen_space(outVerts[k]); to_screen_space(outVerts[k+1]); to_screen_space(outVerts[k+2]);
                        if (is_cull_passing(outVerts[k], outVerts[k+1], outVerts[k+2]))
                        {
                            triangle(fixed32::round(outVerts[k].x), fixed32::round(outVerts[k].y),
                                     fixed32::round(outVerts[k+1].x), fixed32::round(outVerts[k+1].y),
                                     fixed32::round(outVerts[k+2].x), fixed32::round(outVerts[k+2].y), ccs);
                        }
                    }
                }
                i += 3;
                col += 4;
            }
        }

    }




protected:

     auto is_point_inside_near(vec3 const & p) -> bool
    {
        return p.z > near_z_;
    }


     auto clip_line_near(vec3& v0, vec3& v1) -> bool
    {
        //trivial pass
        if(is_point_inside_near(v0) && is_point_inside_near(v1))
        {
            return true;
        }

        //trivial fail
        else if((!is_point_inside_near(v0)) && (!is_point_inside_near(v1)))
        {
            return false;
        }

        //clamp if partial
        return false;
    }

     auto clip_horizontal_line_screen(int32_t& x0, int32_t& y0, int32_t& x1) -> int32_t
    {
        if (y0 < 0)                { return -1; }   // above screen: skip this row only
        if (y0 >= viewport_height_) { return 0; }    // below screen: nothing further can be visible
        util::order(x0,x1);
        if (x0 < 0 && x1 < 0)      { return -1; }
        if (x0 >= viewport_width_ && x1 >= viewport_width_) { return -1; }
        if(x0 < 0) {x0 = 0;}
        if(x1 >= viewport_width_) {x1 = viewport_width_ - 1;}
        return 1;
    }



     auto project_to_ndc(vec3& p) -> void
    {
        p.x = p.x * aspect_ratio_;
        p.x = p.x * invZ(p.z);
        p.y = p.y * invZ(p.z);
    }

    [[nodiscard]] auto is_cull_passing(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> bool
    {
        if (cull_ == FaceCullMode::None) { return true; }
        if (cull_ == FaceCullMode::All) { return false; }

        // Edge vectors sharing vertex v0
        const auto ab_x = v1.x - v0.x;
        const auto ab_y = v1.y - v0.y;
        const auto ac_x = v2.x - v0.x; // Fixed: using v2 - v0 (AC) instead of v2 - v1 (BC)
        const auto ac_y = v2.y - v0.y;

        // 2D cross product determinant (Z-component of AB x AC)
        const auto nz = ab_x * ac_y - ab_y * ac_x;

        if (cull_ == FaceCullMode::Back) [[likely]] {
            return nz < 0.0_fx; // Adjust to < 0.0_fx if your geometry uses Clockwise (CW) front faces
        }
        else if (cull_ == FaceCullMode::Front) {
            return nz > 0.0_fx;
        }

        return false;
    }

    auto to_screen_space(vec3& p) -> void
    {
        // Map from [-1, +1] → [0, 1]
        fixed32 sx = (p.x + 1.0_fx).halved();
        fixed32 sy = (1.0_fx - p.y).halved();

        // Scale to viewport
        p.x = sx * (viewport_width_fx_ + viewport_x_fx_);
        p.y = sy * viewport_height_fx_ + viewport_y_fx_;
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

    int32_t viewport_x_{0};
    int32_t viewport_y_{0};
    fixed32 viewport_x_fx_{0.0_fx};
    fixed32 viewport_y_fx_{0.0_fx};
    int32_t viewport_width_{0};
    int32_t viewport_height_{0};
    fixed32 viewport_width_fx_{0.0_fx};
    fixed32 viewport_height_fx_{0.0_fx};
    fixed32 aspect_ratio_{0_fx};
    fixed32 near_z_{0.0_fx};

    std::inplace_vector<vec3,MAX_VERTS> working_vertex_buffer_;
    std::inplace_vector<uint16_t,MAX_VERTS> working_color_buffer_;
    std::inplace_vector<vec3, 16> post_clip_verts1_;
    std::inplace_vector<vec3, 16> post_clip_verts2_;

    FaceCullMode cull_{FaceCullMode::All};


};



}
