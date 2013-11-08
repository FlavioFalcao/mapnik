/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2013 Artem Pavlenko
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

#ifndef MAPNIK_SYMBOLIZER_HPP
#define MAPNIK_SYMBOLIZER_HPP

// mapnik
#include <mapnik/config.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/image_compositing.hpp>
#include <mapnik/simplify.hpp>
#include <mapnik/expression.hpp>
#include <mapnik/expression_evaluator.hpp>
#include <mapnik/path_expression.hpp>
#include <mapnik/parse_path.hpp>
#include <mapnik/color.hpp>
#include <mapnik/symbolizer_keys.hpp>
// stl
#include <memory>
#include <vector>
#include <string>

namespace agg { struct trans_affine; }

namespace mapnik
{

// fwd declares
// TODO - move these transform declares to own header
namespace detail { struct transform_node; }
typedef std::vector<detail::transform_node> transform_list;
typedef std::shared_ptr<transform_list>   transform_list_ptr;
typedef transform_list_ptr transform_type;
class feature_impl;

MAPNIK_DECL void evaluate_transform(agg::trans_affine& tr, feature_impl const& feature,
                                    transform_type const& trans_expr);

struct  MAPNIK_DECL symbolizer_base
{
    typedef boost::variant<bool,
                           value_integer,
                           value_double,
                           std::string,
                           mapnik::color,
                           mapnik::expression_ptr,
                           mapnik::path_expression_ptr,
                           mapnik::transform_type> value_type;
    typedef mapnik::keys key_type;
    typedef std::map<key_type, value_type> cont_type;
    cont_type properties;
};

template <typename T>
struct evaluate_path_wrapper
{
    typedef T result_type;
    template <typename T1, typename T2>
    result_type operator() (T1 const& expr, T2 const& feature) const
    {
        return result_type();
    }

};

template <>
struct evaluate_path_wrapper<std::string>
{
    template <typename T1, typename T2>
    std::string operator() (T1 const& expr, T2 const& feature) const
    {
        return mapnik::path_processor_type::evaluate(expr, feature);
    }
};


template <typename T>
struct evaluate_expression_wrapper
{
    typedef T result_type;
    template <typename T1, typename T2>
    result_type operator() (T1 const& expr, T2 const& feature) const
    {
        mapnik::value_type result = boost::apply_visitor(mapnik::evaluate<mapnik::feature_impl,mapnik::value_type>(feature), expr);
        return result.convert<result_type>();
    }
};

// specializations for not supported types
// mapnik::color
template <>
struct evaluate_expression_wrapper<mapnik::color>
{
    template <typename T1, typename T2>
    mapnik::color operator() (T1 const& expr, T2 const& feature) const
    {
        return mapnik::color();
    }
};


template <typename T1>
struct extract_value : public boost::static_visitor<T1>
{
    typedef T1 result_type;

    extract_value(mapnik::feature_impl const& feature)
        : feature_(feature) {}

    auto operator() (mapnik::expression_ptr const& expr) const -> result_type
    {
        return evaluate_expression_wrapper<result_type>()(*expr,feature_);
    }

    auto operator() (mapnik::path_expression_ptr const& expr) const -> result_type
    {
        return evaluate_path_wrapper<result_type>()(*expr, feature_);
    }

    auto operator() (result_type const& val) const -> result_type
    {
        return val;
    }

    template <typename T2>
    auto operator() (T2 const& val) const -> result_type
    {
        //std::cerr << val << " " << typeid(val).name() <<" " << typeid(result_type).name() << std::endl;
        return result_type();
    }

    mapnik::feature_impl const& feature_;
};

template <typename T1>
struct extract_raw_value : public boost::static_visitor<T1>
{
    typedef T1 result_type;

    auto operator() (result_type const& val) const -> result_type const&
    {
        return val;
    }

    template <typename T2>
    auto operator() (T2 const& val) const -> result_type
    {
        //std::cerr << val << " " << typeid(val).name() <<" " << typeid(result_type).name() << std::endl;
        return result_type();
    }
};

template <typename T>
void put(symbolizer_base & sym, keys key, T const& val)
{
    sym.properties.insert(std::make_pair(key, val));
}

template <typename T>
T get(symbolizer_base const& sym, keys key, mapnik::feature_impl const& feature, T const& default_value = T())
{
    typedef symbolizer_base::cont_type::const_iterator const_iterator;
    const_iterator itr = sym.properties.find(key);
    if (itr != sym.properties.end())
    {
        return boost::apply_visitor(extract_value<T>(feature), itr->second);
    }
    return default_value;
}

template <typename T>
T get(symbolizer_base const& sym, keys key, T const& default_value = T())
{
    typedef symbolizer_base::cont_type::const_iterator const_iterator;
    const_iterator itr = sym.properties.find(key);
    if (itr != sym.properties.end())
    {
        return boost::apply_visitor(extract_raw_value<T>(), itr->second);
    }
    return default_value;
}

}

#endif // MAPNIK_SYMBOLIZER_HPP
