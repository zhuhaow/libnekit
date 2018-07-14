// MIT License

// Copyright (c) 2018 Zhuhao Wang

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

#pragma once

#include "data_flow_interface.h"

namespace nekit {
namespace data_flow {
class LocalDataFlowInterface : public DataFlowInterface {
 public:
  virtual ~LocalDataFlowInterface() = default;

  virtual utils::Cancelable Open(EventHandler)
      __attribute__((warn_unused_result)) = 0;

  virtual utils::Cancelable Continue(EventHandler)
      __attribute__((warn_unused_result)) = 0;

  virtual LocalDataFlowInterface* NextLocalHop() const {
    return static_cast<LocalDataFlowInterface*>(NextHop());
  }
};
}  // namespace data_flow
}  // namespace nekit
