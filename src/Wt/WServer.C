/*
 * Copyright (C) 2011 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#include "Wt/WConfig.h"

#if !defined(WT_WIN32)
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#else
#include <process.h>
#endif // !_WIN32

#include <boost/algorithm/string.hpp>

#include "Wt/WIOService.h"
#include "Wt/WResource.h"
#include "Wt/WServer.h"

#include "Configuration.h"
#include "WebController.h"
#include "http/uWSRequest.h"

#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <seastar/http/httpd.hh>
#include <seastar/websocket/server.hh>
#include <seastar/http/handlers.hh>
#include <seastar/http/function_handlers.hh>
#include <seastar/http/file_handler.hh>
#include <seastar/core/seastar.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/future.hh>
#include <seastar/http/api_docs.hh>
#include <seastar/core/thread.hh>
#include <seastar/core/prometheus.hh>
#include <seastar/core/print.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/core/file.hh>
//#include "stop_signal.hh"

namespace bpo = boost::program_options;

using namespace seastar;
using namespace httpd;

namespace {
struct PartialArgParseResult {
    std::string wtConfigXml;
    std::string appRoot;
};

static PartialArgParseResult parseArgsPartially(const std::string &applicationPath,
                                                const std::vector<std::string> &args,
                                                const std::string &configurationFile)
{
    std::string wt_config_xml;
    Wt::WLogger stderrLogger;
    stderrLogger.setStream(std::cerr);

    ::http::server::Configuration serverConfiguration(stderrLogger, true);
    serverConfiguration.setOptions(applicationPath, args, configurationFile);

    return PartialArgParseResult {
        serverConfiguration.configPath(),
        serverConfiguration.appRoot()
    };
}
}

namespace Wt {
class gethandler : public httpd::handler_base
{
public:
    gethandler(Wt::WebController* webController, ::http::server::Configuration* serverconfig, EntryPoint* entrypoint) :
        webController_(webController),
        serverconfig_(serverconfig),
        entrypoint_(entrypoint)
    {

    }

    std::string_view test = R""""("<html>"
                  "<head><title>Not Found</title></head>"
                  "<body><h1>404 Not Found</h1></body>"
                  "</html>")"""";

    virtual future<std::unique_ptr<seastar::http::reply>> handle(const sstring& path,
                                                                 std::unique_ptr<seastar::http::request> req,
                                                                 std::unique_ptr<seastar::http::reply> rep)
    {
        //rep->write_body("text/html", seastar::sstring(test));

        if(!request_)
            request_ = new ::http::uWSRequest(rep.get(), req.get(), serverconfig_, entrypoint_);

        request_->reset(rep.get(), req.get(), entrypoint_);

        webController_->handleRequest(request_);

//        int m = co_await seastar::yield().then(seastar::coroutine::lambda([n] () -> future<int> {
//            co_await seastar::coroutine::maybe_yield();
//            // `n` can be safely used here
//            co_return n;
//        }));

        // Define a function to write the response body using the provided output stream
//        auto body_writer = [](seastar::output_stream<char>&& out) {
//            return seastar::do_with(std::move(out), [](auto& out) {
//                // Write the data in chunks
//                return seastar::repeat([&out] {
//                    // Read a chunk of data from the input stream
//                    return stream.read().then([&out](auto buf) {
//                        // If there is no more data to read, stop streaming
//                        if (buf.empty()) {
//                            return seastar::make_ready_future<seastar::stop_iteration>(seastar::stop_iteration::yes);
//                        }

//                        // Write the chunk of data to the output stream
//                        return out.write(std::move(buf)).then([] {
//                            // Continue streaming
//                            return seastar::make_ready_future<seastar::stop_iteration>(seastar::stop_iteration::no);
//                        });
//                    });
//                });
//            });
//        };
//        rep->add_header("Content-Type", "text/html;")
//            //.add_header("Server", "Wt")
//            ._content = dc;


        co_return co_await make_ready_future<std::unique_ptr<seastar::http::reply>>(std::move(rep));
    };
private:
    Wt::WebController* webController_ = 0;
    ::http::server::Configuration* serverconfig_ = 0;
    EntryPoint* entrypoint_ = 0;
    inline static thread_local ::http::uWSRequest *request_ = 0;

};


//LOGGER("WServer");
class stop_signal {
    bool _caught = false;
    seastar::condition_variable _cond;
private:
    void signaled() {
        if (_caught) {
            return;
        }
        _caught = true;
        _cond.broadcast();
    }
public:
    stop_signal() {
        seastar::engine().handle_signal(SIGINT, [this] { signaled(); });
        seastar::engine().handle_signal(SIGTERM, [this] { signaled(); });
    }
    ~stop_signal() {
        // There's no way to unregister a handler yet, so register a no-op handler instead.
        seastar::engine().handle_signal(SIGINT, [] {});
        seastar::engine().handle_signal(SIGTERM, [] {});
    }
    seastar::future<> wait() {
        return _cond.wait([this] { return _caught; });
    }
    bool stopping() const {
        return _caught;
    }
};


  namespace {
    bool CatchSignals = true;
  }

WServer *WServer::instance_ = 0;

WServer::Exception::Exception(const std::string& what)
  : WException(what)
{ }

char ** convert(const std::vector<std::string>& v)
{
    char ** t = new char* [v.size() + 1];

    // Need this for execve
    t[v.size()] = nullptr;

    for (unsigned i = 0 ; i < v.size(); ++i)
    {
        t[i] = strdup(v[i].c_str());
    }

    return t;
}
struct WServer::Impl
{
    Impl()
        : serverConfiguration_(0)//, server_(0)
    {
#ifdef ANDROID
        preventRemoveOfSymbolsDuringLinking();
#endif
    }

    ~Impl()
    {
        delete serverConfiguration_;
    }

    ::http::server::Configuration *serverConfiguration_;
    //http::server::Server        *server_;
    std::vector<std::string> args_;
    int ac_;
    char** av_;
};

void WServer::init(const std::string& wtApplicationPath,
                   const std::string& configurationFile)
{
  customLogger_ = nullptr;

  application_ = wtApplicationPath;
  configurationFile_ = configurationFile;

  ownsIOService_ = true;
  dedicatedProcessEnabled_ = false;
  ioService_ = 0;
  webController_ = 0;
  configuration_ = 0;

  logger_.addField("datetime", false);
  logger_.addField("app", false);
  logger_.addField("session", false);
  logger_.addField("type", false);
  logger_.addField("message", true);

  instance_ = this;

}

void WServer::destroy()
{
  if (ownsIOService_) {
    delete ioService_;
    ioService_ = 0;
  }

  delete webController_;
  delete configuration_;

  instance_ = 0;
}

void WServer
::setLocalizedStrings(const std::shared_ptr<WLocalizedStrings>& stringResolver)
{
  localizedStrings_ = stringResolver;
}

std::shared_ptr<WLocalizedStrings> WServer::localizedStrings() const
{
  return localizedStrings_;
}

WServer::WServer(const std::string &wtApplicationPath, const std::string &wtConfigurationFile)
  : impl_(new Impl())
{
  impl_->args_ = {
      "-c", std::to_string(std::thread::hardware_concurrency()),
      "--network-stack", "posix",
      "--reactor-backend",  "epoll",
      "--collectd", "0"
  };
  impl_->ac_ = impl_->args_.size();
  impl_->av_ = convert(impl_->args_);
  init(wtApplicationPath, wtConfigurationFile);
}

WServer::WServer(int argc, char *argv[], const std::string &wtConfigurationFile)
  : impl_(new Impl())
{
  impl_->ac_ = argc;
  impl_->av_ = argv;
  impl_->args_ = std::vector<std::string>(argv, argv + argc);
  std::string applicationPath = argv[0];
  std::vector<std::string> args(argv + 1, argv + argc);

  setServerConfiguration(applicationPath, args, wtConfigurationFile);
}

WServer::WServer(const std::string &applicationPath, const std::vector<std::string> &args, const std::string &wtConfigurationFile)
  : impl_(new Impl())
{
  impl_->ac_ = args.size();
  impl_->av_ = convert(args);
  impl_->args_ = args;
  init(applicationPath, "");
  setServerConfiguration(applicationPath, args, wtConfigurationFile);
}

void WServer::setIOService(WIOService& ioService)
{
  if (ioService_) {
    //LOG_ERROR("setIOService(): already have an IO service");
    return;
  }

  ioService_ = &ioService;
  ownsIOService_ = false;
}

WIOService& WServer::ioService()
{
  if (!ioService_) {
    ioService_ = new WIOService();
    int numSessionThreads = configuration().numSessionThreads();
    if (dedicatedProcessEnabled_&& numSessionThreads != -1)
      ioService_->setThreadCount(numSessionThreads);
    else
      ioService_->setThreadCount(configuration().numThreads());
  }

  return *ioService_;
}

void WServer::setServerConfiguration(const std::string &applicationPath, const std::vector<std::string> &args, const std::string &serverConfigurationFile)
{
  auto result = parseArgsPartially(applicationPath, args, serverConfigurationFile);

  if (!result.appRoot.empty())
    setAppRoot(result.appRoot);

  if (configurationFile().empty())
    setConfiguration(result.wtConfigXml);

  webController_ = new Wt::WebController(*this);

  impl_->serverConfiguration_ = new ::http::server::Configuration(logger());

  impl_->serverConfiguration_->setSslPasswordCallback(sslPasswordCallback_);

  impl_->serverConfiguration_->setOptions(applicationPath, args, serverConfigurationFile);

  dedicatedProcessEnabled_ = impl_->serverConfiguration_->parentPort() != -1;

  configuration().setDefaultEntryPoint(impl_->serverConfiguration_->deployPath());
}

void WServer::setAppRoot(const std::string& path)
{
  appRoot_ = path;

  if (configuration_)
    configuration_->setAppRoot(path);
}

std::string WServer::appRoot() const
{
  // FIXME we should const-correct Configuration too
  return const_cast<WServer *>(this)->configuration().appRoot();
}

void WServer::setConfiguration(const std::string& file)
{
  setConfiguration(file, application_);
}

void WServer::setConfiguration(const std::string& file,
                               const std::string& application)
{
  if (configuration_)
    //LOG_ERROR("setConfigurationFile(): too late, already configured");

  configurationFile_ = file;
  application_ = application;
}

WLogger& WServer::logger()
{
  return logger_;
}

void WServer::setCustomLogger(const WLogSink& customLogger)
{
  customLogger_ = &customLogger;
}

const WLogSink * WServer::customLogger() const
{
  return customLogger_;
}

WLogEntry WServer::log(const std::string& type) const
{
  if (customLogger_) {
    return WLogEntry(*customLogger_, type);
  }

  WLogEntry e = logger_.entry(type);

  e << WLogger::timestamp << WLogger::sep
    << getpid() << WLogger::sep
    << /* sessionId << */ WLogger::sep
    << '[' << type << ']' << WLogger::sep;

  return e;
}

