#pragma once
#include <type_traits>
#include "piaabo/dutils.h"

template <typename T>
inline constexpr bool type_is_class_or_struct = std::is_class<T>::value;

template <typename T>
inline constexpr bool type_has_copy_constructor = std::is_copy_constructible<T>::value;

template <typename T>
inline constexpr bool type_has_move_constructor = std::is_move_constructible<T>::value;

template <typename T>
inline constexpr bool type_has_copy_assignment = std::is_copy_assignable<T>::value;

template <typename T>
inline constexpr bool type_has_move_assignment = std::is_move_assignable<T>::value;

template <typename T>
inline constexpr bool type_has_virtual_destructor = std::has_virtual_destructor<T>::value;

#define ENFORCE_IS_OBJECT(T) \
    static_assert(type_is_class_or_struct<T>, "[cuwacunu::enforce_architecture] Architecture can only be enforced in classes or structs.");

#define ENFORCE_COPY_CONSTRUCTOR(T) \
    static_assert(type_has_copy_constructor<T>, "[cuwacunu::enforce_architecture] Class or Struct is missing a copy constructor declaration.");

#define ENFORCE_MOVE_CONSTRUCTOR(T) \
    static_assert(type_has_move_constructor<T>, "[cuwacunu::enforce_architecture] Class or Struct is missing a move constructor declaration.");

#define ENFORCE_COPY_ASSIGNMENT(T) \
    static_assert(type_has_copy_assignment<T>, "[cuwacunu::enforce_architecture] Class or Struct is missing a copy assignment operator.");

#define ENFORCE_MOVE_ASSIGNMENT(T) \
    static_assert(type_has_move_assignment<T>, "[cuwacunu::enforce_architecture] Class or Struct is missing a move assignment operator.");

#define ENFORCE_VIRTUAL_DESTRUCTOR(T) \
    static_assert(type_has_virtual_destructor<T>, "[cuwacunu::enforce_architecture] Class or Struct is missing a virtual destructor.");

#define ENFORCE_ARCHITECTURE_DESIGN(T) \
    ENFORCE_IS_OBJECT(T) \
    ENFORCE_COPY_CONSTRUCTOR(T) \
    ENFORCE_MOVE_CONSTRUCTOR(T) \
    ENFORCE_COPY_ASSIGNMENT(T) \
    ENFORCE_MOVE_ASSIGNMENT(T)
