
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
  Licensed under the EUPL, Version 1.1 or - as soon they
  will be approved by the European Commission - subsequent
  versions of the EUPL (the "Licence");
  You may not use this work except in compliance with the
  Licence.
  
  Unless required by applicable law or agreed to in
  writing, software distributed under the Licence is
  distributed on an "AS IS" basis, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either expressed or implied.
  See the Licence for the specific language governing
  permissions and limitations under the Licence.
*/

#ifndef SCOREPRESS_BASETYPES_HH
#define SCOREPRESS_BASETYPES_HH

#include <string>          // std::string
#include "fraction.hh"     // Fraction
#include "export.hh"

namespace ScorePress
{
//  TYPE DEFINTIONS
// -----------------
typedef int           mpx_t;       // milli-pixel (graphical positioning)
typedef unsigned char tone_t;      // note-number (as defined for the MIDI standard: a' = 69)
typedef Fraction      value_t;     // note-values will be represented by exact fractions ("double" is too imprecise)

// note value base exponent (i.e. the exponent of a whole note)
static const int VALUE_BASE = 7;

//  BASE CLASSES
// --------------
template <typename T>
class                 Position;     // graphical position (2-dimensional vector)
struct SCOREPRESS_API Color;        // RGBA-color structure
struct SCOREPRESS_API Font;         // font structure

// graphical position (2-dimensional vector)
template <typename T = int> class Position
{
 public:
    T x; T y;
    Position(const T _x = 0, const T _y = 0) : x(_x), y(_y) {};
    Position& operator += (const Position& p) {x += p.x; y += p.y; return *this;};
    Position& operator -= (const Position& p) {x -= p.x; y -= p.y; return *this;};
    Position& operator *= (const T& p)        {x *= p;   y *= p;   return *this;};
    Position& operator /= (const T& p)        {x /= p;   y /= p;   return *this;};
};

template <typename T> Position<T> operator + (const Position<T>& p, const Position<T>& q) {return Position<T>(p.x + q.x, p.y + q.y);}
template <typename T> Position<T> operator - (const Position<T>& p, const Position<T>& q) {return Position<T>(p.x - q.x, p.y - q.y);}
template <typename T> Position<T> operator * (const Position<T>& p, const T& q)           {return Position<T>(p.x * q,   p.y * q  );}
template <typename T> Position<T> operator * (const T& p,           const Position<T>& q) {return Position<T>(p   * q.x, p   * q.y);}
template <typename T> Position<T> operator / (const Position<T>& p, const T& q)           {return Position<T>(p.x / q,   p.y / q  );}

// RGBA-color structure
struct SCOREPRESS_API Color{unsigned char r; unsigned char g; unsigned char b; unsigned char a;};

inline bool operator == (const Color& c1, const Color& c2) {
    return (c1.r == c2.r) && (c1.g == c2.g) && (c1.b == c2.b) && (c1.a == c2.a);}

// font structure
struct SCOREPRESS_API Font{std::string family; double size; bool bold; bool italic; bool underline; Color color;};

} // end namespace

#endif

