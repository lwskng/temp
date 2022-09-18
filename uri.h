//
// Created by lwskng on 9/6/22.
//

#pragma once

#include "utils.h"
#include "workers.h"

#include <memory>
#include <map>
#include <unordered_map>
#include <queue>
#include <list>
#include <optional>
#include <functional>

#ifndef GAME_SERVER_URI_H
#define GAME_SERVER_URI_H

namespace http_handler {
    namespace api {

        using namespace std::string_view_literals;

        namespace const_values {
            static const char URI_DELIM = '/';
            static const size_t EXPECTED_METHODS_COUNT {10u};
            static const std::string_view ROOT_ENDPOINT_NAME {"root"sv};
        }

        struct Endpoint;
        using EndpointPtr = std::shared_ptr<Endpoint>;

        using http_handler::resources::WorkerResponse;
        using http_handler::resources::Worker;
        using Workers = http_handler::resources::Workers;
        using WorkerId = std::pair<int, bool>;
        struct WorkerIdHasher {
            size_t operator () (const WorkerId &worker_id) const {
                return
                        i_hash(worker_id.first) * i_hash(worker_id.first) +
                        b_hash(worker_id.second);
            }
            std::hash<bool> b_hash;
            std::hash<int> i_hash;
        };
        using WorkerMapping = std::unordered_map<WorkerId, Worker, WorkerIdHasher>;

        //todo: make it a class, restrict ctors
        struct Endpoint final : public std::enable_shared_from_this<Endpoint> {
            using InsertResult = std::pair<bool, EndpointPtr>;
            std::string name_;
            EndpointPtr parent_;
            std::map<std::string, EndpointPtr> children_;
            WorkerMapping workers_mapping_;

            static EndpointPtr makePtr(const std::string_view name);
            InsertResult addChild (const std::string_view name);
            InsertResult addChild (EndpointPtr p_child);
            bool registerWorker (
                    const int verb,
                    const bool is_correct_call,
                    Worker worker);
            WorkerResponse callWorker(
                    const int verb,
                    const bool is_correct_call,
                    const std::string_view data,
                    const Workers &workers);
        };

        class Path final {
        public:
            using Type = std::list<EndpointPtr>;
            using ResolutionResult = std::pair<bool, std::optional<Path::Type>>;

            explicit Path (Type&& p);
            std::optional<std::string> toString () const;
            static ResolutionResult makePath(const std::string_view full_path, EndpointPtr p_top_node);
            static ResolutionResult derivePath (EndpointPtr p_node);
            bool empty() const;
            size_t size() const;
        private:
            Type chain_;
        };

        class Tree final {
        public:
            Tree ();
            Endpoint::InsertResult addApiEndpoint (const std::string_view full_path);
            bool deleteApiEndpoint (); //todo: requires to be completed
            EndpointPtr tryGetApiEndpoint (const std::string_view full_path) const;
            Path::ResolutionResult resolvePath (const std::string_view full_path) const;
            std::vector<EndpointPtr> findApiEndpoints (const std::string_view name) const;
            std::optional<std::string> getPath (EndpointPtr ptr) const;
            const size_t getCount() const; //todo: requires to be completed
            bool isRoot (EndpointPtr endpoint) const;
        private:
            EndpointPtr root_;
            size_t nodes_count_{0};

            std::vector<EndpointPtr> traverse (EndpointPtr top_node, const std::string_view name_to_look) const;
        };
    }//!namespace
}//!namespace

#endif //GAME_SERVER_URI_H
