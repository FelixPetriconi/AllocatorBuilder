#include <iostream>
#include <memory>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  std::cout << "Running main() from gmock_main.cc\n";
  testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
