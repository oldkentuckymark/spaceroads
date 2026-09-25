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
concept HasPlot = requires(T t, int16_t x, int16_t y, uint16_t color) {
    { t.plot(x, y, color) } -> std::same_as<void>;
};

template<typename T>
concept HasLine = requires {
    requires std::is_same_v<
        decltype(&T::line),
        auto (T::*)(int16_t, int16_t, int16_t, int16_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasLineHorizontal = requires {
    requires std::is_same_v<
        decltype(&T::lineHorizontal),
        auto (T::*)(int16_t, int16_t, int16_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasLineVertical = requires {
    requires std::is_same_v<
        decltype(&T::lineVertical),
        auto (T::*)(int16_t, int16_t, int16_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasTriangle = requires {
    requires std::is_same_v<
        decltype(&T::triangle),
        auto (T::*)(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) -> void
        >;
};

template<typename T>
concept HasQuad = requires {
    requires std::is_same_v<
        decltype(&T::quad),
        auto (T::*)(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) -> void
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
                      "CRITICAL: Your derived platform renderer class must implement void plot(int16_t x, int16_t y, uint16_t color).");
    }
    ~BaseContext() = default;

    BaseContext(BaseContext&) = delete;
    auto operator = (BaseContext&) = delete;
    BaseContext(BaseContext&&) = delete;
    auto operator = (BaseContext&&) = delete;

    auto plot(int16_t x, int16_t y, Color color) -> void
    {
        derived().plot(x, y, color);
    }

    auto line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color) -> void
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

    auto lineHorizontal(int16_t x0, int16_t y0, int16_t x1, uint16_t color) -> void
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

     auto lineVertical(int16_t x0, int16_t y0, int16_t y1, uint16_t color) -> void
    {
         if constexpr (HasLineVertical<Derived>) {
             derived().lineVertical(x0, y0, y1, color);
         } else {
             // Default vertical plotting loop

        line(x0, y0, x0, y1, color);
         }
    }

    //use extra pixel/s to fill gaps?
    auto triangle(int16_t x0, int16_t y0,
                  int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2,
                  uint16_t color) -> void
    {
        struct V { int16_t x, y; };
        V v0{ x0, y0 }, v1{ x1, y1 }, v2{ x2, y2 };

        // Sort vertices by y ascending
        if (v1.y < v0.y) std::swap(v0, v1);
        if (v2.y < v1.y) std::swap(v1, v2);
        if (v1.y < v0.y) std::swap(v0, v1);

        // If no vertical extent, draw single horizontal span
        if (v0.y == v2.y) {
            int16_t xmin = std::min({ v0.x, v1.x, v2.x });
            int16_t xmax = std::max({ v0.x, v1.x, v2.x });
            lineHorizontal(xmin, v0.y, xmax, color);
            return;
        }

        // Edge walker: produces an integer x for each scanline y in [y0, y_end)
        struct EdgeWalker {
            int32_t x;      // current x for current y
            int32_t dx;     // abs(delta x)
            int32_t dy;     // delta y (positive)
            int32_t sx;     // sign of delta x
            int32_t err;    // error accumulator
            int16_t y;      // current y
            int16_t y_end;  // end y (stop when y == y_end)

            void init(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
                x = x0;
                int32_t ddx = int32_t(x1) - int32_t(x0);
                dx = ddx >= 0 ? ddx : -ddx;
                sx = (ddx >= 0) ? 1 : -1;
                dy = int32_t(y1) - int32_t(y0);
                if (dy < 0) dy = 0; // we only use walkers with y1 >= y0
                err = 0;
                y = y0;
                y_end = y1;
            }

            // Return current x for the current scanline y.
            int32_t currentX() const { return x; }

            // Advance walker to the next scanline (increment y by 1).
            // After calling advance, y is incremented (or set to y_end if dy==0).
            void advance() {
                if (y >= y_end) return; // already finished
                // If the edge has zero vertical span, jump to end.
                if (dy == 0) {
                    y = y_end;
                    return;
                }
                // Standard Bresenham-like vertical stepping:
                // err += dx; while (err >= dy) { x += sx; err -= dy; }
                err += dx;
                // Use while loop to handle steep slopes where dx >> dy without division.
                while (err >= dy) {
                    x += sx;
                    err -= dy;
                }
                ++y;
            }

            // Whether walker still has a valid x for the current y (y < y_end)
            bool active() const { return y < y_end; }
        };

        EdgeWalker e_long;   // v0 -> v2
        EdgeWalker e_top;    // v0 -> v1
        EdgeWalker e_bot;    // v1 -> v2

        e_long.init(v0.x, v0.y, v2.x, v2.y);
        e_top.init(v0.x, v0.y, v1.x, v1.y);
        e_bot.init(v1.x, v1.y, v2.x, v2.y);

        // Iterate scanlines from top to bottom (y in [v0.y, v2.y) )
        for (int16_t y = v0.y; y < v2.y; ++y) {
            // Get x on long edge for this scanline if active; otherwise clamp to endpoint x.
            int32_t xl = e_long.active() ? e_long.currentX() : e_long.x;

            // Choose which short edge to sample for this scanline:
            // - upper part: use top edge for y in [v0.y, v1.y)
            // - lower part: use bottom edge for y in [v1.y, v2.y)
            int32_t xr;
            if (y < v1.y) {
                // If top edge has zero height (v0.y == v1.y), use v1.x directly.
                xr = e_top.active() ? e_top.currentX() : e_top.x;
            } else {
                xr = e_bot.active() ? e_bot.currentX() : e_bot.x;
            }

            // Determine left/right and draw inclusive horizontal span.
            int16_t xleft  = int16_t(std::min(xl, xr));
            int16_t xright = int16_t(std::max(xl, xr));
            lineHorizontal(xleft, y, xright, color);

            // Advance walkers for next scanline
            e_long.advance();
            if (y < v1.y) e_top.advance(); else e_bot.advance();
        }
    }

    auto quad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color) -> void
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

    auto setViewPort(int16_t const w, int16_t const h) -> void
    {
        viewport_width_ = w;
        viewport_height_ = h;
        viewport_width_fx_ = static_cast<fixed32>(w);
        viewport_height_fx_ = static_cast<fixed32>(h);
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
            else if(current_draw_type_ == DrawType::Triangles)
            {
                    auto outVerts = clip_polygon_near_z<3>(std::span<vec3,3>(&working_vertex_buffer_[i],3));
                    for(auto k = 0ul; k < outVerts.size(); k = k + 3)
                    {
                        if(true ||is_cull_passing(outVerts[k+0],outVerts[k+1],outVerts[k+2]))
                        {
                        project_to_ndc(outVerts[k+0]);project_to_ndc(outVerts[k+1]);project_to_ndc(outVerts[k+2]);
                        if(is_cull_passing(outVerts[k+0],outVerts[k+1],outVerts[k+2]))
                        {
                        to_screen_space(outVerts[k+0]);to_screen_space(outVerts[k+1]);to_screen_space(outVerts[k+2]);
                        triangle(fixed32::round(outVerts[k+0].x), fixed32::round(outVerts[k+0].y),
                                 fixed32::round(outVerts[k+1].x), fixed32::round(outVerts[k+1].y),
                                 fixed32::round(outVerts[k+2].x), fixed32::round(outVerts[k+2].y),ccs);
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
            else if(current_draw_type_ == DrawType::Quads)
            {
                    auto outVerts = clip_polygon_near_z<4>(std::span<vec3,4>(&working_vertex_buffer_[i],4));
                    for(auto k = 0ul; k < outVerts.size(); k = k + 4)
                    {
                        if(true ||is_cull_passing(outVerts[k+0],outVerts[k+1],outVerts[k+2]))
                        {
                        project_to_ndc(outVerts[k+0]);project_to_ndc(outVerts[k+1]);project_to_ndc(outVerts[k+2]);project_to_ndc(outVerts[k+3]);
                        if(is_cull_passing(outVerts[k+0],outVerts[k+1],outVerts[k+2]))
                        {
                            to_screen_space(outVerts[k+0]);to_screen_space(outVerts[k+1]);to_screen_space(outVerts[k+2]);to_screen_space(outVerts[k+3]);
                            quad(static_cast<int16_t>(outVerts[k+0].x),static_cast<int16_t>(outVerts[k+0].y),
                                 static_cast<int16_t>(outVerts[k+1].x),static_cast<int16_t>(outVerts[k+1].y),
                                 static_cast<int16_t>(outVerts[k+2].x),static_cast<int16_t>(outVerts[k+2].y),
                                 static_cast<int16_t>(outVerts[k+3].x),static_cast<int16_t>(outVerts[k+3].y),ccs);
                        }
                        }


                    }


                i = i + 3;
                col = col + 4;
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

     auto clip_triangle_near(vec3 const & v0, vec3 const & v1, vec3 const & v2) -> std::inplace_vector<vec3, 8>
    {
        //trivial pass
        if(is_point_inside_near(v0) && is_point_inside_near(v1) && is_point_inside_near(v2))
        {
            return {v0,v1,v2};
        }

        //trivial fail
        else if((!is_point_inside_near(v0)) && (!is_point_inside_near(v1)) && (!is_point_inside_near(v2)))
        {
            return {};
        }

        //clamp if partial
        return {};

    }

     auto clip_quad_near(vec3 const & v0, vec3 const & v1, vec3 const & v2, vec3 const & v3) -> std::inplace_vector<vec3, 8>
    {
        //trivial pass
        if(is_point_inside_near(v0) && is_point_inside_near(v1) && is_point_inside_near(v2) && (is_point_inside_near(v3)))
        {
            return {v0,v1,v2,v3};
        }

        //trivial fail
        else if((!is_point_inside_near(v0)) && (!is_point_inside_near(v1)) && (!is_point_inside_near(v2)) && (!is_point_inside_near(v3)))
        {
            return {};
        }

        //clamp if partial
        return {};
    }

     auto clip_horizontal_line_screen(int16_t& x0, int16_t& y0, int16_t& x1) -> int32_t
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

    template <int N> requires (N == 3 || N == 4)
     [[nodiscard]] auto clip_polygon_near_z(std::span<vec3 const, N> const verts) -> std::inplace_vector<vec3, ((5 - 2 + (N - 2) - 1) / (N - 2)) * N>
    {

        std::inplace_vector<vec3, 5> clipped;

        for (std::size_t i = 0; i < verts.size(); ++i)
        {
            std::size_t next_i = i + 1 == verts.size() ? 0 : i + 1;
            vec3 const& current = verts[i];
            vec3 const& next = verts[next_i];

            const bool currentIn = current.z >= near_z_;
            const bool nextIn = next.z >= near_z_;

            if (currentIn)
            {
                clipped.emplace_back(current);
            }

            if (currentIn != nextIn)
            {
                const fixed32 t = (near_z_ - current.z) / (next.z - current.z);
                clipped.emplace_back( current.x + (next.x - current.x) * t, current.y + (next.y - current.y) * t, current.z + (next.z - current.z) * t );
            }
        }

        std::inplace_vector<vec3, ((5 - 2 + (N - 2) - 1) / (N - 2)) * N> output;

        const std::size_t m = clipped.size();

        if constexpr(N == 3)
        {
            for (std::size_t i = 1; i + 1 < m; ++i)
            {
                output.push_back(clipped[0]);
                output.push_back(clipped[i]);
                output.push_back(clipped[i + 1]);
            }
        }
        else if constexpr (N == 4)
        {
            for (std::size_t i = 1; i + 1 < m; i += 2)
            {
                output.push_back(clipped[0]);
                output.push_back(clipped[i]);
                output.push_back(clipped[i + 1]);
                output.push_back(i + 2 < m ? clipped[i + 2] : clipped[i + 1]);
            }
        }

        return output;
    }

     auto project_to_ndc(vec3& p) -> void
    {
        p.x = p.x * aspect_ratio_;
        //p.x = p.x / p.z;
        //p.y = p.y / p.z;
        p.x = p.x * invZ(p.z);
        p.y = p.y * invZ(p.z);
    }

     [[nodiscard]] auto is_cull_passing(vec3 const& v0, vec3 const& v1, vec3 const& v2) -> bool
    {
        if (cull_ == FaceCullMode::None) { return true; }
        else if (cull_ == FaceCullMode::All) { return false; }
        else
        {
            const auto ab_x = v1.x - v0.x;
            const auto ab_y = v1.y - v0.y;
            const auto ac_x = v2.x - v1.x;
            const auto ac_y = v2.y - v1.y;
            const auto nz = ab_x * ac_y - ab_y * ac_x;

            if (cull_ == FaceCullMode::Back) [[likely]] { return nz > 0.0_fx; }
            else if (cull_ == FaceCullMode::Front) { return nz < 0.0_fx; }
        }
        return false;
    }

     auto to_screen_space(vec3& p) -> void
    {
        // Map from [-1, +1] → [0, 1]
        fixed32 sx = (p.x + 1.0_fx).halved();
        fixed32 sy = (1.0_fx - p.y).halved();

        // Scale to viewport
        p.x = sx * viewport_width_fx_ - 1.0_fx;
        p.y = sy * viewport_height_fx_ - 1.0_fx;
    }


    [[no_unique_address]] VERTEX_FUNCTION vf_;

    void const* vertex_pointer_{nullptr};
    uint32_t vertex_size_{0};
    uint32_t vertex_stride_{0};
    void const* color_pointer_{nullptr};
    uint16_t color_stride_{0};
    DrawType current_draw_type_{DrawType::Points};

    int16_t viewport_width_{0};
    int16_t viewport_height_{0};
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
