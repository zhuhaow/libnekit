#ifndef A_HPP_
#define A_HPP_

// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

/// \brief This is a class A.
/// \details Yes, it's a class A, with A::foo and A::boo methods.
///
/// \code
/// A a;
/// a.foo();
/// \endcode
class A {
 public:
  /**
    * @brief This is the foo method.
    * @return nothing, just @c void...
    * @throw throw anything (in future)
    */
  void foo();

  /**
    * @brief This is the boo method.
    * @pre Must be called after A::foo
    * @post Leave A in good state
    */
  void boo();
};

#endif // A_HPP_
