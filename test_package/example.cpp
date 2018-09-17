#include <iomanip>
#include <iostream>
#include <memory>

#include <boost/asio.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <nekit/crypto/openssl_stream_cipher.h>
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
#include <nekit/config.h>

using namespace nekit;
using namespace boost::asio;
namespace expr = boost::log::expressions;
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", utils::LogLevel);
BOOST_LOG_ATTRIBUTE_KEYWORD(instance_name, NEKIT_BOOST_LOG_INSTANCE_ATTR_NAME,
                            std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(track_id, NEKIT_BOOST_LOG_TRACK_ID_ATTR_NAME,
                            std::string);

int main() {
  nekit::Initailize();

  boost::log::add_console_log(
      std::cout,
      boost::log::keywords::filter = severity >= utils::LogLevel::Info,
      boost::log::keywords::format =
          (expr::stream
           << std::left << std::setw(7) << std::setfill(' ') << severity
           << expr::if_(expr::has_attr(
                  instance_name))[expr::stream << "[" << instance_name << "]"]
           << expr::if_(expr::has_attr(
                  track_id))[expr::stream << "[" << track_id << "]"]
           << "[" << channel << "]" << expr::smessage));

  Instance instance{"Test"};
  auto proxy_manager = std::make_unique<ProxyManager>(instance.GetRunloop());

  auto rule_manager =
      std::make_unique<rule::RuleManager>(instance.GetRunloop());

  auto resolver =
      std::make_unique<utils::SystemResolver>(instance.GetRunloop(), 5);

  rule::RuleHandler http_proxy_handler =
      [resolver(resolver.get())](std::shared_ptr<utils::Session> session) {
        auto endpoint = std::make_shared<utils::Endpoint>("127.0.0.1", 9090);
        endpoint->set_resolver(resolver);
        return std::make_unique<data_flow::HttpDataFlow>(
            endpoint, session, std::make_unique<transport::TcpSocket>(session),
            nullptr);
      };

  rule::RuleHandler direct_handler =
      [](std::shared_ptr<utils::Session> session) {
        return std::make_unique<transport::TcpSocket>(session);
      };

  auto all_rule = std::make_shared<rule::AllRule>(http_proxy_handler);
  rule_manager->AppendRule(all_rule);

  proxy_manager->SetRuleManager(std::move(rule_manager));
  proxy_manager->SetResolver(std::move(resolver));

  auto socks5_listener = std::make_unique<transport::TcpListener>(
      instance.GetRunloop(), [](auto data_flow) {
        return std::make_unique<data_flow::Socks5ServerDataFlow>(
            std::move(data_flow), data_flow->Session());
      });

  auto http_listener = std::make_unique<transport::TcpListener>(
      instance.GetRunloop(), [](auto data_flow) {
        return std::make_unique<data_flow::HttpServerDataFlow>(
            std::move(data_flow), data_flow->Session());
      });

  socks5_listener->Bind("127.0.0.1", 8081);
  http_listener->Bind("127.0.0.1", 8080);
  proxy_manager->AddListener(std::move(http_listener));
  proxy_manager->AddListener(std::move(socks5_listener));

  instance.AddProxyManager(std::move(proxy_manager));

  boost::asio::signal_set signals(*instance.GetRunloop()->BoostIoContext(),
                                  SIGINT, SIGTERM);
  signals.async_wait([&instance](const boost::system::error_code& ec,
                                 int signal_num) { instance.Stop(); });

  // Don't run it in test.
  // instance.Run();

  return 0;
}
