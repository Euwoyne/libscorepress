
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2013 Dominik Lehmann
  
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

#ifndef SCOREPRESS_SMARTPTR_HH
#define SCOREPRESS_SMARTPTR_HH

namespace ScorePress
{
//  CLASSES
// ---------
template <typename T, template <typename> class trait>
class SmartPtr;                             // a simple deep-copy smart pointer
template <typename T> struct StdTrait;      // clone-trait class for "new"
template <typename T> struct CloneTrait;    // clone-trait class for "T::clone"
template <typename T> struct CopyTrait;     // clone-trait class for "T::copy"


//
//     class SmartPtr
//    ================
//
// Implementation of a simple deep-copy smart pointer.
//
template <typename T, template <typename> class trait = StdTrait> class SmartPtr
{
 private:
    T* data;
    
 public:
    SmartPtr();
    SmartPtr(const SmartPtr<T, trait>& ptr);
    explicit SmartPtr(T* const ptr);
    ~SmartPtr();
    
    T& operator * () const {return *data;};
    T* operator-> () const {return data;};
    
    SmartPtr<T, trait>& operator = (const SmartPtr<T, trait>& ptr);
    
    bool operator ! () const;
    
    template <typename U, template <typename> class traitU> friend bool operator == (const U* ptr1, const SmartPtr<U, traitU>& ptr2);
    template <typename U, template <typename> class traitU> friend bool operator != (const U* ptr1, const SmartPtr<U, traitU>& ptr2);
    template <typename U, template <typename> class traitU> friend bool operator == (const SmartPtr<U, traitU>& ptr1, const U* ptr2);
    template <typename U, template <typename> class traitU> friend bool operator != (const SmartPtr<U, traitU>& ptr1, const U* ptr2);
    
    template <typename U, template <typename> class traitU> bool operator == (const SmartPtr<U, traitU>& ptr) const;
    template <typename U, template <typename> class traitU> bool operator != (const SmartPtr<U, traitU>& ptr) const;
    
    operator void* () const;
    
    template <typename U, template <typename> class traitU> friend U* getRawPtr(const SmartPtr<U, traitU>& ptr);
    template <typename U, template <typename> class traitU> friend void alloc(const SmartPtr<U, traitU>& ptr);
    template <typename U, template <typename> class traitU> friend void free(SmartPtr<U, traitU>& ptr);
};

template <typename T, template <typename> class trait>
inline SmartPtr<T, trait>::SmartPtr() : data(0) {}

template <typename T, template <typename> class trait>
inline SmartPtr<T, trait>::SmartPtr(const SmartPtr<T, trait>& ptr) : data(trait<T>::clone(ptr.data)) {}

template <typename T, template <typename> class trait>
inline SmartPtr<T, trait>::SmartPtr(T* const ptr) : data(ptr) {}

template <typename T, template <typename> class trait>
inline SmartPtr<T, trait>::~SmartPtr() {delete data;}

template <typename T, template <typename> class trait>
inline SmartPtr<T, trait>& SmartPtr<T, trait>::operator = (const SmartPtr<T, trait>& ptr)
{if (data != ptr.data) {delete data; data = trait<T>::clone(ptr.data);}; return *this;}

template <typename T, template <typename> class trait>
inline bool SmartPtr<T, trait>::operator ! () const {return (data == 0);}

template <typename T, template <typename> class trait>
inline bool operator == (const T* ptr1, const SmartPtr<T, trait>& ptr2) {return ptr1 == ptr2.data;}

template <typename T, template <typename> class trait>
inline bool operator != (const T* ptr1, const SmartPtr<T, trait>& ptr2) {return ptr1 != ptr2.data;}

template <typename T, template <typename> class trait>
inline bool operator == (const SmartPtr<T, trait>& ptr1, const T* ptr2) {return ptr1.data == ptr2;}

template <typename T, template <typename> class trait>
inline bool operator != (const SmartPtr<T, trait>& ptr1, const T* ptr2) {return ptr1.data != ptr2;}

template <typename T,template <typename> class traitT>
template <typename U, template <typename> class traitU>
inline bool SmartPtr<T, traitT>::operator == (const SmartPtr<U, traitU>& ptr) const {return data == ptr.data;}

template <typename T,template <typename> class traitT>
template <typename U, template <typename> class traitU>
inline bool SmartPtr<T, traitT>::operator != (const SmartPtr<U, traitU>& ptr) const {return data != ptr.data;}

template <typename T, template <typename> class trait>
inline SmartPtr<T, trait>::operator void* () const {return data;}

template <typename T, template <typename> class trait>
inline T* getRawPtr(const SmartPtr<T, trait>& ptr) {return ptr.data;}

template <typename T, template <typename> class trait>
inline void alloc(const SmartPtr<T, trait>& ptr) {delete ptr.data; ptr.data = new T();}

template <typename T, template <typename> class trait>
inline void free(SmartPtr<T, trait>& ptr) {delete ptr.data; ptr.data = 0;}


// clone-trait class for "new"
template <typename T> struct StdTrait
{
    inline static T* clone(const T* obj) {return obj ? new T(*obj) : 0;} 
};

// clone-trait class for "T::clone"
template <typename T> struct CloneTrait
{
    inline static T* clone(const T* obj) {return obj ? obj->clone() : 0;} 
};

// clone-trait class for "T::copy"
template <typename T> struct CopyTrait
{
    inline static T* clone(const T* obj) {return obj ? obj->copy() : 0;} 
};


} // end namespace

#endif

