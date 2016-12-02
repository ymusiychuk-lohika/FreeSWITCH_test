#ifndef __PJMEDIA_RDC_H__
#define __PJMEDIA_RDC_H__

#include <pjmedia/types.h>

PJ_BEGIN_DECL

// we have 32 bits for flags we take 8 bits of that for the event type
// the remaining 24 bits are uses as normal bit flags, we also
#define RDC_EVENT_TYPE_MASK             0x000000FF /* bottom byte is the event type */
#define RDC_FLAGS_MASK                  0xFFFFFF00 /* bottom byte is the event type */

// mouse event types
#define RDC_MOUSEEVENT_MOVE             1 /* mouse move */
#define RDC_MOUSEEVENT_LEFTDOWN         2 /* left button down */
#define RDC_MOUSEEVENT_LEFTUP           3 /* left button up */
#define RDC_MOUSEEVENT_RIGHTDOWN        4 /* right button down */
#define RDC_MOUSEEVENT_RIGHTUP          5 /* right button up */
#define RDC_MOUSEEVENT_MIDDLEDOWN       6 /* middle button down */
#define RDC_MOUSEEVENT_MIDDLEUP         7 /* middle button up */
#define RDC_MOUSEEVENT_4BUTTONDOWN      8 /* 4 button down */
#define RDC_MOUSEEVENT_4BUTTONUP        9 /* 4 button up */
#define RDC_MOUSEEVENT_5BUTTONDOWN     10 /* 5 button down */
#define RDC_MOUSEEVENT_5BUTTONUP       11 /* 5 button up */
#define RDC_MOUSEEVENT_SCROLL          12 /* wheel button rolled/trackpad scrolled */
// update these if new mouse events are added
#define RDC_MOUSEEVENT_FIRST           RDC_MOUSEEVENT_MOVE
#define RDC_MOUSEEVENT_LAST            RDC_MOUSEEVENT_SCROLL

// keyboard event types
#define RDC_KEYEVENT_KEYDOWN          100 /* key down */
#define RDC_KEYEVENT_KEYUP            101 /* key up */
#define RDC_KEYEVENT_KEYSYNC          102 /* key sync */
// update these if new keyboard events are added
#define RDC_KEYEVENT_FIRST            RDC_KEYEVENT_KEYDOWN
#define RDC_KEYEVENT_LAST             RDC_KEYEVENT_KEYSYNC

// mouse flags
#define RDC_MOUSEEVENTF_DRAG            0x00000100
// keyboard flags
#define RDC_KEYEVENTF_EXTENDEDKEY       0x00010000
#define RDC_KEYEVENTF_SCANCODE          0x00020000
#define RDC_KEYEVENTF_UNICODE           0x00040000
// type flags
#define RDC_INPUT_MOUSE                 0x80000000
#define RDC_INPUT_KEYBOARD              0x40000000

/*
 * structures used for RDC input, either mouse or keyboard
 */
typedef struct bjn_rdc_mouse_input
{
    pj_uint16_t     x;
    pj_uint16_t     y;
    pj_uint32_t     data;
    pj_uint32_t     time;
} bjn_rdc_mouse_input;

typedef struct bjn_rdc_keyboard_input
{
    pj_uint32_t     vkey;
    pj_uint32_t     data;
    pj_uint32_t     time;
} bjn_rdc_keyboard_input;

typedef struct bjn_rdc_input
{
    pj_uint32_t  flags;
    pj_uint8_t   event_type;
    union
    {
        bjn_rdc_mouse_input     mouse;
        bjn_rdc_keyboard_input  keyboard;
    };
} bjn_rdc_input;

// forward declaration
struct bjn_rdc_input;

typedef struct pjmedia_bjn_rdc_queue
{
    pj_pool_t*              pool;
    pj_mutex_t*             mutex;
    struct bjn_rdc_input*   buffer;
    pj_size_t               capacity;
    pj_size_t               in;
    pj_size_t               out;
    pj_size_t               max_queued;
} pjmedia_bjn_rdc_queue;

/* Number of items in the queue before we start dropping input */
/* TODO: adjust this value once the feature is working end to end */
#define RDC_QUEUE_SIZE          500

PJ_DECL(pjmedia_bjn_rdc_queue*) create_rdc_input_queue(pj_pool_t* pool, pj_size_t capacity);
PJ_DECL(pj_bool_t) rdc_input_queue_push(pjmedia_bjn_rdc_queue* queue, struct bjn_rdc_input* item);
PJ_DECL(pj_bool_t) rdc_input_queue_pop(pjmedia_bjn_rdc_queue* queue, struct bjn_rdc_input* item);
PJ_DECL(pj_bool_t) rdc_input_queue_empty(pjmedia_bjn_rdc_queue* queue);
PJ_DECL(void) destroy_rdc_input_queue(pjmedia_bjn_rdc_queue* queue);
PJ_DECL(void) rdc_input_queue_lock(pjmedia_bjn_rdc_queue* queue);
PJ_DECL(void) rdc_input_queue_unlock(pjmedia_bjn_rdc_queue* queue);

// debug only
PJ_DECL(void) rdc_input_dump_item(bjn_rdc_input* rdc, const char* text);

PJ_END_DECL

#endif
