// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "error.hpp"
#include "uipstream.hpp"
#include "int.hpp"
#include "token.hpp"
#include "iostreams.hpp"
#include <cctype>
// Private Member Functions 
inline void mousse::UIPstream::checkEof()
{
  if (externalBufPosition_ == messageSize_)
  {
    setEof();
  }
}
template<class T>
inline void mousse::UIPstream::readFromBuffer(T& t)
{
  const size_t align = sizeof(T);
  externalBufPosition_ = align + ((externalBufPosition_ - 1) & ~(align - 1));
  t = reinterpret_cast<T&>(externalBuf_[externalBufPosition_]);
  externalBufPosition_ += sizeof(T);
  checkEof();
}
inline void mousse::UIPstream::readFromBuffer
(
  void* data,
  size_t count,
  size_t align
)
{
  if (align > 1)
  {
    externalBufPosition_ =
      align
     + ((externalBufPosition_ - 1) & ~(align - 1));
  }
  const char* bufPtr = &externalBuf_[externalBufPosition_];
  char* dataPtr = reinterpret_cast<char*>(data);
  size_t i = count;
  while (i--) *dataPtr++ = *bufPtr++;
  externalBufPosition_ += count;
  checkEof();
}
// Destructor 
mousse::UIPstream::~UIPstream()
{
  if (clearAtEnd_ && eof())
  {
    if (debug)
    {
      Pout<< "UIPstream::~UIPstream() : tag:" << tag_
        << " fromProcNo:" << fromProcNo_
        << " clearing externalBuf_ of size "
        << externalBuf_.size()
        << " messageSize_:" << messageSize_ << endl;
    }
    externalBuf_.clearStorage();
  }
}
// Member Functions 
mousse::Istream& mousse::UIPstream::read(token& t)
{
  // Return the put back token if it exists
  if (Istream::getBack(t))
  {
    return *this;
  }
  char c;
  // return on error
  if (!read(c))
  {
    t.setBad();
    return *this;
  }
  // Set the line number of this token to the current stream line number
  t.lineNumber() = lineNumber();
  // Analyse input starting with this character.
  switch (c)
  {
    // Punctuation
    case token::END_STATEMENT :
    case token::BEGIN_LIST :
    case token::END_LIST :
    case token::BEGIN_SQR :
    case token::END_SQR :
    case token::BEGIN_BLOCK :
    case token::END_BLOCK :
    case token::COLON :
    case token::COMMA :
    case token::ASSIGN :
    case token::ADD :
    case token::SUBTRACT :
    case token::MULTIPLY :
    case token::DIVIDE :
    {
      t = token::punctuationToken(c);
      return *this;
    }
    // Word
    case token::WORD :
    {
      word* pval = new word;
      if (read(*pval))
      {
        if (token::compound::isCompound(*pval))
        {
          t = token::compound::New(*pval, *this).ptr();
          delete pval;
        }
        else
        {
          t = pval;
        }
      }
      else
      {
        delete pval;
        t.setBad();
      }
      return *this;
    }
    // String
    case token::VERBATIMSTRING :
    {
      // Recurse to read actual string
      read(t);
      t.type() = token::VERBATIMSTRING;
      return *this;
    }
    case token::VARIABLE :
    {
      // Recurse to read actual string
      read(t);
      t.type() = token::VARIABLE;
      return *this;
    }
    case token::STRING :
    {
      string* pval = new string;
      if (read(*pval))
      {
        t = pval;
        if (c == token::VERBATIMSTRING)
        {
          t.type() = token::VERBATIMSTRING;
        }
      }
      else
      {
        delete pval;
        t.setBad();
      }
      return *this;
    }
    // Label
    case token::LABEL :
    {
      label val;
      if (read(val))
      {
        t = val;
      }
      else
      {
        t.setBad();
      }
      return *this;
    }
    // floatScalar
    case token::FLOAT_SCALAR :
    {
      floatScalar val;
      if (read(val))
      {
        t = val;
      }
      else
      {
        t.setBad();
      }
      return *this;
    }
    // doubleScalar
    case token::DOUBLE_SCALAR :
    {
      doubleScalar val;
      if (read(val))
      {
        t = val;
      }
      else
      {
        t.setBad();
      }
      return *this;
    }
    // Character (returned as a single character word) or error
    default:
    {
      if (isalpha(c))
      {
        t = word(c);
        return *this;
      }
      setBad();
      t.setBad();
      return *this;
    }
  }
}
mousse::Istream& mousse::UIPstream::read(char& c)
{
  c = externalBuf_[externalBufPosition_];
  externalBufPosition_++;
  checkEof();
  return *this;
}
mousse::Istream& mousse::UIPstream::read(word& str)
{
  size_t len;
  readFromBuffer(len);
  str = &externalBuf_[externalBufPosition_];
  externalBufPosition_ += len + 1;
  checkEof();
  return *this;
}
mousse::Istream& mousse::UIPstream::read(string& str)
{
  size_t len;
  readFromBuffer(len);
  str = &externalBuf_[externalBufPosition_];
  externalBufPosition_ += len + 1;
  checkEof();
  return *this;
}
mousse::Istream& mousse::UIPstream::read(label& val)
{
  readFromBuffer(val);
  return *this;
}
mousse::Istream& mousse::UIPstream::read(floatScalar& val)
{
  readFromBuffer(val);
  return *this;
}
mousse::Istream& mousse::UIPstream::read(doubleScalar& val)
{
  readFromBuffer(val);
  return *this;
}
mousse::Istream& mousse::UIPstream::read(char* data, std::streamsize count)
{
  if (format() != BINARY)
  {
    FATAL_ERROR_IN("UIPstream::read(char*, std::streamsize)")
      << "stream format not binary"
      << mousse::abort(FatalError);
  }
  readFromBuffer(data, count, 8);
  return *this;
}
mousse::Istream& mousse::UIPstream::rewind()
{
  externalBufPosition_ = 0;
  return *this;
}
void mousse::UIPstream::print(Ostream& os) const
{
  os  << "Reading from processor " << fromProcNo_
    << " using communicator " << comm_
    <<  " and tag " << tag_
    << mousse::endl;
}
