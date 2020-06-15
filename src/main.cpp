#include "HttpListener.hpp"
#include "JsonApiRequestHandler.hpp"
#include "StaticRequestHandler.hpp"
#include <boost/asio/signal_set.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <memory>
#include <string>
#include <thread>

// -----------------------------------------------------------------------
// below code for poc purpose only, remove later.
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

namespace
{
class SamplingMock
{
public:
    class SamplingBlock
    {
    public:
        SamplingBlock(size_t id, SamplingMock& mock, int timeout, int snapsNeeded)
        : id{id}
        , mock(mock)
        , timeout{timeout}
        , snapsNeeded{snapsNeeded}
        , loop{&SamplingBlock::run, this}
        {
        }
        SamplingBlock(const SamplingBlock&) = delete;
        SamplingBlock& operator=(const SamplingBlock&) = delete;

        SamplingBlock(SamplingBlock&& block) = delete;
        SamplingBlock& operator=(SamplingBlock&& block) = delete;

        ~SamplingBlock()
        {
            stop();
        }

        bool isRunning() const
        {
            return running;
        }

        void stop()
        {
            if(running)
            {
                running = false;
            }
            loop.join();
        }

        int progress()
        {
            int p0 = 0;
            int p1 = 0;
            if(timeout > 0)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
                p0 = std::min((elapsed * 100 / timeout), 100);
            }
            if(snapsNeeded > 0)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
                p1 = std::min((static_cast<int>(snaps.size()) * 100 / snapsNeeded), 100);
            }
            return std::max(p0, p1);
        }

        void run()
        {
            running = true;
            elapsed = 0;
            while(running)
            {
                std::cout << timeout << " vs " << elapsed << std::endl;
                std::cout << snapsNeeded << " vs " << snaps.size() << std::endl;
                if((timeout > 0 and elapsed >= timeout) or
                   (snapsNeeded > 0 and snaps.size() >= snapsNeeded))
                {
                    running = false;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::seconds{1});
                    ++elapsed;
                    const auto snapId = mock.generateSnap();
                    std::unique_lock<std::mutex> lock(snapsMutex);
                    snaps.emplace_back(snapId);
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
                    while(snaps.size() > 30)
                    {
                        snaps.pop_front();
                    }
                }
            }
        }

        std::string getStatus(bool detail)
        {
            nlohmann::json json = {
                {"ret", 0},
                {"id", id},
            };
            if(running)
            {
                json.emplace("status", "running");
            }
            else
            {
                json.emplace("status", "finished");
            }
            if(detail)
            {
                json.emplace("progress", progress());
                std::unique_lock<std::mutex> lock(snapsMutex);
                json.emplace("snaps", snaps);
            }
            return std::move(json.dump());
        }

    private:
        std::mutex snapsMutex{};
        size_t id{0};
        std::list<size_t> snaps{};
        bool running{true};
        SamplingMock& mock;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        int timeout{60};
        int snapsNeeded{-1};
        int elapsed{0};
        std::thread loop;
    };

    class SnapBlock
    {
    public:
        explicit SnapBlock(size_t id) : id{id}
        {
        }

    private:
        size_t id;
    };

    size_t generateSnap()
    {
        std::unique_lock<std::mutex> lock(snapsMutex);
        auto snap = boost::make_unique<SnapBlock>(snapId);
        snaps.emplace(snapId++, std::move(snap));
        return snapId - 1;
    }

    size_t start(int timeout, int snapsNeeded)
    {
        std::unique_lock<std::mutex> lock(samplingMutex);
        auto sampleBlock =
            boost::make_unique<SamplingBlock>(samplingId, *this, timeout, snapsNeeded);
        samplings.emplace(samplingId++, std::move(sampleBlock));
        return samplingId - 1;
    }

    void stop(size_t sampleId)
    {
        std::unique_lock<std::mutex> lock(samplingMutex);
        samplings.erase(sampleId);
    }

    bool isRunning() const
    {
        std::unique_lock<std::mutex> lock(samplingMutex);
        return std::any_of(samplings.begin(), samplings.end(),
                           [](const Samplings::value_type& it) {
                               return it.second->isRunning();
                           });
    }

    std::string getStatus(size_t sampleId, bool detail)
    {
        std::unique_lock<std::mutex> lock(samplingMutex);
        return std::move(samplings.at(sampleId)->getStatus(detail));
    }

private:
    using Samplings = std::unordered_map<size_t, std::unique_ptr<SamplingBlock>>;
    using Snaps = std::unordered_map<size_t, std::unique_ptr<SnapBlock>>;
    mutable std::mutex samplingMutex{};
    mutable std::mutex snapsMutex{};
    size_t samplingId{1};
    size_t snapId{1};
    Samplings samplings{};
    Snaps snaps;
};

class TrainingMock
{
public:
};

class ModelMock
{
public:
};

SamplingMock samplingMock;

