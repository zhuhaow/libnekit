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

#include "nekit/utils/runtime.h"

namespace nekit {
namespace utils {
Runtime& Runtime::CurrentRuntime() {
  static thread_local Runtime runtime_{};
  return runtime_;
}

rule::RuleSet* Runtime::RuleSet() { return rule_set_; }

void Runtime::SetRuleSet(rule::RuleSet* rule_set) { rule_set_ = rule_set; }

utils::ResolverFactoryInterface* Runtime::ResolverFactory() {
  return resolver_factory_;
}

void Runtime::SetResolverFactory(
    utils::ResolverFactoryInterface* resolver_factory) {
  resolver_factory_ = resolver_factory;
}

boost::asio::io_service* Runtime::IoService() { return io_; }

void Runtime::SetIoService(boost::asio::io_service* io) { io_ = io; }
}  // namespace utils
}  // namespace nekit
