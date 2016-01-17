// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::PrintTable
// Description
//   Print a table in parallel, e.g.;
//   \verbatim
//               Vertex Type Information
//          Proc #        0       1       2       3
//           Total   145680  145278  145751  145359
//        Unassigned        0       0       0       0
//   nExternalFeatureEdge      883     829     828     960
//   nExternalFeaturePoint        8      10      10      12
//     nExternalSurface     9533    9488    9502    9482
//           nFar        0       0       0       0
//         nInternal   125494  125198  125642  125174
//   nInternalFeatureEdge      238     241     241     240
//   nInternalFeaturePoint        2       2       2       2
//   nInternalNearBoundary        0       0       0       0
//     nInternalSurface     9522    9510    9526    9489
//         nReferred     7545    7497    7500    7587
//   \endverbatim
// SourceFiles
//   print_table_i.hpp
//   print_table.cpp
#ifndef print_table_hpp_
#define print_table_hpp_
#include "hash_table.hpp"
#include "ostream.hpp"
namespace mousse
{
template<class KeyType, class DataType>
class PrintTable
{
  typedef HashTable<DataType, KeyType> HashTableData;
  // Private data
    //- Hash table holding the data
    HashTableData table_;
    //- Title of the table
    string title_;
  // Private Member Functions
    //- Disallow default bitwise assignment
    void operator=(const PrintTable<KeyType, DataType>&);
public:
  // Constructors
    //- Null constructor
    PrintTable();
    //- Construct with a title
    explicit PrintTable(const string& title);
    //- Copy constructor
    PrintTable(const PrintTable<KeyType, DataType>& table);
  //- Destructor
  ~PrintTable();
  // Member Functions
    //- Add an entry (D) to the given key(K)
    void add(const KeyType& K, const DataType& D);
    //- Print the table
    void print
    (
      Ostream& os,
      const bool printSum = false,
      const bool printAverage = false
    ) const;
};
}  // namespace mousse
#include "print_table_i.hpp"
#ifdef NoRepository
  #include "print_table.cpp"
#endif
#endif