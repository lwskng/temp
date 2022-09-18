#include "request_handler.h"

namespace http_handler {

    RequestHandler::RequestHandler(model::Game& game, fs::path &&root)
            : workers_(game, std::move(root)) {
        TemporaryInit();
    }


    void RequestHandler::TemporaryInit () {
        using namespace resources;
        auto bad_request = &Workers::BadRequest;
        auto map_not_found = &Workers::MapNotFound;
        auto single_map = &Workers::SingleMap;
        auto all_maps = &Workers::AllMaps;
        auto file = &Workers::File;

        RegisterResource (http::verb::get, "/api"sv, AnyQuery{}, bad_request);
        RegisterResource (http::verb::get, "/api/v1"sv, AnyQuery{}, bad_request);
        RegisterResource (http::verb::get, "/api/v1/maps"sv, Error{}, map_not_found);
        RegisterResource (http::verb::get, "/api/v1/maps"sv, Success{}, all_maps);
        RegisterResource (http::verb::get, "/api/v1/maps/map1"sv, Error{}, bad_request);
        RegisterResource (http::verb::get, "/api/v1/maps/map1"sv, Success{}, single_map);

        RegisterResource (http::verb::get, "/file"sv, AnyQuery{}, file);
    }

    resources::WorkerResponse RequestHandler::CallResource (
            http::verb verb,
            const std::string_view path,
            const std::string_view data) const {
        const auto [ok, derived_path] = uri_handler_.resolvePath(path);
        const bool is_request_to_file = (
                derived_path.has_value() &&
                derived_path.value().size() == 1u &&
                uri_handler_.isRoot(derived_path.value().back()));
        if (is_request_to_file) {
            if (const auto file_api = uri_handler_.tryGetApiEndpoint("/file"); file_api) {
                return file_api->callWorker(static_cast<int>(verb), true, path, workers_);;
            }
            else {
                return resources::WorkerResponse {}; //will be processed while decorating response
            }
        }
        return derived_path->back()->callWorker(static_cast<int>(verb), ok, data, workers_);
    }

    resources::WorkerResponse RequestHandler::PopulateResponse(resources::WorkerResponse &&res_holder,
                                                               unsigned http_version,
                                                               bool keep_alive,
                                                               std::string_view content_type) const {
        using namespace types::response;

        if (auto p_str = res_holder.template TryAs<Str>(); p_str) {
            PopulateResponseHelper(*p_str, http_version, keep_alive, content_type);
        }
        else if (auto p_file = res_holder.template TryAs<File>(); p_file) {
            PopulateResponseHelper(*p_file, http_version, keep_alive, content_type);
        }
        else if (auto p_empty = res_holder.template TryAs<Empty>(); p_empty) {
            PopulateResponseHelper(*p_empty, http_version, keep_alive, content_type);
        }
        else {
            http::response<http::string_body> res;
            res.body() = "I donno know nothin\'";
            res.result(http::status::not_found);
            res.prepare_payload();
            PopulateResponseHelper(res, http_version, keep_alive, content_type);
            resources::WorkerResponse w_res {std::move(res)};
            return w_res;
        }
        return res_holder;
    }


}  // namespace http_handler
