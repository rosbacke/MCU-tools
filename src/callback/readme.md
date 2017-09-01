I have recently been returning to programming microcontrollers.
Had an idea of creating a client for the SerialNet to STM32F4
microcontrollers and immediately got side-tracked into the issue 
of callbacks in C++.

In C++11 and later we have std::function. It works very well
in normal environments. You can throw a bunch of different callable
typas at it and it allows you to call them uniformly.
It does have some drawbacks though. For microcontrollers the most 
grave one is the need for heap allocation. When you assign e.g. a function
object that can be arbitrarily large this is a neccessity. But is does
make it unsuitable for many MCU:s.
Other issues are using virtual dispatch which forces us to have vtables and
std::function can be a bit on the large side. (e.g. gcc uses 24 bytes on 64 bit architectures.)

An example of usage for the std::function:

```
    std::function f = [](int i)->int { return i; };
    auto a = f(3); // a becomes 3.
```

Coming from MCU and C programming, a traditional way to implement callbacks
are to use normal function pointers. You store a pointer to a free function and later call
it in some other context. This works well in C style procedural programs.
Using OO techinques it is common to supply a void* pointer as first argument to distinguish a data object to operate on.

For example. interrupt service routines can be thought of a callbacks. The STM32
MCU family can have 5-10 USART hardware devices on one chip. Each of these have
one ISR.
Using OO techiques one might want to write one class called 'UsartDriver' 
implementing a driver and instantiate one object for each hardware module. 
Assume one member function called 'UsartDriver::isr()' is supposed to be called 
during IRQ calls. 
In practice that means each ISR should call the same member function but use 
a different object in the call. It boils down to traditional function callbacks
also often needs an additional pointer to point out the object to operate on.
To avoid the hassle of member function pointers, it is common to implement
a small adapter function in lines of:

```
    UsartDriver {
        static void IsrAdapter(void* me) {
	    static_cast<UsartDriver*>(me)->isr();
	}
	...
    };
```

It is static so a normal C style function pointer can hold an address to it.

So one question is, can we make something similar to std::function in a more
embedded friendly manner? I would like to see:
- Storage size of the callback should only be one function pointer 
  and an additional data pointer. For a 32 bit system we got 8 bytes of 
  storage which is reasonable.
- No heap allocation. This is a must.
- No exception throwing from the callback itself.
- Ideally, no virtual dispatch. It forces vtable etc on our classes.
- Nice bonus if we can make some callback compile time if they are known then.
Also, std::function allow arbitrary function signatures for the call. Would be
nice to have.

Been hacking away on this problem and come up with some solution. I does
have some limitations but handles 2 common cases with a natural syntax for
user application:

- Storing and calling a free function with the first argument being a stored
  pointer from the user.
- Storing and calling a member function on a specific object from callback
  setup.

During call, the callback allow an arbitrary argument list to be passed from
the caller to the callee, mimicing the std::function support for function
signatures.
There are limited lambda support for those cases where no capture is done.
Missing here are functor support that require larger storage.

How do we handle both member and free function callbacks without virtual
dispatch? Lets start with a simplified example with no arguments and no
return value:

```
    class Callback {
        using CB = void (*)(void *);
        void operator()() { fkn(ptr); }
    private:
        CB fkn;
        void* ptr;
    };
```

This stores the function pointer and data pointer in a general value type
object. Calling operator() will call the stored pointer. Now how to handle
member functions? Trick is to use a specially made adaptor function. A member
function is known at compile time and can be used as a template argument.
So we construct a template function that  implement the adaptor function. 
Use static maker functions to set up the callback.
So we got:

```
    Callback {
        ... // as before.
    public:
        // Callback setup function.
        template<class T, void (T::*memFkn)()>
        static Callback makeMemberCB(T& object);

    private:
        // Adaptor function. Pointed to by 'fkn'. Will call 'o->memFkn()'.
        template<class T, void (T::*memFkn)()>
        inline static void doMemberCB(void* o); 
        ...
    };
```

Lets start with an example of usage:

```
    struct MyStruct {
        void memberFkn();
    };

    MyStruct ms;
    Callback cb = Callback::makeMemberCB<MyStruct, &MyStruct::memberFkn>(ms);

    cb(); // Will call memberFkn on object ms.
```

Interesting part is makeMemberCB. A bit long but this is due to C++ 
way to specify member function pointers. Note that we pass this as template
arguments. So what happens? Lets see the implementation:


```
    template<class T, void (T::*memFkn)(int)>
    Callback Callback::makeMemberCB(T& object)
    {
        auto cb = doMemberCB<T, memFkn>;
        T* obj = &object;
        return Callback(cb, static_cast<void*>(obj));
    }
```

We derive a free function pointer to a templated function 'doMemberCB'.
Since it is a free function it works well with traditional function pointers.
We pass the member function pointer as template argument. However the actual object
pointer comes with the normal argument list and is specified at runtime.
The type of the object is also given as a template argument. We cast away the type
during storage (store as void*) and pass it to the adapter. However it does know the 
original type and can restore it at call time.

So how does the adapter look like?

```
    template<class T, void (T::*memFkn)()>
    void Callback::doMemberCB(void* o)
    {
        T* obj = static_cast<T*>(o);
        ((*obj).*(memFkn))(val);
    }
```

At call time it is called with the object as a void*. We first restore the
type of object. Once done we can call the supplied member function pointer
on this object. Note that since the member function pointer is known at
compile time, the compiler can resolve this call to be just as efficient
as a normal member function call.
The syntax for the actual call looks arcane but it is due to the rules of
member function calls.

