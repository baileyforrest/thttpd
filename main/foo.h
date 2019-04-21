#ifndef MAIN_FOO_H_
#define MAIN_FOO_H_

#include <string>

class Foo {
 public:
  explicit Foo(const std::string& foo);

  const std::string& get_foo() const { return foo_; }

 private:
  const std::string foo_;
};

#endif  // MAIN_FOO_H_
