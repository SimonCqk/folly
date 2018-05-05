/*
 * Copyright 2014-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/Portability.h>
#include <folly/Try.h>
#include <functional>
#include <folly/futures/FutureException.h>

namespace folly {

// forward declaration
template <class T>
class SemiFuture;
template <class T> class Future;

namespace futures {
namespace detail {
template <class T>
class FutureBase;
struct EmptyConstruct {};
template <typename T, typename F>
class CoreCallbackState;
} // namespace detail
} // namespace futures

template <class T>
class Promise {
 public:
  static Promise<T> makeEmpty() noexcept; // equivalent to moved-from

  Promise();
  ~Promise();

  // not copyable
  Promise(Promise const&) = delete;
  Promise& operator=(Promise const&) = delete;

  // movable
  Promise(Promise<T>&&) noexcept;
  Promise& operator=(Promise<T>&&) noexcept;

  /** Return a SemiFuture tied to the shared core state. This can be called only
    once, thereafter FutureAlreadyRetrieved exception will be raised. */
  SemiFuture<T> getSemiFuture();

  /** Return a Future tied to the shared core state. This can be called only
    once, thereafter FutureAlreadyRetrieved exception will be raised.
    NOTE: This function is deprecated. Please use getSemiFuture and pass the
          appropriate executor to .via on the returned SemiFuture to get a
          valid Future where necessary. */
  Future<T> getFuture();

  /** Fulfill the Promise with an exception_wrapper */
  void setException(exception_wrapper ew);

  /** Fulfill the Promise with an exception_ptr, e.g.
    try {
      ...
    } catch (...) {
      p.setException(std::current_exception());
    }
    */
  [[deprecated("use setException(exception_wrapper)")]]
  void setException(std::exception_ptr const&);

  /** Fulfill the Promise with an exception type E, which can be passed to
    std::make_exception_ptr(). Useful for originating exceptions. If you
    caught an exception the exception_wrapper form is more appropriate.
    */
  template <class E>
  typename std::enable_if<std::is_base_of<std::exception, E>::value>::type
  setException(E const&);

  /// Set an interrupt handler to handle interrupts. See the documentation for
  /// Future::raise(). Your handler can do whatever it wants, but if you
  /// bother to set one then you probably will want to fulfill the promise with
  /// an exception (or special value) indicating how the interrupt was
  /// handled.
  void setInterruptHandler(std::function<void(exception_wrapper const&)>);

  /// Sugar to fulfill this Promise<Unit>
  template <class B = T>
  typename std::enable_if<std::is_same<Unit, B>::value, void>::type
  setValue() {
    setTry(Try<T>(T()));
  }

  /** Set the value (use perfect forwarding for both move and copy) */
  template <class M>
  void setValue(M&& value);

  void setTry(Try<T>&& t);

  /** Fulfill this Promise with the result of a function that takes no
    arguments and returns something implicitly convertible to T.
    Captures exceptions. e.g.

    p.setWith([] { do something that may throw; return a T; });
  */
  template <class F>
  void setWith(F&& func);

  bool isFulfilled() const noexcept;

 private:
  template <class>
  friend class futures::detail::FutureBase;
  template <class>
  friend class SemiFuture;
  template <class>
  friend class Future;
  template <class, class>
  friend class futures::detail::CoreCallbackState;

  // Whether the Future has been retrieved (a one-time operation).
  bool retrieved_;

  using CoreType = typename Future<T>::CoreType;
  using corePtr = typename Future<T>::corePtr;

  // Throws NoState if there is no shared state object; else returns it by ref.
  //
  // Implementation methods should usually use this instead of `this->core_`.
  // The latter should be used only when you need the possibly-null pointer.
  CoreType& getCore() {
    return getCoreImpl(core_);
  }
  CoreType const& getCore() const {
    return getCoreImpl(core_);
  }

  template <typename CoreT>
  static CoreT& getCoreImpl(CoreT* core) {
    if (!core) {
      throwNoState();
    }
    return *core;
  }

  // shared core state object
  // usually you should use `getCore()` instead of directly accessing `core_`.
  corePtr core_;

  explicit Promise(futures::detail::EmptyConstruct) noexcept;

  void throwIfFulfilled() const;
  void detach();
};

} // namespace folly

#include <folly/futures/Future.h>
#include <folly/futures/Promise-inl.h>
