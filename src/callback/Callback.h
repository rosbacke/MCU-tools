/*
 * Callback.h
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#ifndef UTILITY_CALLBACK_H_
#define UTILITY_CALLBACK_H_

#include "cpp98_nullptr.h"
#include <utility>

/**
 * Simple callback helper to handle function callback
 * of the form void (*)(void*, int);
 * Fixed format intended to keep things simple. This also compiles
 * in c++98. For a more versatile tool, look at Callback2.
 * Intended use is e.g.g callbacks from async operations signaling
 * completion. The int should be enough to get a status code back.
 *
 * Design intent to push user code toward the user to keep the
 * common case small and fast for embedded devices. In particular,
 * only store 2 pointers and avoid a virtual function call to avoid need for
 * vtable.
 *
 * Main intent is to replace explicit use of function callbacks with void
 * pointers
 * and avoid having to do explicit trampoline code in called classes.
 *
 * The price is less generality (No functors) and a bit of type fiddling
 * with void pointers.
 *
 * Helper functions are provided to handle the following cases:
 * - Call a member function on a specific object of signature void fkn(int);
 * - Call a free function of signature void(*)(T*, int); (T is some arbitrary
 * type).
 * - Call a free function of signature void(*)(void*, int); (function can be a
 * function pointer)
 *
 * In the code storing the callback, construct an object of type 'Callback'.
 * This is a nullable type which can be copied freely, containing 2 pointers.
 *
 * User code assigns this object using macros 'MAKE_MEMBER_CB' or
 * 'MAKE_FREE_CB'.
 *
 * Call the callback by regular operator(). E.g.
 * Callback cb = ...; // Set up some callback target.
 * ...
 * int val = 0;
 * cb(val);
 * For free functions, the supplied void pointer get the value from the call
 * setup.
 */
class Callback
{

    // Adapter functions to handle the void -> type conversions.
    template <class T, void (T::*memFkn)(int)>
    inline static void doMemberCB(void* o, int val)
        __attribute__((always_inline));

    template <class Tptr, void(freeFknWithPtr)(Tptr, int)>
    inline static void doFreeCBWithPtr(void* o, int val)
        __attribute__((always_inline));

    template <void(SimpleFreeFkn)(int)>
    inline static void doFreeCB(void* o, int val)
        __attribute__((always_inline));

    template <class Functor>
    inline static void doFunctor(void* o, int val)
        __attribute__((always_inline));

    typedef void (*CB)(void*, int);

  public:
    // Default construct with stored ptr == nullptr.
    Callback() : m_cb(nullptr), m_ptr(nullptr){};
    ~Callback(){};

    // Call the stored function. Requires: bool(*this) == true;
    void operator()(int val)
    {
        m_cb(m_ptr, val);
    }

    // Return true if a function pointer is stored.
    operator bool() const
    {
        return m_cb != nullptr;
    }

    operator CB() const
    {
        return m_cb;
    }

    void clear()
    {
        m_cb = nullptr;
        m_ptr = nullptr;
    }

    /**
     * Set callback to a member function to a given object.
     */
    template <class T, void (T::*memFkn)(int)>
    Callback& set(T& object)
    {
        m_cb = doMemberCB<T, memFkn>;
        m_ptr = static_cast<void*>(&object);
        return *this;
    }

    /**
     * Set callback to a free function with a specific type on the pointer.
     */
    template <class Tptr, void (*fkn)(Tptr, int)>
    Callback& set(Tptr ptr)
    {
        m_cb = doFreeCBWithPtr<Tptr, fkn>;
        m_ptr = static_cast<void*>(ptr);
        return *this;
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, void (T::*memFkn)(int)>
    static Callback make(T& object)
    {
        return Callback(doMemberCB<T, memFkn>, static_cast<void*>(&object));
    }

    /**
     * Create a callback to a free function with a specific type on the pointer.
     */
    template <class Tptr, void (*fkn)(Tptr, int)>
    static Callback make(Tptr ptr)
    {
        return Callback(doFreeCBWithPtr<Tptr, fkn>, static_cast<void*>(ptr));
    }

    template <void (*fkn)(int)>
    static Callback make()
    {
        return Callback(doFreeCB<fkn>, nullptr);
    }

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    static Callback make(T& object)
    {
        CB cb = &doFunctor<T>;
        T* obj = &object;
        return Callback(cb, static_cast<void*>(obj));
    }

    /**
     * Create a callback to a free function with a void pointer, removing the
     * need for an adapter function.
     */
    template <CB fkn>
    static Callback make(void* ptr = nullptr)
    {
        return Callback(fkn, ptr);
    }

    /**
     * Create a callback using a run-time variable fkn pointer using voids. No
     * adapter function is used.
     */
    static Callback make(CB fkn, void* ptr = nullptr)
    {
        return Callback(fkn, ptr);
    }

  private:
    Callback(CB cb, void* ptr) : m_cb(cb), m_ptr(ptr)
    {
    }
    CB m_cb;
    void* m_ptr;
};

template <class T, void (T::*memFkn)(int)>
void
Callback::doMemberCB(void* o, int val)
{
    T* obj = static_cast<T*>(o);
    ((*obj).*(memFkn))(val);
}

template <class Tptr, void(freeFknWithPtr)(Tptr, int)>
void
Callback::doFreeCBWithPtr(void* o, int val)
{
    Tptr obj = static_cast<Tptr>(o);
    freeFknWithPtr(static_cast<Tptr>(o), val);
}

template <void(simpleFreeFkn)(int)>
void
Callback::doFreeCB(void* o, int val)
{
    simpleFreeFkn(val);
}

// Adapter function for the free function with extra first arg
// in the called function, set at callback construction.
template <class Functor>
void
Callback::doFunctor(void* o, int val)
{
    Functor* obj = static_cast<Functor*>(o);
    (*obj)(val);
}

/**
 * Helper macro to create Callbacks for calling a member function.
 * Example of use:
 *
 * Callback cb = MAKE_MEMBER_CB(SomeClass, SomeClass::memberFunction, obj);
 *
 * where 'obj' is of type 'SomeClass'.
 */
#define MAKE_MEMBER_CB(objType, memFkn, object) \
    \
(Callback::make<objType, &memFkn>(object))

/**
 * Helper macro to create Callbacks for calling a free function
 * or static class function.
 * Example of use:
 *
 * Callback cb = MAKE_FREE_CB(T, fkn, ptr);
 *
 * where 'ptr' is a pointer to some type T and fkn a free
 * function with signature 'void (*)(T*, int);
 */
#define MAKE_FREE_CB(ptr_type, fkn, ptr) \
    \
(Callback::make<ptr_type*, fkn>(ptr))

#endif /* UTILITY_CALLBACK_H_ */
