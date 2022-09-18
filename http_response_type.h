//
// Created by lwskng on 9/13/22.
//

#pragma once

#include "object_holder.h"

#include <variant>
#include <boost/beast/http.hpp>

#ifndef GAME_SERVER_HTTP_RESPONSE_TYPE_H
#define GAME_SERVER_HTTP_RESPONSE_TYPE_H

namespace types {

    namespace response {
        namespace http = boost::beast::http;

        using StrBody = http::string_body;
        using FileBody = http::file_body;
        using EmptyBody = http::empty_body;

        using StrBodyType = http::string_body::value_type;
        using FileBodyType = http::file_body::value_type;
        using EmptyBodyType = http::empty_body::value_type;

        using None = std::monostate;
        using Str = http::response<StrBody>;
        using File = http::response<FileBody>;
        using Empty = http::response<EmptyBody>;

        using Type = std::variant<
                None,
                Str,
                File,
                Empty
        >;

    } // namespace response

    struct HttpResponse final : public types::ObjectType<response::Type> {};

    }//!namespace


#endif //GAME_SERVER_HTTP_RESPONSE_TYPE_H
