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

#include <memory>

#include "../stream_coder/direct_stream_coder.h"
#include "adapter_interface.h"
#include "connector_interface.h"

namespace nekit {
namespace transport {
class DirectAdapter : public AdapterInterface {
 public:
  enum class ErrorCode { NoError, NoAddress };

  DirectAdapter(std::shared_ptr<utils::Session> session,
                std::shared_ptr<ConnectorFactoryInterface> connector_factory);

  void Open(EventHandler&& handler) override;

 private:
  void DoConnect();

  std::shared_ptr<utils::Session> session_;
  std::shared_ptr<ConnectorFactoryInterface> connector_factory_;
  std::unique_ptr<ConnectorInterface> connector_;

  stream_coder::DirectStreamCoderFactory stream_coder_factory_{};
  EventHandler handler_;
};

class DirectAdapterFactory : public AdapterFactoryInterface {
 public:
  DirectAdapterFactory(
      std::shared_ptr<ConnectorFactoryInterface> connector_factory);

  std::unique_ptr<AdapterInterface> Build(
      std::shared_ptr<utils::Session> session) override;

 private:
  std::shared_ptr<ConnectorFactoryInterface> connector_factory_;
};
}  // namespace transport
}  // namespace nekit
