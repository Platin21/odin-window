#include <stdio.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc-runtime.h>

struct MoldPoint {
  double x;
  double y;
};

enum MoldWindowStyle {
  Borderless      = 0,
  Titled          = 1 << 0,
  Closable        = 1 << 1,
  Miniaturizable  = 1 << 2,
  Resizable       = 1 << 3,
};

struct MoldSize {
  double width;
  double height;
};

struct MoldRect {
  MoldPoint origin;
  MoldSize size;
};

enum MoldEventType {
  MoldMouseLeftDown = 1,
  MoldMouseLeftUp  = 2,

  MoldMouseRightDown = 3,
  MoldMouseRightUp  = 4,

  MoldMouseMoved   = 5,
  MoldMouseLeftDrag = 6,
  MoldMouseRightDrag = 7,
  MoldMouseEntered = 8,
  MoldMouseExited  = 9,
  MoldKeyDown      = 10,
  MoldKeyUp        = 11,
};

typedef id (*NSWindowAlloc)(Class window_class, SEL alloc_sel);
typedef void (*NSWindowInit)(id window, SEL init_method, MoldRect rect, unsigned int decorations, unsigned int mask, bool defer);
typedef void (*NSWindowSetBackground)(id window, SEL set_background, id color);
typedef id (*NSColorGet)(Class color_class, SEL color_wanted);
typedef id (*NSWindowGetContentView)(id window, SEL content_view);
typedef void (*NSViewWantsLayer)(id view, SEL wants_layer);
typedef void (*NSWindowAcceptsMouseMovedEvents)(id window, SEL accepts_mouse_moved, bool should_do_or_not);
typedef void (*NSWindowSetTitle)(id window, SEL set_title, CFStringRef titleName);
typedef void (*NSWindowMakeKeyWindow)(id window, SEL make_key_any_order_front);

id create_window(double width, double height, double x, double y, const char* name, unsigned int decorations) {
  MoldRect rect = { .origin = { x, y }, .size = { width, height } };

  NSWindowAlloc window_alloc = (NSWindowAlloc)objc_msgSend;
  NSWindowInit  window_init  = (NSWindowInit)objc_msgSend;
  NSWindowSetBackground window_set_background = (NSWindowSetBackground)objc_msgSend;
  NSColorGet color_get = (NSColorGet)objc_msgSend;
  NSWindowGetContentView window_get_view = (NSWindowGetContentView)objc_msgSend;
  NSViewWantsLayer view_wants_layer = (NSViewWantsLayer)objc_msgSend;
  NSWindowAcceptsMouseMovedEvents window_accept_mouse = (NSWindowAcceptsMouseMovedEvents)objc_msgSend;
  NSWindowSetTitle window_set_title = (NSWindowSetTitle)objc_msgSend;
  NSWindowMakeKeyWindow window_make_key = (NSWindowMakeKeyWindow)objc_msgSend;

  id window = window_alloc(objc_getClass("NSWindow"),
                           sel_getUid("alloc"));

  window_init(window,
              sel_getUid("initWithContentRect:styleMask:backing:defer:"),
              rect,
              decorations,
              2,
              false);

  //window_set_background(window, sel_registerName("setBackgroundColor:"), color_get(objc_getClass("NSColor"), sel_getUid("greenColor")));

  id view = window_get_view(window, sel_registerName("contentView"));
  view_wants_layer(view, sel_registerName("wantsLayer"));

  window_accept_mouse(window, sel_registerName("setAcceptsMouseMovedEvents:"), true);

  if(name) {
    CFStringRef titleName = CFStringCreateWithCString(kCFAllocatorMalloc, name, kCFStringEncodingUTF8);
    CFRetain(titleName);
    window_set_title(window, sel_getUid("setTitle:"), titleName);
    CFRelease(titleName);
  }

  window_make_key(window, sel_getUid("makeKeyAndOrderFront:"));
  return window;
}

typedef MoldRect (*NSWindowGetSizeFrame)(id window, SEL frame);
MoldSize window_get_size(id window) {
  NSWindowGetSizeFrame get_frame_size = (NSWindowGetSizeFrame)objc_msgSend;
  MoldRect rect = get_frame_size(window, sel_getUid("frame"));
  printf("w: %f, h: %f", rect.size.width, rect.size.height);
  return rect.size;
}

typedef MoldPoint (*NSEventLocationInWindowFuncPtr)(id ns_event);
typedef void (*NSAutoreleasePoolFuncPtr)(id ns_auto_relase_pool);
typedef unsigned int (*NSEventKeyCodeFuncPtr)(id ns_event);
typedef unsigned int (*NSEventModFlagsFuncPtr)(id ns_event);
typedef id (*NSGetNextEventMatchingMaskFuncPtr)(id ns_app, SEL selector, int mask, void* time, CFStringRef str, int dequeu);
typedef unsigned long (*NSEventGetTypeFuncPtr) (id ns_event);
typedef void (*NSApplicationSendEventFuncPtr) (id ns_app, SEL selector, id ns_event);
typedef MoldRect (*NSWindowGetFrameFuncPtr)(id ns_window);
typedef void (*NSApplicationUpdateWindowsFuncPtr)(id ns_app, SEL selector);

