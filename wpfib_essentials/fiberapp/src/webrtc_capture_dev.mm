#include <Foundation/NSAutoreleasePool.h>
#include <QTKit/QTKit.h>
#include "skinnysipmanager.h"
#include "webrtc_capture_dev.h"

@interface BJNDelegate: NSObject
{
@public
    void *strm;
    func_ptr          func;
    PJDynamicThreadDescRegistration delegateThread;
}

- (void)run_func;
@end

@implementation BJNDelegate
- (void)run_func
{
    delegateThread.Register("bjn_selector");
    (*func)(strm);
}

@end

void run_func_on_main_thread(void *strm, func_ptr func)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    BJNDelegate *delg = [[BJNDelegate alloc] init];
    
    delg->strm = strm;
    delg->func = func;
    [delg performSelectorOnMainThread:@selector(run_func)
                           withObject:nil waitUntilDone:YES];
    
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);

    [delg release];
    [pool release];    
}

