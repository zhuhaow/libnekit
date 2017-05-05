#ifndef B_HPP_
#define B_HPP_

// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

/**
  * @brief Hello, class B.
  */
class B {
 public:

  /**
    * @brief Very good B::foo method.
    * @param x This is the parameter @c x of type @c int
    * @param y This is the parameter @c y of type @c int
    * @return 3.14159 + x + y
    */
  double foo(int x, int y);

  /**
    * @internal
    * @brief developer hack method, @b not for user sensitive eyes
    * @param It's @c x!
    * @return 7.0
    */
  double hacky_method(int x);
};

#endif // B_HPP_