bool WServer::dedicatedSessionProcess() const {
  return dedicatedProcessEnabled_;
}

void WServer::initLogger(const std::string& logFile,
                         const std::string& logConfig)
{
  if (!logConfig.empty())
    logger_.configure(logConfig);

  if (!logFile.empty())
    logger_.setFile(logFile);

  if (!description_.empty()){
    //LOG_INFO("initializing " << description_);
  }
}

Configuration& WServer::configuration()
{
  if (!configuration_) {
    if (appRoot_.empty())
      appRoot_ = Configuration::locateAppRoot();
    if (configurationFile_.empty())
      configurationFile_ = Configuration::locateConfigFile(appRoot_);

    configuration_ = new Configuration(application_, appRoot_,
                                       configurationFile_, this);
  }

  return *configuration_;
}

WebController *WServer::controller()
{
  return webController_;
}

bool WServer::readConfigurationProperty(const std::string& name,
                                        std::string& value) const
{
  WServer *self = const_cast<WServer *>(this);
  return self->configuration().readConfigurationProperty(name, value);
}

void WServer::post(const std::string& sessionId,
                   const std::function<void ()>& function,
                   const std::function<void ()>& fallbackFunction)
{
  schedule(std::chrono::milliseconds{0}, sessionId, function, fallbackFunction);
}

