// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef SCOPE_GUARD_H_8971632487321434
#define SCOPE_GUARD_H_8971632487321434

#include <cassert>
#include "legacy_compiler.h" //std::uncaught_exceptions

namespace zen
{
/*  Scope Guard: best of Zen, Loki and C++20

        auto guardAio = zen::makeGuard<ScopeGuardRunMode::onExit>([&] { ::CloseHandle(hDir); });
            ...
        guardAio.dismiss();

    Scope Exit:
        ZEN_ON_SCOPE_EXIT   (CleanUp());
        ZEN_ON_SCOPE_FAIL   (UndoTemporaryWork());
        ZEN_ON_SCOPE_SUCCESS(NotifySuccess());                    */

enum class ScopeGuardRunMode
{
    onExit,
    onSuccess,
    onFail
};

template <ScopeGuardRunMode runMode, typename F>
class ScopeGuard
{
public:
    explicit ScopeGuard(const F&  fun) : fun_(fun) {}
    explicit ScopeGuard(      F&& fun) : fun_(std::move(fun)) {}

    //ScopeGuard(ScopeGuard&& tmp) :
    //    fun_(std::move(tmp.fun_)),
    //    exeptionCount_(tmp.exeptionCount_),
    //    dismissed_(tmp.dismissed_) { tmp.dismissed_ = true; }

    ~ScopeGuard() noexcept(runMode == ScopeGuardRunMode::onFail)
    {
        if (!dismissed_)
        {
            const bool failed = std::uncaught_exceptions() > exeptionCount_;

            if constexpr (runMode == ScopeGuardRunMode::onSuccess ||
                          runMode == ScopeGuardRunMode::onExit)
            {
                if (!failed)
                    fun_(); //throw X
            }

            if constexpr (runMode == ScopeGuardRunMode::onFail ||
                          runMode == ScopeGuardRunMode::onExit)
            {
                if (failed)
                    try { fun_(); }
                    catch (...) { assert(false); }
            }
        }
    }

    void dismiss() { dismissed_ = true; }

private:
    ScopeGuard           (const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    const F fun_;
    const int exeptionCount_ = std::uncaught_exceptions();
    bool dismissed_ = false;
};


template <ScopeGuardRunMode runMode, class F> inline
auto makeGuard(F&& fun) { return ScopeGuard<runMode, std::decay_t<F>>(std::forward<F>(fun)); }
}

#define ZEN_CONCAT_SUB(X, Y) X ## Y
#define ZEN_CONCAT(X, Y) ZEN_CONCAT_SUB(X, Y)

#define ZEN_CHECK_CASE_FOR_CONSTANT(X) case X: return ZEN_CHECK_CASE_FOR_CONSTANT_IMPL(#X)
#define ZEN_CHECK_CASE_FOR_CONSTANT_IMPL(X) L ## X

#define ZEN_ON_SCOPE_EXIT(X)    [[maybe_unused]] auto ZEN_CONCAT(scopeGuard, __LINE__) = zen::makeGuard<zen::ScopeGuardRunMode::onExit   >([&]{ X; });
#define ZEN_ON_SCOPE_FAIL(X)    [[maybe_unused]] auto ZEN_CONCAT(scopeGuard, __LINE__) = zen::makeGuard<zen::ScopeGuardRunMode::onFail   >([&]{ X; });
#define ZEN_ON_SCOPE_SUCCESS(X) [[maybe_unused]] auto ZEN_CONCAT(scopeGuard, __LINE__) = zen::makeGuard<zen::ScopeGuardRunMode::onSuccess>([&]{ X; });

#endif //SCOPE_GUARD_H_8971632487321434
