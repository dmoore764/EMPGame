/**
 * The event functions are in charge of letting objects register for events of interest and then sending out those events when they occur
 */

#include <EASTL/fixed_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/priority_queue.h>
#include <EASTL/fixed_set.h>

#include "auto_event_struct.h"

// Use namespace to hide internal stuff
namespace _Evt {
	using namespace eastl;

	static void* eventBuffer;
	static fixed_allocator eventAllocator;

	EventCallback* registrations[Component::TYPE_COUNT][Event::TYPE_COUNT];

	fixed_set<CmpWeakRef, MAX_COMPONENTS, false> componentsByEvent[Event::TYPE_COUNT];

	double accumulatedTime = 0.0;

	struct DeferedEvent {
		double timeStamp;
		EvtRef evt;

		inline DeferedEvent(EvtRef evt, float delayInSeconds) {
			timeStamp = accumulatedTime + delayInSeconds;
			this->evt = evt;
		}
		DeferedEvent(const DeferedEvent&) = default;
		DeferedEvent& operator=(const DeferedEvent&) = default;

	};

	struct DeferedCmpEvent : DeferedEvent {
		CmpWeakRef component;

		inline DeferedCmpEvent(EvtRef evt, float delayInSeconds) : DeferedEvent(evt, delayInSeconds) {}

		DeferedCmpEvent(const DeferedCmpEvent&) = default;
		DeferedCmpEvent& operator=(const DeferedCmpEvent&) = default;
	};

	struct DeferedGoEvent : DeferedEvent {
		GoWeakRef gameObject;

		inline DeferedGoEvent(EvtRef evt, float delayInSeconds) : DeferedEvent(evt, delayInSeconds) {}

		DeferedGoEvent(const DeferedGoEvent&) = default;
		DeferedGoEvent& operator=(const DeferedGoEvent&) = default;
	};

	// Comparison class that determines the order of events in the queue
	// Right now it's just sorting by time
	class QueueComparator {
	public:
		bool operator() (const DeferedEvent& a, const DeferedEvent& b) {
			return a.timeStamp > b.timeStamp;
		}
	};

	// Define a type for the priority_queue for each type of event.
	// Makes it easier to refer to(it's very long)
	typedef priority_queue<DeferedEvent, fixed_vector<DeferedEvent, MAX_EVENTS, false>, QueueComparator> GlobalEventQueue;
	typedef priority_queue<DeferedEvent, fixed_vector<DeferedCmpEvent, MAX_EVENTS, false>, QueueComparator> CmpEventQueue;
	typedef priority_queue<DeferedEvent, fixed_vector<DeferedGoEvent, MAX_EVENTS, false>, QueueComparator> GoEventQueue;

	// Declare the global event queue
	GlobalEventQueue globalEventQueue;
	CmpEventQueue cmpEventQueue;
	GoEventQueue goEventQueue;

	// And finally a pool to allocate DeferedEvent object Ids
	IdPool<DeferedEvent> eventPool(MAX_EVENTS);

	void EnqueueEvent(EvtRef evt, float delayInSeconds) {
		// Push the event into the priority queue for later dispatch
		// Note that the event must be copied because it may be on the stack
		DeferedEvent entry(evt, delayInSeconds);
		
		globalEventQueue.push(entry);
	}

	void EnqueueEvent(EvtRef evt, CmpRef component, float delayInSeconds) {
		// Push the event into the priority queue for later dispatch
		// Note that the event must be copied because it may be on the stack
		DeferedCmpEvent entry(evt, delayInSeconds);

		entry.component = component;

		cmpEventQueue.push(entry);
	}

	void EnqueueEvent(EvtRef evt, GoRef gameObject, float delayInSeconds) {
		// Push the event into the priority queue for later dispatch
		// Note that the event must be copied because it may be on the stack
		DeferedGoEvent entry(evt, delayInSeconds);

		entry.gameObject = gameObject;

		goEventQueue.push(entry);
	}

	void EvtQueueComponentsOfType(EvtRef evt) {
		using namespace _Evt;

		// Iterate through all the components that have registered for this type of event
		// and call their event handler
		for (auto it = componentsByEvent[evt->GetType()].begin(); it != componentsByEvent[evt->GetType()].end(); ++it) {
			// If the component still exists, invoke the event
			if (auto cmp = it->lock()) {
				EvtQueueComponent(cmp, evt, 0.0f);
			}
		}
	}