void WServer::postAll(const std::function<void ()>& function)
{
  if(!webController_) return;

  std::vector<std::string> sessions = webController_->sessions(true);
  for (std::vector<std::string>::const_iterator i = sessions.begin();
      i != sessions.end(); ++i) {
    schedule(std::chrono::milliseconds{0}, *i, function);
  }
}

void WServer::schedule(std::chrono::steady_clock::duration duration,
                       const std::string& sessionId,
                       const std::function<void ()>& function,
                       const std::function<void ()>& fallbackFunction)
{
  auto event = std::make_shared<ApplicationEvent>(sessionId, function, fallbackFunction);

  ioService().schedule(duration, [this, event] () {
          webController_->handleApplicationEvent(event);
  });
}

std::string WServer::prependDefaultPath(const std::string& path)
{
  assert(!configuration().defaultEntryPoint().empty() &&
         configuration().defaultEntryPoint()[0] == '/');
  if (path.empty())
    return configuration().defaultEntryPoint();
  else if (path[0] != '/') {
    const std::string &defaultPath = configuration().defaultEntryPoint();
    if (defaultPath[defaultPath.size() - 1] != '/')
      return defaultPath + "/" + path;
    else
      return defaultPath + path;
  } else
    return path;
}

void WServer::addEntryPoint(EntryPointType type, ApplicationCreator callback,
                            const std::string& path, const std::string& favicon)
{
  configuration().addEntryPoint(
        EntryPoint(type, callback, prependDefaultPath(path), favicon));
}

