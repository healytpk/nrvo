#ifndef HEADER_INCLUSION_GUARD_NRVO_HPP
#define HEADER_INCLUSION_GUARD_NRVO_HPP

// There are five implementations in this file:
// (1) The common implementation that works on most machines
// (2) m68k  : Motorola M68000
// (3) arm64 : ARM 64-Bit (aarch64)
// (4) sh4   : SuperH 4 (sh4)
// (5) hppa  : Hewlett Packard Precision Architecture

#if !defined(__MC68K__) && !defined(__MC68000__) && !defined(__M68K__) && !defined(__m68k__) \
 && !defined(__aarch64__) && !defined(_M_ARM64)                                              \
 && !defined(__sh__) && !defined(__SH__) && !defined(__sh4__) && !defined(__SH4__)           \
 && !defined(__hppa__) && !defined(__hppa64__)                                               \
/* Don't remove this empty line -- it's needed */
#    define NRVO_USE_COMMON_IMPLEMENTATION
#endif

#ifdef NRVO_USE_COMMON_IMPLEMENTATION  // ============ start of common code for 'nrvo' ============

#include <cassert>           // assert
#include <functional>        // function
#include <optional>          // optional
#include <utility>           // move, forward
#include <variant>           // variant

template<typename R, typename... Params>
R nrvo(std::function<void(R*,Params...)> const &f, Params... args)
{
    using F = std::function<void(R*,Params...)>;
    using FuncPtrT = void (*)(F const *,R*,Params...);

    auto const invoke_swap_1st_with_2nd =
      [](R *const r, F const *const pf, FuncPtrT const fptr, Params... args2) -> void
      {
        // This helper function is needed in order to ensure that when calling
        // "std::function::operator()", the address of the “std::function” object
        // is placed in 1st position (instead of the return value address)
        fptr( pf, r, std::forward<Params>(args2)... );
      };

    void (*const y)(R*,F const*,FuncPtrT,Params...) = invoke_swap_1st_with_2nd;
    auto const z = reinterpret_cast<R (*)(F const *,FuncPtrT,Params...)>(y);

    auto const invoke_functor =
      [](F const *const p,R *const r,Params... args2) -> void
      {
        // The GNU compiler g++ allows you to convert a member function pointer
        // to a normal function pointer:
        //    https://gcc.gnu.org/onlinedocs/gcc/Bound-member-functions.html
        // however some compilers such as LLVM clang++ don't -- and that's why
        // this helper function is needed.
        p->operator()( r, std::forward<Params>(args2)... );
      };

     return z( &f, invoke_functor, std::forward<Params>(args)... );
}

#else  // ====================== end of common code for 'nrvo' ================================

__asm__(
".text                      \n"
"detail_nrvo_asm:           \n"
#  if defined(__MC68K__) || defined(__MC68000__) || defined(__M68K__) || defined(__m68k__)
"    move.l  8(%a7),   %d0  \n"                   // scratch_register = second_argument
"    move.l    %a1 , 8(%a7) \n"                   // second_argument  = address_of_return_value
"    jmp     (%d0)          \n"                   // program_counter  = scratch_register
#        define compare(value)                                \
             "addq.l    #1, %a0                           \n" \
             "move.b (%a0), %d1                           \n" \
             "move.l #0x" #value ", %d3                   \n" \
             "cmp.b    %d1, %d3                           \n" \
             "bne detail_nrvo_invoke2_loop                \n"
#  elif defined(__aarch64__) || defined(_M_ARM64)
"    mov     x9, x1         \n"                   // scratch_register = second_argument
"    mov     x1, x8         \n"                   // second_argument  = address_of_return_value
"    br      x9             \n"                   // program_counter  = scratch_register
#        define compare(value)                                \
             "add x9, x9, #1                              \n" \
             "ldrb w10, [x9]                              \n" \
             "mov w11, #0x" #value "                      \n" \
             "cmp w10, w11                                \n" \
             "b.ne detail_nrvo_invoke2_loop               \n"
#  elif defined(__sh__) || defined(__SH__) || defined(__sh4__) || defined(__SH4__)
"    mov     r5, r3         \n"                   // scratch_register = second_argument
"    mov     r2, r5         \n"                   // second_argument  = address_of_return_value
"    jmp     @r3            \n"                   // program_counter  = scratch_register
"    nop                    \n"                   // extra instruction needed for alignment
#        define compare(value)                                \
             "add #1, r0                                  \n" \
             "mov.b @r0, r1                               \n" \
             "mov #0x" #value ", r3                       \n" \
             "cmp/eq r1, r3                               \n" \
             "bf detail_nrvo_invoke2_loop                 \n"
#  elif defined(__hppa__) || defined(__hppa64__)
"    copy    %r25, %r20     \n"                   // scratch_register = second_argument
"    copy    %r28, %r25     \n"                   // second_argument  = address_of_return_value
"    bv,n    %r0(%r20)      \n"                   // program_counter  = scratch_register
#        define compare(value)                                 \
             "ldi 0x" #value ", %r20                       \n" \
             "ldb -1(%r21), %r22                           \n" \
             "cmpb,<> %r20, %r22, detail_nrvo_invoke2_loop \n" \
             "ldo -1(%r21), %r21                           \n"
#endif
);

