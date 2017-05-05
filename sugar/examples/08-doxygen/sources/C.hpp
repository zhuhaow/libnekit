#ifndef C_HPP_
#define C_HPP_

// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#if (A_TARGET_MACRO) == 1
# include <stdexcept>
#endif
#if (A_DIRECTORY_MACRO) == 1
# include <cstdio>
#endif
#if (A_DEBUG) == 1
# include "A.hpp"
#endif
#if (A_NDEBUG) == 1
# include "B.hpp"
#endif

/**
  * @brief Tester of macro expansion
  */
struct C {
#if (A_TARGET_MACRO) == 1
  /**
    * @brief Tester of @c A_TARGET_MACRO
    */
  int target;
#endif
#if (A_DIRECTORY_MACRO) == 1
  /**
    * @brief Tester of @c A_DIRECTORY_MACRO
    */
  int directory;
#endif
#if (A_DEBUG) == 1
  /**
    * @brief Tester of @c A_DEBUG
    */
  int debug;
#endif
#if (A_NDEBUG) == 1
  /**
    * @brief Tester of @c A_NDEBUG
    */
  int release;
#endif
#if (A_UNKNOWN) == 1
  /**
    * @brief Must not be seen in documentation
    */
  int unknown;
#endif
};

#endif // C_HPP_
