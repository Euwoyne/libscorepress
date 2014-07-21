
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

#include "fraction.hh"    // Fraction
#include <climits>
using namespace ScorePress;


//
//     class Fraction
//    =================
//
// The fraction class implements all arithmetic operations on the field of
// rational numbers.
//

#define _abs(x) (((x)<0)?-(x):(x))          // macro calcualting the absolute value
#define _sgn(x) (((x)<0)?-1:(((x)>0)?1:0))  // macro returning the signum

// return the greatest common divisor
unsigned long Fraction::gcd(unsigned long x, unsigned long y)
{
    unsigned long t;                    // temporary swap variable
    if (x < y) {t = x; x = y; y = t;};  // ensure x to be the bigger one
    if (x == 0) return 1;               // special case x=y=0
    while (true)                        // run euclid's algorithm
        if      (y == 0) return x;
        else if (y == 1) return y;
        else            {t = y; y = x % y; x = t;};
}

// create from an integer
Fraction::Fraction(const long x) : enumerator(x), denominator(1) {}

// create from a pair of enumerator and denominator
Fraction::Fraction(const long enu, const unsigned long deno) : enumerator(enu), denominator(deno)
{
    const unsigned long r = gcd(_abs(enu), deno);
    enumerator  /= r;
    denominator /= r;
}

// create from a mixed fraction
Fraction::Fraction(const long x, const long enu, const unsigned long deno) : enumerator(enu), denominator(deno)
{
    const unsigned long r = gcd(_abs(enu), deno);
    enumerator  /= r;
    denominator /= r;
#    ifdef SIZECHECK
        long long t(x);
        t *= denominator;
        t += enumerator;
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);
#    else
        enumerator += x * denominator;
#    endif
}

// copy constructor
Fraction::Fraction(const Fraction& fract) : enumerator(fract.enumerator), denominator(fract.denominator) {}

// assign value
Fraction& Fraction::operator = (const Fraction& fract)
{
    enumerator = fract.enumerator;
    denominator = fract.denominator;
    return *this;
}

// assign integer value
Fraction& Fraction::operator = (const long fract)
{
    enumerator = fract;
    denominator = 1;
    return *this;
}

// sum of two fractions
Fraction& Fraction::operator += (const Fraction& fract)
{
    // handle infinite values
    if (denominator == 0) return *this;
    if (fract.denominator == 0) {denominator = 0; enumerator = fract.enumerator; return *this;};
    
    // calculate enumerator and denominator
    const unsigned long r = gcd(denominator, fract.denominator);
#    ifdef SIZECHECK
        {long long t(enumerator);
        t *= fract.denominator / r;
        t += static_cast<long long>(fract.enumerator) * (denominator / r);
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);}
        
        {unsigned long long t(denominator);
        t *= fract.denominator / r;
        if (t > ULONG_MAX)          // unsigned long sizecheck
        {
            enumerator = 0;
            denominator = 1;
        }
        else denominator = static_cast<unsigned long>(t);}
#    else
        enumerator  *= fract.denominator / r;
        enumerator  += fract.enumerator * (denominator / r);
        denominator *= fract.denominator / r;
#    endif
        
    return *this;   // return this instance
}

// sum of fraction and number
Fraction& Fraction::operator += (const long fract)
{
    // handle infinite values
    if (denominator == 0) return *this;
    
#    ifdef SIZECHECK
        long long t(fract);
        t *= denominator;
        t += enumerator;
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);
#    else
        enumerator += fract * denominator;
#    endif
    
    return *this;   // return this instance
}

// difference of two fractions
Fraction& Fraction::operator -= (const Fraction& fract)
{
    // handle infinite values
    if (denominator == 0) return *this;
    if (fract.denominator == 0) {denominator = 0; enumerator = -fract.enumerator; return *this;};
    
    // calculate enumerator and denominator
    const unsigned long r = gcd(denominator, fract.denominator);
#    ifdef SIZECHECK
        {long long t(enumerator);
        t *= fract.denominator / r;
        t -= static_cast<long long>(fract.enumerator) * (denominator / r);
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);}
        
        {unsigned long long t(denominator);
        t *= fract.denominator / r;
        if (t > ULONG_MAX)          // unsigned long sizecheck
        {
            enumerator = 0;
            denominator = 1;
        }
        else denominator = static_cast<unsigned long>(t);}