void WServer::addResource(WResource *resource, const std::string& path)
{
  bool success = configuration().tryAddResource(
        EntryPoint(resource, prependDefaultPath(path)));
  if (success)
    resource->setInternalPath(path);
  else {
    WString error(Wt::utf8("WServer::addResource() error: "
                           "a static resource was already deployed on path '{1}'"));
    throw WServer::Exception(error.arg(path).toUTF8());
  }
}

void WServer::removeEntryPoint(const std::string& path){
  configuration().removeEntryPoint(path);
}

void WServer::run(int argc, char** argv)
{
  httpd::http_server_control prometheus_server;
  prometheus::config pctx;
  app_template app;

  app.add_options()("port", bpo::value<uint16_t>()->default_value(10000), "HTTP Server port");
  app.add_options()("prometheus_port", bpo::value<uint16_t>()->default_value(9180), "Prometheus port. Set to zero in order to disable.");
  app.add_options()("prometheus_address", bpo::value<sstring>()->default_value("0.0.0.0"), "Prometheus address");
  app.add_options()("prometheus_prefix", bpo::value<sstring>()->default_value("seastar_httpd"), "Prometheus metrics prefix");

  app.run_deprecated(argc, argv, [&] { //BUG with io_uring - hanging

      std::cout << "number of threads : " << seastar::smp::count << "\n";

      return seastar::async([&] {
          stop_signal stop_signal;
          auto&& config = app.configuration();
          httpd::http_server_control prometheus_server;

          uint16_t pport = config["prometheus_port"].as<uint16_t>();
          if (pport) {
              prometheus::config pctx;
              net::inet_address prom_addr(config["prometheus_address"].as<sstring>());

              pctx.metric_help = "seastar::httpd server statistics";
              pctx.prefix = config["prometheus_prefix"].as<sstring>();

              std::cout << "starting prometheus API server" << std::endl;
              prometheus_server.start("prometheus").get();

              prometheus::start(prometheus_server, pctx).get();

              prometheus_server.listen(socket_address{prom_addr, pport}).handle_exception([prom_addr, pport] (auto ep) {
                                                                            std::cerr << seastar::format("Could not start Prometheus API server on {}:{}: {}\n", prom_addr, pport, ep);
                                                                            return make_exception_future<>(ep);
                                                                        }).get();
          }

          uint16_t port = config["port"].as<uint16_t>();
          auto server = new http_server_control();
          auto rb = std::make_shared<api_registry_builder>("apps/httpd/");
          server->start().get();
          server->set_routes([&] (auto& routes){


              auto& entrypoints = configuration().entryPoints_;

              for (auto &ep : entrypoints)
              {
//                  function_handler* hget = new function_handler([](const_req req) {
//                      return "hello";
//                  });
                  function_handler* hpost = new function_handler([](const_req req) {
                      return "hello";
                  });

                  gethandler* hget = new gethandler(webController_, impl_->serverConfiguration_, &ep);

                  routes.add(operation_type::GET, seastar::url(ep.path()), hget);
                  routes.add(operation_type::POST, seastar::url(ep.path()), hpost);

                  if(ep.type() != Wt::EntryPointType::StaticResource)
                  {

                  }
              }

              //auto h1 = new handl;
              function_handler* h1 = new function_handler([](const_req req) {
                  return "hello";
              });
//              function_handler* h2 = new function_handler([](std::unique_ptr<http::request> req) {
//                  return make_ready_future<json::json_return_type>("json-future");
//              });
              routes.add(operation_type::GET, seastar::url("/"), h1);
          }).get();
//          server->set_routes(set_routes).get();
//          server->set_routes([rb](routes& r){rb->set_api_doc(r);}).get();
//          server->set_routes([rb](routes& r) {rb->register_function(r, "demo", "hello world application");}).get();
          server->listen(port).get();

          std::cout << "Seastar HTTP server listening on port " << port << " ...\n";

          auto ws = new experimental::websocket::server;
//          ws->register_handler("echo", [] (input_stream<char>& in, output_stream<char>& out) {
//              return repeat([&in, &out]() {
//                  return in.read().then([&out](temporary_buffer<char> f) {
//                      std::cerr << "f.size(): " << f.size() << "\n";
//                      if (f.empty()) {
//                          return make_ready_future<stop_iteration>(stop_iteration::yes);
//                      } else {
//                          return out.write(std::move(f)).then([&out]() {
//                              return out.flush().then([] {
//                                  return make_ready_future<stop_iteration>(stop_iteration::no);
//                              });
//                          });
//                      }
//                  });
//              });
//          });
//          ws->listen(socket_address(ipv4_addr("127.0.0.1", 10001)));
//          std::cout << "Listening on 127.0.0.1:8123 for 1 hour (interruptible, hit Ctrl-C to stop)..." << std::endl;


          engine().at_exit([&prometheus_server, server, ws, pport] {
              return [pport, &prometheus_server] {
                  if (pport) {
                      std::cout << "Stoppping Prometheus server" << std::endl;
                      return prometheus_server.stop();
                  }
                  return make_ready_future<>();
              }().then([ws] {
                             return ws->stop();
                         }).finally([server] {
                             std::cout << "Stoppping HTTP server" << std::endl;
                             return server->stop();
                         });
          });

          stop_signal.wait().get();
          //waitForShutdown();
      });
  });
}

