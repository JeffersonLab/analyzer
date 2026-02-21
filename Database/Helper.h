#ifndef Podd_Helper_
#define Podd_Helper_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Helper                                                              //
//                                                                           //
// Helper classes and functions                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"     // for UInt_t, Int_t
#include <cstddef>      // for size_t
#include <algorithm>    // for for_each, copy
#include <cassert>      // for assert
#include <iostream>     // for basic_ostream, operator<<, cout, char_traits
#include <iterator>     // for ostream_iterator, ssize
#include <stdexcept>    // for out_of_range
#include <type_traits>  // for make_signed_t, is_integral_v, is_signed_v
#include <vector>       // for vector, operator==
#include <utility>      // for cmp_less
#ifndef NDEBUG
# include <limits>      // for numeric_limits
#endif

#define ALL( c ) (c).begin(), (c).end()

namespace Podd {

//___________________________________________________________________________
  // Cast unsigned to signed where unsigned is expected to be within signed range
  template<typename T> requires (std::is_integral_v<T> and std::is_unsigned_v<T>)
  std::make_signed_t<T> SINT( T uint )
  {
#ifndef NDEBUG
    if( uint > static_cast<decltype(uint)>(
                 std::numeric_limits<std::make_signed_t<T>>::max()))
      throw std::out_of_range("Unsigned integer out of signed integer range");
#endif
    return static_cast<std::make_signed_t<T>>(uint);
  }

  // Trivial case
  template<typename T> requires (std::is_integral_v<T> and std::is_signed_v<T>)
  T SINT( T ival ) { return ival; }

  // For backward compatibility with existing code
  template<typename Container>
  auto SSIZE( const Container& c ) { return std::ssize(c); }

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
  inline Int_t NumberOfSetBits( UInt_t v )
  {
    // Count number of bits set in 32-bit integer. From
    // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel

    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return SINT((((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24);
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


///////////////////////////////////////////////////////////////////////////////

} // end namespace Podd

#endif  // Podd_Helper_
