/*
 * BitValue.hpp
 *
 *  Created on: Nov 29, 2015
 *      Author: marukyu
 */

#ifndef SRC_SHARED_UTILS_BITSTREAM_BITVALUE_HPP_
#define SRC_SHARED_UTILS_BITSTREAM_BITVALUE_HPP_

#include <cstdint>

namespace bits
{
class InputBitStream;

template <unsigned int bitGroup, unsigned int... bitGroups>
class Int
{
};

typedef Int<7, 7, 7, 7, 7, 7, 7, 7, 8> VInt64;
typedef Int<7, 7, 7, 7, 4> VInt32;

template <unsigned int bits>
class FixInt
{
};

template <unsigned int bitGroup>
InputBitStream & operator<<(InputBitStream & stream, const Int<bitGroup> & number)
{
	uint64_t value = number;
}

template <unsigned int bitGroup, unsigned int... bitGroups>
InputBitStream & operator<<(InputBitStream & stream, const Int<bitGroup, bitGroups...> & number)
{
	Int<bitGroup, bitGroups...> maskedNumber = number & ((1 << bitGroup) - 1);

	// Check if the remaining value is small enough to be represented entirely within the current bit group.
	if (number != maskedNumber)
	{
		// Number is not small enough: encode current part of number, then recurse with downshifted number.
		Int<bitGroups...> reducedNumber = number >> bitGroup;
	}
	else
	{
		// Number is small enough: encode current part of number, then stop.
		// maskedNumber = ;
	}

	return stream;
}

}

#endif
