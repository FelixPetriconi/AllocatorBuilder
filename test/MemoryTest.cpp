///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#include <gtest/gtest.h>
#include <alb/allocator_base.hpp>
#include <alb/affix_allocator.hpp>
#include <alb/mallocator.hpp>
#include "TestHelpers/AllocatorBaseTest.h"

using MyAllocator = alb::affix_allocator<alb::mallocator, size_t>;

struct HelpObjectNonTrivialDestructable
{
  HelpObjectNonTrivialDestructable()
    : v(42)
    , s(std::to_string(++objectCounter))
  {}

  HelpObjectNonTrivialDestructable(int p1, const std::string& p2)
    : v(p1)
    , s(p2)
  {
    ++objectCounter;
  }

  ~HelpObjectNonTrivialDestructable()
  {
    --objectCounter;
  }
  
  int v;
  std::string s;
  static int objectCounter;
};

int HelpObjectNonTrivialDestructable::objectCounter = 0;


class MakeUniqueTest : public alb::test_helpers::AllocatorBaseTest<MyAllocator>
{
};

TEST_F(MakeUniqueTest, TestThatMakeUniqueOfASingleObjectWithoutPassingCTorsArgumentsAllocatesAnObject)
{
  {
    auto t = alb::make_unique<HelpObjectNonTrivialDestructable>(sut);
    EXPECT_EQ(1, HelpObjectNonTrivialDestructable::objectCounter);
  }
  EXPECT_EQ(0, HelpObjectNonTrivialDestructable::objectCounter);
}

TEST_F(MakeUniqueTest, TestThatMakeUniqueOfASingleObjectWithPassingCtorsArgumentsAllocatesAnObject)
{
  {
    auto t = alb::make_unique<HelpObjectNonTrivialDestructable>(sut, 42, "Don't panic" );
    EXPECT_EQ(1, HelpObjectNonTrivialDestructable::objectCounter);
    EXPECT_EQ(42, t->v);
    EXPECT_EQ(std::string("Don't panic"), t->s);
  }
  EXPECT_EQ(0, HelpObjectNonTrivialDestructable::objectCounter);
}

TEST_F(MakeUniqueTest, TestThatMakeUniqueOfMultipleNonDefaultDestructableObjectsWithoutPassingCtorsArgumentsAllocatesAnObject)
{
  {
    auto t = alb::make_unique<HelpObjectNonTrivialDestructable[]>(sut, 42);
    for (auto i = 0u; i < 42; ++i)
    {
      EXPECT_EQ(42, t[i].v);
      EXPECT_EQ(std::to_string(i+1), t[i].s);
    }
    EXPECT_EQ(42, HelpObjectNonTrivialDestructable::objectCounter);
  }
  EXPECT_EQ(0, HelpObjectNonTrivialDestructable::objectCounter);
}


struct HelpObjectTrivialyDestructable
{
  HelpObjectTrivialyDestructable()
    : v(42)
  {
    s[0] = '\0';
  }

  int v;
  char s[32];
};

TEST_F(MakeUniqueTest, TestThatMakeUniqueOfMultipleDefaultDestructableObjectsWithoutPassingCtorsArgumentsAllocatesAnObject)
{
  auto t = alb::make_unique<HelpObjectTrivialyDestructable[]>(sut, 42);
}