#    else
        enumerator  *= fract.denominator / r;
        enumerator  -= fract.enumerator * (denominator / r);
        denominator *= fract.denominator / r;
#    endif
        
    return *this;   // return this instance
}

// difference of fraction and number
Fraction& Fraction::operator -= (const long fract)
{
    // handle infinite values
    if (denominator == 0) return *this;
    
#    ifdef SIZECHECK
        long long t(fract);
        t *= denominator;
        t = enumerator - t;
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);
#    else
        enumerator -= fract * denominator;
#    endif
    
    return *this;   // return this instance
}

// product of two fractions
Fraction& Fraction::operator *= (const Fraction& fract)
{
    // handle infinite values
    if (denominator == 0) {enumerator = _sgn(enumerator * fract.enumerator); return *this;};
    if (fract.denominator == 0) {enumerator = fract.enumerator; denominator = 0; return *this;};
    
    // calculate enumerator and denominator
    const unsigned long r1 = gcd(_abs(enumerator), fract.denominator);
    const unsigned long r2 = gcd(_abs(fract.enumerator), denominator);
#    ifdef SIZECHECK
        {long long t(enumerator / r1);
        t *= fract.enumerator / r2;
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);}
        
        {unsigned long long t(denominator / r2);
        t *= fract.denominator / r1;
        if (t > ULONG_MAX)          // unsigned long sizecheck
        {
            enumerator = 0;
            denominator = 1;
        }
        else denominator = static_cast<long>(t);}
#    else
        enumerator  = (enumerator / r1) * (fract.enumerator / r2);
        denominator = (denominator / r2) * (fract.denominator / r1);
#    endif
    
    return *this;   // return this instance
}

// product of fraction and number
Fraction& Fraction::operator *= (const long fract)
{
    // handle infinite values
    if (denominator == 0) {enumerator = _sgn(enumerator*fract); return *this;};
    
    // calculate enumerator and denominator
    const unsigned long r = gcd(_abs(fract), denominator);
#    ifdef SIZECHECK
        {long long t(enumerator);
        t *= fract / r;
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);}
        denominator /= r;
#    else
        enumerator  *= fract / r;
        denominator /= r;
#    endif
    
    return *this;   // return this instance
}

// quotient of two fractions
Fraction& Fraction::operator /= (const Fraction& fract)
{
    // handle infinite values
    if (enumerator == 0 && denominator == 0) return *this;
    if (fract.enumerator == 0 && fract.denominator == 0) {enumerator = 0; denominator = 0; return *this;};
    if (denominator == 0 && fract.enumerator == 0) return *this;
    if (enumerator == 0 && fract.denominator == 0) return *this;
    if (denominator == 0 && fract.denominator == 0) {enumerator = 0; return *this;};
    if (enumerator == 0 && fract.enumerator == 0) {denominator = 0; return *this;};
    
    // calculate enumerator and denominator
    const unsigned long r1 = gcd(_abs(enumerator), fract.enumerator);
    const unsigned long r2 = gcd(fract.denominator, denominator);
#    ifdef SIZECHECK
        {long long t(enumerator / r1);
        t *= fract.denominator / r2;
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = (fract.enumerator < 0) ? -_sgn(t) : _sgn(t);
            denominator = 0;
        }
        else enumerator = (fract.enumerator < 0) ? -static_cast<long>(t) : static_cast<long>(t);}
        
        {unsigned long long t(denominator / r2);
        t *= _abs(fract.enumerator) / r1;
        if (t > ULONG_MAX)                  // unsigned long sizecheck
        {
            enumerator = 0;
            denominator = 1;
        }
        else denominator = static_cast<long>(t);}
#    else
        enumerator  = (enumerator / r1) * (fract.denominator / r2);
        if (fract.enumerator < 0) enumerator = -enumerator;
        denominator = (denominator / r2) * (fract.enumerator / r1);
#    endif
    
    return *this;   // return this instance
}

