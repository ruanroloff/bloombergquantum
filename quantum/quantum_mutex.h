/*
** Copyright 2018 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef BLOOMBERG_QUANTUM_MUTEX_H
#define BLOOMBERG_QUANTUM_MUTEX_H

#include <atomic>
#include <quantum/quantum_traits.h>
#include <quantum/quantum_spinlock.h>
#include <quantum/interface/quantum_icontext.h>
#include <quantum/quantum_yielding_thread.h>
#include <quantum/quantum_task_id.h>

namespace Bloomberg {
namespace quantum {

//==============================================================================================
//                                      class Mutex
//==============================================================================================
/// @class Mutex.
/// @brief Coroutine-compatible implementation of a mutex.
/// @details This mutex wraps a specialized form of spinlock. The mutex must be used to protect
///          a critical region which is shared between coroutines and (optionally) other code
///          running in a non-coroutine (i.e. regular threaded) context.
class Mutex
{
public:
    /// @brief Constructor. The object is in the unlocked state.
    Mutex() = default;
    
    Mutex(const Mutex& other) = delete;
    Mutex& operator=(const Mutex& other) = delete;
    
    /// @brief Locks this mutex.
    /// @details The mutex object yields (or conditionally sleeps) the current thread for a short
    ///          period of time until locking succeeds. See YieldingThreadDuration for more details.
    /// @note Must be called in a non-coroutine context.
    /// @warning Wrongfully calling this method from a coroutine will block all coroutines running in the
    ///          same queue and thus result in noticeable performance degradation.
    void lock();
    
    /// @brief Locks this mutex.
    /// @details The mutex object yields the current coroutine until locking succeeds.
    /// @param[in] sync Pointer to a coroutine synchronization object.
    /// @note Must be called from a coroutine.
    void lock(ICoroSync::Ptr sync);
    
    /// @brief Tries to lock the mutex object.
    /// @return True if succeeds, false otherwise.
    bool tryLock();
    
    /// @brief Unlock this mutex.
    void unlock();
    
    /// @brief Indicates if this mutex is locked or not
    /// @return True if locked.
    bool isLocked() const;
    
    //==============================================================================================
    //                                      class Mutex::Guard
    //==============================================================================================
    /// @class Mutex::Guard
    /// @brief RAII-style mechanism for mutex ownership.
    ///        Acquires a mutex on construction and releases it inside the destructor.
    class Guard
    {
    public:
        /// @brief Construct this object and lock the passed-in mutex.
        /// @param[in] mutex Mutex which protects a scope during the lifetime of the Guard.
        /// @param[in] tryToLock If supplied, tries to lock the mutex instead of unconditionally locking it.
        /// @param[in] adoptLock If supplied, assumes the current 'locked' state of the lock.
        /// @param[in] deferLock If supplied, assumes the lock is 'unlocked' and does not lock it.
        explicit Guard(Mutex& mutex);
        Guard(Mutex& mutex,
              LockTraits::TryToLock tryLock);
        Guard(Mutex& mutex,
              LockTraits::AdoptLock adoptLock);
        Guard(Mutex& mutex,
              LockTraits::DeferLock deferLock);
    
        /// @brief Construct this object and lock the passed-in mutex. Same as above but using a coroutine
        ///        synchronization context.
        Guard(ICoroSync::Ptr sync,
              Mutex& mutex);
        
        /// @brief Destructor. This will unlock the underlying mutex if it has ownership.
        ~Guard();
        
        /// @brief see Mutex::lock()
        void lock();
        void lock(ICoroSync::Ptr sync);
        
        /// @brief see Mutex::tryLock()
        bool tryLock();
        
        /// @brief Unlocks the underlying mutex if it has ownership.
        void unlock();
        
        /// @brief Releases the associated mutex without unlocking it.
        void release();
        
        /// @brief Determines if this object owns the underlying mutex.
        /// @return True if mutex is locked, false otherwise.
        bool ownsLock() const;
        
    private:
        //Members
        Mutex*          _mutex{nullptr};
        bool            _ownsLock{false};
    };
    
    //==============================================================================================
    //                                      class Mutex::ReverseGuard
    //==============================================================================================
    /// @class Mutex::ReverseGuard
    /// @brief Opposite form of RAII-style mechanism for mutex ownership.
    ///        Releases a mutex on construction and acquires it inside the destructor.
    class ReverseGuard
    {
    public:
        /// @brief Construct this object and unlock the passed-in mutex.
        /// @param[in] mutex Mutex which remains unlocked during the lifetime of this object.
        /// @note This constructor must be used in a non-coroutine context.
        /// @warning Wrongfully calling this method from a coroutine will block all coroutines running in the
        ///          same queue when this object is destroyed and thus result in noticeable performance degradation.
        explicit ReverseGuard(Mutex& mutex);
    
        /// @brief Construct this object and unlock the passed-in mutex.
        /// @param[in] sync Pointer to a coroutine synchronization object.
        /// @param[in] mutex Mutex which remains unlocked during the lifetime of this object.
        /// @note This constructor must be used in a coroutine context.
        ReverseGuard(ICoroSync::Ptr sync,
                     Mutex& mutex);
        
        /// @brief Destroys this object and locks the underlying mutex.
        ~ReverseGuard();
        
    private:
        //Members
        Mutex*              _mutex;
        ICoroSync::Ptr      _sync;
    };
    
private:
    //Members
    mutable SpinLock  _spinlock;
    mutable TaskId    _taskId;
};

}}

#include <quantum/impl/quantum_mutex_impl.h>

#endif //BLOOMBERG_QUANTUM_MUTEX_H
