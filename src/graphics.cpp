#include <EASTL/fixed_set.h>
#include "auto_component.h"
#include "graphics.h"
using namespace eastl;

// Hide private things in a namespace
namespace _Gfx {

	fixed_set<CmpWeakRef, MAX_COMPONENTS, false> drawableSet;

	GetVisibleBoundsCallback* boundsCallbacks[Component::TYPE_COUNT] = {};
}

void GfxRegisterBoundsCallback(Component::Type type, GetVisibleBoundsCallback* callback) {
	_Gfx::boundsCallbacks[type] = callback;
}

CollisionObject* GfxGetVisibleBounds(CmpRef component) {
	using namespace _Gfx;

	GetVisibleBoundsCallback* callback = boundsCallbacks[component->GetType()];

	return callback == NULL ? NULL : callback(component);
}

void GfxAdd(CmpRef component) {
	_Gfx::drawableSet.insert(component);
}

void GfxRemove(CmpRef component) {
	_Gfx::drawableSet.erase(component);
}

void GfxDraw() {
	using namespace _Gfx;

	// Make screen AABB
	CollisionObject screenBounds;
	screenBounds.type = CollisionObject::AABB;
	screenBounds.aabb.min = vec2(-SCREEN_WIDTH / 2, -SCREEN_HEIGHT / 2);
	screenBounds.aabb.max = vec2(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	
	for (auto it = drawableSet.begin(); it != drawableSet.end(); ++it) {
		auto drawEvent = Event::Create<Event>(Event::DRAW);
		// Only draw components that have not been deleted
		if (auto cmp = it->lock()) {
			// Only draw things that are within the screen
			CollisionObject* co = GfxGetVisibleBounds(cmp);
			if (co == NULL || ColShapesIntersect(co, &screenBounds)) {
				EvtQueueComponent(cmp, drawEvent, 0.0f);
			}
		}
	}

	EvtDispatchEvents();
}