So does it work? Yes. The virtual dispatch is avoided by having the adapter
function. To create a free function call, we simply add another make function with will set up another adapter function doing a normal free function call.
Once set up, the Callback class can not tell the difference so we can also do
old fashioned direct call to a void(*)(void*) function directly if speed is of
the essence.

The basic idea is here implemented with a concrete class callback that only
allow one type of function call signature. However it is straight forward
to generalize this to a std::function style template. One could do this
like this using partial template spezialization:

```
    template<typename T>
    class Callback2;

    template<typename R, typename... Args>
    class Callback2<R(Args...)>
    {
        ... // Implementation.
    };
```

Tried using this approach in my UsartDriver class. Some interesting code parts
with supplied ARM (cortex-m4) disassembly. Compiled with -Os optimization.

First, the ISR handler setup and call:

```
    struct {
        Callback2<void()> usart1;
        Callback2<void()> usart2;
        Callback2<void()> usart3;
	...
    } isrCB;

    extern "C" void USART1_IRQHandler(void)
    {
        isrCB.usart1();
    }
```

After compilation:

```
08000684 <_ZN9Callback2IFvvEEclEv>:
 8000684:	6803      	ldr	r3, [r0, #0]
 8000686:	6840      	ldr	r0, [r0, #4]
 8000688:	4718      	bx	r3
	...

0800068c <USART1_IRQHandler>:
 800068c:	b508      	push	{r3, lr}
 800068e:	4802      	ldr	r0, [pc, #8]	; (8000698 <USART1_IRQHandler+0xc>)
 8000690:	f7ff fff8 	bl	8000684 <_ZN9Callback2IFvvEEclEv>
 8000694:	bd08      	pop	{r3, pc}
 8000696:	bf00      	nop
 8000698:	20000044 	.word	0x20000044
```

So we got push for function setup. The LDR reads a pointer to isrCB.usart1
at 0x20000044. Then bl do the jump to operator() in Callback. (0x8000690).
We return and pop. This looks more or less normal for a function.

At 08000684 we have operator() for Callback. It loads the pointer and
function ptr to registers and jump. 3 instructions in total. So where does
it jump?
Some searching shows this function is called:

```
080007b4 <_ZN9Callback2IFvvEE10doMemberCBI12UsartServiceXadL_ZNS3_3isrEvEEEEvPv>:
 80007b4:	f7ff bfb2 	b.w	800071c <_ZN12UsartService3isrEv>
```

One jump instruction calling our UsartService::isr(). It would be hard to do it 
better than one instruction for our adaptor function.

This seems really decent. Can it be improved? Turns out, yes.
This was compiled with -Os to save space. Gcc really don't want to 
inline using -Os. By forcing Callback2::operator() to inline:

```
    Callback2... {
        ...
        R operator()(Args... args) __attribute__((always_inline)) {
            return m_cb(m_ptr, args...);
        }
        ...
    };
```

I got this:

```
080005e8 <USART1_IRQHandler>:
 80005e8:	4b01      	ldr	r3, [pc, #4]	; (80005f0 <USART1_IRQHandler+0x8>)
 80005ea:	681a      	ldr	r2, [r3, #0]
 80005ec:	6858      	ldr	r0, [r3, #4]
 80005ee:	4710      	bx	r2
 80005f0:	20000044 	.word	0x20000044
```

The call to operator() is removed and 'bx r2' goes directly to our one
instruction adapter function. So in addtioiton to remove 3 instruction
function we got a 1 instruction shorter ISR handler. Not bad.

It is possible to tweak gcc settings to consider some inlining in -Os which
usually saves some space. But main point to take away is that we succeeded
in making a really efficient callback mechanism, while still being able to
handle member function callbacks in a clean user friendly way.


Final part, the setup of the callback. I have a UsartService::init() function.
It contain this lines:

```
    ...
    // Enable TX empty interrupt.
    isrCB.usart1 = MAKE_MEMBER_CB2(void(), UsartService::isr, *this);

    cr1 |= USART_CR1_TXEIE | USART_CR1_IDLEIE;
    USART1->CR1 = cr1;

The macro MAKE_MEMBER_CB2 take away some of the ugliness assigning member
functions. Expanded it would look something like this:


```
    isrCB.usart1 = Callback2<void()>::makeMemberCB<UsartDriver,  &UsartDriver::isr>(*this);
```

With the macro it reads fairly ok. Without, a bit non-intuitive and verbose
for my taste. The disassembly 

```
 8000680:	4a07      	ldr	r2, [pc, #28]	; (80006a0 <_ZN12UsartService4initEv+0x7c>)
 8000682:	4c08      	ldr	r4, [pc, #32]	; (80006a4 <_ZN12UsartService4initEv+0x80>)
 8000684:	6014      	str	r4, [r2, #0]
 800068a:	6050      	str	r0, [r2, #4]
```

Basically, load pointer to the Callback2 object. Then load fkn pointer to
adapter function, store fkn pointer and this pointer (r0) in Callback2 object.
This is close to optimal.

This callback code turned out really nice. A useful class I'll probably
drop into most of my upcoming MCU project. A note on C++14 and MCU:s.
I'm using Atollic IDE which comes with gcc 5.4. Using an ARM Cortex-M class
MCU with more than 32kb ram, C++ really works well. The additional control and
help with compiler diagnostics pays off quickly. All that is needed is some
disipline to avoid using the heap and other expensive operations.
Templates used right, gives the compiler more tools to optimize your code.
It really shines in encoding compile time data and usually allow more
aggressive optimizations. Also, Arduino (8 bit AT mega 328) uses
(a limited variant) C++. So go for it.
