
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2016 Dominik Lehmann
  
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
#include <limits>
using namespace ScorePress;


//
//     class Fraction
//    =================
//
// The fraction class implements all arithmetic operations on the field of
// rational numbers.
//

#ifdef SIZECHECK
#define _abs(x) (((x)<0l)?-(x):(x))             // macro calcualting the absolute value
#endif

// return the greatest common divisor
long Fraction::gcd(long x, long y)
{
    long t;                             // temporary swap variable
    if (x < 0l) x = -x;                 // ensure x to be positive
    if (y < 0l) y = -y;                 // ensure y to be positive
    if (x < y) {t = x; x = y; y = t;};  // ensure x to be the bigger one
    if (x == 0l) return 1l;             // special case x=y=0
    while (true)                        // run euclid's algorithm
        if      (y == 0l) return x;
        else if (y == 1l) return y;
        else              {t = y; y = x % y; x = t;};
}

// create from an integer
Fraction::Fraction(const long x) : enumerator(x), denominator(1l) {}

// create from a pair of enumerator and denominator
Fraction::Fraction(const long enu, const long deno) : enumerator(enu), denominator(deno)
{
    const long r = gcd(enu, deno);
    enumerator  /= r;
    denominator /= r;
}

// create from a mixed fraction
Fraction::Fraction(const long x, const long enu, const long deno) : enumerator((deno < 0l) ? -enu : enu), denominator((deno < 0l) ? -deno : deno)
{
    const long r = gcd(enumerator, denominator);
    enumerator  /= r;
    denominator /= r;
#ifdef SIZECHECK
    if (   (enumerator > 0 && x > (numeric_limits<long>::max() - enumerator) / denominator)
        || (enumerator < 0 && x < (numeric_limits<long>::min() - enumerator) / denominator))
    {
        enumerator = _sgn(x);
        denominator = 0l;
    }
    else
#endif
    {
        enumerator += x * denominator;
    };
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
    denominator = 1l;
    return *this;
}

// sum of two fractions
Fraction& Fraction::operator += (const Fraction& fract)
{
    // handle infinite values
    if (denominator == 0l) return *this;
    if (fract.denominator == 0l) {denominator = 0l; enumerator = fract.enumerator; return *this;};
    
    // calculate enumerator and denominator
    const long r = gcd(denominator, fract.denominator);

#ifdef SIZECHECK
    if (_abs(enumerator) > numeric_limits<long>::max() / (fract.denominator / r))
    {
        enumerator = _sgn(enumerator);
        if (denominator > numeric_limits<long>::max() / (fract.denominator / r))
            denominator = 1l;
        else
            denominator = 0l;
        return *this;
    };
    if (denominator > numeric_limits<long>::max() / (fract.denominator / r))
    {
        enumerator = 0l;
        denominator = 1l;
        return *this;
    }
#endif    
    
    enumerator *= fract.denominator / r;

#ifdef SIZECHECK
    if (   (enumerator > 0 && fract.enumerator > (numeric_limits<long>::max() - enumerator) / (denominator / r))
        || (enumerator < 0 && fract.enumerator < (numeric_limits<long>::min() - enumerator) / (denominator / r)))
    {
        enumerator = _sgn(fract.enumerator);
        denominator = 0l;
        return *this;
    };
#endif

    enumerator += fract.enumerator * (denominator / r);
    denominator *= fract.denominator / r;
    
    return *this;   // return this instance
}

// sum of fraction and number
Fraction& Fraction::operator += (const long fract)
{
    // handle infinite values
    if (denominator == 0) return *this;
    

#ifdef SIZECHECK
    if (   (enumerator > 0 && fract > (numeric_limits<long>::max() - enumerator) / denominator)
        || (enumerator < 0 && fract < (numeric_limits<long>::min() - enumerator) / denominator))
    {
        enumerator = _sgn(fract);
        denominator = 0l;
    }
    else
#endif
    {
        enumerator += fract * denominator;
    };
    return *this;   // return this instance
}

