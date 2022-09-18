#pragma once

#include "http_server.h"
#include "model.h"
#include "uri.h"

#include <vector>
#include <filesystem>

namespace http_handler {

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;
    using namespace std::string_view_literals;
    using namespace std::string_literals;

    using resources::WorkerResponse;

    class RequestHandler {
    private:

        struct Success {};
        struct Error {};
        struct AnyQuery {};

        template <typename Result>
        std::vector<bool> GetResourceCallOptions (Result);

        struct ContentType {
            ContentType() = delete;
            constexpr static std::string_view TEXT_HTML = "text/html"sv;
            constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
            // При необходимости внутрь ContentType можно добавить и другие типы контента
        };

        const std::string ALLOWED_METHODS {"GET,HEAD"};

    public:
        explicit RequestHandler(model::Game& game, fs::path &&root);
        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

        template <typename Result>
        bool RegisterResource (const http::verb verb,
                               const std::string_view path,
                               const Result path_resolution_result,
                               resources::Worker worker);
    private:
        resources::Workers workers_;
        api::Tree uri_handler_;

        //todo: transform into reusable solution,
        // move Resource Initialization to AddMap or After as a separate Initialization procedure
        void TemporaryInit ();

        auto HandleRequest(auto&& req);

        resources::WorkerResponse CallResource (http::verb verb,
                const std::string_view path,
                const std::string_view data) const;


        resources::WorkerResponse PopulateResponse(resources::WorkerResponse &&res_holder,
                                                   unsigned http_version,
                                                   bool keep_alive,
                                                   std::string_view content_type = ContentType::APPLICATION_JSON) const;

        template <typename Body>
        void PopulateResponseHelper (http::response<Body> &res,
                 unsigned http_version,
                 bool keep_alive,
                 std::string_view content_type = ContentType::APPLICATION_JSON) const;

    };//!class

    template <typename Body, typename Allocator, typename Send>
    void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto response = HandleRequest(std::move(req));
        send(std::move(response)); //todo: initially there was no "move"
    }

    template <typename Result>
    std::vector<bool> RequestHandler::GetResourceCallOptions (Result) {
        if constexpr (std::is_same_v<decltype(std::decay_t<Result>()), Success>) {
            return {true};
        }
        else if constexpr (std::is_same_v<decltype(std::decay_t<Result>()), Error>){
            return {false};
        }
        else if constexpr (std::is_same_v<decltype(std::decay_t<Result>()), AnyQuery>){
            return {true, false};
        }
        else throw std::invalid_argument ("Can't decompose path resolution result");
    }

    template <typename Result>
    bool RequestHandler::RegisterResource (
            const http::verb verb,
            const std::string_view path,
            const Result path_resolution_result,
            resources::Worker worker) {
        bool success = true;
        const auto resource_call_options = GetResourceCallOptions(path_resolution_result);
        const auto [path_resolved_ok, endpoint] = uri_handler_.addApiEndpoint(path);
        if (not endpoint || not path_resolved_ok) return not success; //not endpoint - extra check
        for (const auto call_option : resource_call_options) {
            bool inserted = endpoint->registerWorker(
                    static_cast<int>(verb),
                    call_option,
                    worker);
            success = success && inserted;
        }
        return success;
    }

    auto RequestHandler::HandleRequest(auto&& req) {
        const std::string path = {req.target().begin(), req.target().end()};
        auto res_holder = CallResource(req.method(), path, ""s);
        res_holder = PopulateResponse(std::move(res_holder), req.version(), req.keep_alive());
        return res_holder;
    }

    template <typename Body>
    void RequestHandler::PopulateResponseHelper (http::response<Body> &res,
                             unsigned http_version,
                             bool keep_alive,
                             std::string_view content_type) const {
        res.version(http_version);
        res.set(http::field::content_type, content_type);
        res.set(http::field::allow, ALLOWED_METHODS);
        res.keep_alive(keep_alive);
    }


} //!namespace
