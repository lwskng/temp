//
// Created by lwskng on 9/13/22.
//

#pragma once

#include <variant>
#include <utility>

#ifndef GAME_SERVER_OBJECT_HOLDER_H
#define GAME_SERVER_OBJECT_HOLDER_H


namespace types {

    template <typename Variant>
    struct ObjectType : Variant {
        using Variant::Variant;

        ObjectType(Variant other_variant) : Variant(std::move(other_variant)) {}

        template <typename T>
        bool Is() const {
            return std::holds_alternative<T>(*this);
        }

        template <typename T>
        const T &As() const {
            return std::get<T>(*this);
        }

        template <typename T>
        T &As() {
            return std::get<T>(*this);
        }

        template <typename T>
        const T *TryAs() const {
            return std::get_if<T>(this);
        }

        template <typename T>
        T *TryAs() {
            return std::get_if<T>(this);
        }

        const Variant &GetValue() const { return *this; }
        Variant &GetValue() { return *this; }
    };

}//!namespace




#endif //GAME_SERVER_OBJECT_HOLDER_H