// difference of two fractions
Fraction& Fraction::operator -= (const Fraction& fract)
{
    // handle infinite values
    if (denominator == 0l) return *this;
    if (fract.denominator == 0l) {denominator = 0l; enumerator = fract.enumerator; return *this;};
    
    // calculate enumerator and denominator
    const long r = gcd(denominator, fract.denominator);

#ifdef SIZECHECK
    if (_abs(enumerator) > numeric_limits<long>::max() / (fract.denominator / r))
    {
        enumerator = _sgn(enumerator);
        if (denominator > numeric_limits<long>::max() / (fract.denominator / r))
            denominator = 1l;
        else
            denominator = 0l;
        return *this;
    };
    if (denominator > numeric_limits<long>::max() / (fract.denominator / r))
    {
        enumerator = 0l;
        denominator = 1l;
        return *this;
    }
#endif    
    
    enumerator *= fract.denominator / r;

#ifdef SIZECHECK
    if (   (enumerator > 0 && -fract.enumerator > (numeric_limits<long>::max() - enumerator) / (denominator / r))
        || (enumerator < 0 && -fract.enumerator < (numeric_limits<long>::min() - enumerator) / (denominator / r)))
    {
        enumerator = _sgn(-fract.enumerator);
        denominator = 0l;
        return *this;
    };
#endif

    enumerator -= fract.enumerator * (denominator / r);
    denominator *= fract.denominator / r;
    
    return *this;   // return this instance
}

// difference of fraction and number
Fraction& Fraction::operator -= (const long fract)
{
    // handle infinite values
    if (denominator == 0) return *this;
    

#ifdef SIZECHECK
    if (   (enumerator > 0 && -fract > (numeric_limits<long>::max() - enumerator) / denominator)
        || (enumerator < 0 && -fract < (numeric_limits<long>::min() - enumerator) / denominator))
    {
        enumerator = _sgn(-fract);
        denominator = 0l;
    }
    else
#endif
    {
        enumerator -= fract * denominator;
    };
    return *this;   // return this instance
}

// product of two fractions
Fraction& Fraction::operator *= (const Fraction& fract)
{
    // handle infinite values
    if (denominator == 0l)
    {
        if (enumerator == 0l) return *this;
        
        if      (fract.enumerator == 0)                      enumerator = 0l;
        else if ((enumerator > 0) == (fract.enumerator > 0)) enumerator = 1l;
        else                                                 enumerator = -1l;
        return *this;
    };
    if (fract.denominator == 0l)
    {
        enumerator = ((enumerator > 0) == (fract.enumerator > 0) ? 1l : -1l);
        denominator = 0l;
        return *this;
    };
    
    // calculate enumerator and denominator
    const long r1 = gcd(enumerator, fract.denominator);
    const long r2 = gcd(fract.enumerator, denominator);
    
    enumerator  /= r1;
    denominator /= r2;
    
#ifdef SIZECHECK
    if ((enumerator > 0) == (fract.enumerator > 0) &&  _abs(enumerator) > numeric_limits<long>::max() / (_abs(fract.enumerator) / r2))
    {
        enuemrator = 1l;
        denominator = 0l;
    }
    else if ((enumerator > 0) != (fract.enumerator > 0) && -_abs(enumerator) > numeric_limits<long>::min() / (_abs(fract.enumerator) / r2))
    {
        enuemrator = -1l;
        denominator = 0l;
    }
    else if ((enumerator > 0) == (fract.enumerator > 0) &&  _abs(enumerator) > numeric_limits<long>::max() / (_abs(fract.enumerator) / r2))
    {
        enuemrator = 1l;
        denominator = 0l;
    }
    else if ((enumerator > 0) != (fract.enumerator > 0) && -_abs(enumerator) > numeric_limits<long>::min() / (_abs(fract.enumerator) / r2))
    {
        enuemrator = -1l;
        denominator = 0l;
    }
    else
#endif
    {
        enumerator  *= fract.enumerator / r2;
        denominator *= fract.denominator / r1;
    };
    
    return *this;   // return this instance
}

