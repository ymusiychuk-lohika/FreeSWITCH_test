/*
 * libjingle
 * Copyright 2008, Google Inc.
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



#import "talk/base/macwindowpickerobjc.h"
#import <AppKit/AppKit.h>


@implementation BJNApplicationNotification

- (id) init
{
    self = [super init];
    if (self)
    {
        // init here
        _watcher = NULL;
        _watchedApplicationPid = 0;
    }
    return self;
}

- (void) dealloc
{
    // remove notification callback
    NSNotificationCenter* center = [[NSWorkspace sharedWorkspace] notificationCenter];
    [center removeObserver:self];
    [super dealloc];
}

- (void) registerAppExitNotify:(pid_t)appPid withWatcher:(talk_base::DesktopWatcher*)watcher
{
    _watcher = watcher;
    _watchedApplicationPid = appPid;
    // setup notification callback
    NSNotificationCenter* center = [[NSWorkspace sharedWorkspace] notificationCenter];
    [center addObserver:self
               selector:@selector(appTerminated:)
                   name:NSWorkspaceDidTerminateApplicationNotification
                 object:nil];
}

- (void)appTerminated:(NSNotification *)note
{
    NSDictionary* userDict = [note userInfo];
    // get the pid of the terminated aplication
    pid_t appPid = [[userDict objectForKey:@"NSApplicationProcessIdentifier"] intValue];
    if (_watchedApplicationPid == appPid)
    {
        // send a notification
        if (_watcher) {
            _watcher->AppTerminated(appPid);
        }
    }
}

@end

namespace talk_base {

    CGImageRef GetApplicationIcon(pid_t pid, int width, int height)
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        CGImageRef iconRef = NULL;
        
        NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
        if (app != nil)
        {
            NSRect iconRect = NSMakeRect(0, 0, width, height);
            NSImage* iconImage = [app icon];
            CGImageRef appIconRef = [iconImage CGImageForProposedRect:&iconRect context:NULL hints:NULL];
            if (appIconRef)
            {
                // we must make a copy as we don't own appIconRef
                iconRef = CGImageCreateCopy(appIconRef);
                // releasing appIconRef will cause the application to crash...
                // I assume that it is released when the NSImage is released
            }
        }
        [pool release];
        return iconRef;
    }

    BJNApplicationNotification* AddAppTermNotify(pid_t pid, DesktopWatcher* watcher)
    {
        BJNApplicationNotification* notify = [[BJNApplicationNotification alloc] init];
        if (notify)
        {
            [notify registerAppExitNotify:pid withWatcher:watcher];
        }
        return notify;
    }

    void RemoveAppTermNotify(BJNApplicationNotification* notify)
    {
        if (notify)
        {
            [notify release];
        }
    }

}  // namespace talk_base


