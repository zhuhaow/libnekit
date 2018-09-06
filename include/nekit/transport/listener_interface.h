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

#pragma once

#include <functional>
#include <memory>
#include <system_error>

#include "../data_flow/local_data_flow_interface.h"
#include "../utils/async_interface.h"
#include "../utils/error.h"
#include "../utils/result.h"

namespace nekit {
namespace transport {

enum class ListenerErrorCode { AddressInUse = 1 };

class ListenerErrorCategory : public utils::ErrorCategory {
 public:
  NE_DEFINE_STATIC_ERROR_CATEGORY(ListenerErrorCategory)

  std::string Description(const utils::Error& error) const override {
    (void)error;
    return "address in use";
  }
  std::string DebugDescription(const utils::Error& error) const override {
    return Description(error);
  }
};

NE_DEFINE_NEW_ERROR_CODE(Listener)

class ListenerInterface : public utils::AsyncInterface {
 public:
  using EventHandler = std::function<void(
      utils::Result<std::unique_ptr<data_flow::LocalDataFlowInterface>>&&)>;

  using DataFlowHandler =
      std::function<std::unique_ptr<data_flow::LocalDataFlowInterface>(
          std::unique_ptr<data_flow::LocalDataFlowInterface>&&)>;

  virtual ~ListenerInterface() = default;

  virtual void Accept(EventHandler) = 0;

  virtual void Close() = 0;
};
}  // namespace transport
}  // namespace nekit