// product of fraction and number
Fraction& Fraction::operator *= (const long fract)
{
    // handle infinite values
    if (denominator == 0) {enumerator = ((enumerator > 0) == (fract > 0) ? 1l : -1l); return *this;};
    
    // calculate enumerator and denominator
    const long r = gcd(fract, denominator);
    
#ifdef SIZECHECK
    if ((enumerator > 0) == (fract > 0) &&  _abs(enumerator) > numeric_limits<long>::max() / (_abs(fract) / r))
    {
        enuemrator = 1l;
        denominator = 0l;
    }
    else if ((enumerator > 0) != (fract > 0) && -_abs(enumerator) > numeric_limits<long>::min() / (_abs(fract) / r))
    {
        enuemrator = -1l;
        denominator = 0l;
    }
    else
#endif
    {
        enumerator  *= fract / r;
        denominator /= r;
    };
    
    return *this;   // return this instance
}

// quotient of two fractions
Fraction& Fraction::operator /= (const Fraction& fract)
{
    // handle infinite values
    if (denominator == 0l)
    {
        if (enumerator == 0l) return *this;
        
        if      (fract.denominator == 0)                     enumerator = 0l;
        else if ((enumerator > 0) == (fract.enumerator > 0)) enumerator = 1l;
        else                                                 enumerator = -1l;
        return *this;
    };
    if (fract.enumerator == 0l)
    {
        if      (enumerator > 0l) enumerator = 1l;
        else if (enumerator < 0l) enumerator = -1l;
        denominator = 0l;
        return *this;
    };
    
    // calculate enumerator and denominator
    const long r1 = gcd(enumerator, fract.enumerator);
    const long r2 = gcd(fract.denominator, denominator);
    
    enumerator  /= r1;
    denominator /= r2;
    
#ifdef SIZECHECK
    if ((enumerator > 0) == (fract.denominator > 0) &&  _abs(enumerator) > numeric_limits<long>::max() / (_abs(fract.denominator) / r2))
    {
        enuemrator = 1l;
        denominator = 0l;
    }
    else if ((enumerator > 0) != (fract.denominator > 0) && -_abs(enumerator) > numeric_limits<long>::min() / (_abs(fract.denominator) / r2))
    {
        enuemrator = -1l;
        denominator = 0l;
    }
    else if ((enumerator > 0) == (fract.denominator > 0) &&  _abs(enumerator) > numeric_limits<long>::max() / (_abs(fract.denominator) / r2))
    {
        enuemrator = 1l;
        denominator = 0l;
    }
    else if ((enumerator > 0) != (fract.denominator > 0) && -_abs(enumerator) > numeric_limits<long>::min() / (_abs(fract.denominator) / r2))
    {
        enuemrator = -1l;
        denominator = 0l;
    }
    else
#endif
    {
        if (fract.enumerator < 0l)
        {
            enumerator  *= -fract.denominator / r2;
            denominator *= -fract.enumerator / r1;
        }
        else
        {
            enumerator  *= fract.denominator / r2;
            denominator *= fract.enumerator / r1;
        };
    };
    
    return *this;   // return this instance
}

