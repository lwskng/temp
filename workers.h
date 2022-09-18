//
// Created by lwskng on 9/11/22.
//

#pragma once

#include "json_handler.h"
#include "model.h"
#include "errors.h"
#include "object_holder.h"
#include "http_response_type.h"

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

#include <functional>
#include <variant>
#include <string>
#include <filesystem>

#ifndef GAME_SERVER_WORKERS_H
#define GAME_SERVER_WORKERS_H

namespace http_handler {
    namespace resources {

        namespace http = boost::beast::http;
        namespace fs = std::filesystem;

        using WorkerResponse = types::HttpResponse;
        using WorkerCallerId = std::string;

        class Workers final {
        public:
            Workers (model::Game& game, fs::path &&root);

            //todo: another option to organize workers, keeping that for a while
            WorkerResponse CallWorker (const std::string &worker_name, const std::string_view data) const;

            WorkerResponse SingleMap (const std::string_view data) const;
            WorkerResponse AllMaps (const std::string_view data) const;
            WorkerResponse MapNotFound (const std::string_view data) const;
            WorkerResponse BadRequest (const std::string_view data) const;
            WorkerResponse File (const std::string_view data) const;
            WorkerResponse FileNotFound (const std::string_view data) const;
            WorkerResponse ObjectNotFound (const std::string_view data) const;

        private:
            model::Game game_;
            const fs::path wwwroot_;

            struct Ok {};
            struct BadRequest_ {};
            struct NotFound {};

            template <typename Status, typename Body, typename BodyType>
            WorkerResponse makeResponse (BodyType &&body) const;

            template <typename Body>
            WorkerResponse wrapIt (http::response<Body> &&res) const;

            //todo: another option to organize workers, keeping that for a while
            static std::unordered_map<
                    WorkerCallerId,
                    WorkerResponse ((Workers::*)(const std::string_view) const)
            > CALL_WORKER;
        };

        template <typename Status, typename Body, typename BodyType>
        WorkerResponse Workers::makeResponse (BodyType &&body) const {
            http::response<Body> res;
            res.body() = std::forward<BodyType>(body);

            if constexpr (std::is_same_v<Ok, Status>) {
                res.result(http::status::ok);
            }
            else if constexpr (std::is_same_v<BadRequest_, Status>) {
                res.result(http::status::bad_request);
            }
            else if constexpr (std::is_same_v<NotFound, Status>) {
                res.result(http::status::not_found);
            }
            else {
                throw std::runtime_error ("unknown response status");
            }
            res.prepare_payload();

            return wrapIt<Body>(std::move(res));
        }


        template <typename Body>
        WorkerResponse Workers::wrapIt (http::response<Body> &&res) const {
            using namespace types::response;
            if constexpr (
                    std::disjunction_v<
                            std::is_same<StrBody, Body>,
                            std::is_same<FileBody, Body>,
                            std::is_same<EmptyBody, Body>
                    >
                    ) {
                return {std::move(res)};
            } else {
                throw std::runtime_error("unknown response type");
            }
        }


        using Worker = std::function<WorkerResponse(const Workers&, const std::string_view)>;


    }//!namespace
}//!namespace




#endif //GAME_SERVER_WORKERS_H
