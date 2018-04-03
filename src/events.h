#pragma once

#ifndef EventCallback
#define EVENT_CALLBACK(Name) void Name(CmpRef self, GoRef go, EvtRef evt)
typedef EVENT_CALLBACK(EventCallback);
#endif

#include "auto_component.h"
using namespace glm;

#define MAX_EVENTS KILOBYTES(1)

// Registers a function that will be called for the given component type when an event of the given type occurs
void EvtRegisterCallback(Component::Type componentType, Event::Type eventType, EventCallback *callback);

void EvtInitialize();

// Same as above but with a delay so the event occurs in the future
void EvtQueueGlobal(EvtRef evt, float delayInSeconds = 0.0f);

// Send an event to a specific component to be executed immediately.
// If the component is listening for the message, processing will occur immediately.
// Otherwise nothing will happen.
void EvtInvokeComponent(CmpRef target, EvtRef evt);

// Send a static event for all component types that have registered for it.
// The callback will be invoked whether a component of that type exists or not.
// Useful for static one-time initialization.
void EvtInvokeStatic(EvtRef evt);

// Same as above but with a delay so the event occurs in the future
void EvtQueueComponent(CmpRef target, EvtRef evt, float delayInSeconds);

// Same as above but with a delay so the event occurs in the future
void EvtQueueGameObject(GoRef gameObject, EvtRef evt, float delayInSeconds = 0.0f);

// Dispatches events that were delayed
void EvtDispatchEvents(float deltaTime = 0.0f);

// Functions that will be automatically called by component system

// Adds a new component to the system so that it will be able to recieve events.
// All calls to EvtRegisterCallback should be complete before this is called.
// Will be called automatically when a component is created.
void EvtAddComponent(CmpRef target);

// Removes a component from the system so that it will no longer recieve events.
// All calls to EvtRegisterCallback should be complete before this is called.
// Will be called automatically when a component is destroyed.
void EvtRemoveComponent(CmpRef target);


