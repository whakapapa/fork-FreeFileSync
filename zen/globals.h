// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef GLOBALS_H_8013740213748021573485
#define GLOBALS_H_8013740213748021573485

#include <atomic>
#include <memory>
#include "scope_guard.h"

namespace zen
{
/*  Solve static destruction order fiasco by providing shared ownership and serialized access to global variables

    => e.g. accesses to "Global<T>::get()" during process shutdown: _("") used by message in debug_minidump.cpp or by some detached thread assembling an error message!
    => use POD member variables only that (technically) can be accessed even *after* Global<> destruction!

    ATTENTION: *never* place static Global<> in function scope! This will have the compiler generate "magic statics" due to non-trivial ~Global()!
               => "magic" locking code will crash or leak memory when accessed after global shutdown */

#define GLOBAL_RUN_ONCE(X)                               \
    struct ZEN_CONCAT(GlobalInitializer, __LINE__)       \
    {                                                    \
        ZEN_CONCAT(GlobalInitializer, __LINE__)() { X; } \
    } ZEN_CONCAT(globalInitializer, __LINE__)


class PodMutex
{
public:
    bool tryLock() { return !flag_.test_and_set(std::memory_order_acquire); }

    void lock()
    {
        while (!tryLock())
            flag_.wait(true, std::memory_order_relaxed);
    }

    void unlock()
    {
        flag_.clear(std::memory_order_release);
        flag_.notify_one();
    }
private:
    std::atomic_flag flag_{}; /* => avoid potential contention with worker thread during Global<> construction!
    - "For an atomic_flag with static storage duration, this guarantees static initialization:" => just what the doctor ordered!
    - "[default initialization] initializes std::atomic_flag to clear state" - since C++20      =>
    - "std::atomic_flag is [...] guaranteed to be lock-free"
    - interestingly, is_trivially_constructible_v<> is false, thanks to constexpr! https://developercommunity.visualstudio.com/content/problem/416343/stdatomic-no-longer-is-trivially-constructible.html    */
};


template <class T>
class PodSharedPtr
{
public:
    void construct(std::unique_ptr<T>&& newInst)
    {
        assert(!alive_);
        new (rawMem_) std::shared_ptr<T>(std::move(newInst));
        alive_ = true;
    }
    void destruct()
    {
        assert(alive_);
        ref().~shared_ptr();
        alive_ = false;
    }

    bool isAlive() const { return alive_; }

    std::shared_ptr<T>& ref() { assert(alive_); return *reinterpret_cast<std::shared_ptr<T>*>(rawMem_); }

private:
    alignas(std::shared_ptr<T>) std::byte rawMem_[sizeof(std::shared_ptr<T>)] = {};
    bool alive_ = false; //placing *after* rawMem_ avoids: "warning C4324: 'PodSharedPtr<T>': structure was padded due to alignment specifier"
};


template <class T>
class Global //don't use for function-scope statics!
{
public:
    consteval Global() {}; //demand static initialization!

    ~Global()
    {
        spinLock_.lock();
        globalShutdown_ = true;
        spinLock_.unlock();

        if (ptr_.isAlive())  //careful: no ptr_ access after "globalShutdown_"! otherwise: race-condition!
            ptr_.destruct(); //
    }

    std::shared_ptr<T> get() //=> return std::shared_ptr to let instance life time be handled by caller (MT usage!)
    {
        spinLock_.lock();
        ZEN_ON_SCOPE_EXIT(spinLock_.unlock());

        if (!globalShutdown_ && ptr_.isAlive())
            return ptr_.ref();
        return nullptr;
    }

    void set(std::unique_ptr<T>&& newInst)
    {
        spinLock_.lock();
        ZEN_ON_SCOPE_EXIT(spinLock_.unlock());

        assert(!globalShutdown_);
        if (!globalShutdown_)
        {
            if (ptr_.isAlive())
                ptr_.ref() = std::move(newInst);
            else
                ptr_.construct(std::move(newInst));
        }
    }

    //for initialization via a frequently-called function (which may be running on parallel threads)
    template <class Function>
    void setOnce(Function&& getInitialValue /*-> std::unique_ptr<T>*/)
    {
        spinLock_.lock();
        ZEN_ON_SCOPE_EXIT(spinLock_.unlock());

        assert(!globalShutdown_);
        if (!globalShutdown_ && !ptr_.isAlive())
            ptr_.construct(getInitialValue()); //throw ?
    }

private:
    static_assert(std::is_trivially_destructible_v<PodMutex> && //can't use std::mutex: has non-trival destructor
                  std::is_trivially_destructible_v<PodSharedPtr<T>>, "this memory needs to live forever");

    PodMutex spinLock_; //rely entirely on static zero-initialization! => avoid potential contention with worker thread during Global<> construction!
    bool globalShutdown_ = false;
    PodSharedPtr<T> ptr_;
};
}

#endif //GLOBALS_H_8013740213748021573485
