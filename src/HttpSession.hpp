#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/make_unique.hpp>
#include <memory>
#include <vector>

class HttpUriRouter;

class HttpSession : public std::enable_shared_from_this<HttpSession>
{
    using tcp = boost::asio::ip::tcp;
    using error_code = boost::system::error_code;

public:
    class Queue
    {
        enum
        {
            limit = 16
        };

        class Worker
        {
        public:
            virtual ~Worker() = default;
            virtual void operator()() = 0;
        };

    public:
        explicit Queue(HttpSession& session);

        bool isFull() const
        {
            return workers.size() >= limit;
        }

        template <bool isRequest, class Body, class Fields>
        void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg)
        {
            class WorkerImpl : public Worker
            {
            public:
                WorkerImpl(HttpSession& session,
                           boost::beast::http::message<isRequest, Body, Fields>&& msg)
                : msg(std::move(msg)), self(session)
                {
                }

                void operator()() override
                {
                    auto session = self.shared_from_this();
                    auto exec = [session, this](error_code error, size_t) {
                        session->onWrite(error, msg.need_eof());
                    };
                    boost::beast::http::async_write(
                        self.socket, msg, boost::asio::bind_executor(self.strand, exec));
                }

            private:
                boost::beast::http::message<isRequest, Body, Fields> msg;
                HttpSession& self;
            };

            workers.emplace_back(boost::make_unique<WorkerImpl>(self, std::move(msg)));

            // If there was no previous work, start this one
            if(workers.size() == 1)
            {
                auto& worker = workers.front();
                (*worker)();
            }
        }

        bool onWrite();

    private:
        HttpSession& self;
        std::vector<std::unique_ptr<Worker>> workers;
    };

    using Request = boost::beast::http::request<boost::beast::http::string_body>;

    HttpSession(tcp::socket, HttpUriRouter&);

    void run();

    void doRead();

    void onTimer(error_code);

    void onRead(error_code, size_t);

    void onWrite(error_code, bool);

    void doClose();

private:
    tcp::socket socket;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::asio::steady_timer timer;
    boost::beast::flat_buffer buffer;
    Request request;
    Queue queue;
    HttpUriRouter& router;
};

#endif // HTTP_SESSION_HPP
