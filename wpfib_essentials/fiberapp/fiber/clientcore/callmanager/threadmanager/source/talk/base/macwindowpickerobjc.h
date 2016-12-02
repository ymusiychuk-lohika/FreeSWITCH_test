/*
 * libjingle
 * Copyright 2008 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Automatically initialize and and free an autoreleasepool. Never allocate
// an instance of this class using "new" - that will result in a compile-time
// error. Only use it as a stack object.
//
// Note: NSAutoreleasePool docs say that you should not normally need to
// declare an NSAutoreleasePool as a member of an object - but there's nothing
// that indicates it will be a problem, as long as the stack lifetime of the
// pool exactly matches the stack lifetime of the object.

#ifndef TALK_BASE_MAC_WINDOW_PICKER_OBJC_H__
#define TALK_BASE_MAC_WINDOW_PICKER_OBJC_H__

#if defined(IOS) || defined(OSX)

#include "talk/base/common.h"
#include "talk/base/windowpicker.h"
#include <CoreVideo/CoreVideo.h>
#include <unistd.h>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>

@interface BJNApplicationNotification : NSObject
{
    pid_t                       _watchedApplicationPid;
    talk_base::DesktopWatcher*  _watcher;
}
@end

#else // __OBJC__

class BJNApplicationNotification;

#endif // __OBJC__


namespace talk_base {

    CGImageRef GetApplicationIcon(pid_t pid, int width, int height);
    BJNApplicationNotification* AddAppTermNotify(pid_t pid, DesktopWatcher* watcher);
    void RemoveAppTermNotify(BJNApplicationNotification* notify);

}  // namespace talk_base


#endif  // IOS || OSX
#endif  // TALK_BASE_MAC_WINDOW_PICKER_OBJC_H__
