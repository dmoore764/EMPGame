#pragma once

#include "auto_component.h"

class GameObject {
	friend class GoManager;
public:
	// Gets the component of the requested type.
	// No casting is done, so it will not be possible to work with the component's values until it is cast.
	CmpRef Get(Component::Type type);

	// Gets the component of the requested type and casts it so that the values can be used immediately.
	template <typename T>
	shared_ptr<T> Get();

	// Checks whether the object has a component of the given type.
	bool Has(Component::Type type);

	// Different version of the previous function that uses template syntax to be more like other functions.
	template <typename T>
	bool Has();

	// Adds a component of the given type and returns a reference to it.
	// No casting is done, so it will not be possible to work with the component's values until it is cast.
	CmpRef Add(Component::Type type, char* fileName = NULL);

	// Adds a component of the given type and casts it so that the values can be used immediately.
	template <typename T>
	shared_ptr<T> Add(char* fileName = NULL);

	// Remove the component of the given type
	void Remove(Component::Type type);

	// Different version of the previous function that uses template syntax to be more like other functions.
	template <typename T>
	void Remove();

	// Remove the given component
	void Remove(CmpRef component);

	// Shortcut to get the game object that contains a component reference.
	static GoRef Of(CmpRef component);

	// Shortcut to get the game object that contains a weak component reference.
	static GoRef Of(CmpWeakRef component);

	// Used to enable iteration.  Typically used with auto.
	class Iterator;

	// Get an iterator pointing to the first valid component in the list.
	// Must be lowercase to enable range-based for loops
	Iterator begin();

	// Get an iterator pointing to the end of the list of components
	// Must be lowercase to enable range-based for loops
	inline const Iterator& end();

	// Used for iteration.  See Begin and End functions.
	class Iterator {
		friend class GameObject;
	public:
		inline CmpRef& operator*();

		inline CmpRef& operator->();

		inline bool operator==(const Iterator& rhs);

		inline bool operator!=(const Iterator& rhs);

		inline void operator++();
	private:
		inline Iterator(GoRef gameObject, size_t index);

		GoRef gameObject;
		size_t index;
	};
private:
	GoWeakRef self;

	GameObject(GoRef centralReference);
	~GameObject();

	CmpRef components[Component::TYPE_COUNT];
	static const Iterator endIterator;
};

// Inline function definitions

CmpRef& GameObject::Iterator::operator*() {
	return gameObject->components[index];
}

CmpRef& GameObject::Iterator::operator->() {
	return gameObject->components[index];
}

bool GameObject::Iterator::operator==(const Iterator& rhs) {
	return index == rhs.index;
}

bool GameObject::Iterator::operator!=(const Iterator& rhs) {
	return index != rhs.index;
}

void GameObject::Iterator::operator++() {
	index++;
	while (gameObject->components[index] == NULL && index < Component::TYPE_COUNT) {
		index++;
	}
}

const GameObject::Iterator& GameObject::end() {
	return endIterator;
}

GameObject::Iterator::Iterator(GoRef gameObject, size_t index) {
	this->gameObject = gameObject;
	this->index = index;
}

// Make range-based for-loop work with GoRef

inline GameObject::Iterator begin(const GoRef& goRef) {
	return goRef->begin();
}

inline GameObject::Iterator end(const GoRef& goRef) {
	return goRef->end();
}
