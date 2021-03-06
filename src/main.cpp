#include "HttpListener.hpp"
#include "JsonApiRequestHandler.hpp"
#include "PictureCache.hpp"
#include "PicturePreviewHandler.hpp"
#include "StaticRequestHandler.hpp"
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <iostream>
#include <memory>
#include <spdlog/spdlog-inl.h>
#include <string>
#include <thread>

using boost::system::error_code;
using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;

// -----------------------------------------------------------------------
// below code for poc purpose only, remove later.
#include <boost/asio/signal_set.hpp>
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
                if((timeout > 0 and elapsed >= timeout) or
                   (snapsNeeded > 0 and static_cast<int>(snaps.size()) >= snapsNeeded))
                {
                    running = false;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::seconds{1});
                    ++elapsed;
                    const auto newSnapId = mock.generateSnap();
                    std::unique_lock<std::mutex> lock(snapsMutex);
                    snaps.emplace_back(newSnapId);
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
                    while(snaps.size() > 30)
                    {
                        snaps.pop_front();
                    }
                }
            }
        }

        nlohmann::json getStatus(bool detail)
        {
            nlohmann::json json{
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
            return json;
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
        // NOLINTNEXTLINE(clang-diagnostic-unused-private-field)
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

    nlohmann::json getStatus(size_t sampleId, bool detail)
    {
        std::unique_lock<std::mutex> lock(samplingMutex);
        return samplings.at(sampleId)->getStatus(detail);
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
        std::make_shared<JsonApiRequestHandler>([](Request& req) -> nlohmann::json {
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
        std::make_shared<JsonApiRequestHandler>([](Request& req) -> nlohmann::json {
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
        std::make_shared<JsonApiRequestHandler>([](Request& req) -> nlohmann::json {
            try
            {
                auto json = nlohmann::json::parse(req.body());
                size_t id = json.at("id");
                samplingMock.stop(id);
                return nlohmann::json{};
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
        });
    server.registHandler("^/api/v1/calibration/sampling/stop$", samplingStopHandler);

    constexpr size_t picPreviewCacheNum{30};
    auto picHandler = std::make_shared<PicturePreviewHandler>(picPreviewCacheNum);

    // GET /api/v1/realtime/preview/snap/<uuid>/[0-4]
    server.registHandler(
        "^/api/v1/realtime/preview/picture/[a-f0-9-]+/(snaps|tracks)/[0-9]$", picHandler);

    // POST /api/v1/realtime/preview/pictures
    auto pictureCache = std::make_shared<PictureCache>(picHandler);
    auto picPostHandler = std::make_shared<JsonApiRequestHandler>(
        [pictureCache](Request& req) -> nlohmann::json {
            try
            {
                PostPictureWrapper postPicture;
                auto json = nlohmann::json::parse(req.body());
                json.get_to(postPicture.picture());
                pictureCache->addPicture(std::move(postPicture));
                return nlohmann::json{};
            }
            catch(const nlohmann::json::parse_error& ex)
            {
                throw std::runtime_error(ex.what());
            }
            catch(const std::exception& err)
            {
                throw std::runtime_error(err.what());
            }
        });
    server.registHandler("^/api/v1/realtime/preview/pictures$", picPostHandler);

    auto picListHandler = std::make_shared<JsonApiRequestHandler>(
        [picHandler](Request& req) -> nlohmann::json {
            auto target = req.target().to_string();
            const std::string lookup = "?prev=";
            auto pos = target.rfind(lookup);
            std::string prev{""};
            if(pos != boost::string_view::npos)
            {
                prev = target.substr(pos + lookup.length());
            }
            constexpr size_t returnLimit{10};
            return picHandler->listPreview(prev, returnLimit);
        });
    server.registHandler("^/api/v1/realtime/preview(\\?prev=.+)?$", picListHandler);
}
} // namespace
// end poc
// -------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    using address = boost::asio::ip::address_v4;
    using boost::asio::io_context;
    using boost::asio::ip::tcp;

    constexpr uint16_t defaultPort{8088};
    size_t threads = std::thread::hardware_concurrency();

    const char* documentRootPath{"/tmp"};
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

    io_context ioContext{static_cast<int>(threads)};

    const tcp::endpoint endpoint{address::any(), listenPort};
    auto documentRoot = std::make_shared<std::string const>(documentRootPath);

    // run http server
    auto server = std::make_shared<HttpListener>(ioContext, endpoint, documentRoot);

    if(server->lastError())
    {
        exit(1);
    }
    try
    {
        // install api handler
        auto versionApiHandler =
            std::make_shared<JsonApiRequestHandler>([](Request&) -> std::string {
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
        signals.async_wait([&](error_code const&, int) {
            // Stop the `io_context`. This will cause `run()`
            // to return immediately, eventually destroying the
            // `io_context` and all of the sockets in it.
            ioContext.stop();
        });

        // run with threads
        std::vector<std::thread> runners{};
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
        spdlog::error("error: {}", ex.what());
        ioContext.stop();
    }
    return 0;
}
