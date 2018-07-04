/*
 * const_array.h
 *
 *  Created on: Jul 3, 2018
 *      Author: mikaelr
 */

#ifndef SRC_BITOPS_CONST_ARRAY_H_
#define SRC_BITOPS_CONST_ARRAY_H_

#include <cstdlib>

/**
 * Helper array to set up constexpr arrays.
 */
template <typename T, std::size_t N>
class const_array
{
    constexpr static std::size_t size_ = N;
    T data_[N]{};

  public:
    constexpr std::size_t size() const
    {
        return N;
    }
    constexpr T& operator[](std::size_t n)
    {
        return data_[n];
    }
    constexpr const T& operator[](std::size_t n) const
    {
        return data_[n];
    }
    using iterator = T*;
    constexpr iterator begin()
    {
        return &data_[0];
    }
    constexpr iterator end()
    {
        return &data_[N];
    }
};

#endif /* SRC_BITOPS_CONST_ARRAY_H_ */