extern "C" const CFStringRef NSWindowWillCloseNotification;
extern "C" const CFStringRef NSWindowWillEnterFullScreenNotification;
extern "C" const CFStringRef NSWindowDidResizeNotification;
extern "C" const CFStringRef NSWindowDidMoveNotification;
extern "C" const CFStringRef NSWindowDidMiniaturizeNotification;
extern "C" const CFStringRef NSWindowDidDeminiaturizeNotification;

typedef id (*NSApplicationSharedApp)(Class nsapplication, SEL shared_application);
typedef id (*NSNotificationCenterDefault)(Class nsnotificationcenter, SEL default_center);
typedef void (*NSNotificationCenterAddHandler)(id ns_center, SEL add_observer, void* x1, void* x2, void* x3,struct event_block_literal* literal);
typedef CFStringRef (*NSEventGetName)(id ns_event, SEL name);
typedef void (*NSWindowRelease)(id window, SEL exit);

typedef id (*NSAutoreleasePoolInit)(id ns_autorelease_pool, SEL init);
typedef id (*NSAutoreleasePoolAlloc)(id ns_autorelease_pool, SEL alloc);

typedef void* (*ns_menu_allocate)(Class menu_class, SEL alloc_sel);
typedef void* (*ns_menu_init)(void* menu, SEL init_sel, CFStringRef str);

typedef id (*NSEventObject)();

static bool willClose = false;
static bool inResize  = false;
static bool inMove    = false;
static bool isMiniaturized = false;

static NSEventGetName nsevent_get_name = (NSEventGetName)objc_msgSend;

static void process_events(void* block, id event) {
  CFStringRef str = nsevent_get_name(event, sel_registerName("name"));

  if(CFStringCompare(str, NSWindowWillCloseNotification, kCFCompareEqualTo) == 0) {
    willClose = true;
  } else if(CFStringCompare(str, NSWindowDidResizeNotification, kCFCompareEqualTo) == 0) {
    inResize = true;
  }  else if(CFStringCompare(str, NSWindowDidMoveNotification, kCFCompareEqualTo) == 0) {
    inMove = true;
  } else if(CFStringCompare(str,NSWindowDidMiniaturizeNotification, kCFCompareEqualTo) == 0) {
    isMiniaturized = true;
  } else if(CFStringCompare(str,NSWindowDidDeminiaturizeNotification, kCFCompareEqualTo) == 0) {
    isMiniaturized = false;
  } else {
    printf("SEL: %s\n",CFStringGetCStringPtr(str, kCFStringEncodingUTF8));
  }
}

struct event_block_literal {
  void *isa;
  int flags;
  int reserved;
  void (*invoke)(void* block, id event);
  struct event_block_descriptor *descriptor;
};

static struct event_block_descriptor {
  unsigned long int reserved;
  unsigned long int Block_size;
  void (*copy_helper)(struct event_block_literal *dst, struct event_block_literal *src);
  void (*dispose_helper)(struct event_block_literal *);
  const char* signature;
} event_block_descriptor_instance = { 0, sizeof(struct event_block_literal), NULL, NULL };

