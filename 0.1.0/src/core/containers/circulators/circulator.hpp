// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::Circulator
// Description
//   Walks over a container as if it were circular. The container must have the
//   following members defined:
//     - value_type
//     - size_type
//     - difference_type
//     - iterator
//     - reference
//   Examples
//   \code
//     face f(identity(5));
//     // Construct Circulator from the face
//     Circulator<face> circ(f);
//     // First check that the Circulator has a size to iterate over.
//     // Then circulate around the list starting and finishing at the fulcrum.
//     if (circ.size()) do
//     {
//       circ() += 1;
//       Info<< "Iterate forwards over face : " << circ() << endl;
//     } while (circ.circulate(CirculatorBase::CLOCKWISE));
//   \endcode
// SourceFiles
//   circulator_i.hpp
#ifndef circulator_hpp_
#define circulator_hpp_
#include "circulator_base.hpp"
namespace mousse
{
template<class ContainerType>
class Circulator
:
  public CirculatorBase
{
protected:
  // Protected data
    //- Iterator pointing to the beginning of the container
    typename ContainerType::iterator begin_;
    //- Iterator pointing to the end of the container
    typename ContainerType::iterator end_;
    //- Random access iterator for traversing ContainerType.
    typename ContainerType::iterator iter_;
    //- Iterator holding the location of the fulcrum (start and end) of
    //  the container. Used to decide when the iterator should stop
    //  circulating over the container
    typename ContainerType::iterator fulcrum_;
public:
  // STL type definitions
    //- Type of values ContainerType contains.
    typedef typename ContainerType::value_type      value_type;
    //- The type that can represent the size of ContainerType
    typedef typename ContainerType::size_type       size_type;
    //- The type that can represent the difference between any two
    //  iterator objects.
    typedef typename ContainerType::difference_type difference_type;
    //- Random access iterator for traversing ContainerType.
    typedef typename ContainerType::iterator        iterator;
    //- Type that can be used for storing into
    //  ContainerType::value_type objects.
    typedef typename ContainerType::reference       reference;
  // Constructors
    //- Construct null
    inline Circulator();
    //- Construct from a container.
    inline explicit Circulator(ContainerType& container);
    //- Construct from two iterators
    inline Circulator(const iterator& begin, const iterator& end);
    //- Construct as copy
    inline Circulator(const Circulator<ContainerType>&);
  //- Destructor
  ~Circulator();
  // Member Functions
    //- Return the range of the iterator
    inline size_type size() const;
    //- Circulate around the list in the given direction
    inline bool circulate(const CirculatorBase::direction dir = NONE);
    //- Set the fulcrum to the current position of the iterator
    inline void setFulcrumToIterator();
    //- Set the iterator to the current position of the fulcrum
    inline void setIteratorToFulcrum();
    //- Return the distance between the iterator and the fulcrum. This is
    //  equivalent to the number of rotations of the Circulator.
    inline difference_type nRotations() const;
    //- Dereference the next iterator and return
    inline reference next() const;
    //- Dereference the previous iterator and return
    inline reference prev() const;
  // Member Operators
    //- Assignment operator for Circulators that operate on the same
    //  container type
    inline void operator=(const Circulator<ContainerType>&);
    //- Prefix increment. Increments the iterator.
    //  Sets the iterator to the beginning of the container if it reaches
    //  the end
    inline Circulator<ContainerType>& operator++();
    //- Postfix increment. Increments the iterator.
    //  Sets the iterator to the beginning of the container if it reaches
    //  the end
    inline Circulator<ContainerType> operator++(int);
    //- Prefix decrement. Decrements the iterator.
    //  Sets the iterator to the end of the container if it reaches
    //  the beginning
    inline Circulator<ContainerType>& operator--();
    //- Postfix decrement. Decrements the iterator.
    //  Sets the iterator to the end of the container if it reaches
    //  the beginning
    inline Circulator<ContainerType> operator--(int);
    //- Check for equality of this iterator with another iterator that
    //  operate on the same container type
    inline bool operator==(const Circulator<ContainerType>& c) const;
    //- Check for inequality of this iterator with another iterator that
    //  operate on the same container type
    inline bool operator!=(const Circulator<ContainerType>& c) const;
    //- Dereference the iterator and return
    inline reference operator*() const;
    //- Dereference the iterator and return
    inline reference operator()() const;
    //- Return the difference between this iterator and another iterator
    //  that operate on the same container type
    inline difference_type operator-
    (
      const Circulator<ContainerType>& c
    ) const;
};
}  // namespace mousse
#include "circulator_i.hpp"
#endif