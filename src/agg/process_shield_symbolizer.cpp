/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/agg_rasterizer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/symbolizer_helpers.hpp>
#include <mapnik/pixel_position.hpp>
#include <mapnik/font_util.hpp>

// boost

namespace mapnik {

template <typename T0, typename T1>
void  agg_renderer<T0,T1>::process(shield_symbolizer const& sym,
                               mapnik::feature_impl & feature,
                               proj_transform const& prj_trans)
{
    box2d<double> clip_box = clipping_extent();
    shield_symbolizer_helper<face_manager<freetype_engine>,
        label_collision_detector4> helper(
            sym, feature, prj_trans,
            width_, height_,
            scale_factor_,
            t_, font_manager_, *detector_,
            clip_box);

    halo_rasterizer_enum halo_rasterizer = get<halo_rasterizer_enum>(sym, keys::halo_rasterizer, feature);
    composite_mode_e comp_op = get<composite_mode_e>(sym, keys::comp_op, feature, src_over);
    text_renderer<buffer_type> ren(*current_buffer_,
                                   font_manager_,
                                   halo_rasterizer,
                                   comp_op,
                                   scale_factor_);

    double opacity = get<double>(sym,keys::opacity,feature, 1.0);

    while (helper.next())
    {
        placements_type const& placements = helper.placements();
        for (unsigned int ii = 0; ii < placements.size(); ++ii)
        {
            // get_marker_position returns (minx,miny) corner position,
            // while (currently only) agg_renderer::render_marker newly
            // expects center position;
            // until all renderers and shield_symbolizer_helper are
            // modified accordingly, we must adjust the position here
            pixel_position pos = helper.get_marker_position(placements[ii]);
            pos.x += 0.5 * helper.get_marker_width();
            pos.y += 0.5 * helper.get_marker_height();
            render_marker(pos,
                          helper.get_marker(),
                          helper.get_image_transform(),
                          opacity,
                          comp_op);

            ren.prepare_glyphs(placements[ii]);
            ren.render(placements[ii].center);
        }
    }
}


template void agg_renderer<image_32>::process(shield_symbolizer const&,
                                              mapnik::feature_impl &,
                                              proj_transform const&);

}
