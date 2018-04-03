#pragma once

#ifndef GetVisibleBoundsCallback
#define GET_VISIBLE_BOUNDS_CALLBACK(Name) CollisionObject* Name(CmpRef self)
typedef GET_VISIBLE_BOUNDS_CALLBACK(GetVisibleBoundsCallback);
#endif

// Register a callback which will return visible bounds for the object that has been drawn.
void GfxRegisterBoundsCallback(Component::Type type, GetVisibleBoundsCallback* callback);

// Get the visible bounds of a component.
// Will return NULL if the component has not registered a callback with GfxRegisterBoundsCallback
CollisionObject* GfxGetVisibleBounds(CmpRef component);

// Add a component to the graphical scene.
// Typically called in a drawable component's CREATED event callback.
void GfxAdd(CmpRef component);

// Remove a component from the graphical scene.
// Typically called in a drawable component's DESTROYED event callback.
void GfxRemove(CmpRef component);

// Draw all the objects thare are visible on screen.
// To be safe, it uses queued events, but if we can guarantee that draw events never
// delete other drawable objects, then invoke could be used instead.
void GfxDraw();