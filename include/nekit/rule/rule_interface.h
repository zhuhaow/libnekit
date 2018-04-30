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

#include <boost/noncopyable.hpp>

#include "../data_flow/remote_data_flow_interface.h"
#include "../utils/session.h"
#include "match_result.h"

namespace nekit {
namespace rule {
typedef std::function<std::unique_ptr<data_flow::RemoteDataFlowInterface>(
    std::shared_ptr<utils::Session>)>
    RuleHandler;

class RuleInterface : private boost::noncopyable {
 public:
  using RuleHandler =
      std::function<std::unique_ptr<data_flow::RemoteDataFlowInterface>(
          std::shared_ptr<utils::Session>)>;

  virtual ~RuleInterface() = default;

  virtual MatchResult Match(std::shared_ptr<utils::Session> session) = 0;
  virtual std::unique_ptr<data_flow::RemoteDataFlowInterface> GetDataFlow(
      std::shared_ptr<utils::Session> session) = 0;
};
}  // namespace rule
}  // namespace nekit
