#ifndef Podd_Helper
#define Podd_Helper

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::Helper                                                              //
//                                                                           //
// Helper classes and functions                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <vector>
#include <cassert>
#include <functional>
#include <algorithm>
#include <iterator>

namespace Podd {

  //___________________________________________________________________________
  template< typename VectorElem > inline void
  NthCombination( UInt_t n, const std::vector<std::vector<VectorElem> >& vec,
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

    UInt_t vsiz = vec.size(), k;
    selected.resize( vsiz );
    typename vector< vector<VectorElem> >::const_iterator iv = vec.begin();
    typename vector<VectorElem>::iterator is = selected.begin();
    while( iv != vec.end() ) {
      typename std::vector<VectorElem>::size_type npt = (*iv).size();
      assert(npt);
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
  template< typename Container > struct SizeMul :
    public std::binary_function< typename Container::size_type, Container,
				 typename Container::size_type >
  {
    // Function object to get the product of the sizes of the containers in a
    // container, ignoring empty ones
    typedef typename Container::size_type csiz_t;
    csiz_t operator() ( csiz_t val, const Container& c ) const
    { return ( c.empty() ? val : val * c.size() ); }
  };

  //___________________________________________________________________________
  // Iterator over unique combinations of k out of N elements. The element
  // numbers are accessible in a vector<int> of size k.
  class UniqueCombo {
    typedef std::vector<int> vint_t;
  public:
    UniqueCombo( int N, int k ) : fN(N), fGood(true)
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
    const UniqueCombo operator++(int)
    { UniqueCombo clone(*this); ++*this; return clone; }
    const vint_t& operator()() const { return fCurrent; }
    const vint_t& operator*()  const { return fCurrent; }
    bool operator==( const UniqueCombo& rhs ) const
    { return ( fN == rhs.fN and fCurrent == rhs.fCurrent ); }
    bool operator!=( const UniqueCombo& rhs ) const { return !(*this==rhs); }
    operator bool() const  { return fGood; }
    bool operator!() const { return !((bool)*this); }

  private:
    bool recursive_plus( vint_t::size_type pos ) {
      if( fCurrent[pos] < fN+int(pos-fCurrent.size()) ) {
	++fCurrent[pos];
	return true;
      }
      if( pos==0 )
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
  struct DeleteObject {
    template< typename T >
    void operator() ( const T* ptr ) const { delete ptr; }
  };

  //___________________________________________________________________________
  template< typename Container >
  inline void DeleteContainer( Container& c )
  {
    // Delete all elements of given container of pointers
    for_each( c.begin(), c.end(), DeleteObject() );
    c.clear();
  }

  //___________________________________________________________________________
  template< typename ContainerOfContainers >
  inline void DeleteContainerOfContainers( ContainerOfContainers& cc )
  {
    // Delete all elements of given container of containers of pointers
    for_each( cc.begin(), cc.end(),
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
    return (((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
  }

  //_____________________________________________________________________________
  template <typename VectorElem>
  void PrintArray( const std::vector<VectorElem>& arr )
  {
    if( arr.empty() ) {
      std::cout << "(empty)";
    } else {
      std::copy( arr.begin(), --arr.end(),
          std::ostream_iterator<VectorElem>(std::cout, ", ") );
      std::cout << arr.back();
    }
    std::cout << std::endl;
  }

///////////////////////////////////////////////////////////////////////////////

} // end namespace TreeSearch


#endif