#define compare_bytes \
        compare(20)   \
        compare(b6)   \
        compare(90)   \
        compare(93)   \
        compare(a4)   \
        compare(a1)   \
        compare(4e)   \
        compare(79)   \
        compare(93)   \
        compare(af)   \
        compare(36)   \
        compare(dd)   \
        compare(d0)   \
        compare(e0)   \
        compare(a3)   \
        compare(c7)

#include <cassert>           // assert
#include <climits>           // CHAR_BIT
#include <functional>        // function
#include <optional>          // optional
#include <utility>           // move, forward
#include <variant>           // variant

extern "C" void detail_nrvo_asm(void);

template<typename R, typename... Params>
R nrvo(std::function<void(R*,Params...)> const &f, Params... args)
{
    using F = std::function<void(R*,Params...)>;

    auto const invoke_functor = [](F const *const p, R *const r, Params... args2) -> void
      {
        // The GNU compiler g++ allows you to convert a member function pointer
        // to a normal function pointer:
        //    https://gcc.gnu.org/onlinedocs/gcc/Bound-member-functions.html
        // however some compilers such as LLVM clang++ don't -- and that's why
        // this helper function is needed.
        p->operator()( r, std::forward<Params>(args2)... );
      };

    void (*const funcptr)(F const*,R*,Params...) = invoke_functor;

    auto const z = reinterpret_cast<R(*)(F const*,R*,Params...)>( &detail_nrvo_asm );

    return z( &f, reinterpret_cast<R*>(funcptr), std::forward<Params>(args)... );
}

#endif  // ifdef NRVO_USE_COMMON_IMPLEMENTATION

// The following template allows for 'nrvo' to be used
// with a simple function pointer e.g. "void (*)(R*)"
// instead of an std::function object.
template<typename R, typename... Params>
R nrvo(void (*const funcptr)(R*,Params...), Params... args)
{
    return nrvo( std::function<void(R*,Params...)>(funcptr), std::forward<Params>(args)... );
}

#if defined(__MC68K__) || defined(__MC68000__) || defined(__M68K__) || defined(__m68k__)

__asm__(
    "detail_nrvo_invoke2:         \n"
        "move.l %a0, -(%a7)       \n"
        "move.l %d1, -(%a7)       \n"
        "move.l %d2, -(%a7)       \n"
        "move.l %d3, -(%a7)       \n"
        "move.l %a7,   %a0        \n" // Copy the stack pointer to %a0
        "subq.l  #1,   %a0        \n"
    "detail_nrvo_invoke2_loop:    \n"
         compare_bytes
        "addq.l    #1, %a0        \n"
        "move.l (%a0), %a1        \n" // The address of the return value
        "addq.l    #4, %a0        \n"
        "move.l (%a0), %d0        \n" // The address to jump to
        "move.l (%a7)+, %d3       \n"
        "move.l (%a7)+, %d2       \n"
        "move.l (%a7)+, %d1       \n"
        "move.l (%a7)+, %a0       \n"
        "jmp (%d0)                \n" // Jump to the address in %d3
);

#elif defined(__aarch64__) || defined(_M_ARM64)

