// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef LEGACY_COMPILER_H_839567308565656789
#define LEGACY_COMPILER_H_839567308565656789

#include <version> //contains all __cpp_lib_<feature> macros
#include <string>

/*  C++ standard conformance:
    https://en.cppreference.com/w/cpp/feature_test
    https://en.cppreference.com/w/User:D41D8CD98F/feature_testing_macros
    https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations

    MSVC https://docs.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance

    GCC       https://gcc.gnu.org/projects/cxx-status.html
    libstdc++ https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html

    Clang  https://clang.llvm.org/cxx_status.html
    Xcode  https://developer.apple.com/xcode/cpp
    libc++ https://libcxx.llvm.org/cxx2a_status.html                                     */

namespace std
{



//W(hy)TF is this not standard? https://stackoverflow.com/a/47735624
template <class Char, class Traits, class Alloc> inline
basic_string<Char, Traits, Alloc> operator+(basic_string<Char, Traits, Alloc>&& lhs, const basic_string_view<Char> rhs)
{ return std::move(lhs.append(rhs.begin(), rhs.end())); } //the move *is* needed!!!

//template <class Char> inline
//basic_string<Char> operator+(const basic_string<Char>& lhs, const basic_string_view<Char>& rhs) { return basic_string<Char>(lhs) + rhs; }
//-> somewhat inefficient: better implement using only a single memory allocation!!!
}
//---------------------------------------------------------------------------------

namespace zen
{
double fromChars(const char* first, const char* last);
char* toChars(char* first, char* last, double num);
}

#endif //LEGACY_COMPILER_H_839567308565656789
