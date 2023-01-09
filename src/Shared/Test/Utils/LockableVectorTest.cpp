/*
#include <gtest/gtest.h>

#include "Shared/Utils/LockableVector.hpp"

static const int arraySize = 10;

static LockableVector<int> getTestVector()
{
    LockableVector<int> vec;

    for (unsigned int i = 0; i < arraySize; ++i)
    {
        vec.push_back((int)i);
    }

    return vec;
}

TEST(LockableVectorTest, ForwardIteration)
{
    LockableVector<int> vec = getTestVector();

    int i = 0;

    for (auto it = vec.lbegin(); it != vec.lend(); ++it)
    {
        ASSERT_EQ(i, *it);
        ++i;
    }
}

TEST(LockableVectorTest, ReverseIteration)
{
    LockableVector<int> vec = getTestVector();

    int i = arraySize - 1;

    for (auto it = vec.lrbegin(); it != vec.lrend(); ++it)
    {
        ASSERT_EQ(i, *it);
        --i;
    }
}

TEST(LockableVectorTest, Locking)
{
    LockableVector<int> vec = getTestVector();

    auto it = vec.lbegin();
    int i = 0;

    vec.invalidate();
    vec.clear();

    for (; it != vec.lend(); ++it)
    {
        ASSERT_EQ(i, *it);
        ++i;
    }
}
 */