__asm__(
    "detail_nrvo_invoke2:         \n"
        "mov x9, sp               \n"  // Copy the stack pointer to x9
        "sub x9, x9, #1           \n"  // Decrement x9 by 1 byte
    "detail_nrvo_invoke2_loop:    \n"
         compare_bytes
        "add x9, x9, #1           \n"
        "ldr x8, [x9]             \n"  // The address of the return value
        "add x9, x9, #8           \n"
        "ldr x9, [x9]             \n"  // The address to jump to
        "br x9                    \n");

#elif defined(__sh__) || defined(__SH__) || defined(__sh4__) || defined(__SH4__)

__asm__(
    "detail_nrvo_invoke2:         \n"
        "mov r15, r0              \n"
        "add #-1, r0              \n"
    "detail_nrvo_invoke2_loop:    \n"
         compare_bytes
        "add #1, r0               \n"
        "mov.l @r0, r2            \n"  // The address of the return value
        "add #4, r0               \n"
        "mov.l @r0, r3            \n"  // The address to jump to
        "jmp @r3                  \n");

#elif defined(__hppa__) || defined(__hppa64__)

// Stack grows upwards on HPPA -- every other architecture grows downards!
// That's why we search for the UUID backwards

__asm__ (
    ".text                        \n"
    "detail_nrvo_invoke2:         \n"
        "copy %sp, %r21           \n"  // p = std::stack_pointer() (points at empty space)
        "ldo  1(%r21), %r21       \n"  // ++p    [20 b6 90 93 a4 a1 4e 79 93 af 36 dd d0 e0 a3 c7]
    "detail_nrvo_invoke2_loop:    \n"
        "ldo -1(%r21), %r21       \n"  // --p
        compare(c7)  // [20 b6 90 93 a4 a1 4e 79 93 af 36 dd d0 e0 a3 c7]
        compare(a3)
        compare(e0)
        compare(d0)
        compare(dd)
        compare(36)
        compare(af)
        compare(93)
        compare(79)
        compare(4e)
        compare(a1)
        compare(a4)
        compare(93)
        compare(90)
        compare(b6)
        compare(20)
        "ldo 0x10(%r21), %r21    \n"  // p += 16u
        "ldw 0(%r21), %r28       \n"  // address of retval = *(uint32_t*)r21
        "ldo 0x04(%r21), %r21    \n"  // p +=  4u
        "ldw 0(%r21), %r20       \n"  // void (*f)(void) = *(uint32_t*)r21
        "bv,n %r0(%r20)          \n");// f()

#endif

extern "C" void detail_nrvo_invoke2(void);

namespace detail_nrvo {

    template<typename R, typename... Params> class Invoker;

    class PutRetvalIn_Class_Base {
        std::function<void(void)> cleanup;
        void *const p;
    public:
        PutRetvalIn_Class_Base(void *const arg_p, std::function<void(void)> &&arg_cleanup) noexcept
          : cleanup(std::move(arg_cleanup)), p(arg_p) {}
        PutRetvalIn_Class_Base(PutRetvalIn_Class_Base       &&) = delete;
        PutRetvalIn_Class_Base(PutRetvalIn_Class_Base const & ) = delete;
        PutRetvalIn_Class_Base &operator=(PutRetvalIn_Class_Base       &&) = delete;
        PutRetvalIn_Class_Base &operator=(PutRetvalIn_Class_Base const & ) = delete;
        template<typename R, typename... Params> friend class Invoker;
    };

    template<typename R, typename... Params>
    class Invoker {

        typedef std::function<R(Params...)> F;

        // The constructor of this class is private so that you can't just pull
        // one out of thin air. There is only one friend: PutRetvalIn<R>

        // An object of type 'Invoker<R,Params...>' is returned
        // by value from PutRetvalIn<R>::operator()<Params...>

        PutRetvalIn_Class_Base &&pricb;
        std::variant<F const *, R(*)(Params...)> ptr;

        Invoker(PutRetvalIn_Class_Base &&argP,       F const &argF        ) noexcept
            : pricb( std::move(argP) ), ptr(&argF) {}

        Invoker(PutRetvalIn_Class_Base &&argP, R (*const argF)(Params...) ) noexcept
            : pricb( std::move(argP) ), ptr( argF) {}

