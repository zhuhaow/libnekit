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

#ifndef NEKIT_RULE_RULE_INTERFACE
#define NEKIT_RULE_RULE_INTERFACE

#include <memory>

#include <boost/noncopyable.hpp>

#include "../stream_coder/stream_coder_interface.h"
#include "../transport/connector_interface.h"
#include "../utils/session.h"
#include "match_result.h"

namespace nekit {
namespace rule {
class RuleInterface : boost::noncopyable {
 public:
  virtual ~RuleInterface() = default;

  virtual MatchResult Match(std::shared_ptr<utils::Session> session) = 0;

  virtual std::unique_ptr<transport::ConnectorFactoryInterface> GetTransport(
      std::shared_ptr<const utils::Session> session) = 0;

  virtual std::unique_ptr<stream_coder::StreamCoderInterface> GetStreamCoder(
      std::shared_ptr<const utils::Session>) = 0;
};
}  // namespace rule
}  // namespace nekit

#endif /* NEKIT_RULE_RULE_INTERFACE */
