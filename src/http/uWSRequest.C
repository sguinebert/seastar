/*
 * Copyright (C) 2008 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include "uWSRequest.h"
#include "Configuration.h"
//#include "WtReply.h"

#include "Wt/Utils.h"
#include "Wt/WConfig.h"
#include "Wt/WLogger.h"
#include "Wt/WSslInfo.h"
#include "Wt/Http/Message.h"
#include "Wt/Json/json.hpp"
#include "Wt/Json/Parser.h"

#include "web/SslUtils.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include<boost/algorithm/string.hpp>

#define PEM_HEADER "-----BEGIN CERTIFICATE-----"
#define PEM_FOOTER "-----END CERTIFICATE-----"

#define PEM_ESCAPED_HEADER "-----BEGIN%20CERTIFICATE-----"
#define PEM_ESCAPED_FOOTER "-----END%20CERTIFICATE-----%0A"

namespace Wt
{
  LOGGER("wthttp");
}

using namespace http;
using namespace seastar::http;

//std::string_view statusToText(Reply::status_type status)
//{
//  switch (status)
//  {
//  case Reply::switching_protocols:
//    return "101 Switching Protocol\r\n";
//    break;
//  case Reply::ok:
//    return  "200 OK\r\n";
//    break;
//  case Reply::created:
//    return  "201 Created\r\n";
//    break;
//  case Reply::accepted:
//    return  "202 Accepted\r\n";
//    break;
//  case Reply::no_content:
//    return  "204 No Content\r\n";
//    break;
//  case Reply::partial_content:
//    return  "206 Partial Content\r\n";
//    break;
//  case Reply::multiple_choices:
//    return  "300 Multiple Choices\r\n";
//    break;
//  case Reply::moved_permanently:
//    return  "301 Moved Permanently\r\n";
//    break;
//  case Reply::found:
//    return  "302 Found\r\n";
//    break;
//  case Reply::see_other:
//    return  "303 See Other\r\n";
//    break;
//  case Reply::not_modified:
//    return "304 Not Modified\r\n";
//    break;
//  case Reply::moved_temporarily:
//    return "307 Moved Temporarily\r\n";
//    break;
//  case Reply::bad_request:
//    return "400 Bad Request\r\n";
//    break;
//  case Reply::unauthorized:
//    return "401 Unauthorized\r\n";
//    break;
//  case Reply::forbidden:
//    return "403 Forbidden\r\n";
//    break;
//  case Reply::not_found:
//    return "404 Not Found\r\n";
//    break;
//  case Reply::request_entity_too_large:
//    return "413 Request Entity too Large\r\n";
//    break;
//  case Reply::requested_range_not_satisfiable:
//    return "416 Requested Range Not Satisfiable\r\n";
//    break;
//  case Reply::not_implemented:
//    return "501 Not Implemented\r\n";
//    break;
//  case Reply::bad_gateway:
//    return "502 Bad Gateway\r\n";
//    break;
//  case Reply::service_unavailable:
//    return "503 Service Unavailable\r\n";
//    break;
//  case Reply::version_not_supported:
//    return "505 HTTP Version Not Supported\r\n";
//    break;
//  case Reply::no_status:
//  case Reply::internal_server_error:
//    return "500 Internal Server Error\r\n";
//    break;
//  default:
//    return (int) status + " Unknown\r\n";
//  }
//}

namespace http
{

    const std::string uWSRequest::empty_;

    uWSRequest::uWSRequest(seastar::http::reply *reply,
                           seastar::http::request *request,
                           //uWS::WebSocket<false, true, PerSocketData> *ws,
                           http::server::Configuration *serverConfiguration,
                           const Wt::EntryPoint *entryPoint)
        :
        serverConfiguration_(serverConfiguration),
        uwsreply_(reply),
        request_(request),
        out_(&out_buf_), in_(&in_mem_),
        isWebSocket_(false)
    {
        entryPoint_ = entryPoint;
        scriptname_ = request_->_url;
    }

    void uWSRequest::reset(seastar::http::reply *reply,
                           seastar::http::request *request,
                           const Wt::EntryPoint *entryPoint)
    {
      WebRequest::reset();

      uwsreply_ = reply;
      request_ = request;
      entryPoint_ = entryPoint;

      in_mem_.str("");
      in_mem_.clear();


    }

    bool uWSRequest::done() const
    {
      return uwsreply_ == nullptr;
    }

    void uWSRequest::flush(ResponseState state, const WriteCallback &callback)
    {
      //WtReplyPtr ptr = reply_;
//      return;
//      if (done())
//        return;

      //out_buf_.write(uwsreply_->_content).get();

//      const char *data = boost::asio::buffer_cast<const char *>(out_buf_.data());
//      int size = boost::asio::buffer_size(out_buf_.data());
//      uwsreply_->write_body(contenttype_, seastar::sstring(data, size));
//      out_buf_.consume(size);

//      const char *data = asio::buffer_cast<const char *>(out_buf_.data());
//      int size = asio::buffer_size(out_buf_.data());
//      uwsreply_->end(std::string_view(data, size));
//      out_buf_.consume(size);


      // if (status_ != 200)
      //   uwsreply_->end("404 not found");
      //if (state == ResponseState::ResponseDone)
      //  reply_.reset();
      

      //ptr->send(callback, state == ResponseState::ResponseDone);
      //uwsreply_->end();
    }

    void uWSRequest::readWebSocketMessage(const ReadCallback &callback)
    {
      //reply_->readWebSocketMessage(callback);
    }

    bool uWSRequest::webSocketMessagePending() const
    {
      //return reply_->readAvailable();
      return false;
    }

    bool uWSRequest::detectDisconnect(const DisconnectCallback &callback)
    {
      //reply_->detectDisconnect(callback);
      return true;
    }

    void uWSRequest::setStatus(int status)
    {
      status_ = (reply::status_type)status;
      uwsreply_->set_status(status_);
    }

    void uWSRequest::setContentLength(::int64_t length)
    {
      //reply_->setContentLength(length);
      //uwsreply_->setContentLength(length);
      contentLength_ = length;
    }

    void uWSRequest::addHeader(const std::string &name, const std::string &value)
    {
      uwsreply_->add_header(name, value);
    }

    void uWSRequest::setContentType(const std::string &value)
    {
      //std::cout << "set Content-Type" << " - " <<  value << std::endl;
      //reply_->setContentType(value);
      uwsreply_->add_header("Content-Type", value);
    }

    void uWSRequest::setRedirect(const std::string &url)
    {
      uwsreply_->add_header("Location", url);
    }

    std::string_view uWSRequest::headerValue(const char *name) const
    {
      if (!request_) {
        auto it = headers_.find(name);
        
        std::string_view sv;
        if(it != headers_.end())
            sv = it->second;
        //std::cout << "%%%%%%%%% string " << sv << std::endl;
        return sv;
      }

      auto i = request_->get_header(name);
      return i;
    }

    std::vector<Wt::Http::Message::Header> uWSRequest::headers() const
    {
      std::vector<Wt::Http::Message::Header> headerVector;
//      WtReplyPtr p = reply_;
//      if (!p.get())
//        return headerVector;

//      const std::list<Request::Header> &headers = p->request().headers;

//      for (std::list<Request::Header>::const_iterator it = headers.begin(); it != headers.end(); ++it)
//      {
//        if (cstr(it->name))
//        {
//          headerVector.push_back(Wt::Http::Message::Header(it->name.str(), it->value.str()));
//        }
//      }

      if (!request_)
        return headerVector;

      for (auto &[key, value] : request_->_headers)
      {
          headerVector.push_back(Wt::Http::Message::Header(key, value));
      }

      return headerVector;
    }

//    const char *uWSRequest::cstr(const buffer_string &bs) const
//    {
//      if (!bs.next)
//        return bs.data;
//      else
//      {
//        s_.push_back(bs.str());
//        return s_.back().c_str();
//      }
//    }

    ::int64_t uWSRequest::contentLength() const
    {
      // WtReplyPtr p = reply_;
      // if (!p.get())
      //   return 0;

      return contentLength_;

      //return p->request().contentLength;
    }

    std::string_view uWSRequest::contentType() const
    {
      //auto i = headerValue("content-type");
      //std::cout << "contenttype " << i << std::endl; 
      // if(!contenttype_.empty())
      //    return contenttype_;
      return headerValue("content-type"); 
    }

    std::string_view uWSRequest::envValue(const char *name) const
    {
      if (strcmp(name, "CONTENT_TYPE") == 0)
      {
        return headerValue("content-type");
      }
      else if (strcmp(name, "CONTENT_LENGTH") == 0)
      {
        return headerValue("content-length");
      }
      else if (strcmp(name, "SERVER_SIGNATURE") == 0)
      {
        return "<address>Wt httpd server</address>";
      }
      else if (strcmp(name, "SERVER_SOFTWARE") == 0)
      {
        return "Wthttpd/" WT_VERSION_STR;
      }
      else if (strcmp(name, "SERVER_ADMIN") == 0)
      {
        return "webmaster@localhost";
      }
      else if (strcmp(name, "REMOTE_ADDR") == 0)
      {
        return remoteAddr().data();
      }
      else if (strcmp(name, "DOCUMENT_ROOT") == 0)
      {
        return serverConfiguration_->docRoot().c_str();
      }
      else
        return std::string_view();
    }

    std::string_view uWSRequest::serverName() const
    {
      if (!request_)
        return empty_;

      return serverConfiguration_->serverName();
    }

    std::string_view uWSRequest::serverPort() const
    {
      if (!request_)
        return empty_;

      if (serverPort_.empty())
        serverPort_ = empty_;

      return serverPort_;
    }

    std::string_view uWSRequest::scriptName() const
    {
      //WtReplyPtr p = reply_;
      if (!request_)
        return empty_;

      //std::cerr << "scriptName " << request_->getUrl() << std::endl;
      return scriptname_;
      //return request_->getUrl();
      //return p->request().request_path;
    }

    std::string_view uWSRequest::requestMethod() const
    {
      //WtReplyPtr p = reply_;
      if (!request_)
        return method_;
      //uwsreply_->getmethod();
      // if(request_->getMethod().empty() && !method_.empty())
      //   return method_;

      //std::cerr << "method " << request_->getMethod() << std::endl;
      return request_->_method;
    }

    std::string_view uWSRequest::queryString() const
    {
      //WtReplyPtr p = reply_;
      if (!request_)
        return query_;
      //std::cerr << "query " << request_->getQuery() << std::endl;
      return request_->parse_query_param();
    }

    std::string_view uWSRequest::pathInfo() const
    {
      //WtReplyPtr p = reply_;
      if (!request_)
        return empty_;

      //std::cerr << "pathInfo " << "" << std::endl;
      //return p->request().request_extra_path;
      return "";
    }

    std::string_view uWSRequest::remoteAddr() const
    {
      //WtReplyPtr p = reply_;
      if (!uwsreply_)
        return empty_;

      //std::cerr << "remoteAddr " << uwsreply_->getRemoteAddressAsText() << std::endl;
      return "uwsreply_->getRemoteAddress()";
      //return p->request().remoteIP;
    }

    const char *uWSRequest::urlScheme() const
    {
      if (!request_)
        return "http";

      return request_->get_protocol_name().c_str();
    }

    bool uWSRequest::isSynchronous() const
    {
      return false;
    }

    std::unique_ptr<Wt::WSslInfo> uWSRequest::sslInfo(const Wt::Configuration &conf) const
    {
      return sslInfoFromHeaders();

//      auto result = reply_->request().sslInfo();
//      if (conf.behindReverseProxy() ||
//          conf.isTrustedProxy(remoteAddr()))
//      {
//#ifdef HTTP_WITH_SSL
//        if (!result)
//          result = sslInfoFromJson();
//#endif // HTTP_WITH_SSL
//        if (!result)
//          result = sslInfoFromHeaders();
//      }
//      return result;
    }

#ifdef HTTP_WITH_SSL
    std::unique_ptr<Wt::WSslInfo> uWSRequest::sslInfoFromJson() const
    {
      const char *const ssl_client_certificates = std::string(headerValue("x-wt-ssl-client-certificates")).c_str();

      if (!ssl_client_certificates)
        return nullptr;

      Wt::Json::Object obj;
      Wt::Json::ParseError error;
      if (!Wt::Json::parse(Wt::Utils::base64Decode(ssl_client_certificates), obj, error))
      {
        LOG_ERROR("error while parsing client certificates");
        return nullptr;
      }

      std::string clientCertificatePem = obj["client-certificate"].get_string("");

      X509 *cert = Wt::Ssl::readFromPem(clientCertificatePem);

      if (cert)
      {
        Wt::WSslCertificate clientCert = Wt::Ssl::x509ToWSslCertificate(cert);
        X509_free(cert);

        Wt::Json::Array &arr = obj["client-pem-certification-chain"].as_array();

        std::vector<Wt::WSslCertificate> clientCertChain;

        for (const auto &cert : arr)
        {
          clientCertChain.push_back(Wt::Ssl::x509ToWSslCertificate(Wt::Ssl::readFromPem(cert.get_string(""))));
        }

        Wt::ValidationState state = static_cast<Wt::ValidationState>(obj["client-verification-result-state"].get_int64(0));
        Wt::WString message = obj["client-verification-result-message"].get_string("");

        return std::make_unique<Wt::WSslInfo>(clientCert,
                                              clientCertChain,
                                              Wt::WValidator::Result(state, message));
      }

      return nullptr;
    }
#endif // HTTP_WITH_SSL

    std::unique_ptr<Wt::WSslInfo> uWSRequest::sslInfoFromHeaders() const
    {
      return nullptr;
      auto client_verify = headerValue("x-ssl-client-verify");
      auto client_s_dn = headerValue("x-ssl-client-s-dn");
      auto client_i_dn = headerValue("x-ssl-client-i-dn");
      auto validity_start = headerValue("x-ssl-client-v-start");
      auto validity_end = headerValue("x-ssl-client-v-end");
      auto client_cert = headerValue("x-ssl-client-cert");

      if (client_verify.empty())
        return nullptr;

      if (boost::iequals(client_verify, "none"))
        return nullptr;

      enum Verify
      {
        SUCCESS,
        FAILED,
        GENEROUS
      };

      const char *failedReason = nullptr;

      Verify v = FAILED;
      if (boost::iequals(client_verify, "success"))
        v = SUCCESS;
      else if (boost::iequals(client_verify, "generous"))
        v = GENEROUS;
      else if (boost::istarts_with(client_verify, "failed:"))
      {
        v = FAILED;
        failedReason = client_verify.data() + sizeof("FAILED");
      }
      else
        return nullptr;

      std::string clientCertStr;
      if (!client_cert.empty())
      {
        clientCertStr = client_cert;
        boost::trim(clientCertStr);
        if (boost::starts_with(clientCertStr, PEM_HEADER))
        {
          const std::size_t start = sizeof(PEM_HEADER) - 1;
          const std::size_t end = clientCertStr.find(PEM_FOOTER);
          if (end != std::string::npos)
          {
            for (std::size_t i = start; i < end; ++i)
            {
              if (clientCertStr[i] == ' ')
                clientCertStr[i] = '\n';
            }
          }
          else
          {
            clientCertStr.clear();
          }
        }
        else if (boost::starts_with(clientCertStr, PEM_ESCAPED_HEADER) &&
                 boost::ends_with(clientCertStr, PEM_ESCAPED_FOOTER))
        {
          clientCertStr = Wt::Utils::urlDecode(clientCertStr);
        }
        else
        {
          clientCertStr.clear();
        }
      }

#ifdef WT_WITH_SSL
      if (!clientCertStr.empty())
      {
        // try parse cert, use cert for all other info
        X509 *cert = Wt::Ssl::readFromPem(clientCertStr);

        if (cert)
        {
          Wt::WSslCertificate clientCert = Wt::Ssl::x509ToWSslCertificate(cert);
          return std::make_unique<Wt::WSslInfo>(clientCert,
                                                std::vector<Wt::WSslCertificate>(),
                                                Wt::WValidator::Result(v == SUCCESS ? Wt::ValidationState::Valid : Wt::ValidationState::Invalid,
                                                                       failedReason ? Wt::utf8(failedReason) : Wt::WString::Empty));
        }
      }
#endif // WT_WITH_SSL

      if (!client_s_dn.empty() &&
          !client_i_dn.empty() &&
          !validity_start.empty() &&
          !validity_end.empty())
      {
        std::vector<Wt::WSslCertificate::DnAttribute> subjectDn =
            Wt::WSslCertificate::dnFromString(client_s_dn);
        std::vector<Wt::WSslCertificate::DnAttribute> issuerDn =
            Wt::WSslCertificate::dnFromString(client_i_dn);

        const Wt::WString validityFormat = Wt::utf8("MMM dd hh:mm:ss yyyy 'GMT'");
        Wt::WDateTime validityStart = Wt::WDateTime::fromString(
            Wt::utf8(std::string(validity_start)), validityFormat);
        Wt::WDateTime validityEnd = Wt::WDateTime::fromString(
            Wt::utf8(std::string(validity_end)), validityFormat);

        Wt::WSslCertificate clientCert(subjectDn,
                                       issuerDn,
                                       validityStart,
                                       validityEnd,
                                       clientCertStr);
        return std::make_unique<Wt::WSslInfo>(clientCert,
                                              std::vector<Wt::WSslCertificate>(),
                                              Wt::WValidator::Result(v == SUCCESS ? Wt::ValidationState::Valid : Wt::ValidationState::Invalid,
                                                                     failedReason ? Wt::utf8(failedReason) : Wt::WString::Empty));
      }
      return nullptr;
    }

    const std::vector<std::pair<std::string, std::string>> &uWSRequest::urlParams() const
    {
//      WtReplyPtr p = reply_;
//      if (!p.get())
//        return WebRequest::urlParams();
//      return p->request().url_params;

        if (!request_)
          return WebRequest::urlParams();

        std::vector<std::pair<std::string, std::string>> urlparams;
        for(auto& [key, value] : request_->query_parameters)
          urlparams.push_back(std::make_pair(key, value));

        return std::move(urlparams);
    }
} // namespace http
