# libnekit

<!--[![Codacy Badge](https://api.codacy.com/project/badge/Grade/50682cd3b084494f8d0ddb09fda139ab)](https://www.codacy.com/app/zhuhaow/libnekit?utm_source=github.com&utm_medium=referral&utm_content=zhuhaow/libnekit&utm_campaign=badger)
[![Build Status](https://travis-ci.org/zhuhaow/libnekit.svg?branch=master)](https://travis-ci.org/zhuhaow/libnekit) [![Build status](https://ci.appveyor.com/api/projects/status/03jy4k9c8etur68u?svg=true)](https://ci.appveyor.com/project/zhuhaow/libnekit) [![codecov](https://codecov.io/gh/zhuhaow/libnekit/branch/master/graph/badge.svg)](https://codecov.io/gh/zhuhaow/libnekit)-->


libnekit is a cross-platform version of [NEKit](https://github.com/zhuhaow/NEKit) written in C++ to build applications which route or analyze network traffic quick and simple.

Every connection is abstracted as a data flow chain (actually, two inter-connected chains) where data can be transformed at every hop. Data flow chain can be composed arbitrarily to provide flexibility for any scenario. 

For example, you can put a HTTP debug data flow between some data flows where the data is plain text HTTP request and response. You then analyze the data and pass the data to the next hop intact, rewrite the header adding `DNT: 1` to every request and hope the websites behave, or whatever you want.

Accompanying with build-in data flows for common protocols and other tools and utilities for  encryption, geoip support, HTTP header rewriting, etc, you can quickly create an application manipulating network traffic.


## Compilation

### Build Dependencies
Though libnekit itself is cross-platform, you still have to make sure its dependencies build on the target platform.

libnekit depends on Boost, OpenSSL, libsodium and maxminddb. 

Only Boost is required. But currently the CMake script requires all dependencies provided.

libnekit provide a script to build all the dependencies and iOS(`ios`), OSX(`mac`) and linux x86/x64(`linux`) are supported as of now. Windows, Android and linux ARM will also be supported later.

Build the dependencies requires Python 3, install pipenv first then (use the correct `PLATFORM`):

```
pipenv run scripts/build_deps.py PLATFORM
```

The script will download all dependencies source codes and compile them.

### Compile libnekit

*It's very likely you don't have to compile libnekit independently, check next subsection.*

After all dependencies are successfully built, use CMake to configure the project:

```zsh
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Projects/libnekit/cmake/toolchain/PLATFORM.cmake -DPLATFORM=PLATFORM
cmake --build . -- -j2
```

libnekit is built but not installed yet. If you want to distribute it, you need to copy all header files from `include/`, dependencies from `deps/PLATFORM/` and `libnekit.a` to the proper place.

I may add an `install` target later, but since libnekit requires Boost, the distribution would be too large. 

### Use libnekit in Your Project
If you project is build with CMake, then it should be very simple and straight forward, just add the root folder of libnekit to your project and configure it with the proper toolchain file. If you use your own toolchain config file, it's very likely libnekit will just compile with it so you don't need to change anything.

If you are not using CMake, then use CMake generator to generate a compatible project with the toolchain you are working with.

## Working with libnekit

### Define Rules

libnekit provides several rules and you can easily create new ones based on the property of the tunnel connection. Each rule instance is associated with a lambda that returns a `RemoteDataFlowInterface*` that handles the data from local data flow. How you chain the remote data flows together is unlimited as long as it makes sense.

### A simple example

Now, with everything at hand, let's take a look at a real example.

```cpp
#include <boost/asio.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <nekit/data_flow/http_data_flow.h>
#include <nekit/data_flow/http_server_data_flow.h>
#include <nekit/data_flow/socks5_data_flow.h>
#include <nekit/data_flow/socks5_server_data_flow.h>
#include <nekit/data_flow/speed_data_flow.h>
#include <nekit/init.h>
#include <nekit/instance.h>
#include <nekit/rule/all_rule.h>
#include <nekit/rule/dns_fail_rule.h>
#include <nekit/rule/geo_rule.h>
#include <nekit/transport/tcp_listener.h>
#include <nekit/transport/tcp_socket.h>
#include <nekit/utils/logger.h>
#include <nekit/utils/system_resolver.h>

using namespace nekit;
using namespace boost::asio;

// Setting up log, refer to Boost.Log doc.
namespace expr = boost::log::expressions;
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", utils::LogLevel);
BOOST_LOG_ATTRIBUTE_KEYWORD(instance_name, "Instance", std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string);

int main() {
  nekit::Initailize();
  assert(utils::Maxmind::Initalize("test/GeoLite2-Country.mmdb"));

  // Setting up log, refer to Boost.Log doc.
  boost::log::add_console_log(
      std::cout,
      boost::log::keywords::filter = severity >= utils::LogLevel::Info,
      boost::log::keywords::format =
          (expr::stream
           << std::left << std::setw(7) << std::setfill(' ') << severity
           << expr::if_(expr::has_attr(
                  instance_name))[expr::stream << "[" << instance_name << "]"]
           << "[" << channel << "]" << expr::smessage));

  // One instance is one run loop. You can create multiple instances if you want to use multi threads.
  Instance instance{"Specht"};
  
  // Proxy manager manages all the local proxies.
  auto proxy_manager = std::make_unique<ProxyManager>();

  // Rule manager handles rule matching.
  auto rule_manager = std::make_unique<rule::RuleManager>(instance.io());

  // Unless you understand everything, make sure there is only one resolve used for one instance.
  auto resolver = std::make_unique<utils::SystemResolver>(instance.io(), 5);

  // This is the lambda to be called to create a data flow chain connects to remote http proxy.
  rule::RuleHandler http_proxy_handler =
      [resolver(resolver.get())](std::shared_ptr<utils::Session> session) {
        auto endpoint = std::make_shared<utils::Endpoint>("1.2.3.4", 9090);
        endpoint->set_resolver(resolver);
        // HttpDataFlow (1.2.3.4:9090) -> TcpSocket
        return std::make_unique<data_flow::HttpDataFlow>(
            endpoint, session, std::make_unique<transport::TcpSocket>(session),
            nullptr);
      };

  // This is the lambda to be called to create a data flow chain automatically selects the fastest data flow becomes ready.
  rule::RuleHandler speed_handler =
      [resolver(resolver.get())](std::shared_ptr<utils::Session> session) {
        auto endpoint = std::make_shared<utils::Endpoint>("1.2.3.4", 9091);
        endpoint->set_resolver(resolver);
        std::vector<
            std::pair<std::unique_ptr<data_flow::RemoteDataFlowInterface>, int>>
            data_flows;
        data_flows.emplace_back(
            // Socks5DataFlowv(1.2.3.4:9091) -> TcpSocket, delay 100 milliseconds before connecting
            std::make_pair(std::make_unique<data_flow::Socks5DataFlow>(
                               endpoint, session,
                               std::make_unique<transport::TcpSocket>(session)),
                           100));
        data_flows.emplace_back(
            // Start connecting directly with no delay
            std::make_pair(std::make_unique<transport::TcpSocket>(session), 0));

        return std::make_unique<data_flow::SpeedDataFlow>(
            session, std::move(data_flows));
      };
  
  // This is the lambda to be called to connect directly.
  rule::RuleHandler direct_handler =
      [](std::shared_ptr<utils::Session> session) {
        return std::make_unique<transport::TcpSocket>(session);
      };
  
  // If target host is in China, connect direct by using direct_handler.
  std::shared_ptr<rule::RuleInterface> geo_cn_rule =
     std::make_shared<rule::GeoRule>(utils::CountryIsoCode::CN, true,
                                     direct_handler);
                                     
  // If resolution fails, connect through proxy.                      
  auto dns_fail_rule = std::make_shared<rule::DnsFailRule>(http_proxy_handler);
  // Choose which ways is fast automatically.
  auto all_rule = std::make_shared<rule::AllRule>(speed_handler);
  // The rule will be matched in order.
  rule_manager->AppendRule(geo_cn_rule);
  rule_manager->AppendRule(dns_fail_rule);
  rule_manager->AppendRule(all_rule);

  proxy_manager->SetRuleManager(std::move(rule_manager));
  proxy_manager->SetResolver(std::move(resolver));

  // When a new connection comes in locally, we treat it as a connection to a SOCSK5 server.
  auto socks5_listener = std::make_unique<transport::TcpListener>(
      instance.io(), [](auto data_flow) {
        return std::make_unique<data_flow::Socks5ServerDataFlow>(
            std::move(data_flow), data_flow->Session());
      });
  
  // When a new connection comes in locally, we treat it as a connection to a http server.
  auto http_listener = std::make_unique<transport::TcpListener>(
      instance.io(), [](auto data_flow) {
        return std::make_unique<data_flow::HttpServerDataFlow>(
            std::move(data_flow), data_flow->Session());
      });

  socks5_listener->Bind("127.0.0.1", 8081);
  http_listener->Bind("127.0.0.1", 8080);
  proxy_manager->AddListener(std::move(http_listener));

  // We can add as many proxy managers as we want, each listens on different ports which different rules (note rules are created with `std::shared_ptr`, they can be shared among different rule manages as along as they all belongs to the same instance.
  instance.AddProxyManager(std::move(proxy_manager));

  boost::asio::signal_set signals(*instance.io(), SIGINT, SIGTERM);
  signals.async_wait([&instance](const boost::system::error_code& ec,
                                 int signal_num) { instance.Stop(); });

  instance.Run();

  return 0;
}
```

## Extend libnekit

### Handle Data by `DataFlowInterface`
Data flow is the core of libnekit. Let's take a look at how data flow is handled in libnekit first.

Below is the outline of `DataFlowInterface`.

```cpp
using DataEventHandler = std::function<void(utils::Buffer&&, utils::Error)>;
using EventHandler = std::function<void(utils::Error)>;

virtual utils::Cancelable Read(utils::Buffer&&, DataEventHandler)
      __attribute__((warn_unused_result)) = 0;
      
virtual utils::Cancelable Write(utils::Buffer&&, EventHandler)
      __attribute__((warn_unused_result)) = 0;

virtual utils::Cancelable CloseWrite(EventHandler)
      __attribute__((warn_unused_result)) = 0;

virtual const FlowStateMachine& StateMachine() const = 0;

virtual DataFlowInterface* NextHop() const = 0;
```

The thing we want to archive with `DataFlowInterface` is straight forward. We can read from and write to data flow. Each data flow has a `FlowStateMachine` which encapsulates the state of the flow. The data flow is chained as `NextHop()` will get us the next hop. 

So to create a data flow, let's say, a data flow that communicates with a remote HTTP server, we can have a HTTP proxy data flow which has a TCP socket data flow as the next hop. When we write to the HTTP proxy data flow, it delegates the write request to the underlying transmission TCP socket. 

This brings great flexibility and allows you to basically do anything you want with the data without interfering with anythings else. 

Let's get back to the interface definition and take a closer look at the seemingly daunting definition of read method.

```cpp
virtual utils::Cancelable Read(utils::Buffer&&, DataEventHandler)
      __attribute__((warn_unused_result)) = 0;
```

We pass in a buffer and a handler, since almost everything in libnekit is asynchronous, we shouldn't expect the method will block until it really gets something. Instead, we give an event handler which will be called when there is some data read into the buffer. The `DataEventHandler` which is defined as `std::function<void(utils::Buffer&&, utils::Error)>` is simply a lambda which takes a buffer filled with data and an error code. We should check the if there is any error before we process the data. 

The only thing left unclear is `utils::Cancelable`, which is a little tricky.

### Cancelable Asynchronous Task

You should get hold of how those libraries implementing event loop works first. It involves a lot of subtleties to implement correctly but is actually not very complex. Fortunately, you don't have to write one yourself so it is not a problem. The [doc of Boost Asio](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/overview.html) provides a clear overview of how it works (which happens to be what libnekit is based on) and everyone should read first before working with libnekit.

Now, as you have read and understood how asio works, let's have a quick recap and think about how the handler we provide is called. 

It's very tempting to write a data flow that handle  data encryption (with a FPE algorithm which keeps and ciphertext and plaintext and same length, like most block cipher) as this (only the read part):

```cpp
class SecuredDataFlow {
 public:
  void Read(std::function<void(const boost::system::error_code &, std::size_t)>
                handler) {
    socket_.async_read_some(read_buffer_,
                            [this, handler](const boost::system::error_code &ec,
                                            std::size_t bytes_transferred) {
                              if (ec) {
                                handler(ec, bytes_transferred);
                                return;
                              }

                              Decrypt(bytes_transferred);
                              handler(ec, bytes_transferred);
                              return;
                            });
  }

  void *ReadBuffer() { return read_buffer_; }

 private:
  void Decrypt(size_t len);

  boost::asio::ip::tcp::socket socket_;
  uint8_t read_buffer_[1024];
}

```

Everything seems fine, except for one, what if we decide we no longer need the socket? For example, this data flow is reading data from remote but our local application encountered an error when processing the stream and can't not proceed. It makes little sense to keep the socket (data flow) available, but can we just release it immediately?

Considering when the socket is destructed it will "cancel any outstanding asynchronous operations associated with the socket", we should be able to just do it, and let boost cancel the pending read request if there is one. 

Think. When the kernel notifies the `io_context` that there is data available to read (on *nix) or the buffer is filled with data (on Windows), asio will put the callback handler we provide to a Completion Event Queue. The logic of observing event and executing callback are decoupled by utilizing this queue. There are several things that are probmatic:

1. As we say event observation and callback execution is decoupled, we are actually saying Asio provides a very simple model for a multi thread application. Several thread can fetch task from the queue and execute them concurrently as long as the queue is thread-safe and the handler does not share anything not thread-safe. However, `uint8_t read_buffer_[1024]` is obviously not thread-safe, as kernel is filling data into the buffer, another thread may be trying to release the `SecuredDataFlow`! (Cancellation in proactor model is really a little too complicate so we should try to avoid thinking about it.)
2. Another thing is a little more of an implementation choice: when a handler is pushed into the queue, there is no way to cancel it in Asio. If handler is pushed into the queue, then `SecuredDataFlow` is released, the handler won't know anything about it. When an execution thread fetch the handler from the queue, and call it, it can't dereference `this`.

This tells us two things. First, the buffer's lifetime should exceeds the handler. Second, the handler needs to know the existence of `this`.

Usually, this is done by using `std::shared_ptr`, the handler capture the shared pointer so the lifetime of the data flow exceeds the handler. It works, apart from the hassle that you have to inherit from  `std::enable_shared_from_this` and enforce that this class can only be managed with shared pointer, even if you never share it semantically.

Let's take a look at what is in the handler, usually, the handler capture the logic to proceed, which means it should also capture some shared pointer of some data flows and other instances, as long as the handler exists, all of the instances will exist, albeit not needed.

Apart from that, if you think carefully, this pattern actually creates a self-contained loop, where the handler keeps everything existing, and since the handler will proceed by creating more handler, this loop will never stop unless we tell every instances that they have to stop executing the logic in the handler since it is cancelled. The handler needs to check some state flag first in some the captured instance to make sure it can proceed, which is cumbersome.

In order to avoid all of that, libnekit uses a very simple class holding a shared flag indicating whether this handler should be called or not.

The core of `utils::Cancelable` is just a `std::shared_ptr<bool>`.

```cpp
class Cancelable {
 public:
  void Cancel();
  
  ...
  
  bool canceled() const;

 private:
  std::shared_ptr<bool> canceled_;
```

If we want to cancel the handler, we just call the `Cancel()` method on the returned `utils::Cancelable`. The data flow will first check if the **captured** `utils::Cancelable` is cancelled or not. 

Usually, the destructor of a data flow will explicitly cancel or the `utils::Cancelable` returned by the next hop data flow, so the handler won't be called.

You can find more examples in `tcp_socket.cc`.

### Manipulate Data with Minimal Copy

Suppose we pass data by a plain memory block between the data flow chain, and some flow wants to encrypt the data with some authentication header in the front, it has to copy the whole block into an other new block or memmove it which basically means copy the whole block twice. Consider, however, that `iovec` is widely supported on nowadays OS, we can actually put the header in a standalone buffer and send the two buffers to the kernel in one call. 

`utils::Buffer` is created for this scenario (and some others). Internally, `Buffer` is a linked list of `Buf`s which is just a memory block. You can copy the data in and out as if the `Buffer` is a monolithic block. You can insert and remove space from any offset and `Buffer` will try to make the necessary copy minimal.

### Forward Data with `Tunnel`

`Tunnel` (just as what it does in NEKit) forwards data back and forth between local and remote. The data `Tunnel` gets should be the raw data that you expect to transmit. There is really nothing too fancy to talk about here.

### Errors and Exceptions

The rule of thumb is: asynchronous methods don't throw, while synchronous ones may. (As of now, no method throws yet. But you should expect the error will be thrown instead of passed by method call.)

Throw exception from a callback handler does not make too much sense since the whole call stack is already gone. We can do very little about the error other than ignore it or terminate the application. However, I find throw exception significantly simplify the method signature though I don't like that C++ does not enforce labeling the "throwness" of method. 

libnekit will use exception whenever possible. All exceptions from libnekit should be a subclass of `utils::Error`.