        void invoke(F const &f, Params... args)
        {
            auto const invoke_functor =
              [](F const *const p, Params... args2) -> R
              {
                // The GNU compiler g++ allows you to convert a member function pointer
                // to a normal function pointer:
                //    https://gcc.gnu.org/onlinedocs/gcc/Bound-member-functions.html
                // however some compilers such as LLVM clang++ don't -- and that's why
                // this helper function is needed.
                return p->operator()( std::forward<Params>(args2)... );
              };

            R (*const y)(F const*,Params...) = invoke_functor;

            try
            {
#ifdef NRVO_USE_COMMON_IMPLEMENTATION
                auto const z = reinterpret_cast<void (*)(R*,F const*,Params...)>(y);
                z( static_cast<R*>(pricb.p), &f, std::forward<Params>(args)... );
#else
                auto const z = reinterpret_cast<void (*)(F const*,Params...)>(y);

                static_assert( 8u == CHAR_BIT, "Not ready yet for Texas Instruments microcontrollers");

                static char unsigned const static_uuid[16u] = {
                    0x20U, 0xb6U, 0x90U, 0x93U, 0xa4U, 0xa1U, 0x4eU, 0x79U,
                    0x93U, 0xafU, 0x36U, 0xddU, 0xd0U, 0xe0U, 0xa3U, 0xc7U,
                };

                // This following 'stack_uuid' is where we're putting the UUID + data on the stack
                alignas(16u) char unsigned volatile stack_uuid[16u + sizeof(void*) + sizeof(void(*)(void)) ];

                char unsigned volatile *p = stack_uuid;

                for ( unsigned i = 0u; i < 16u; ++i )
                    *p++ = static_uuid[i];
                for ( unsigned i = 0u; i < sizeof(void*); ++i )
                    *p++ = static_cast<char unsigned const*>(static_cast<void const*>(&pricb.p))[i];
                for ( unsigned i = 0u; i < sizeof(void(*)(void)); ++i )
                    *p++ = reinterpret_cast<char unsigned const*>(&z)[i];

                auto const zz = reinterpret_cast<void (*)(F const*,Params...)>(&detail_nrvo_invoke2);

                zz( &f, std::forward<Params>(args)... );
#endif
            }
            catch (...)
            {
                if ( pricb.cleanup ) pricb.cleanup();

                throw;
            }
        }

    public:
        void operator()(Params... args) /* deliberately non-const */
        {
            if ( 0u == this->ptr.index() )
                this->invoke(   *std::get<0u>(this->ptr)  , std::forward<Params>(args)... );
            else
                this->invoke( F( std::get<1u>(this->ptr) ), std::forward<Params>(args)... );
        }

        Invoker(Invoker       &&) = delete;
        Invoker(Invoker const & ) = delete;
        Invoker &operator=(Invoker       &&) = delete;
        Invoker &operator=(Invoker const & ) = delete;

        friend class PutRetvalIn_Class;
        friend class PutRetvalIn_Class_nrvo;
    };

    class PutRetvalIn_Class : public PutRetvalIn_Class_Base {
    public:
        explicit PutRetvalIn_Class(void *const arg_p, std::function<void(void)> &&arg_cleanup = {}) noexcept
          : PutRetvalIn_Class_Base(arg_p, std::move(arg_cleanup) ) {}

        template<typename R, typename... Params>
        detail_nrvo::Invoker<R,Params...> operator()(std::function<R(Params...)> const &arg)
        {
            return detail_nrvo::Invoker<R,Params...>( std::move(*this), arg);
        }

        template<typename R, typename... Params>
        detail_nrvo::Invoker<R,Params...> operator()( R(*const arg)(Params...) )
        {
            return detail_nrvo::Invoker<R,Params...>( std::move(*this), arg);
        }

        PutRetvalIn_Class(PutRetvalIn_Class       &&) = delete;
        PutRetvalIn_Class(PutRetvalIn_Class const & ) = delete;
        PutRetvalIn_Class &operator=(PutRetvalIn_Class       &&) = delete;
        PutRetvalIn_Class &operator=(PutRetvalIn_Class const & ) = delete;
    };

