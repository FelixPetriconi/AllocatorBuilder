#include <iostream>
#include <memory>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
  std::cout << "Running main() from gmock_main.cc\n";
  return RUN_ALL_TESTS();
}