	void EvtQueueComponentsInGameObject(GoRef gameObject, EvtRef evt) {
		using namespace _Evt;

		// Send the message to all components attached to the game object
		for (auto& cmp : gameObject) {
			EvtQueueComponent(cmp, evt, 0.0f);
		}
	}

	void DeleteEvent(Event* evt) {
		eventAllocator.deallocate(evt, sizeof(ALL_EVENTS_UNION));
	}

	EvtRef EvtAllocate() {
		auto temp = (Event*)eventAllocator.allocate(sizeof(ALL_EVENTS_UNION));
		assert(temp);
		return EvtRef(temp, DeleteEvent);
	}
}

void EvtRegisterCallback(Component::Type componentType, Event::Type eventType, EventCallback *callback) {
	using namespace _Evt;

	registrations[componentType][eventType] = callback;
}

void EvtInitialize() {
	using namespace _Evt;

	eventBuffer = malloc(sizeof(ALL_EVENTS_UNION) * MAX_EVENTS);

	eventAllocator.init(eventBuffer, sizeof(ALL_EVENTS_UNION) * MAX_EVENTS, sizeof(ALL_EVENTS_UNION), __alignof(Event));
}

void EvtQueueGlobal(EvtRef evt, float delayInSeconds) {
	// Send two nulls for global events
	_Evt::EnqueueEvent(evt, delayInSeconds);
	
}

void EvtInvokeComponent(CmpRef component, EvtRef evt) {
	using namespace _Evt;

	// Try to look up the component in the map that corresponds to the event type
	EventCallback* callback = registrations[component->GetType()][evt->GetType()];
	if (callback != NULL) {
		// Found a matching component that is listening for the event
		// Call the callback on it
		if (auto object = component->Object()) {
			callback(component, object, evt);
		}
	}
}

void EvtInvokeStatic(EvtRef evt) {
	using namespace _Evt;

	// Call the handler for the component type whether or not a component exists.  
	// Pass null for the "self" pointer.
	for (int i = 0; i < Component::TYPE_COUNT; ++i) {
		EventCallback* callback = registrations[i][evt->GetType()];
		if (callback != NULL) {
			callback(NULL, NULL, evt);
		}
	}
}

void EvtQueueComponent(CmpRef component, EvtRef evt, float delayInSeconds) {
	// Send two component
	_Evt::EnqueueEvent(evt, component, delayInSeconds);
}

void EvtQueueGameObject(GoRef gameObject, EvtRef evt, float delayInSeconds) {
	// Send game object ID
	_Evt::EnqueueEvent(evt, gameObject, delayInSeconds);
}

void EvtAddComponent(CmpRef component) {
	using namespace _Evt;
	
	// Add the component to the set for each event type that it implements a callback for
	for (size_t i = 0; i < Event::TYPE_COUNT; ++i) {
		if (registrations[component->GetType()][i] != NULL) {
			componentsByEvent[i].insert(component);
		}
	}
}

void EvtRemoveComponent(CmpRef component) {
	using namespace _Evt;

	// Remove the component from the set for each event type that it implements a callback for
	for (size_t i = 0; i < Event::TYPE_COUNT; ++i) {
		if (registrations[component->GetType()][i] != NULL) {
			componentsByEvent[i].erase(component);
		}
	}
}