    class PutRetvalIn_Class_nrvo : public PutRetvalIn_Class_Base {
        alignas(std::function<void(void)>) char unsigned buf[sizeof(std::function<void(void)>)];
        std::function<void(void)> destroy_buf;

        template<typename R, typename... Params>
        detail_nrvo::Invoker<R,Params...> common(std::function<R(Params...)> &&arg)
        {
            typedef std::function<R(Params...)> F;
            static_assert( sizeof (std::function<void(void)>) == sizeof (F) );
            static_assert( alignof(std::function<void(void)>) == alignof(F) );

            F *const pf = static_cast<F*>(static_cast<void*>(this->buf));
            ::new(pf) F( std::move(arg) );
            destroy_buf = [pf](){ pf->~F(); };

            return detail_nrvo::Invoker<R,Params...>( std::move(*this), *pf );
        }

    public:

        ~PutRetvalIn_Class_nrvo(void)
        {
            if ( destroy_buf ) destroy_buf();
        }

        // cppcheck-suppress uninitMemberVar
        explicit PutRetvalIn_Class_nrvo(void *const arg_p, std::function<void(void)> &&arg_cleanup = {}) noexcept
          : PutRetvalIn_Class_Base(arg_p, std::move(arg_cleanup) ) {}

        template<typename R, typename... Params>
        detail_nrvo::Invoker<R,Params...> operator()(std::function<void(R*,Params...)> const &arg)
        {
            auto const mylambda = [&arg](Params... args) -> R { return nrvo(arg, std::forward<Params>(args)... ); };
            return this->common<R,Params...>( std::function<R(Params...)>(mylambda) );
        }

        template<typename R, typename... Params>
        detail_nrvo::Invoker<R,Params...> operator()( void(*const arg)(R*,Params...) )
        {
            auto const mylambda = [arg](Params... args) -> R { return nrvo(arg, std::forward<Params>(args)... ); };
            return this->common<R,Params...>( std::function<R(Params...)>(mylambda) );
        }

        PutRetvalIn_Class_nrvo(PutRetvalIn_Class_nrvo       &&) = delete;
        PutRetvalIn_Class_nrvo(PutRetvalIn_Class_nrvo const & ) = delete;
        PutRetvalIn_Class_nrvo &operator=(PutRetvalIn_Class_nrvo       &&) = delete;
        PutRetvalIn_Class_nrvo &operator=(PutRetvalIn_Class_nrvo const & ) = delete;
    };

    template<class R, class Helper>
    Helper PutRetvalIn_optional(std::optional<R> &arg)
    {
        arg.reset();

        struct Dummy {
            alignas(R) char unsigned buf[sizeof(R)];  // deliberately not initialised
            Dummy(void) {}                            // cppcheck-suppress uninitMemberVar
            Dummy(Dummy const & ) = delete;
            Dummy(Dummy       &&) = delete;
        };

        std::optional<Dummy> &b = *static_cast< std::optional<Dummy> * >(static_cast<void*>(&arg));
        static_assert( sizeof(arg) == sizeof(b) );

        b.emplace();  // This sets the 'has_value' to true

        assert( static_cast<void*>(&arg.value()) == static_cast<void*>(&b.value()) );

        return Helper( static_cast<void*>(&arg.value()), [&b](void)->void { b.reset(); } );
    }
} // close namespace 'detail_nrvo'

detail_nrvo::PutRetvalIn_Class PutRetvalIn(void *const arg)
{
    return detail_nrvo::PutRetvalIn_Class(arg);
}

detail_nrvo::PutRetvalIn_Class_nrvo nrvo_PutRetvalIn(void *const arg)
{
    return detail_nrvo::PutRetvalIn_Class_nrvo(arg);
}

template<class R>
detail_nrvo::PutRetvalIn_Class PutRetvalIn(std::optional<R> &arg)
{
    return detail_nrvo::PutRetvalIn_optional< R, detail_nrvo::PutRetvalIn_Class >(arg);
}

template<class R>
detail_nrvo::PutRetvalIn_Class_nrvo nrvo_PutRetvalIn(std::optional<R> &arg)
{
    return detail_nrvo::PutRetvalIn_optional< R, detail_nrvo::PutRetvalIn_Class_nrvo >(arg);
}

#endif  //  ifndef HEADER_INCLUSION_GUARD_NRVO_HPP
