/******************************************************************************
 * Prevent copy construction/assignment, the old school C++ way
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Oct 28, 2013
 *****************************************************************************/

#ifndef BWMGR_NONCOPYABLE_H
#define BWMGR_NONCOPYABLE_H

namespace bwmgr
{

// While the C++11 way of ensuring you can't copy construct or assign is to
// 'delete' the relevant function, this is the old fashioned way when C++11
// isn't available.  Just do a private inherit from this class, specifying
// the derived class as the template arg
template <typename DerivedClassName>
class NonCopyable
{
protected:
    NonCopyable() {}
    ~NonCopyable() {}
private:
    NonCopyable(const NonCopyable &);
    NonCopyable &operator =(const NonCopyable &);
};

} // namespace bwmgr

#endif // ifndef BWMGR_NONCOPYABLE_H