// quotient of fraction and number
Fraction& Fraction::operator /= (const long fract)
{
    // handle infinite values
    if (denominator == 0l)
    {
        if (enumerator == 0l) return *this;
        
        else if ((enumerator > 0) == (fract > 0)) enumerator = 1l;
        else                                      enumerator = -1l;
        return *this;
    };
    if (fract == 0l)
    {
        if      (enumerator > 0l) enumerator = 1l;
        else if (enumerator < 0l) enumerator = -1l;
        denominator = 0l;
        return *this;
    };
    
    // calculate enumerator and denominator
    const long r = gcd(fract, enumerator);
    
#ifdef SIZECHECK
    if (fract > 0 &&  denominator > numeric_limits<long>::max() / (_abs(fract) / r))
    {
        enuemrator = 1l;
        denominator = 0l;
    }
    else if (fract < 0 && -denominator > numeric_limits<long>::min() / (_abs(fract) / r))
    {
        enuemrator = -1l;
        denominator = 0l;
    }
    else
#endif
    {
        if (fract > 0l)
        {
            enumerator  /= r;
            denominator *= fract / r;
        }
        else
        {
            enumerator  /= -r;
            denominator *= -fract / r;
        };
    };
    
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
    if (fract.denominator == 0l &&  // check if both fractions are infinite
        denominator == 0l &&        // and check sign
        ((enumerator < 0l && fract.enumerator < 0l) || (enumerator > 0l && fract.enumerator > 0l))) return true;
    
    return (enumerator == fract.enumerator && denominator == fract.denominator);    // else check normal equality
}

bool Fraction::operator == (const long fract) const
{
    return (enumerator == fract && denominator == 1l);  // check integer equality
}

// check inequality
bool Fraction::operator != (const Fraction& fract) const
{
    if (enumerator == fract.enumerator && denominator == fract.denominator) return false;   // check for normal equality
    return (fract.denominator != 0l ||  // else check for infinite equality
            denominator != 0l ||        // and sign
            ((enumerator >= 0l || fract.enumerator >= 0l) && (enumerator <= 0l || fract.enumerator <= 0l)));
}

bool Fraction::operator != (const long fract) const
{
    return (enumerator != fract || denominator != 1l);  // check integer inequality
}

// check less or equal
bool Fraction::operator <= (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator <= 0l);}
bool Fraction::operator <= (const long      fract) const {return ((Fraction(*this) -= fract).enumerator <= 0l);}

// check greater or equal
bool Fraction::operator >= (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator >= 0l);}
bool Fraction::operator >= (const long      fract) const {return ((Fraction(*this) -= fract).enumerator >= 0l);}

// check less
bool Fraction::operator < (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator < 0l);}
bool Fraction::operator < (const long      fract) const {return ((Fraction(*this) -= fract).enumerator < 0l);}

// check greater
bool Fraction::operator > (const Fraction& fract) const {return ((Fraction(*this) -= fract).enumerator > 0l);}
bool Fraction::operator > (const long      fract) const {return ((Fraction(*this) -= fract).enumerator > 0l);}

// cast operators
Fraction::operator double() const {return static_cast<double>(enumerator) / static_cast<double>(denominator);}

// setting methods
void Fraction::set(const long enu, const long deno)
{
    if (deno < 0l)
    {
        enumerator = -enu;
        denominator = -deno;
    }
    else
    {
        enumerator = enu;
        denominator = deno;
    };
    const long r = gcd(enumerator, denominator);
    enumerator  /= r;
    denominator /= r;
}

void Fraction::set(const long x, const long enu, const long deno)
{
    if (deno < 0l)
    {
        enumerator = -enu;
        denominator = -deno;
    }
    else
    {
        enumerator = enu;
        denominator = deno;
    };
    const unsigned long r = gcd(enumerator, denominator);
    enumerator  /= r;
    denominator /= r;
#ifdef SIZECHECK
    if (   (enumerator > 0 && x > (numeric_limits<long>::max() - enumerator) / denominator)
        || (enumerator < 0 && x < (numeric_limits<long>::min() - enumerator) / denominator))
    {
        enumerator = _sgn(x);
        denominator = 0l;
    }
    else
#endif
    {
        enumerator += x * denominator;
    };
}

// static constants
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"

const Fraction Fraction::POS_INFINITY(1, 0);    // positive infinite fraction
const Fraction Fraction::NEG_INFINITY(-1, 0);   // negative infinite fraction
const Fraction Fraction::NDN(0, 0);             // not a number (for values as 0/0 or INF/INF)

#pragma clang diagnostic pop
