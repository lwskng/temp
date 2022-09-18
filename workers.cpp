//
// Created by lwskng on 9/11/22.
//

#include "workers.h"
#include "utils.h"

#include <iostream>

namespace http_handler {
    namespace resources {

        using namespace std::string_literals;
        using namespace types::response;

        namespace beast = boost::beast;
        namespace sys = boost::system;
        namespace http = boost::beast::http;
        namespace json = boost::json;
        namespace errors = http_handler::errors;

        Workers::Workers (model::Game& game, fs::path &&root)
                : game_(game)
                , wwwroot_ (std::move(root))
        {}

        WorkerResponse Workers::SingleMap (const std::string_view data) const {
            //todo: should be redessigned once Boost 1.80 and Boost URL are in place
//            model::Map::Id id(data.data());
//            auto found = game_.FindMap(id);
//            if (not found) throw std::runtime_error("mismatch between uri resources and model data");

            model::Map::Id id("map1"s);
            auto found = game_.FindMap(id);
            json::object json_map = json_handler::MakeJson(*found);

            return makeResponse<Ok, StrBody, StrBodyType>(serialize(json_map));
        }

        WorkerResponse Workers::AllMaps ([[maybe_unused]] const std::string_view data) const {
            const auto json_maps = json_handler::MakeJson(game_.GetMaps());
            return makeResponse<Ok, StrBody, StrBodyType>(serialize(json_maps));
        }

        WorkerResponse Workers::MapNotFound ([[maybe_unused]] const std::string_view data) const {
            return makeResponse<NotFound, StrBody, StrBodyType>(serialize(errors::MAP_NOT_FOUND)); //todo: stupid work
        }

        WorkerResponse Workers::BadRequest ([[maybe_unused]] const std::string_view data) const {
            return makeResponse<BadRequest_, StrBody, StrBodyType>(serialize(errors::BAD_REQUEST)); //todo: stupid work
        }

        WorkerResponse Workers::File (const std::string_view data) const {
            if (data.empty()) return WorkerResponse{};
            fs::path requested_file_path = wwwroot_;
            requested_file_path += fs::path(utils::decodeFromURL(data));
            fs::path path_to_check =  fs::weakly_canonical(wwwroot_ / requested_file_path);
            std::cerr << wwwroot_ << '\n' << requested_file_path  << '\n' << path_to_check << '\n';

            if (not utils::IsSubPath(path_to_check, wwwroot_)) {
                return FileNotFound(data);
            }
            http::file_body::value_type file;
            if (sys::error_code ec; file.open(requested_file_path.c_str(), beast::file_mode::read, ec), ec) {
                return FileNotFound(data);
            }
            else {
                return makeResponse<Ok, FileBody, FileBodyType>(std::move(file));
            }
        }

        WorkerResponse Workers::FileNotFound ([[maybe_unused]] const std::string_view data) const {
            return makeResponse<NotFound, StrBody, StrBodyType>(serialize(errors::FILE_NOT_FOUND)); //todo: stupid work
        }

        WorkerResponse Workers::ObjectNotFound ([[maybe_unused]] const std::string_view data) const {
            return makeResponse<NotFound, StrBody, StrBodyType>(serialize(errors::OBJECT_NOT_FOUND)); //todo: stupid work
        }

        std::unordered_map<
                WorkerCallerId,
                WorkerResponse ((Workers::*)(const std::string_view data) const)
        > Workers::CALL_WORKER = {
                {"Object Not Found"s, &Workers::ObjectNotFound},
        };

        WorkerResponse Workers::CallWorker (const std::string &worker_name, const std::string_view data) const {
            if (auto found = CALL_WORKER.find(worker_name); found != CALL_WORKER.end()) {
                (this->*CALL_WORKER.at(worker_name))(data);
            }
            else {
                return (this->*CALL_WORKER.at("Object Not Found"s))(data);
            }
        }


    }//!namespace
}//!namespace