/*
 * delegate.h
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#ifndef UTILITY_DELEGATE_H_
#define UTILITY_DELEGATE_H_

// #include <string.h>
#include <utility>

/**
 * Simple storage of a callable object for free and member functions.
 *
 * Design intent is to do part of what std::function do but
 * without heap allocation, virtual function call and with minimal
 * memory footprint. (2 pointers).
 * Use case is embedded systems where previously raw function pointers
 * where used, using a void* pointer to point out a particular structure/object.
 *
 * The price is less generality (No functors) and a bit of type fiddling
 * with void pointers.
 *
 * Helper functions are provided to handle the following cases:
 * - Call a member function on a specific object.
 * - Call a free function of signature  R(*)(T*, Args...); (T is some arbitrary
 * type).
 * - Call a free function of signature void(*)(void*, Args...); (function can be
 * a function pointer)
 *
 * In the code storing the callback, construct an object of type 'delegate'.
 * This is a nullable type which can be copied freely, containing 2 pointers.
 *
 * User code assigns this object using 'assign' member functions.
 *
 * Call the delegate by regular operator(). E.g.
 * delegate cb = ...; // Set up some callback target.
 * ...
 * int val = 0;
 * cb(val);
 * For free functions, the supplied void pointer get the value from the call
 * setup.
 */
template <typename T>
class delegate;

/**
 * Class for storing the callable object
 * Stores a pointer to an adapter function and a void* pointer to the
 * Object.
 *
 * @param R type of the return value from calling the callback.
 * @param Args Argument list to the function when calling the callback.
 */
template <typename R, typename... Args>
class delegate<R(Args...)>
{
    // Adaptor function for the case where void* is not forwarded
    // to the caller. (Just a normal function pointer.)
    template <R(freeFkn)(Args...)>
    inline static R doFreeCB(void* v, Args... args)
    {
        return freeFkn(args...);
    }

    // Adapter function for the member + object calling.
    template <class T, R (T::*memFkn)(Args...)>
    inline static R doMemberCB(void* o, Args... args)
    {
        T* obj = static_cast<T*>(o);
        return (((*obj).*(memFkn))(args...));
    }

    // Adapter function for the free function with extra first arg
    // in the called function, set at callback construction.
    template <class Tptr, R(freeFknWithPtr)(Tptr, Args...)>
    inline static R doFreeCBWithPtr(void* o, Args... args)
    {
        Tptr obj = static_cast<Tptr>(o);
        return freeFknWithPtr(obj, args...);
    }

    // Adapter function for when the stored object is a pointer to a
    // callable object (stored elsewhere). Call it using operator().
    template <class Functor>
    inline static R doFunctor(void* o, Args... args)
    {
        auto obj = static_cast<Functor*>(o);
        return (*obj)(args...);
    }

    // Type of the function pointer for the trampoline functions.
    using Trampoline = R (*)(void*, Args...);

    // Signature presented to the user when calling the callback.
    using TargetFreeCB = R (*)(Args...);

  public:
    // Default construct with stored ptr == nullptr.
    delegate() : m_cb(nullptr), m_ptr(nullptr){};

    ~delegate(){};

    // Call the stored function. Requires: bool(*this) == true;
    // Will call trampoline fkn which will call the final fkn.
    R operator()(Args... args) __attribute__((always_inline))
    {
        return m_cb(m_ptr, args...);
    }

    // Return true if a function pointer is stored.
    operator bool() const noexcept
    {
        return m_cb != nullptr;
    }

    // Return the function pointer to allow easy checking versus nullptr
    operator Trampoline() const noexcept
    {
        return m_cb;
    }

    void clear()
    {
        m_cb = nullptr;
        m_ptr = nullptr;
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    constexpr delegate& set()
    {
        m_cb = &doFreeCB<fkn>;
        m_ptr = nullptr;
        return *this;
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args)>
    constexpr delegate& set(T& object)
    {
        m_cb = &doMemberCB<T, memFkn>;
        m_ptr = static_cast<void*>(&object);
        return *this;
    }

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    constexpr delegate& set(T& object)
    {
        m_cb = &doFunctor<T>;
        m_ptr = static_cast<void*>(&object);
        return *this;
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    static constexpr delegate make()
    {
        auto cb = &doFreeCB<fkn>;
        return delegate(cb, nullptr);
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args)>
    static constexpr delegate make(T& object)
    {
        auto cb = &doMemberCB<T, memFkn>;
        T* obj = &object;
        return delegate(cb, static_cast<void*>(obj));
    }

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    static constexpr delegate make(T& object)
    {
        auto cb = &doFunctor<T>;
        T* obj = &object;
        return delegate(cb, static_cast<void*>(obj));
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <class Tptr, R (*fkn)(Tptr, Args... args)>
    static constexpr delegate makeFreeCBWithPtr(Tptr ptr)
    {
        auto cb = &doFreeCBWithPtr<Tptr, fkn>;
        return delegate(cb, static_cast<void*>(ptr));
    }

    /**
     * Create a callback to a free function with a void pointer,
     * removing the need for an adapter function.
     */
    template <Trampoline fkn>
    static constexpr delegate makeVoidCB(void* ptr = nullptr)
    {
        return delegate(fkn, ptr);
    }

    /**
     * Create a callback using a run-time variable fkn pointer
     * using voids. No adapter function is used.
     */
    static delegate makeVoidCB(Trampoline fkn, void* ptr = nullptr)
    {
        return delegate(fkn, ptr);
    }

  private:
    // Create ordinary free function pointer callback.
    constexpr delegate(Trampoline cb, void* ptr) : m_cb(cb), m_ptr(ptr) {}

    Trampoline m_cb;
    void* m_ptr;
};

/**
 * Helper macro to create a delegate for calling a member function.
 * Example of use:
 *
 * auto cb = MAKE_MEMBER_DEL(void(), SomeClass::memberFunction, obj);
 *
 * where 'obj' is of type 'SomeClass'.
 *
 * @param fknType Template parameter for the function signature in std::function
 * style.
 * @param memFknPtr address of member function pointer. C++ require full name path.
 * @object object which the member function should be called on.
 */
#define MAKE_MEMBER_DEL(fknType, memFknPtr, object)                             \
                                                                             \
    (delegate<fknType>::make<std::remove_reference<decltype(object)>::type, \
                              memFkn>(object))

/**
 * Helper macro to create a delegate for calling a free function
 * or static class function.
 * Example of use:
 *
 * auto cb = MAKE_FREE_DEL(void(), fkn, ptr);
 *
 * where 'ptr' is a pointer to some type T and fkn a free
 * function.
 *
 * @param fknType Template parameter for the function signature in std::function
 * style.
 * @param fkn Free function to be called. signature: R (*)(T*, Args...);
 * @object Pointer to be supplied as first argument to function.
 *
 */
#define MAKE_FREE_DEL(fknType, fkn, ptr)                                  \
                                                                          \
    (delegate<fknType>::make<std::remove_reference<decltype(ptr)>::type, \
                              fkn>(ptr))

#endif /* UTILITY_CALLBACK_H_ */
