#ifndef _KEY_MUTABLE_SET_H_
#define _KEY_MUTABLE_SET_H_
#include <set>
#include <assert.h>
#include <list>
template <class T> class MutableShell {
	mutable T t;
public:
	MutableShell (const T &t) : t(t) {}
	T &get ()const {return t;}
	T &operator* ()const {return t;}
	T *operator-> ()const {return &t;}
	operator T& () const {return t;}
	bool operator< (const MutableShell <T>& other) const {return t<other.t;}
};

///This set inherits from the STL multiset, with a slight variation: The value is nonconst--that means you are allowed to change things but must not alter the key.
///This set inherits from the STL multiset, with a slight variation: You are allowed to update the key of a particular iterator that you have obtained.
/** Note: T is the type that each element is pointing to. */
template <class T, class _Compare = std::less <MutableShell<T> > >
class KeyMutableSet : public std::multiset <MutableShell<T>,_Compare> {
	typedef std::multiset <MutableShell<T>,_Compare> SUPER;
public:
	/// This just checks the order of the set for testing purposes..
	void checkSet () {
		_Compare comparator;
		if (this->begin()!=this->end()) {
			for (typename SUPER::iterator newiter=this->begin(), iter=newiter++;newiter!=this->end();iter=newiter++) {
                          assert(!comparator(*newiter,*iter));
			}
		}
	}
	///Given an iterator you can alter that iterator's key to be the one passed in.
	/** The type must have a function called changeKey(const Key &newKey) that
		changes its key to the specified new key.
	 */

	void changeKey (typename SUPER::iterator &iter, const T & newKey, typename SUPER::iterator &templess, typename SUPER::iterator &rettempmore) {
        MutableShell<T> newKeyShell(newKey);
        templess=rettempmore=iter;
        ++rettempmore;
        typename SUPER::iterator tempmore=rettempmore;
        if (tempmore==this->end())
          --tempmore;
        if (templess!=this->begin())
          --templess;

        _Compare comparator;

        //O(1) amortized time on the insertion - Yippie!
        bool byebye=false;
        if (comparator(newKeyShell,*templess)) {
          rettempmore=templess=this->insert(templess,newKeyShell); 
          byebye = true;
        } else if (comparator(*tempmore,newKeyShell)) {
          rettempmore=templess=this->insert(tempmore,newKeyShell); 
          byebye = true;
        } else (*iter).get()=newKey;

        if (byebye) {
          this->erase(iter);
          iter=templess;
          ++rettempmore;
          if (templess!=this->begin())
            --templess;
        }

		//return iter;
	}

	typename SUPER::iterator changeKey (typename SUPER::iterator iter, const T & newKey) {
		typename SUPER::iterator templess=iter,tempmore=iter;
		changeKey(iter,newKey,templess,tempmore);
                return iter;
	}
	typename SUPER::iterator insert (const T & newKey,typename SUPER::iterator hint) {

		return this->SUPER::insert(newKey);
                 
	}
	typename SUPER::iterator insert (const T & newKey) {

		return this->SUPER::insert(newKey);
                 
	}
};



template <class T, class _Compare = std::less <T > >
class ListMutableSet : public std::list <T> {
	typedef std::list <T> SUPER;
public:
	/// This just checks the order of the set for testing purposes..
	void checkSet () {
		_Compare comparator;
		if (this->begin()!=this->end()) {
			for (typename SUPER::iterator newiter=this->begin(), iter=newiter++;newiter!=this->end();iter=newiter++) {
                          assert(!comparator(*newiter,*iter));
			}
		}
	}
	///Given an iterator you can alter that iterator's key to be the one passed in.
	/** The type must have a function called changeKey(const Key &newKey) that
		changes its key to the specified new key.
	 */
	void changeKey (typename SUPER::iterator &iter, const T & newKey, typename SUPER::iterator &templess, typename SUPER::iterator &rettempmore) {
		MutableShell<T> newKeyShell(newKey);
		templess=rettempmore=iter;
		++rettempmore;
                typename SUPER::iterator tempmore=rettempmore;
                if (tempmore==this->end())
                  --tempmore;
		if (templess!=this->begin())
                  --templess;
		_Compare comparator;
		if (comparator(newKeyShell,*templess)||comparator(*tempmore,newKeyShell)) {
                  rettempmore=templess=iter=this->insert (newKeyShell,this->erase(iter));
                  ++rettempmore;
                  if (templess!=this->begin())
                    --templess;                  
		}else {
                  **iter=newKey;
		}
		//return iter;
	}

	typename SUPER::iterator changeKey (typename SUPER::iterator iter, const T & newKey) {
		typename SUPER::iterator templess=iter,tempmore=iter;
		changeKey(iter,newKey,templess,tempmore);
                return iter;
	}
	typename SUPER::iterator insert (const T & newKey,typename SUPER::iterator hint) {
          bool gequal=false,lequal=false;
          _Compare comparator;
          while(1) {
            if (hint!=this->end()) {
              bool tlequal=!comparator (*hint,newKey);
              bool tgequal=!comparator (newKey,*hint);
              if (tlequal) {
                if (gequal||tgequal||hint==this->begin()) {
                  return this->SUPER::insert(hint,newKey);
                }else {
                  lequal=true;
                  --hint;
                }
              }
              if (tgequal) {
                gequal=true;
                ++hint;
                if (lequal||tlequal) {
                  return this->SUPER::insert(hint,newKey);                  
                }
              }
            }else if (hint==this->begin()) {
              this->SUPER::insert(hint,newKey);
            }else {
              if (gequal){
                return this->SUPER::insert(hint,newKey);
              }else {
                --hint;
              }
            }
          }
	}
	typename SUPER::iterator insert (const T & newKey) {
          return this->insert(newKey,this->begin());
                 
	}
};

#endif
