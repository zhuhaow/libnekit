// MIT License

// Copyright (c) 2017 Zhuhao Wang

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "nekit/utils/log.h"

namespace nekit {
namespace utils {
std::ostream& operator<<(std::ostream& stream, LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      stream << "Trace";
      break;
    case LogLevel::Debug:
      stream << "Debug";
      break;
    case LogLevel::Info:
      stream << "Info";
      break;
    case LogLevel::Warning:
      stream << "Warning";
      break;
    case LogLevel::Error:
      stream << "Error";
      break;
    case LogLevel::Fatal:
      stream << "Fatal";
      break;
  }
  return stream;
}

Logger& default_logger() {
  static thread_local Logger logger_;
  return logger_;
}
}  // namespace utils
}  // namespace nekit