// quotient of fraction and number
Fraction& Fraction::operator /= (const long fract)
{
    // calculate enumerator and denominator
    const unsigned long r = gcd(enumerator, _abs(fract));
#    ifdef SIZECHECK
        enumerator /= r;
        if (fract < 0) enumerator = -enumerator;
        {unsigned long long t(denominator);
        t *= _abs(fract) / r;
        if (t > ULONG_MAX)      // unsigned long sizecheck
        {
            enumerator = 0;
            denominator = 1;
        }
        else denominator = static_cast<long>(t);}
#    else
        enumerator /= r;
        if (fract < 0) enumerator = -enumerator;
        denominator *= _abs(fract) / r;
#    endif
    
    return *this;   // return this instance
}

// modulo operator for two fractions
Fraction& Fraction::operator %= (const Fraction& fract)
{
    Fraction temp(*this);       // copy instance
    temp /= fract;              // execute division
    *this -= fract * temp.i();  // calculate reminder
    return *this;               // return this instance
}

// modulo operator for fraction and number
Fraction& Fraction::operator %= (const long fract)
{
    Fraction temp(*this);       // copy instance
    temp /= fract;              // execute division
    *this -= fract * temp.i();  // calculate reminder
    return *this;               // return this instance
}

// check equality
bool Fraction::operator == (const Fraction& fract) const
{
    if (    fract.denominator == 0 &&   // check if both fractions are infinite
        denominator == 0 &&             // and check sign
        ((enumerator < 0 && fract.enumerator < 0) || (enumerator > 0 && fract.enumerator > 0))) return true;
    
    return (enumerator == fract.enumerator && denominator == fract.denominator);    // else check normal equality
}

bool Fraction::operator == (const long fract) const
{
    return (enumerator == fract && denominator == 1);   // check integer equality
}

// check inequality
bool Fraction::operator != (const Fraction& fract) const
{
    if (enumerator == fract.enumerator && denominator == fract.denominator) return false;   // check for normal equality
    return (    fract.denominator != 0 ||   // else check for infinite equality
            denominator != 0 ||             // and sign
            ((enumerator >= 0 || fract.enumerator >= 0) && (enumerator <= 0 || fract.enumerator <= 0)));
}

bool Fraction::operator != (const long fract) const
{
    return (enumerator != fract || denominator != 1);   // check integer inequality
}

// check less or equal
bool Fraction::operator <= (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator <= 0);}
bool Fraction::operator <= (const long fract) const {return ((Fraction(*this) -= fract).enumerator <= 0);}

// check greater or equal
bool Fraction::operator >= (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator >= 0);}
bool Fraction::operator >= (const long fract) const {return ((Fraction(*this) -= fract).enumerator >= 0);}

// check less
bool Fraction::operator < (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator < 0);}
bool Fraction::operator < (const long fract) const {return ((Fraction(*this) -= fract).enumerator < 0);}

// check greater
bool Fraction::operator > (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator > 0);}
bool Fraction::operator > (const long fract) const {return ((Fraction(*this) -= fract).enumerator > 0);}

// cast operators
Fraction::operator double() const {return static_cast<double>(enumerator) / denominator;}

// setting methods
void Fraction::set(const long enu, const long deno)
{
    enumerator = enu;    // set values
    denominator = deno;
    const unsigned long r = gcd(_abs(enu), deno);
    enumerator  /= r;
    denominator /= r;
}

void Fraction::set(const long x, const long enu, const long deno)
{
    enumerator = enu;
    denominator = deno;
    const unsigned long r = gcd(_abs(enu), deno);
    enumerator  /= r;
    denominator /= r;
#    ifdef SIZECHECK
        long long t(x);
        t *= denominator;
        t += enumerator;
        if ((t < 0) ? (t < LONG_MIN) : (t > LONG_MAX))  // long sizecheck
        {
            enumerator = _sgn(t);
            denominator = 0;
        }
        else enumerator = static_cast<long>(t);
#    else
        enumerator += x * denominator;
#    endif
}

// static constants
const Fraction Fraction::POS_INFINITY(1, 0);    // positive infinite fraction
const Fraction Fraction::NEG_INFINITY(-1, 0);   // negative infinite fraction
const Fraction Fraction::NDN(0, 0);             // not a number (for values as 0/0 or INF/INF)

