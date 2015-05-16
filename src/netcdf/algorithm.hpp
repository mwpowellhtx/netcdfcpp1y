#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <vector>
#include <functional>

#pragma once

template<
    typename _Ty
    , typename _Result
    , class _AggregateFunc = std::function<_Result(_Result const &, _Ty const &)>
    , typename _Vector = std::vector<_Ty>
>
_Result aggregate(
    _Vector const & values
    , _Result value
    , _AggregateFunc const & func) {

    auto result = value;

    for (auto current = values.cbegin(); current != values.cend(); current++) {
        result = func(result, *current);
    }

    return result;
}

#endif //ALGORITHM_HPP
