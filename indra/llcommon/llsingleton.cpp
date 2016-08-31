/** 
 * @file llsingleton.cpp
 * @author Brad Kittenbrink
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llsingleton.h"

#include "llerror.h"
#include "llerrorcontrol.h"         // LLError::is_available()
#include "lldependencies.h"
#include <boost/foreach.hpp>
#include <algorithm>
#include <iostream>                 // std::cerr in dire emergency
#include <sstream>
#include <stdexcept>

namespace {
void log(LLError::ELevel level,
         const char* p1="", const char* p2="", const char* p3="", const char* p4="");
} // anonymous namespace

// Our master list of all LLSingletons is itself an LLSingleton. We used to
// store it in a function-local static, but that could get destroyed before
// the last of the LLSingletons -- and ~LLSingletonBase() definitely wants to
// remove itself from the master list. Since the whole point of this master
// list is to help track inter-LLSingleton dependencies, and since we have
// this implicit dependency from every LLSingleton to the master list, make it
// an LLSingleton.
class LLSingletonBase::MasterList:
    public LLSingleton<LLSingletonBase::MasterList>
{
private:
    friend class LLSingleton<LLSingletonBase::MasterList>;

public:
    // No need to make this private with accessors; nobody outside this source
    // file can see it.
    LLSingletonBase::list_t mList;
};

//static
LLSingletonBase::list_t& LLSingletonBase::get_master()
{
    return LLSingletonBase::MasterList::instance().mList;
}

void LLSingletonBase::add_master()
{
    // As each new LLSingleton is constructed, add to the master list.
    get_master().push_back(this);
}

void LLSingletonBase::remove_master()
{
    // When an LLSingleton is destroyed, remove from master list.
    // add_master() used to capture the iterator to the newly-added list item
    // so we could directly erase() it from the master list. Unfortunately
    // that runs afoul of destruction-dependency order problems. So search the
    // master list, and remove this item IF FOUND. We have few enough
    // LLSingletons, and they are so rarely destroyed (once per run), that the
    // cost of a linear search should not be an issue.
    get_master().remove(this);
}

// Wrapping our initializing list in a static method ensures that it will be
// constructed on demand. This list doesn't also need to be in an LLSingleton
// because (a) it should be empty by program shutdown and (b) none of our
// destructors reference it.
//static
LLSingletonBase::list_t& LLSingletonBase::get_initializing()
{
    static list_t sList;
    return sList;
}

LLSingletonBase::LLSingletonBase():
    mCleaned(false),
    mDeleteSingleton(NULL)
{
    // Make this the currently-initializing LLSingleton.
    push_initializing();
}

LLSingletonBase::~LLSingletonBase() {}

void LLSingletonBase::push_initializing()
{
    get_initializing().push_back(this);
}

void LLSingletonBase::pop_initializing()
{
    list_t& list(get_initializing());
    if (list.empty())
    {
        logerrs("Underflow in stack of currently-initializing LLSingletons at ",
                demangle(typeid(*this).name()).c_str(), "::getInstance()");
    }
    if (list.back() != this)
    {
        LLSingletonBase* back(list.back());
        logerrs("Push/pop mismatch in stack of currently-initializing LLSingletons: ",
                demangle(typeid(*this).name()).c_str(), "::getInstance() trying to pop ",
                demangle(typeid(*back).name()).c_str());
    }
    // Here we're sure that list.back() == this. Whew, pop it.
    list.pop_back();
}

void LLSingletonBase::capture_dependency(EInitState initState)
{
    // Did this getInstance() call come from another LLSingleton, or from
    // vanilla application code? Note that although this is a nontrivial
    // method, the vast majority of its calls arrive here with initializing
    // empty().
    list_t& initializing(get_initializing());
    if (! initializing.empty())
    {
        // getInstance() is being called by some other LLSingleton. But -- is
        // this a circularity? That is, does 'this' already appear in the
        // initializing stack?
        // For what it's worth, normally 'initializing' should contain very
        // few elements.
        list_t::const_iterator found =
            std::find(initializing.begin(), initializing.end(), this);
        if (found != initializing.end())
        {
            // Report the circularity. Requiring the coder to dig through the
            // logic to diagnose exactly how we got here is less than helpful.
            std::ostringstream out;
            for ( ; found != initializing.end(); ++found)
            {
                // 'found' is an iterator; *found is an LLSingletonBase*; **found
                // is the actual LLSingletonBase instance.
                LLSingletonBase* foundp(*found);
                out << demangle(typeid(*foundp).name()) << " -> ";
            }
            // We promise to capture dependencies from both the constructor
            // and the initSingleton() method, so an LLSingleton's instance
            // pointer is on the initializing list during both. Now that we've
            // detected circularity, though, we must distinguish the two. If
            // the recursive call is from the constructor, we CAN'T honor it:
            // otherwise we'd be returning a pointer to a partially-
            // constructed object! But from initSingleton() is okay: that
            // method exists specifically to support circularity.
            // Decide which log helper to call based on initState. They have
            // identical signatures.
            ((initState == CONSTRUCTING)? logerrs : logwarns)
                ("LLSingleton circularity: ", out.str().c_str(),
                 demangle(typeid(*this).name()).c_str(), "");
        }
        else
        {
            // Here 'this' is NOT already in the 'initializing' stack. Great!
            // Record the dependency.
            // initializing.back() is the LLSingletonBase* currently being
            // initialized. Store 'this' in its mDepends set.
            LLSingletonBase* current(initializing.back());
            if (current->mDepends.insert(this).second)
            {
                // only log the FIRST time we hit this dependency!
                log(LLError::LEVEL_DEBUG, demangle(typeid(*current).name()).c_str(),
                    " depends on ", demangle(typeid(*this).name()).c_str());
            }
        }
    }
}

//static
LLSingletonBase::vec_t LLSingletonBase::dep_sort()
{
    // While it would theoretically be possible to maintain a static
    // SingletonDeps through the life of the program, dynamically adding and
    // removing LLSingletons as they are created and destroyed, in practice
    // it's less messy to construct it on demand. The overhead of doing so
    // should happen basically twice: once for cleanupAll(), once for
    // deleteAll().
    typedef LLDependencies<LLSingletonBase*> SingletonDeps;
    SingletonDeps sdeps;
    list_t& master(get_master());
    BOOST_FOREACH(LLSingletonBase* sp, master)
    {
        // Build the SingletonDeps structure by adding, for each
        // LLSingletonBase* sp in the master list, sp itself. It has no
        // associated value type in our SingletonDeps, hence the 0. We don't
        // record the LLSingletons it must follow; rather, we record the ones
        // it must precede. Copy its mDepends to a KeyList to express that.
        sdeps.add(sp, 0,
                  SingletonDeps::KeyList(),
                  SingletonDeps::KeyList(sp->mDepends.begin(), sp->mDepends.end()));
    }
    vec_t ret;
    ret.reserve(master.size());
    // We should be able to effect this with a transform_iterator that
    // extracts just the first (key) element from each sorted_iterator, then
    // uses vec_t's range constructor... but frankly this is more
    // straightforward, as long as we remember the above reserve() call!
    BOOST_FOREACH(SingletonDeps::sorted_iterator::value_type pair, sdeps.sort())
    {
        ret.push_back(pair.first);
    }
    // The master list is not itself pushed onto the master list. Add it as
    // the very last entry -- it is the LLSingleton on which ALL others
    // depend! -- so our caller will process it.
    ret.push_back(MasterList::getInstance());
    return ret;
}

//static
void LLSingletonBase::cleanupAll()
{
    // It's essential to traverse these in dependency order.
    BOOST_FOREACH(LLSingletonBase* sp, dep_sort())
    {
        // Call cleanupSingleton() only if we haven't already done so for this
        // instance.
        if (! sp->mCleaned)
        {
            sp->mCleaned = true;

            log(LLError::LEVEL_DEBUG, "calling ",
                demangle(typeid(*sp).name()).c_str(), "::cleanupSingleton()");
            try
            {
                sp->cleanupSingleton();
            }
            catch (const std::exception& e)
            {
                logwarns("Exception in ", demangle(typeid(*sp).name()).c_str(),
                         "::cleanupSingleton(): ", e.what());
            }
            catch (...)
            {
                logwarns("Unknown exception in ", demangle(typeid(*sp).name()).c_str(),
                         "::cleanupSingleton()");
            }
        }
    }
}

//static
void LLSingletonBase::deleteAll()
{
    // It's essential to traverse these in dependency order.
    BOOST_FOREACH(LLSingletonBase* sp, dep_sort())
    {
        // Capture the class name first: in case of exception, don't count on
        // being able to extract it later.
        const std::string name = demangle(typeid(*sp).name());
        try
        {
            // Call static method through instance function pointer.
            if (! sp->mDeleteSingleton)
            {
                // This Should Not Happen... but carry on.
                logwarns(name.c_str(), "::mDeleteSingleton not initialized!");
            }
            else
            {
                // properly initialized: call it.
                log(LLError::LEVEL_DEBUG, "calling ", name.c_str(), "::deleteSingleton()");
                // From this point on, DO NOT DEREFERENCE sp!
                sp->mDeleteSingleton();
            }
        }
        catch (const std::exception& e)
        {
            logwarns("Exception in ", name.c_str(), "::deleteSingleton(): ", e.what());
        }
        catch (...)
        {
            logwarns("Unknown exception in ", name.c_str(), "::deleteSingleton()");
        }
    }
}

/*------------------------ Final cleanup management ------------------------*/
class LLSingletonBase::MasterRefcount
{
public:
    // store a POD int so it will be statically initialized to 0
    int refcount;
};
static LLSingletonBase::MasterRefcount sMasterRefcount;

