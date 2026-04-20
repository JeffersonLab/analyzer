#ifndef Podd_Helper_
#define Podd_Helper_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Helper                                                              //
//                                                                           //
// Helper classes and functions                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"      // for UInt_t, Int_t, Long64_t
#include <algorithm>     // for for_each, copy
#include <cassert>       // for assert
#include <charconv>      // for from_chars
#include <cstddef>       // for size_t
#include <iostream>      // for ostream, cout, char_traits, operator<<, endl
#include <iterator>      // for ostream_iterator, ssize
#include <limits>        // for numeric_limits
#include <stdexcept>     // for out_of_range
#include <string_view>   // for string_view
#include <system_error>  // for errc
#include <type_traits>   // for is_integral_v, is_same_v, is_floating_point_v, make_signed_t, is_signed_v, is_unsigned_v
#include <utility>       // for cmp_greater, cmp_less, in_range
#include <vector>        // for vector, operator==
#include <version>       // for __cpp_lib_bitops
#if __cpp_lib_bitops >= 201907L
#include <bit>           // for popcount
#endif

#define ALL( c ) (c).begin(), (c).end()

namespace Podd {

//___________________________________________________________________________
  // Cast unsigned to signed where unsigned is expected to be within signed range
  template<typename T> requires (std::is_integral_v<T> and std::is_unsigned_v<T>)
  constexpr std::make_signed_t<T> SINT( T uint )
  {
    if( std::cmp_greater(uint,
      std::numeric_limits<std::make_signed_t<T>>::max()) ) [[unlikely]]
      throw std::out_of_range("Unsigned integer out of signed integer range");
    return static_cast<std::make_signed_t<T>>(uint);
  }

  // Trivial case
  template<typename T> requires (std::is_integral_v<T> and std::is_signed_v<T>)
  T SINT( T ival ) { return ival; }

  // For backward compatibility with existing code
  template<typename Container>
  auto SSIZE( const Container& c ) { return std::ssize(c); }

  // Force safe conversion to int from any integer
  template<typename T>
    requires (not std::is_same_v<T,int> and std::is_integral_v<T>)
  constexpr int ToInt( T val )
  {
    if( !std::in_range<int>(val) ) [[unlikely]]
      throw std::out_of_range("Integer value out of int range");
    return static_cast<int>(val);
  }
  // Trivial case - no-op
  template<typename T> requires std::is_same_v<T,int>
  constexpr int ToInt( T sint ) { return sint; }

  //___________________________________________________________________________
  // Error-detecting conversion of string to integer (any type).
  // Returns 0 on success, 1 on error.
  template<typename T> requires std::is_integral_v<T>
  Int_t ParseInt( const std::string_view str, T& val )
  {
    constexpr auto noerr = std::errc{}; // NOLINT(*-invalid-enum-default-initialization)
    auto [ptr, ec] = std::from_chars(ALL(str), val);
    return ptr != str.end() || ec != noerr;
  }


  //___________________________________________________________________________
  template< typename VectorElem >
  void NthCombination( UInt_t n, const std::vector<std::vector<VectorElem>>& vec,
                       std::vector<VectorElem>& selected )
  {
    // Get the n-th combination of the elements in "vec" and
    // put result in "selected". selected[k] is one of the
    // vec[k].size() elements in the k-th plane.
    //
    // This function is equivalent to vec.size() nested/recursive loops
    // over the elements of each of the vectors in vec.

    using std::vector;

    assert( !vec.empty() );

    selected.resize( vec.size() );
    typename vector< vector<VectorElem> >::const_iterator iv = vec.begin();
    typename vector<VectorElem>::iterator is = selected.begin();
    while( iv != vec.end() ) {
      typename std::vector<VectorElem>::size_type npt = (*iv).size();
      assert(npt);
      UInt_t k;
      if( npt == 1 )
	k = 0;
      else {
	k = n % npt;
	n /= npt;
      }
      // Copy the selected element
      (*is) = (*iv)[k];
      ++iv;
      ++is;
    }
  }

  //___________________________________________________________________________
  template<typename Container>
  struct SizeMul
  {
    // Function object to get the product of the sizes of the containers in a
    // container, ignoring empty ones
    typedef Container::size_type csiz_t;
    csiz_t operator()( csiz_t val, const Container& c ) const
    {
      return (c.empty() ? val : val * c.size());
    }
  };

  //___________________________________________________________________________
  // Iterator over unique combinations of k out of N elements. The element
  // numbers are accessible in a vector<int> of size k.
  class UniqueCombo {
    typedef std::vector<int> vint_t;
  public:
    UniqueCombo( const int N, const int k ) : fN(N), fGood(true)
    { assert( 0<=k && k<=N ); for( int i=0; i<k; ++i ) fCurrent.push_back(i); }
    // Default copy and assignment are fine

    UniqueCombo& operator++()
    {
      if( fCurrent.empty() )
	fGood = false;
      else
	fGood = recursive_plus( fCurrent.size()-1 );
      return *this;
    }
    UniqueCombo operator++(int)
    {
      UniqueCombo clone(*this); ++*this; return clone;
    }
    const vint_t& operator()() const { return fCurrent; }
    const vint_t& operator*()  const { return fCurrent; }
    bool operator==( const UniqueCombo& rhs ) const
    {
      return ( fN == rhs.fN and fCurrent == rhs.fCurrent );
    }
    bool operator!=( const UniqueCombo& rhs ) const { return !(*this==rhs); }
    explicit operator bool() const  { return fGood; }
    bool operator!() const { return !static_cast<bool>(*this); }

