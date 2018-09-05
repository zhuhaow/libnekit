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

#include "nekit/utils/system_resolver.h"

#include "nekit/utils/boost_error.h"
#include "nekit/utils/error.h"
#include "nekit/utils/log.h"

#undef NECHANNEL
#define NECHANNEL "System resolver"

namespace nekit {
namespace utils {

SystemResolver::SystemResolver(Runloop* runloop, size_t thread_count)
    : runloop_{runloop}, thread_count_{thread_count} {
  work_guard_ = std::make_unique<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      boost::asio::make_work_guard(*resolve_runloop_.BoostIoContext()));

  for (size_t i = 0; i < thread_count_; i++) {
    thread_group_.create_thread(boost::bind(&boost::asio::io_context::run,
                                            resolve_runloop_.BoostIoContext()));
  }
}

Cancelable SystemResolver::Resolve(std::string domain,
                                   AddressPreference preference,
                                   EventHandler handler) {
  // Preference is ignored. Let the OS do the choice as of now.
  (void)preference;

  NETRACE << "Start resolving " << domain << ".";

  Cancelable cancelable{};

  resolve_runloop_.Post([this, domain, handler, cancelable,
                         lifetime{lifetime_}]() mutable {
    // Note it will be guaranteed that the `runloop_` will never be
    // released before all the instances implementing `AsyncInterface`
    // which will return that `runloop_` are released. This resolver will
    // never be released before `runloop_` is released. In the destructor
    // this thread is guaranteed to exit before resolver is released, so
    // there is no need to worry about any thread issues here. Resolver and
    // `runloop_` will exist when this thread is running.

    if (cancelable.canceled() || lifetime.canceled()) {
      return;
    }

    auto resolver =
        boost::asio::ip::tcp::resolver(*resolve_runloop_.BoostIoContext());

    NEDEBUG << "Trying to resolve " << domain << ".";

    boost::system::error_code ec;
    auto result = resolver.resolve(domain, "", ec);

    if (ec) {
      auto error = BoostErrorCategory::FromBoostError(ec);
      NEERROR << "Failed to resolve " << domain << " due to " << error << ".";

      runloop_->Post([handler, error{std::move(error)}, cancelable,
                      life_time{lifetime_}]() mutable {
        if (cancelable.canceled() || life_time.canceled()) {
          return;
        }

        handler(utils::MakeErrorResult(std::move(error)));
      });
      return;
    }

    auto addresses = std::make_shared<std::vector<boost::asio::ip::address>>();

    for (auto iter = result.begin(); iter != result.end(); iter++) {
      addresses->emplace_back(iter->endpoint().address());
    }

    NEINFO << "Successfully resolved domain " << domain << ".";

    runloop_->Post([handler, addresses, cancelable, life_time{lifetime_}]() {
      if (cancelable.canceled() || life_time.canceled()) {
        return;
      }

      handler(addresses);
    });
  });

  return cancelable;
}

void SystemResolver::Stop() {
  work_guard_.reset();
  thread_group_.join_all();
}

SystemResolver::~SystemResolver() {
  Stop();
  lifetime_.Cancel();
}

utils::Runloop* SystemResolver::GetRunloop() { return runloop_; }
}  // namespace utils
}  // namespace nekit
