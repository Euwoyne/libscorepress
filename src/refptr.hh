
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2012 Dominik Lehmann
  
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

#ifndef SCOREPRESS_REFPTR_HH
#define SCOREPRESS_REFPTR_HH

namespace ScorePress
{
//  CLASSES
// ---------
template <typename T>
class RefPtr;           // a simple reference counting smart pointer


//
//     class RefPtr
//    ==============
//
// Implementation of a simple reference counting smart pointer.
//
template <typename T> class RefPtr
{
 private:
    T* data;                // data pointer
    unsigned int* count;    // reference counter
    
    void dealloc();         // deallocate a reference (decrease count)
    
 public:
    RefPtr();                       // default constructor (NULL-pointer)
    RefPtr(const RefPtr<T>& ptr);   // copy constructor (increase count)
    explicit RefPtr(T* const ptr);  // raw-pointer wrapper
    ~RefPtr();                      // destructor
    
    // dereferece operators
    T& operator * () const {return *data;};
    T* operator-> () const {return data;};
    
    // assignment operator
    RefPtr<T>& operator = (const RefPtr<T>& ptr);
    
    // logical operators
    bool operator ! () const;   // NULL check
    
    template <typename U> friend bool operator == (const U* ptr1, const RefPtr<U>& ptr2);
    template <typename U> friend bool operator != (const U* ptr1, const RefPtr<U>& ptr2);
    template <typename U> friend bool operator == (const RefPtr<U>& ptr1, const U* ptr2);
    template <typename U> friend bool operator != (const RefPtr<U>& ptr1, const U* ptr2);
    
    template <typename U> bool operator == (const RefPtr<U>& ptr) const;
    template <typename U> bool operator != (const RefPtr<U>& ptr) const;
    
    // pointer cast (for cast to boolean, preventing cast to integral type)
    operator void* () const;
    
    // maintenance functions
    template <typename U> friend U*           getRawPtr(const RefPtr<U>& ptr);   // return internal raw-pointer
    template <typename U> friend unsigned int getRefCount(const RefPtr<U>& ptr); // return reference-count
    template <typename U> friend void         alloc(const RefPtr<U>& ptr);       // allocate new object
    template <typename U> friend void         free(RefPtr<U>& ptr);              // deallocate reference
};

// method implementations
template <typename T>
void RefPtr<T>::dealloc() {if (count && !--*count) {delete data; delete count;}; count = 0; data = 0;}

template <typename T>
inline RefPtr<T>::RefPtr() : data(0), count(0) {}

template <typename T>
inline RefPtr<T>::RefPtr(const RefPtr<T>& ptr) : data(ptr.data), count(ptr.count) {if (count) ++*count;}

template <typename T>
inline RefPtr<T>::RefPtr(T* const ptr) : data(ptr), count(ptr ? new unsigned int(1) : 0) {}

template <typename T>
inline RefPtr<T>::~RefPtr() {dealloc();}

template <typename T>
inline RefPtr<T>& RefPtr<T>::operator = (const RefPtr<T>& ptr)
{if (data != ptr.data) {dealloc(); data = ptr.data; if ((count = ptr.count)) ++*count;}; return *this;}

template <typename T>
inline bool RefPtr<T>::operator ! () const {return (data == 0 || count == 0);}

template <typename T>
inline bool operator == (const T* ptr1, const RefPtr<T>& ptr2) {return ptr1 == ptr2.data;}

template <typename T>
inline bool operator != (const T* ptr1, const RefPtr<T>& ptr2) {return ptr1 != ptr2.data;}

template <typename T>
inline bool operator == (const RefPtr<T>& ptr1, const T* ptr2) {return ptr1.data == ptr2;}

template <typename T>
inline bool operator != (const RefPtr<T>& ptr1, const T* ptr2) {return ptr1.data != ptr2;}

template <typename T> template <typename U>
inline bool RefPtr<T>::operator == (const RefPtr<U>& ptr) const {return data == ptr.data;}

template <typename T> template <typename U>
inline bool RefPtr<T>::operator != (const RefPtr<U>& ptr) const {return data != ptr.data;}

template <typename T>
inline RefPtr<T>::operator void* () const {return data;}

template <typename T>
inline T* getRawPtr(const RefPtr<T>& ptr) {return ptr.data;}

template <typename T>
inline unsigned int getRefCount(const RefPtr<T>& ptr) {if (ptr.count) return *ptr.count; else return 0;}

template <typename T>
inline void alloc(const RefPtr<T>& ptr) {ptr.dealloc(); ptr.count = new unsigned int(1); ptr.data = new T();}

template <typename T>
inline void free(RefPtr<T>& ptr) {ptr.dealloc();}

} // end namespace

#endif

