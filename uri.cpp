//
// Created by lwskng on 9/6/22.
//

#include "uri.h"

#include <stdexcept>

namespace http_handler {
    namespace api {

        EndpointPtr Endpoint::makePtr(const std::string_view name) {
            EndpointPtr p_node = std::make_shared<Endpoint>();
            p_node->name_ = std::string(name);
            return p_node;
        }

        Endpoint::InsertResult Endpoint::addChild (const std::string_view name) {
            auto str_name = std::string(name);
            if (auto found = children_.find(str_name); found != children_.end()) {
                return {false, found->second};
            }
            else {
                auto inserted_ptr = makePtr(str_name);
                inserted_ptr->parent_ = this->shared_from_this();
                this->children_[str_name] = std::move(inserted_ptr);
                return {true, children_[str_name]->shared_from_this()};
            }
        }

        //todo: not tested
        Endpoint::InsertResult Endpoint::addChild (EndpointPtr p_child) {
            if (auto found = children_.find(p_child->name_); found != children_.end()) {
                if (found->second == p_child) return {false, found->second};
                else throw std::runtime_error ("URI naming conflict");
            }
            else {
                children_.insert({p_child->name_, p_child->shared_from_this()});
                return {true, p_child};
            }
        }

        bool Endpoint::registerWorker (
                const int verb,
                const bool is_correct_call,
                Worker worker) {
            size_t bc = workers_mapping_.bucket_count();
            size_t elem_count = workers_mapping_.size();
            if (bc <= elem_count) workers_mapping_.reserve(bc * 2);

            WorkerId worker_id {verb, is_correct_call};
            auto [_, ok] = workers_mapping_.insert({worker_id, worker});
            return ok;
        }

        WorkerResponse Endpoint::callWorker(
                const int verb,
                const bool is_correct_call,
                const std::string_view data,
                const Workers &workers) {
            WorkerId worker_id {verb, is_correct_call};
            if (auto found = workers_mapping_.find(worker_id); found != workers_mapping_.end()) {
                return found->second(workers, data);
            } else {
                return WorkerResponse{};
            }
        }

        Path::Path (Type&& p)
                : chain_(std::move(p))
        {}

        std::optional<std::string> Path::toString () const {
            if (empty()) return std::nullopt;
            std::vector<std::string> names (size());
            int i = 0;
            for (auto curr = std::begin(chain_), end = std::end(chain_);
                 curr != end;
                 curr = std::next(curr)) {
                if (*curr) names[i++] = (*curr)->name_;
                else return std::nullopt;
            }
            std::string result = utils::concatFromVector(names, const_values::URI_DELIM, true);
            return {result};
        }

        Path::ResolutionResult Path::makePath(const std::string_view full_path, EndpointPtr p_top_node) {
            auto names = utils::splitIntoWords(full_path, const_values::URI_DELIM);
            if (names.empty()) return {false, std::nullopt};
            if (not p_top_node) return {false, std::nullopt};

            Type result;
            result.emplace_front(p_top_node);

            for (auto curr = std::begin(names),
                         end = std::end(names);
                 curr != end;
                 curr = std::next(curr)) {
                if (auto child = result.back()->children_.find(std::string(*curr));
                        child != result.back()->children_.end()) {
                    result.emplace_back(child->second);
                }
                else {
                    return {false, result};
                }
            }
            return {true, result};
        }

        Path::ResolutionResult Path::derivePath (EndpointPtr p_node) {
            Path::Type result;
            if (!p_node) return {false, std::nullopt};
            result.emplace_front(p_node);
            auto curr = p_node->parent_;
            while (curr) {
                result.emplace_front(curr);
                curr = curr->parent_;
            }
            return {true, result};
        }

        bool Path::empty() const {
            return chain_.empty();
        }

        size_t Path::size() const {
            return chain_.size();
        }

        Tree::Tree () {
            root_ = std::move(Endpoint::makePtr(const_values::ROOT_ENDPOINT_NAME));
        }

        Endpoint::InsertResult Tree::addApiEndpoint (const std::string_view full_path) {
            const auto names = utils::splitIntoWords(full_path, const_values::URI_DELIM);
            if (names.empty() ||
                std::begin(names)->empty() ||
                not root_)
                return {false, nullptr};

            auto ptr = root_;
            for (auto curr = std::begin(names), end = std::end(names);
                 curr != end;
                 curr = std::next(curr)) {
                if (not curr->empty() && *curr != const_values::ROOT_ENDPOINT_NAME) {
                    auto [ok, inserted] = ptr->addChild(*curr);
                    ptr = inserted; //inserted is a child with curr->name
                    if (ok) ++nodes_count_;
                }
                else return {false, ptr};
            }
            return {true, ptr};
        }

        bool Tree::deleteApiEndpoint () {
            --nodes_count_;
            return true;
        }

        EndpointPtr Tree::tryGetApiEndpoint (const std::string_view full_path) const {
            auto [ok, derived_path] = Path::makePath(full_path, root_);
            return ok ? derived_path->back() : nullptr;
        }

        std::vector<EndpointPtr> Tree::findApiEndpoints (const std::string_view name) const {
            return traverse(root_, name);
        }

        Path::ResolutionResult Tree::resolvePath (const std::string_view full_path) const {
            return Path::makePath(full_path, root_);
        }

        std::optional<std::string> Tree::getPath (EndpointPtr ptr) const {
            if (auto [ok, p] = Path::derivePath(ptr); p) {
                Path path (std::move(*p));
                return path.toString();
            } else {
                return std::nullopt;
            }
        }

        const size_t Tree::getCount() const {
            return nodes_count_;
        }

        bool Tree::isRoot (EndpointPtr endpoint) const {
            return endpoint == root_;
        }

        std::vector<EndpointPtr> Tree::traverse (EndpointPtr top_node, const std::string_view name_to_look) const {
            std::vector<EndpointPtr> found_nodes;
            std::queue<EndpointPtr> q;
            q.push(top_node);
            while (!q.empty()) {
                auto curr = q.front();
                if (name_to_look == curr->name_) found_nodes.emplace_back(curr);
                q.pop();
                for (const auto &[name, p_child]  : curr->children_){
                    q.push(p_child);
                }
            }
            return found_nodes;
        }

    }//!namespace
}//!namespace