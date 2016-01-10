// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "ulist.hpp"
#include "ostream.hpp"
#include "token.hpp"
#include "sl_list.hpp"
#include "contiguous.hpp"

// Ostream Operator
template<class T>
void mousse::UList<T>::writeEntry(Ostream& os) const
{
  if (size() 
      && token::compound::isCompound
         (
           "List<" + word(pTraits<T>::typeName) + '>'
         ))
  {
    os  << word("List<" + word(pTraits<T>::typeName) + '>') << " ";
  }
  os << *this;
}


template<class T>
void mousse::UList<T>::writeEntry(const word& keyword, Ostream& os) const
{
  os.writeKeyword(keyword);
  writeEntry(os);
  os << token::END_STATEMENT << endl;
}


template<class T>
mousse::Ostream& mousse::operator<<(mousse::Ostream& os, const mousse::UList<T>& L)
{
  // Write list contents depending on data format
  if (os.format() == IOstream::ASCII || !contiguous<T>())
  {
    bool uniform = false;
    if (L.size() > 1 && contiguous<T>())
    {
      uniform = true;
      FOR_ALL(L, i)
      {
        if (L[i] != L[0])
        {
          uniform = false;
          break;
        }
      }
    }
    if (uniform)
    {
      // Write size and start delimiter
      os << L.size() << token::BEGIN_BLOCK;
      // Write contents
      os << L[0];
      // Write end delimiter
      os << token::END_BLOCK;
    }
    else if (L.size() <= 1 || (L.size() < 11 && contiguous<T>()))
    {
      // Write size and start delimiter
      os << L.size() << token::BEGIN_LIST;
      // Write contents
      FOR_ALL(L, i)
      {
        if (i > 0) os << token::SPACE;
        os << L[i];
      }
      // Write end delimiter
      os << token::END_LIST;
    }
    else
    {
      // Write size and start delimiter
      os << nl << L.size() << nl << token::BEGIN_LIST;
      // Write contents
      FOR_ALL(L, i)
      {
        os << nl << L[i];
      }
      // Write end delimiter
      os << nl << token::END_LIST << nl;
    }
  }
  else
  {
    os << nl << L.size() << nl;
    if (L.size())
    {
      os.write(reinterpret_cast<const char*>(L.v_), L.byteSize());
    }
  }
  // Check state of IOstream
  os.check("Ostream& operator<<(Ostream&, const UList&)");
  return os;
}


template<class T>
mousse::Istream& mousse::operator>>(Istream& is, UList<T>& L)
{
  is.fatalCheck("operator>>(Istream&, UList<T>&)");
  token firstToken{is};
  is.fatalCheck("operator>>(Istream&, UList<T>&) : reading first token");
  if (firstToken.isCompound())
  {
    List<T> elems;
    elems.transfer
    (
      dynamicCast<token::Compound<List<T>>>
      (
        firstToken.transferCompoundToken(is)
      )
    );
    // Check list length
    label s = elems.size();
    if (s != L.size())
    {
      FATAL_IO_ERROR_IN("operator>>(Istream&, UList<T>&)", is)
        << "incorrect length for UList. Read " << s
        << " expected " << L.size()
        << exit(FatalIOError);
    }
    for (label i=0; i<s; i++)
    {
      L[i] = elems[i];
    }
  }
  else if (firstToken.isLabel())
  {
    label s = firstToken.labelToken();
    // Set list length to that read
    if (s != L.size())
    {
      FATAL_IO_ERROR_IN("operator>>(Istream&, UList<T>&)", is)
        << "incorrect length for UList. Read " << s
        << " expected " << L.size()
        << exit(FatalIOError);
    }
    // Read list contents depending on data format
    if (is.format() == IOstream::ASCII || !contiguous<T>())
    {
      // Read beginning of contents
      char delimiter = is.readBeginList("List");
      if (s)
      {
        if (delimiter == token::BEGIN_LIST)
        {
          for (label i=0; i<s; i++)
          {
            is >> L[i];
            is.fatalCheck
            (
              "operator>>(Istream&, UList<T>&) : reading entry"
            );
          }
        }
        else
        {
          T element;
          is >> element;
          is.fatalCheck
          (
            "operator>>(Istream&, UList<T>&) : "
            "reading the single entry"
          );
          for (label i=0; i<s; i++)
          {
            L[i] = element;
          }
        }
      }
      // Read end of contents
      is.readEndList("List");
    }
    else
    {
      if (s)
      {
        is.read(reinterpret_cast<char*>(L.data()), s*sizeof(T));
        is.fatalCheck
        (
          "operator>>(Istream&, UList<T>&) : reading the binary block"
        );
      }
    }
  }
  else if (firstToken.isPunctuation())
  {
    if (firstToken.pToken() != token::BEGIN_LIST)
    {
      FATAL_IO_ERROR_IN("operator>>(Istream&, UList<T>&)", is)
        << "incorrect first token, expected '(', found "
        << firstToken.info()
        << exit(FatalIOError);
    }
    // Putback the opening bracket
    is.putBack(firstToken);
    // Now read as a singly-linked list
    SLList<T> sll(is);
    if (sll.size() != L.size())
    {
      FATAL_IO_ERROR_IN("operator>>(Istream&, UList<T>&)", is)
        << "incorrect length for UList. Read " << sll.size()
        << " expected " << L.size()
        << exit(FatalIOError);
    }
    // Convert the singly-linked list to this list
    label i = 0;
    for
    (
      typename SLList<T>::const_iterator iter = sll.begin();
      iter != sll.end();
      ++iter
    )
    {
      L[i] = iter();
    }
  }
  else
  {
    FATAL_IO_ERROR_IN("operator>>(Istream&, UList<T>&)", is)
      << "incorrect first token, expected <int> or '(', found "
      << firstToken.info()
      << exit(FatalIOError);
  }
  return is;
}