void WServer::restart(int argc, char **argv, char **envp)
{
#ifndef WT_WIN32
  char *path = realpath(argv[0], 0);

  // Try a few times since this may fail because we have an incomplete
  // binary...
  for (int i = 0; i < 5; ++i) {
    int result = execve(path, argv, envp);
    if (result != 0)
      sleep(1);
  }

  perror("execve");
#endif
}

void WServer::restart(const std::string &applicationPath,
                      const std::vector<std::string> &args)
{
#ifndef WT_WIN32
  std::unique_ptr<char*[]> argv(new char*[args.size() + 1]);
  argv[0] = const_cast<char*>(applicationPath.c_str());
  for (unsigned i = 0; i < args.size(); ++i) {
    argv[i+1] = const_cast<char*>(args[i].c_str());
  }

  restart(static_cast<int>(args.size() + 1), argv.get(), nullptr);
#endif
}

void WServer::setCatchSignals(bool catchSignals)
{
  CatchSignals = catchSignals;
}

#if defined(WT_WIN32) && defined(WT_THREADED)

std::mutex terminationMutex;
bool terminationRequested = false;
std::condition_variable terminationCondition;

void WServer::terminate()
{
  std::unique_lock<std::mutex> terminationLock(terminationMutex);
  terminationRequested = true;
  terminationCondition.notify_all(); // should be just 1
}

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
  switch (ctrl_type)
  {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    {
      WServer::terminate();
      return TRUE;
    }
  default:
    return FALSE;
  }
}