void EvtDispatchEvents(float deltaTime) {
	using namespace _Evt;

	// Add the delta time to the accumulated time
	accumulatedTime += deltaTime;

	// Start with global queue
	// Dispatch all the events for items at the top of the queue whos time stamp is now in the past
	while (!globalEventQueue.empty() && globalEventQueue.top().timeStamp <= accumulatedTime) {
		// Get the deferedEvent that's on top of the queue
		auto deferedEvent = globalEventQueue.top();

		// Dispatch the event to all components that subscribe to its type
		EvtQueueComponentsOfType(deferedEvent.evt);

		// Remove completed event from queue
		globalEventQueue.pop();
	}

	// Game Object event queue
	while (!goEventQueue.empty() && goEventQueue.top().timeStamp <= accumulatedTime) {
		// Get the deferedEvent that's on top of the queue
		auto deferedEvent = goEventQueue.top();

		// Send the event to the game object if it has not been deleted
		if (auto go = deferedEvent.gameObject.lock()) {
			EvtQueueComponentsInGameObject(go, deferedEvent.evt);
		}

		// Remove completed event from queue
		goEventQueue.pop();
	}

	// Component event queue
	while (!cmpEventQueue.empty() && cmpEventQueue.top().timeStamp <= accumulatedTime) {
		// Get the deferedEvent that's on top of the queue
		auto deferedEvent = cmpEventQueue.top();

		// Send the event to the game object if it has not been deleted
		if (auto cmp = deferedEvent.component.lock()) {
			EvtInvokeComponent(cmp, deferedEvent.evt);
		}

		// Remove completed event from queue
		cmpEventQueue.pop();
	}
}

// Event Creation Function

template <typename T>
shared_ptr<T> Event::Create(Event::Type type) {
	using namespace _Evt;

	// Allocate memory big enough to store the largest event
	EvtRef temp = EvtAllocate();

	// Construct the right object type
	switch (type) {
	case Event::ANIMATION_UPDATE:
	case Event::GENERAL_UPDATE:
	case Event::GAME_LOGIC_UPDATE:
	case Event::PHYSICS_UPDATE:
	case Event::BOUNDS_UPDATE:
		new (temp.get()) Evt::Update(type, temp);
		break;
	case Event::CREATED:
		new (temp.get()) Evt::Create(type, temp);
		break;
	case Event::MOUSE:
		new (temp.get()) Evt::Mouse(type, temp);
		break;
	case Event::KEYBOARD_KEY_DOWN:
	case Event::KEYBOARD_KEY_HELD:
	case Event::KEYBOARD_KEY_UP:
		new (temp.get()) Evt::Keyboard(type, temp);
		break;
	case Event::GAME_KEY_DOWN:
	case Event::GAME_KEY_HELD:
	case Event::GAME_KEY_UP:
		new (temp.get()) Evt::GameKey(type, temp);
		break;
	/*case Event::SPAWN_REQUESTED:
		new (temp.get()) Evt::SpawnRequested(type, temp);
		break;*/
	case Event::EDITOR_MENU_MESSAGE:
	case Event::EDITOR_PANEL_MESSAGE:
		new (temp.get()) Evt::Message(type, temp);
		break;
	case Event::SPAWN_COMPLETED:
		new (temp.get()) Evt::SpawnCompleted(type, temp);
		break;
	case Event::COLLISION:
		new (temp.get()) Evt::Collision(type, temp);
		break;
	case Event::SOUND_LOOPED:
	case Event::SOUND_STARTED:
	case Event::SOUND_STOPPED:
		new (temp.get()) Evt::Sound(type, temp);
		break;
	default:
		new (temp.get()) Event(type, temp);
		break;
	}

	// Try to do a dynamic cast to the requested type
	shared_ptr<T> typedEvent = dynamic_pointer_cast<T>(temp);

	if (!typedEvent) {
		// Dynamic cast failed, so return null
		// Event will be deleted naturally as smart pointer goes out of scope
		return NULL;
	}

	// Cast succeeded.
	return typedEvent;
}

template <typename T>
shared_ptr<T> Event::From(EvtRef evt) {
	// Pass through NULL to allow chaining
	if (evt == NULL) {
		return NULL;
	}

	// Try to do a dynamic cast to the requested type
	if (auto ref = evt->self.lock()) {
		return ref->To<T>();
	}

	// Self was deleted?
	return NULL;
}

template <typename T>
shared_ptr<T> Event::To() {
	// Try to do a dynamic cast to the requested type
	if (auto ref = self.lock()) {
		shared_ptr<T> typedEvent = dynamic_pointer_cast<T>(ref);

		if (!typedEvent) {
			// Dynamic cast failed, so return null
			return NULL;
		}

		return typedEvent;
	}

	// Self was deleted?
	return NULL;
}

EvtRef Event::Create(Event::Type type) {
	return Event::Create<Event>(type);
}