LLSingletonBase::ref_ptr_t LLSingletonBase::get_master_refcount()
{
    // Calling this method constructs a new ref_ptr_t, which implicitly calls
    // intrusive_ptr_add_ref(MasterRefcount*).
    return &sMasterRefcount;
}

void intrusive_ptr_add_ref(LLSingletonBase::MasterRefcount* mrc)
{
    // Count outstanding SingletonLifetimeManager instances.
    ++mrc->refcount;
}

void intrusive_ptr_release(LLSingletonBase::MasterRefcount* mrc)
{
    // Notice when each SingletonLifetimeManager instance is destroyed.
    if (! --mrc->refcount)
    {
        // The last instance was destroyed. Time to kill any remaining
        // LLSingletons -- but in dependency order.
        LLSingletonBase::deleteAll();
    }
}

/*---------------------------- Logging helpers -----------------------------*/
namespace {
void log(LLError::ELevel level,
         const char* p1, const char* p2, const char* p3, const char* p4)
{
    // Check whether we're in the implicit final LLSingletonBase::deleteAll()
    // call. We've carefully arranged for deleteAll() to be called when the
    // last SingletonLifetimeManager instance is destroyed -- in other words,
    // when the last translation unit containing an LLSingleton instance
    // cleans up static data. That could happen after std::cerr is destroyed!
    // The is_available() test below ensures that we'll stop logging once
    // LLError has been cleaned up. If we had a similar portable test for
    // std::cerr, this would be a good place to use it. As we do not, just
    // don't log anything during implicit final deleteAll(). Detect that by
    // the master refcount having gone to zero.
    if (sMasterRefcount.refcount == 0)
        return;

    // Check LLError::is_available() because some of LLError's infrastructure
    // is itself an LLSingleton. If that LLSingleton has not yet been
    // initialized, trying to log will engage LLSingleton machinery... and
    // around and around we go.
    if (LLError::is_available())
    {
        lllog(level, false, "LLSingleton") << p1 << p2 << p3 << p4 << LL_ENDL;
    }
    else
    {
        // Caller may be a test program, or something else whose stderr is
        // visible to the user.
        std::cerr << p1 << p2 << p3 << p4 << std::endl;
    }
}
} // anonymous namespace        

//static
void LLSingletonBase::logwarns(const char* p1, const char* p2, const char* p3, const char* p4)
{
    log(LLError::LEVEL_WARN, p1, p2, p3, p4);
}

//static
void LLSingletonBase::logerrs(const char* p1, const char* p2, const char* p3, const char* p4)
{
    log(LLError::LEVEL_ERROR, p1, p2, p3, p4);
    // The other important side effect of LL_ERRS() is
    // https://www.youtube.com/watch?v=OMG7paGJqhQ (emphasis on OMG)
    LLError::crashAndLoop(std::string());
}

std::string LLSingletonBase::demangle(const char* mangled)
{
    return LLError::Log::demangle(mangled);
}
