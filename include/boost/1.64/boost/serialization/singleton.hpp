#ifndef BOOST_SERIALIZATION_SINGLETON_HPP
#define BOOST_SERIALIZATION_SINGLETON_HPP

/////////1/////////2///////// 3/////////4/////////5/////////6/////////7/////////8
//  singleton.hpp
//
// Copyright David Abrahams 2006. Original version
//
// Copyright Robert Ramey 2007.  Changes made to permit
// application throughout the serialization library.
//
// Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// The intention here is to define a template which will convert
// any class into a singleton with the following features:
//
// a) initialized before first use.
// b) thread-safe for const access to the class
// c) non-locking
//
// In order to do this,
// a) Initialize dynamically when used.
// b) Require that all singletons be initialized before main
// is called or any entry point into the shared library is invoked.
// This guarentees no race condition for initialization.
// In debug mode, we assert that no non-const functions are called
// after main is invoked.
//

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif 

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/force_include.hpp>

#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/serialization/config.hpp>

#ifdef USE_UNM_SINGLETON
#include <boost/thread.hpp>
#endif

#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#  pragma warning(disable : 4127) // 条件表达式是常量
#endif

namespace boost { 
namespace serialization { 

//////////////////////////////////////////////////////////////////////
// Provides a dynamically-initialized (singleton) instance of T in a
// way that avoids LNK1179 on vc6.  See http://tinyurl.com/ljdp8 or
// http://lists.boost.org/Archives/boost/2006/05/105286.php for
// details.
//

// singletons created by this code are guarenteed to be unique
// within the executable or shared library which creates them.
// This is sufficient and in fact ideal for the serialization library.
// The singleton is created when the module is loaded and destroyed
// when the module is unloaded.

// This base class has two functions.

// First it provides a module handle for each singleton indicating
// the executable or shared library in which it was created. This
// turns out to be necessary and sufficient to implement the tables
// used by serialization library.

// Second, it provides a mechanism to detect when a non-const function
// is called after initialization.

// make a singleton to lock/unlock all singletons for alteration.
// The intent is that all singletons created/used by this code
// are to be initialized before main is called. A test program
// can lock all the singletons when main is entereed.  This any
// attempt to retieve a mutable instances while locked will
// generate a assertion if compiled for debug.

#ifdef USE_UNM_SINGLETON

template <typename T>
class singleton_wrapper : public T
{
};

template <typename T, bool threadsafe = true>
class singleton : public boost::noncopyable
{
private:
	static boost::mutex mtx;
	static bool initialized;
	static T &get_instance()
	{
		bool locked = false;
		if (threadsafe && !initialized)
		{
			mtx.lock();
			locked = true;
		}
		static singleton_wrapper<T> t;
		initialized = true;
		if (locked)
		{
			mtx.unlock();
		}
		return static_cast<T &>(t);
	}

public:
	static T &get_mutable_instance()
	{
		return get_instance();
	}

	static T const &get_const_instance()
	{
		return get_instance();
	}
};

template<typename T, bool lock>
boost::mutex singleton<T, lock>::mtx;

template<typename T, bool lock>
bool singleton<T, lock>::initialized = false;

// non-threadsafe version
template<typename T>
class singleton<T, false> : public boost::noncopyable
{
private:
	static T &get_instance()
	{
		static T t;
		return t;
	}

public:
	static T &get_mutable_instance()
	{
		return get_instance();
	}

	static T const &get_const_instance()
	{
		return get_instance();
	}
};

#else
	
class BOOST_SYMBOL_VISIBLE singleton_module :
    public boost::noncopyable
{
private:
    BOOST_SERIALIZATION_DECL static bool & get_lock();
public:
    static void lock(){
        get_lock() = true;
    }

    static void unlock(){
        get_lock() = false;
    }

    static bool is_locked(){
        return get_lock();
    }
};

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

namespace detail {

template<class T>
class singleton_wrapper : public T
{
public:
    static bool m_is_destroyed;
    ~singleton_wrapper(){
        m_is_destroyed = true;
    }
};

template<class T>
bool detail::singleton_wrapper< T >::m_is_destroyed = false;

} // detail

// note usage of BOOST_DLLEXPORT.  These functions are in danger of
// being eliminated by the optimizer when building an application in
// release mode. Usage of the macro is meant to signal the compiler/linker
// to avoid dropping these functions which seem to be unreferenced.
// This usage is not related to autolinking.

template <class T>
class singleton : public singleton_module
{
private:
    BOOST_DLLEXPORT static T & instance;
    // include this to provoke instantiation at pre-execution time
    static void use(T const *) {}
    BOOST_DLLEXPORT static T & get_instance() {
        static detail::singleton_wrapper< T > t;
        // refer to instance, causing it to be instantiated (and
        // initialized at startup on working compilers)
        BOOST_ASSERT(! detail::singleton_wrapper< T >::m_is_destroyed);
        use(& instance);
        return static_cast<T &>(t);
    }
public:
    BOOST_DLLEXPORT static T & get_mutable_instance(){
        BOOST_ASSERT(! is_locked());
        return get_instance();
    }
    BOOST_DLLEXPORT static const T & get_const_instance(){
        return get_instance();
    }
    BOOST_DLLEXPORT static bool is_destroyed(){
        return detail::singleton_wrapper< T >::m_is_destroyed;
    }
};

template<class T>
BOOST_DLLEXPORT T & singleton< T >::instance = singleton< T >::get_instance();

#endif // USE_UNM_SINGLETON

} // namespace serialization
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif // BOOST_SERIALIZATION_SINGLETON_HPP