#endif

int WServer::waitForShutdown()
{
#if !defined(WT_WIN32)
  if (!CatchSignals) {
    for(;;)
      sleep(0x1<<16);
  }
#endif // WIN32

#ifdef WT_THREADED

#if !defined(WT_WIN32)
  sigset_t wait_mask;
  sigemptyset(&wait_mask);

  // Block the signals which interest us
  sigaddset(&wait_mask, SIGHUP);
  sigaddset(&wait_mask, SIGINT);
  sigaddset(&wait_mask, SIGQUIT);
  sigaddset(&wait_mask, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &wait_mask, 0);

  for (;;) {
    int rc, sig= -1;

    // Wait for a signal to be raised
    rc= sigwait(&wait_mask, &sig);

    // branch based on return value of sigwait().
    switch (rc) {
      case 0: // rc indicates one of the blocked signals was raised.

        // branch based on the signal which was raised.
        switch(sig) {
          case SIGHUP: // SIGHUP means re-read the configuration.
            if (instance())
              instance()->configuration().rereadConfiguration();
            break;

          default: // Any other blocked signal means time to quit.
            return sig;
        }

        break;
      case EINTR:
        // rc indicates an unblocked signal was raised, so we'll go
        // around again.

        break;
      default:
        // report the error and return an obviously illegitimate signal value.
        throw WServer::Exception(std::string("sigwait() error: ")
                                 + strerror(rc));
        return -1;
    }
  }

#else  // WIN32

  std::unique_lock<std::mutex> terminationLock(terminationMutex);
  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
  while (!terminationRequested)
    terminationCondition.wait(terminationLock);
  SetConsoleCtrlHandler(console_ctrl_handler, FALSE);
  return 0;

#endif // WIN32
#else
  return 0;
#endif // WT_THREADED
}

bool WServer::expireSessions()
{
  return webController_->expireSessions();
}

void WServer::scheduleStop()
{
#ifdef WT_THREADED
  #ifndef WT_WIN32
    kill(getpid(), SIGTERM);
  #else // WT_WIN32
    terminate();
  #endif // WT_WIN32
#else // !WT_THREADED
  if (stopCallback_)
    stopCallback_();
#endif // WT_THREADED
}

void WServer::updateProcessSessionId(const std::string& sessionId) {
  if (updateProcessSessionIdCallback_)
    updateProcessSessionIdCallback_(sessionId);
}

}
