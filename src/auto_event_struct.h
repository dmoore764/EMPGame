#pragma once 
#include <EASTL/shared_ptr.h> 

using namespace eastl; 
using namespace glm; 
struct Component; 
struct GameObject; 
struct Event; 

typedef shared_ptr<Component> CmpRef; 
typedef shared_ptr<GameObject> GoRef; 
typedef weak_ptr<Component> CmpWeakRef; 
typedef weak_ptr<GameObject> GoWeakRef; 
typedef shared_ptr<Event> EvtRef; 
typedef weak_ptr<Event> EvtWeakRef; 

struct Event { 
    enum Type { 
        NONE,
        TYPE_COUNT 
    }; 

    // Gets the type of the event.  This prevents us from accidentally changing the event type after construction, which is not allowed. 
    inline Type GetType() { 
        return type;
    } 

    // Whether the event has been handled.  May cause some types of events to stop propogating.
    bool handled;

    // Creates an event of the given type and casts it to that type so that you can immediately interact with all its values.
    // The reason you have to type both the type parameter and the type enum value is that, unlike components, there are sometimes everal events
    // that use the same type, and also many events that don't use any type.
    template <typename T>
    static shared_ptr<T> Create(Event::Type type);

    // Create a simple event that doesn't require any casting to work with its values.
    static EvtRef Create(Event::Type type);

    // Casts an event from one type to another.
    // If the cast is not allowed, or if the event is NULL to start with, then NULL will be returned.
    template <typename T>
    shared_ptr<T> static From(EvtRef evt);

    // Casts an event to another type.
    // Note that this will cause a crash if the pointer you call it on is NULL, unlike the longer version above.
    // Use it in cases where you already know the value is not NULL;
    template <typename T>
    shared_ptr<T> To();

protected:
    Type type;
    EvtWeakRef self;

    inline Event(Event::Type type, EvtRef self) {
        this->type = type;
        this->self = self;
    }

    // Added so that Event supports dynamic_cast
    inline virtual ~Event() {}
};

// Put derived events into a namespace

namespace Evt {

}  // End namespace 

//Used to figure out the maximum size of any event type 
struct ALL_EVENTS_UNION {
};

