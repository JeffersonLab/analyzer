#ifndef Podd_CustomAlloc_h_
#define Podd_CustomAlloc_h_

// Custom allocators for Decoder package

#include <memory>

namespace Decoder {
//--------------------------------------------------------------------------
// From https://stackoverflow.com/questions/21028299/is-this-behavior-of-vectorresizesize-type-n-under-c11-and-boost-container/21028912#21028912
// Allocator adaptor that interposes construct() calls to
// convert value initialization into default initialization.
template<typename T, typename A=std::allocator<T>>
class default_init_allocator : public A {
  typedef std::allocator_traits<A> a_t;
public:
  template<typename U>
  struct rebind {
    using other =
    default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
  };

  using A::A;

  template<typename U>
  void construct( U* ptr )
  noexcept(std::is_nothrow_default_constructible_v<U>) {
    ::new(static_cast<void*>(ptr)) U;
  }
  template<typename U, typename...Args>
  void construct( U* ptr, Args&& ... args ) {
    a_t::construct(static_cast<A&>(*this),
                   ptr, std::forward<Args>(args)...);
  }
};

//--------------------------------------------------------------------------
template<typename T, size_t L=alignof(T), typename A=std::allocator<T>>
class aligned_default_init_allocator : public A {
  typedef std::allocator_traits<A> a_t;
public:
  using A::A;

  T* allocate( size_t n ) {
    return static_cast<T*>(
      ::operator new(n * sizeof(T), static_cast<std::align_val_t>(L))
    );
  }

  void deallocate( T* ptr, size_t ) noexcept {
    ::operator delete(ptr, static_cast<std::align_val_t>(L));
  }

  template<typename U> struct
  rebind {
    using other =
      aligned_default_init_allocator<U, L, typename a_t::template rebind_alloc<U>>;
  };

  template<typename U>
  void construct( U* ptr )
  noexcept(std::is_nothrow_default_constructible_v<U>) {
    ::new(static_cast<void*>(ptr)) U;
  }

  template<typename U, typename...Args>
  void construct( U* ptr, Args&& ... args ) {
    a_t::construct(static_cast<A&>(*this), ptr, std::forward<Args>(args)...);
  }
};

//--------------------------------------------------------------------------
using VectorUInt = std::vector<UInt_t>;
// std::vector that does NOT zero-initialize its elements on resize()
using VectorUIntNI = std::vector<UInt_t, default_init_allocator<UInt_t>>;

} // namespace Decoder

#endif //Podd_CustomAlloc_h_
