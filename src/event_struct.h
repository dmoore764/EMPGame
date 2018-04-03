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
		STATIC_INIT,
		CREATED,
		AFTER_INIT,
		GAME_LOGIC_UPDATE,
		PHYSICS_UPDATE,
		BOUNDS_UPDATE,
		DESTROYED,
		KEYBOARD_KEY_DOWN,
		KEYBOARD_KEY_HELD,
		KEYBOARD_KEY_UP,
		GAME_KEY_DOWN,
		GAME_KEY_HELD,
		GAME_KEY_UP,
		MOUSE,
		DELAY_TEST,
		DRAW,
		DRAW_GUI,
		COLLISION,
		SPAWN_COMPLETED,
		SCORE_CHANGED,
		TYPE_COUNT // Alawys must be at the end
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

	struct Update : Event {
		float deltaTime = 0.0f; // Time that has passed since the last update event

	protected:
		inline Update(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};

	struct Create : Event {
		GoWeakRef gameObject; // The id of the game object that the component added to

	protected:
		inline Create(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};

	struct Mouse : Event {
		bool press[3] = {};
		bool down[3] = {};
		bool release[3] = {};
		bool isOver = false;
		uint32_t count = 0; // The number of times the button was clicked (2 = double click, 3 = tripple click, etc.)

		GuiLayer layer = GuiLayer::GUI;
		vec2 coord; // The mouse coordinates in the game world

		vec2 gameDelta; // The mouse movement since the last mouse event in the game world coordinate system
		vec2 screenDelta; // The movement since the last mouse event in the screen coordinate system

		bool altKey = false; // True if the Alt/Option key was down when the mouse event was fired.
		bool ctrlKey = false; // True if the Ctrl/Command key was down when the mouse event was fired.
		bool shiftKey = false; // True if the Shift key was down when the mouse event was fired.
		bool handled = false;

	protected:
		inline Mouse(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};

	struct Keyboard : Event {
		keyboard_keys code = keyboard_keys::KB_NONE; // The enumerated value of the physical key that was pressed

		bool altKey = false; // True if the Alt/Option key was down when the mouse event was fired.
		bool ctrlKey = false; // True if the Ctrl/Command key was down when the mouse event was fired.
		bool shiftKey = false; // True if the Shift key was down when the mouse event was fired.

	protected:
		inline Keyboard(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};

	struct GameKey : Event {
		game_keys code = game_keys::KEY_Action1; // The enumerated game key

	protected:
		inline GameKey(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};

	/*struct SpawnRequested : Event {
		PrefabType prefabType = PrefabType::NONE; // The type of object to spawn
		vec2 pos; // The position to spawn it at
		float rot = 0.0f; // The initial rotation
		GoWeakRef requester; // The game object ID of the game object that is interested in knowing when the spawn completes.  Can be zero if no interest.

	protected:
		inline SpawnRequested(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};*/

	struct SpawnCompleted : Event {
		GoWeakRef spawnedObject; // The game object ID of the object that was spawned.

	protected:
		inline SpawnCompleted(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};

	struct Collision : Event {
		GoWeakRef otherObject; // The other object involved in the collision
		size_t ownColliderIndex;
		size_t otherColliderIndex;

	protected:
		inline Collision(Event::Type type, EvtRef self) : Event(type, self) {}
		friend class Event;
	};
} // End namespace

// Used to figure out the maximum size of any event type
struct ALL_EVENTS_UNION {
	Evt::Create creationEvent;
	Evt::Mouse mouseEvent;
	Evt::Keyboard keyboardEvent;
	Evt::Message messageEvent;
	Evt::GameKey gameKeyEvent;
	Evt::SpawnCompleted spawnCompletedEvent;
	Evt::Collision collisionEvent;
};
