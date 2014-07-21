
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
  Licensed under the EUPL, Version 1.1 or - as soon they
  will be approved by the European Commission - subsequent
  versions of the EUPL (the "Licence");
  You may not use this work except in compliance with the
  Licence. You may obtain a copy of the Licence at
  <http://ec.europa.eu/idabc/eupl/>.
  
  Unless required by applicable law or agreed to in
  writing, software distributed under the Licence is
  distributed on an "AS IS" basis, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either expressed or implied.
  See the Licence for the specific language governing
  permissions and limitations under the Licence.
*/

#ifndef SCOREPRESS_FRACTION_HH
#define SCOREPRESS_FRACTION_HH

#include "export.hh"

namespace ScorePress
{
//  CLASSES
// ---------
class SCOREPRESS_API Fraction;    // class representing a rational number


//
//     class Fraction
//    ================
//
// The fraction class implements all arithmetic operations on the field of
// rational numbers.
//
class SCOREPRESS_API Fraction
{
 private:
    long enumerator;            // the enumerator
    unsigned long denominator;  // the denominator
    
    // return the greatest common divisor
    SCOREPRESS_LOCAL static unsigned long gcd(unsigned long x, unsigned long y);
    
 public:
    // constructors
    Fraction(const long x = 0);                             // create from an integer
    Fraction(const long enu, const unsigned long deno);     // create from a pair of enumerator and denominator
    Fraction(const long x, const long enu, const unsigned long deno);   // create from a mixed fraction
    Fraction(const Fraction& fract);                        // copy constructor
    
    // assignment operators
    Fraction& operator = (const Fraction& fract);   // assign value
    Fraction& operator = (const long fract);        // assign integer value
    
    // arithmetic operators
    Fraction& operator += (const Fraction& fract);  // sum of two fractions
    Fraction& operator += (const long fract);       // sum of fraction and number
    Fraction& operator -= (const Fraction& fract);  // difference of two fractions
    Fraction& operator -= (const long fract);       // difference of fraction and number
    Fraction& operator *= (const Fraction& fract);  // product of two fractions
    Fraction& operator *= (const long fract);       // product of fraction and number
    Fraction& operator /= (const Fraction& fract);  // quotient of two fractions
    Fraction& operator /= (const long fract);       // quotient of fraction and number
    Fraction& operator %= (const Fraction& fract);  // modulo operator for two fractions
    Fraction& operator %= (const long fract);       // modulo operator for fraction and number
    
    // equality operators
    bool operator == (const Fraction& fract) const; // check equality
    bool operator == (const long fract) const;
    bool operator != (const Fraction& fract) const; // check inequality
    bool operator != (const long fract) const;
    bool operator <= (const Fraction& fract) const; // check less or equal
    bool operator <= (const long fract) const;
    bool operator >= (const Fraction& fract) const; // check greater or equal
    bool operator >= (const long fract) const;
    bool operator < (const Fraction& fract) const;  // check less
    bool operator < (const long fract) const;
    bool operator > (const Fraction& fract) const;  // check greater
    bool operator > (const long fract) const;
    
    // cast operators
    operator double() const;        // casts to floating point values
    inline double real() const {return operator double();};
    
    // access methods
    long i() const;                     // returns the integral part of the mixed fraction
    long e_short() const;               // returns the enumerator of the mixed fraction
    long e() const;                     // returns the internal enumerator
    long d() const;                     // returns the internal denominator
    Fraction abs() const;               // returns the absolute value
    
    // setting methods
    void set(const long enu, const long deno);                  // set internal values
    void set(const long x, const long enu, const long deno);    // convert a mixed fraction
    
    // static constants
    static const Fraction POS_INFINITY; // positive infinite fraction
    static const Fraction NEG_INFINITY; // negative infinite fraction
    static const Fraction NDN;          // not a number (for values as 0/0 or INF/INF)
};

// access methods
inline long     Fraction::i()       const {return (denominator == 0) ? enumerator : enumerator / denominator;}
inline long     Fraction::e_short() const {return (denominator != 0) ? (enumerator - (enumerator / denominator) * denominator) : (enumerator);}
inline long     Fraction::e()       const {return enumerator;}
inline long     Fraction::d()       const {return denominator;}
inline Fraction Fraction::abs()     const {return Fraction((enumerator >= 0) ? enumerator : -enumerator, denominator);}

// arithmetic operators
inline Fraction operator + (const Fraction& f1, const Fraction& f2) {return Fraction(f1) += f2;}
inline Fraction operator + (const Fraction& f1, const long      f2) {return Fraction(f1) += f2;}
inline Fraction operator + (const long      f1, const Fraction& f2) {return Fraction(f1) += f2;}
inline Fraction operator - (const Fraction& f)                      {return Fraction(-f.e(), f.d());}
inline Fraction operator - (const Fraction& f1, const Fraction& f2) {return Fraction(f1) -= f2;}
inline Fraction operator - (const Fraction& f1, const long      f2) {return Fraction(f1) -= f2;}
inline Fraction operator - (const long      f1, const Fraction& f2) {return Fraction(f1) -= f2;}
inline Fraction operator * (const Fraction& f1, const Fraction& f2) {return Fraction(f1) *= f2;}
inline Fraction operator * (const Fraction& f1, const long      f2) {return Fraction(f1) *= f2;}
inline Fraction operator * (const long      f1, const Fraction& f2) {return Fraction(f1) *= f2;}
inline Fraction operator / (const Fraction& f1, const Fraction& f2) {return Fraction(f1) /= f2;}
inline Fraction operator / (const Fraction& f1, const long      f2) {return Fraction(f1) /= f2;}
inline Fraction operator / (const long      f1, const Fraction& f2) {return Fraction(f1) /= f2;}
inline Fraction operator % (const Fraction& f1, const Fraction& f2) {return Fraction(f1) %= f2;}
inline Fraction operator % (const Fraction& f1, const long      f2) {return Fraction(f1) %= f2;}
inline Fraction operator % (const long      f1, const Fraction& f2) {return Fraction(f1) %= f2;}

} // end namespace

#endif

