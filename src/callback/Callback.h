/*
 * Callback.h
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#ifndef UTILITY_CALLBACK_H_
#define UTILITY_CALLBACK_H_

#include <utility>

/**
 * Simple callback helper to handle function callback
 * of the form void (*)(void*, int);
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

    template <class Tptr, void(freeFkn)(Tptr, int)>
    inline static void doFreeCB(void* o, int val)
        __attribute__((always_inline));

    using CB = void (*)(void*, int);

  public:
    // Default construct with stored ptr == nullptr.
    Callback(){};
    ~Callback(){};

    // Call the stored function. Requires: bool(*this) == true;
    void operator()(int val)
    {
        m_cb(m_ptr, val);
    }

    // Return true if a function pointer is stored.
    explicit operator bool() const noexcept
    {
        return m_cb != nullptr;
    }

    void clear()
    {
        m_cb = nullptr;
        m_ptr = nullptr;
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, void (T::*memFkn)(int)>
    static Callback makeMemberCB(T& object)
    {
        auto cb = doMemberCB<T, memFkn>;
        T* obj = &object;
        return Callback(cb, static_cast<void*>(obj));
    }

    /**
     * Create a callback to a free function with a specific type on the pointer.
     */
    template <class Tptr, void (*fkn)(Tptr, int)>
    static Callback makeFreeCB(Tptr ptr)
    {
        auto cb = &doFreeCB<Tptr, fkn>;
        return Callback(cb, static_cast<void*>(ptr));
    }

    /**
     * Create a callback to a free function with a void pointer, removing the
     * need for an adapter function.
     */
    template <CB fkn>
    static Callback makeVoidCB(void* ptr = nullptr)
    {
        return Callback(fkn, ptr);
    }

    /**
     * Create a callback using a run-time variable fkn pointer using voids. No
     * adapter function is used.
     */
    static Callback makeVoidCB(CB fkn, void* ptr = nullptr)
    {
        return Callback(fkn, ptr);
    }

  private:
    Callback(CB cb, void* ptr) : m_cb(cb), m_ptr(ptr)
    {
    }
    CB m_cb = nullptr;
    void* m_ptr = nullptr;
};

template <class T, void (T::*memFkn)(int)>
void
Callback::doMemberCB(void* o, int val)
{
    T* obj = static_cast<T*>(o);
    ((*obj).*(memFkn))(val);
}

template <class Tptr, void(freeFkn)(Tptr, int)>
void
Callback::doFreeCB(void* o, int val)
{
    Tptr obj = static_cast<Tptr>(o);
    freeFkn(obj, val);
}

/**
 * Helper macro to create Callbacks for calling a member function.
 * Example of use:
 *
 * Callback cb = MAKE_MEMBER_CB(SomeClass::memberFunction, obj);
 *
 * where 'obj' is of type 'SomeClass'.
 */
#define MAKE_MEMBER_CB(memFkn, object)                                 \
    \
(Callback::makeMemberCB<std::remove_reference<decltype(object)>::type, \
                        &memFkn>(object))

/**
 * Helper macro to create Callbacks for calling a free function
 * or static class function.
 * Example of use:
 *
 * Callback cb = MAKE_FREE_CB(fkn, ptr);
 *
 * where 'ptr' is a pointer to some type T and fkn a free
 * function with signature 'void (*)(T*, int);
 */
#define MAKE_FREE_CB(fkn, ptr) \
    \
(Callback::makeFreeCB<std::remove_reference<decltype(ptr)>::type, fkn>(ptr))

#endif /* UTILITY_CALLBACK_H_ */