int main(int argc, char **argv) {

  static event_block_literal event_blk_literal = {
    &_NSConcreteGlobalBlock,
    (1 << 28) | (1 << 29) | (1 << 30),
    NULL,
    process_events,
    &event_block_descriptor_instance,
  };

  Class NSApplication = objc_getClass("NSApplication");

  NSApplicationSharedApp nsapplication_get_shared = (NSApplicationSharedApp)objc_msgSend;
  NSNotificationCenterDefault nsnotification_center_default = (NSNotificationCenterDefault)objc_msgSend;
  NSNotificationCenterAddHandler nsnotification_center_add = (NSNotificationCenterAddHandler)objc_msgSend;

  NSAutoreleasePoolInit  nsautoreleasepool_init = (NSAutoreleasePoolInit)objc_msgSend;
  NSAutoreleasePoolAlloc nsautoreleasepool_alloc = (NSAutoreleasePoolAlloc)objc_msgSend;
  NSWindowRelease window_exit = (NSWindowRelease)objc_msgSend;

  void *menu = ((ns_menu_allocate)objc_msgSend)(objc_getClass("NSMenu"),
                                                sel_getUid("alloc"));

  ((ns_menu_init)objc_msgSend)(menu, sel_getUid("initWithTitle:"), CFSTR("Apple"));

  void* app = (void*)nsapplication_get_shared(NSApplication,sel_registerName("sharedApplication"));

  ((void (*)(void* shared, SEL selector, void* menu))objc_msgSend)(app, sel_registerName("setMainMenu:"), menu);

  // todo add a proper menu here!

  id ns_window = create_window(800, 600, 0, 0, "My Window", (Titled | Closable | Resizable | Miniaturizable));

  /* get notification center*/
  id ns_center = nsnotification_center_default(objc_getClass("NSNotificationCenter"),
                 sel_registerName("defaultCenter"));

  nsnotification_center_add(ns_center,
               sel_registerName("addObserverForName:object:queue:usingBlock:"),
               NULL,
               ns_window,
               NULL,
               &event_blk_literal);

  // imp chaching!
  // autorelase pool functions
  NSAutoreleasePoolFuncPtr autoreleasePoolRelase =
  (NSAutoreleasePoolFuncPtr)class_getMethodImplementation(objc_getClass("NSAutoreleasePool"),sel_registerName("release"));
  // event location in  window function

  NSEventGetTypeFuncPtr eventTypeFunc =
  (NSEventGetTypeFuncPtr)class_getMethodImplementation(objc_getClass("NSEvent"), sel_registerName("type"));

  NSEventLocationInWindowFuncPtr locationInWindowFunc =
  (NSEventLocationInWindowFuncPtr)class_getMethodImplementation(objc_getClass("NSEvent"), sel_registerName("locationInWindow"));

  NSEventKeyCodeFuncPtr keyCodeFunc =
  (NSEventKeyCodeFuncPtr)class_getMethodImplementation(objc_getClass("NSEvent"), sel_registerName("keyCode"));

  NSEventModFlagsFuncPtr modFlagsFunc =
  (NSEventModFlagsFuncPtr)class_getMethodImplementation(objc_getClass("NSEvent"), sel_registerName("modifierFlags"));

  NSGetNextEventMatchingMaskFuncPtr nextEventFunc =
  (NSGetNextEventMatchingMaskFuncPtr)class_getMethodImplementation(NSApplication, sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"));

  NSApplicationSendEventFuncPtr sendEvent =
  (NSApplicationSendEventFuncPtr)class_getMethodImplementation(NSApplication, sel_registerName("sendEvent:"));

  NSWindowGetFrameFuncPtr window_get_frame =
  (NSWindowGetFrameFuncPtr)class_getMethodImplementation(objc_getClass("NSWindow"), sel_registerName("frame"));

  NSApplicationUpdateWindowsFuncPtr update_windows =
  (NSApplicationUpdateWindowsFuncPtr)class_getMethodImplementation(NSApplication, sel_registerName("updateWindows"));

  bool isInWindow = true;
  bool running = true;
  while(running) {

    // enable spellcheck next time it will definitly safe you some hours of debugging it's NSAutoreleasePool not NSAutoReleasePool
    id pool = nsautoreleasepool_init(nsautoreleasepool_alloc((id)objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")),sel_registerName("init"));

    if(willClose) {
      puts("got closed?");
      running = false;
    }

    id ns_event = nextEventFunc((id)app, nullptr, INT_MAX, nullptr, CFSTR("kCFRunLoopDefaultMode"), 1 );
    if(ns_event) {
      unsigned long type = eventTypeFunc(ns_event);
      if(!isMiniaturized) {
        switch (type) {
          case MoldMouseMoved:
          {
            MoldRect frame = window_get_frame(ns_window);
            MoldPoint mouse_position = locationInWindowFunc(ns_event);

            if((mouse_position.x >= 0 && mouse_position.y >= 0) &&
               (mouse_position.x <= frame.size.width && mouse_position.y <= frame.size.height))
            {
              isInWindow = true;
              fprintf(stdout,"(in:true)(%f,%f)\n", mouse_position.x, mouse_position.y);
            }
            /*
            else {
              isInWindow = false;
              fprintf(stdout,"(in:false)(%f,%f)\n", mouse_position.x, mouse_position.y);
            }
            */
            break;
          }
          case MoldKeyDown:
          {
            unsigned long key_code  = keyCodeFunc(ns_event);
            unsigned long mod_flags = modFlagsFunc(ns_event);

            fprintf(stdout,"DOWN(%lu)\n", key_code);

            if(key_code == 0xC && mod_flags & (1 << 20)) { // on q+cmd kill!
              puts("by key code!");
              running = false;
            }
            break;
          }
          case MoldKeyUp:
          {
            unsigned short key_code = keyCodeFunc(ns_event);
            fprintf(stdout,"UP(%u)\n", key_code);
            break;
          }
          default:
            if(inMove) {
              puts("Moveing!");
              inMove = false;
            } else if(inResize) {
              puts("Resizing!");
              inResize = false;
            }

            sendEvent((id)app, nullptr, ns_event);
            update_windows((id)app, nullptr);
        }
      } else {
        sendEvent((id)app, nullptr, ns_event);
        update_windows((id)app, nullptr);
      }
    }
    autoreleasePoolRelase(pool);
    usleep(11000); // don't ask it help's somehow?
  }

  window_exit(ns_window, sel_registerName("release"));
  return 0;
}
