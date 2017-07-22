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

#ifndef NEKIT_TRANSPORT_CONNECTOR_INTERFACE
#define NEKIT_TRANSPORT_CONNECTOR_INTERFACE

#include <functional>
#include <memory>
#include <system_error>
#include <vector>

#include "../utils/endpoint.h"
#include "connection_interface.h"

namespace nekit {
namespace transport {
class ConnectorInterface {
 public:
  virtual ~ConnectorInterface() = default;

  using EventHandler = std::function<void(
      std::unique_ptr<ConnectionInterface>&&, std::error_code)>;

  virtual void Connect(std::unique_ptr<std::vector<utils::Endpoint>>&&,
                       EventHandler&&) = 0;

  virtual std::error_code Bind(utils::Endpoint) = 0;
};
}  // namespace transport
}  // namespace nekit

#endif /* NEKIT_TRANSPORT_CONNECTOR_INTERFACE */
