#include <iostream>
#include <memory>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
  std::cout << "Running main() from gmock_main.cc\n";
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