  private:
    bool recursive_plus( const vint_t::size_type pos ) { // NOLINT(misc-no-recursion)
      if( std::cmp_less(fCurrent[pos], fN+pos-fCurrent.size()) ) {
	++fCurrent[pos];
	return true;
      }
      if( pos == 0 )
	return false;
      if( recursive_plus(pos-1) ) {
	fCurrent[pos] = fCurrent[pos-1]+1;
	return true;
      }
      return false;
    }
    int    fN;
    vint_t fCurrent;
    bool   fGood;
  };

  //___________________________________________________________________________
  class DeleteObject {
  public:
    template<typename T>
    void operator()( const T* ptr ) const { delete ptr; }
  };

  //___________________________________________________________________________
  template<typename Container>
  void DeleteContainer( Container& c )
  {
    // Delete all elements of given container of pointers
    std::for_each( ALL(c), DeleteObject() );
    c.clear();
  }

  //___________________________________________________________________________
  template<typename ContainerOfContainers>
  void DeleteContainerOfContainers( ContainerOfContainers& cc )
  {
    // Delete all elements of given container of containers of pointers
    std::for_each( ALL(cc),
	      DeleteContainer<typename ContainerOfContainers::value_type> );
    cc.clear();
  }

  //___________________________________________________________________________
  constexpr Int_t NumberOfSetBits( UInt_t v )
  {
    // Count number of bits set in 32-bit integer
#if __cpp_lib_bitops >= 201907L
    return std::popcount(v);
#else
    // From
    // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return SINT((((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24);
#endif
  }

  //___________________________________________________________________________
  // Return the value of the bits in the range [first,last] (inclusive).
  // The number of bits considered is last-first+1.
  template<typename T> requires std::is_unsigned_v<T>
  constexpr T bitval(const T b, const unsigned first, const unsigned last)
  {
    if( first >= sizeof(T) << 3U || last < first ) return 0U;
    if( last + 1 >= sizeof(T) << 3U ) return b >> first;
    return (b & (1ULL << (last + 1)) - 1) >> first;
  }

  //___________________________________________________________________________
  template<typename Container>
  void PrintArray( const char* name, const Container& c )
  {
    std::cout << name << " = ";
    if( c.empty() ) {
      std::cout << "(empty)";
    } else {
      auto end1 = c.end(); --end1;
      std::copy(c.begin(), end1,
        std::ostream_iterator<typename Container::value_type>(std::cout, ", "));
      std::cout << *c.rbegin();
    }
    std::cout << std::endl;
  }

  //___________________________________________________________________________
  // Helper function to print a single field of a structure in a container/range
  template<typename Container, typename Proj>
  void PrintArrayField( const char* name, const Container& c, Proj p )
  {
    size_t sz = c.size(), k = 0;
    auto prnt = [sz, &k]( const auto& f ) {
      std::cout << f;
      if( ++k < sz ) std::cout << ", ";
    };
    std::cout << name << " = ";
    if( c.empty() )
      std::cout << "(empty)";
    else
      std::ranges::for_each(c, prnt, p);
    std::cout << std::endl;
  }

  //___________________________________________________________________________
  // Create a std::vector from a C-array of Item structures
  template<class Item>
  std::vector<Item> MakeVectorFromList( const Item* list)
  {
    if( const Item* item = list ) {
      while( item->name ) ++item;
      return {list, item};
    }
    return {};
  }

  //_____________________________________________________________________________
  // Function to check if source value of type S will fit into target value
  // of type T. T and S must either both be integer or floating point types.
  // Trivial case of identical types.
  template<typename T, typename S> requires std::is_same_v<T, S>
  constexpr bool is_in_range( S )
  {
    return true;
  }

  //_____________________________________________________________________________
  // Non-trivial case of different types.
  template<typename T, typename S> requires (not std::is_same_v<T, S> and (
    (std::is_integral_v<T> and std::is_integral_v<S> ) or
    (std::is_floating_point_v<T> and std::is_floating_point_v<S>)))
  constexpr bool is_in_range( S val )
  {
    bool ret = (val <= std::numeric_limits<T>::max());
    if( ret ) {
      if( std::is_integral_v<T> ) {
        if( std::is_signed_v<S> ) {
          if( std::is_unsigned_v<T> )
            ret = (val >= 0);
          else
            ret = (val >= std::numeric_limits<T>::min());
        }
      } else if( std::is_floating_point_v<T> )
        ret = (val >= -std::numeric_limits<T>::max());
    }
    return ret;
  }

  //_____________________________________________________________________________
  // Get number of printable characters of integer number 'n'
  constexpr UInt_t IntDigits( Long64_t n )
  {
    if( n == 0 ) return 1;
    UInt_t j = 0;
    if( n < 0 ) {
      ++j;
      n = -n;
    }
    while( n != 0 ) {
      n /= 10;
      ++j;
    }
    return j;
  }

///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

#endif  // Podd_Helper_
