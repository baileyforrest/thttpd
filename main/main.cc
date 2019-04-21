#include <cstdlib>
#include <iostream>

#include "absl/memory/memory.h"
#include "main/foo.h"

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    auto foo = absl::make_unique<Foo>(argv[i]);
    std::cout << foo->get_foo() << "\n";
  }
  return EXIT_SUCCESS;
}