void installPoc(HttpListener& server)
{
    // POST /api/v1/calibration/sampling/start
    auto samplingStartHandler =
        std::make_shared<JsonApiRequestHandler>([](HttpSession::Request& req) -> std::string {
            auto& body = req.body();
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            int timeout{60};
            int snaps{-1};
            if(body.size() > 0)
            {
                try
                {
                    auto json = nlohmann::json::parse(body);
                    auto itTimeout = json.find("timeout");
                    if(itTimeout != json.end())
                    {
                        timeout = itTimeout.value();
                    }
                    auto itSnaps = json.find("snaps");
                    if(itSnaps != json.end())
                    {
                        snaps = itSnaps.value();
                    }
                }
                catch(const nlohmann::json::parse_error& ex)
                {
                    throw std::runtime_error(ex.what());
                }
            }
            if(samplingMock.isRunning())
            {
                throw std::runtime_error("already running");
            }
            auto sampleId = samplingMock.start(timeout, snaps);
            try
            {
                return samplingMock.getStatus(sampleId, false);
            }
            catch(const std::out_of_range&)
            {
                throw JsonApiRequestHandler::NotFound(
                    (std::string("id: ") + std::to_string(sampleId)).c_str());
            }
        });
    server.registHandler("^/api/v1/calibration/sampling/start$", samplingStartHandler);

    // GET /api/v1/calibration/sampling/\d+
    auto samplingStatusHandler =
        std::make_shared<JsonApiRequestHandler>([](HttpSession::Request& req) -> std::string {
            std::string target = req.target().to_string();
            auto pos = target.find('/');
            auto idStr = target.substr(pos + 1);
            size_t sampleId{0};
            boost::conversion::try_lexical_convert(idStr, sampleId);
            if(sampleId == 0)
            {
                throw std::runtime_error("invalid id format");
            }
            try
            {
                return samplingMock.getStatus(sampleId, true);
            }
            catch(const std::out_of_range&)
            {
                throw JsonApiRequestHandler::NotFound(target.c_str());
            }
        });
    server.registHandler("^/api/v1/calibration/sampling/\\d+$", samplingStatusHandler);

    // POST /api/v1/calibration/sampling/stop
    auto samplingStopHandler =
        std::make_shared<JsonApiRequestHandler>([](HttpSession::Request& req) -> std::string {
            try
            {
                auto json = nlohmann::json::parse(req.body());
                size_t id = json.at("id");
                samplingMock.stop(id);
            }
            catch(const nlohmann::json::parse_error& ex)
            {
                throw std::runtime_error(ex.what());
            }
            catch(const nlohmann::json::out_of_range& outOfRange)
            {
                throw std::runtime_error(outOfRange.what());
            }
            catch(const std::out_of_range&)
            {
                throw JsonApiRequestHandler::NotFound("id may not exist");
            }
            return nlohmann::json{{"ret", 0}}.dump();
        });
    server.registHandler("^/api/v1/calibration/sampling/stop$", samplingStopHandler);

    // TODO: GET /api/v1/realtime/preview/picture/<uuid>/[0-4]
    // TODO: GET /api/v1/realtime/preview?prev=<uuid>
}
} // namespace
// end poc
// -------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    constexpr uint16_t defaultPort{8088};
    namespace asio = boost::asio;
    using tcp = asio::ip::tcp;
    using address = asio::ip::address_v4;
    size_t threads = std::thread::hardware_concurrency();

    const char* documentRootPath{"/root/beast-examples/cmake-build-debug"};
    uint16_t listenPort{defaultPort};
    if(argc >= 3)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::string port{argv[2]};
        boost::conversion::try_lexical_convert(port, listenPort);
    }
    if(argc >= 2)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        documentRootPath = argv[1];
    }

    asio::io_context ioContext{static_cast<int>(threads)};

    const tcp::endpoint endpoint{address::any(), listenPort};
    auto documentRoot = std::make_shared<std::string const>(documentRootPath);

    // run http server
    auto server = std::make_shared<HttpListener>(ioContext, endpoint, documentRoot);

    try
    {
        // install api handler
        auto versionApiHandler =
            std::make_shared<JsonApiRequestHandler>([](HttpSession::Request&) -> std::string {
                return R"({"ret":0,"version":"1.0"})";
            });
        server->registHandler("^/api/v1/version$", versionApiHandler);

        // POC, TODO: replace with real impl later.
        installPoc(*server);

        // install static handler
        auto staticRequestHandler = std::make_shared<StaticRequestHandler>(*documentRoot);
        server->registHandler(".*", staticRequestHandler);

        server->run();

        boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code const&, int) {
            // Stop the `io_context`. This will cause `run()`
            // to return immediately, eventually destroying the
            // `io_context` and all of the sockets in it.
            ioContext.stop();
        });

        // run with threads
        std::vector<std::thread> runners;
        runners.reserve(threads - 1);
        for(auto i = threads - 1; i > 0; --i)
        {
            runners.emplace_back([&ioContext] { ioContext.run(); });
        }
        ioContext.run();

        // wait all threads
        for(auto& runner : runners)
        {
            runner.join();
        }
    }
    catch(const std::exception& ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
        ioContext.stop();
    }

    return 0;
}