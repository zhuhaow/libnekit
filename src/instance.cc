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

#include "nekit/instance.h"

#include "nekit/utils/error.h"
#include "nekit/utils/log.h"
#include "nekit/utils/system_resolver.h"

#undef NECHANNEL
#define NECHANNEL "Instance"

namespace nekit {
using nekit::utils::LogLevel;

Instance::Instance(std::string name)
    : AsyncIoInterface{io_}, name_{name}, io_{} {}

void Instance::SetRuleManager(
    std::unique_ptr<rule::RuleManager> &&rule_manager) {
  rule_manager_ = std::move(rule_manager);
  NEDEBUG << "Set new rules for instance " << name_ << ".";
}

void Instance::AddListener(
    std::unique_ptr<transport::ServerListenerInterface> &&listener) {
  listeners_.push_back(std::move(listener));
  NEDEBUG << "Add new listener to instance " << name_ << ".";
}

void Instance::Run() {
  assert(rule_manager_);
  assert(ready_);

  BOOST_LOG_SCOPED_THREAD_ATTR(
      "Instance", boost::log::attributes::constant<std::string>(name_));

  for (auto &listener : listeners_) {
    listener->Accept(
        [this](std::unique_ptr<transport::ConnectionInterface> &&conn,
               std::unique_ptr<stream_coder::ServerStreamCoderInterface>
                   &&stream_coder,
               std::error_code ec) {
          if (ec) {
            NEERROR << "Error happened when accepting new socket " << ec;
            exit(1);
          }

          tunnel_manager_
              .Build(std::move(conn), std::move(stream_coder),
                     rule_manager_.get())
              .Open();
        });
  }

  NEINFO << "Start running instance.";
  io_.run();

  NEINFO << "Instance stopped.";
}

void Instance::Stop() {
  io_.stop();
  ready_ = false;
}

void Instance::ResetNetwork() {
  tunnel_manager_.CloseAll();
  listeners_.clear();
  ready_ = true;
}

}  // namespace nekit
