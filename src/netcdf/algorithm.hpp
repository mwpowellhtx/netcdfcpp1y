#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <vector>
#include <functional>

#pragma once

template<typename _Ty, typename _AggResult>
_AggResult aggregate(
    std::vector<_Ty> const & arr
    , _AggResult value
    , std::function<void(_AggResult &, _Ty const &)> const & func) {

    auto result = value;

    for (auto current = arr.cbegin(); current != arr.cend(); current++) {
        func(result, *current);
    }

    return result;
}

#endif //ALGORITHM_HPP
