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

#include "../stream_coder/stream_coder_interface.h"
#include "../transport/connector_interface.h"
#include "rule_interface.h"

namespace nekit {
namespace rule {
class AllRule : public RuleInterface {
  AllRule(std::shared_ptr<transport::AdapterFactoryInterface> adapter_factory);

  MatchResult Match(std::shared_ptr<utils::Session> session) override;
  std::unique_ptr<transport::AdapterInterface> GetAdapter(
      std::shared_ptr<utils::Session> session) override;

 private:
  std::shared_ptr<transport::AdapterFactoryInterface> adapter_factory_;
};
}  // namespace rule
}  // namespace nekit
