#include <assert.h>

#include "game_object.h"
#include "go_manager.h"

GameObject::GameObject(GoRef centralReference) {
	// Save a weak version of the central reference used to hold this game object.
	this->self = centralReference;

	// Make sure all the component references are null.
	for (size_t i = 0; i < Component::TYPE_COUNT; ++i) {
		components[i].reset();
	}
}

GameObject::~GameObject() {
	// Clear all components
	for (size_t i = 0; i < Component::TYPE_COUNT; ++i) {
		components[i].reset();
	}
}

CmpRef GameObject::Get(Component::Type type) {
	assert(type >= 0 && type < Component::TYPE_COUNT);

	// The components array is indexed by type
	return components[type];
}

template <typename T>
shared_ptr<T> GameObject::Get() {
	CmpRef component = Get(T::TYPE);

	if (component == NULL) {
		return NULL;
	}

	return Component::From<T>(component);
}

bool GameObject::Has(Component::Type type) {
	assert(type >= 0 && type < Component::TYPE_COUNT);

	return components[type] != NULL;
}

template <typename T>
bool GameObject::Has() {
	return Has(T::TYPE);
}

CmpRef GameObject::Add(Component::Type type, char* fileName) {
	assert(type >= 0 && type < Component::TYPE_COUNT);
	assert(!components[type]);

	// Make a reference to reduce typing
	CmpRef& newComponent = components[type];

	// Allocate memory for new component
	newComponent = GoManager::AllocateComponent();

	// call constructor
	Component::ConstructComponent(type, self.lock(), newComponent);

	// Add component to event system so it can get events
	EvtAddComponent(newComponent);

	//If a file name and a deseralize callback were supplied, deseralie the component
	if (fileName != NULL && GoManager::deseralizeCallback != NULL) {
		GoManager::deseralizeCallback(newComponent.get(), fileName);
	}

	// Send a create event to the component so it will construct itself
	// It can search for other components of interest and cache their values at this time.
	auto createEvent = Event::Create<Evt::Create>(Event::CREATED);
	EvtInvokeComponent(newComponent, createEvent);

	return newComponent;
}

template <typename T>
shared_ptr<T> GameObject::Add(char* fileName) {
	auto ref = Add(T::TYPE, fileName);

	if (ref == NULL) {
		return NULL;
	}

	return Component::From<T>(ref);
}

void GameObject::Remove(Component::Type type) {
	assert(type >= 0 && type < Component::TYPE_COUNT);
	assert(components[type]);

	// Set the component reference to NULL.
	// This will release our hold on it, but it will actually be deleted once all outstanding references are released.
	components[type].reset();
}

void GameObject::Remove(CmpRef component) {
	assert(this == component->Object().get());

	// Defer to type-based version since it knows the offset of the component in the list
	// and thus won't have to search for it.
	Remove(component->GetType());
}

template <typename T>
void GameObject::Remove() {
	Remove(T::TYPE);
}

GameObject::Iterator const GameObject::endIterator = GameObject::Iterator(NULL, Component::TYPE_COUNT);

GameObject::Iterator GameObject::begin() {
	if (auto ref = self.lock()) {
		// Find the first valid component
		for (size_t i = 0; i < Component::TYPE_COUNT; ++i) {
			if (components[i]) {
				// Found a valid component
				return Iterator(ref, i);
			}
		}
	}

	// Couldn't find anything
	return Iterator(NULL, Component::TYPE_COUNT);
}

GoRef GameObject::Of(CmpRef component) {
	// Pass through a null that is passed in to allow safe chaining
	if (component == NULL) {
		return NULL;
	}

	return component->Object();
}
GoRef GameObject::Of(CmpWeakRef component) {
	// Try to lock the component
	if (auto cmp = component.lock()) {
		return cmp->Object();
	}

	// Failed to lock the object, it must be deleted
	return NULL;
}


// Implementation for component functions

GoRef Component::Object() {
	// Try to get the game object from the coponent
	if (auto go = gameObject.lock()) {
		return go;
	}

	// Failed to lock the object, it must be deleted
	return NULL;
}

CmpRef Component::From(GoRef gameObject, Component::Type type) {
	// Pass through a null that is passed in to allow safe chaining
	if (gameObject == NULL) {
		return NULL;
	}

	return gameObject->Get(type);
}

CmpRef Component::From(GoWeakRef gameObject, Component::Type type) {
	// Try to get the game object from the reference
	if (auto go = gameObject.lock()) {
		return go->Get(type);
	}

	// Failed to lock the object, it must be deleted
	return NULL;
}

CmpRef Component::From(CmpWeakRef component) {
	// Try to get the component object from the reference
	if (auto cmp = component.lock()) {
		return cmp;
	}

	return NULL;
}

template <typename T>
shared_ptr<T> Component::From(GoRef gameObject) {
	auto ref = From(gameObject, T::TYPE);

	if (ref == NULL) {
		return NULL;
	}

	// https://stackoverflow.com/questions/3786360/confusing-template-error
	return ref->template To<T>();
}

template <typename T>
shared_ptr<T> Component::From(GoWeakRef gameObject) {
	auto ref = From(gameObject, T::TYPE);

	if (ref == NULL) {
		return NULL;
	}

	// https://stackoverflow.com/questions/3786360/confusing-template-error
	return ref->template To<T>();
}

template <typename T>
shared_ptr<T> Component::From(CmpWeakRef component) {
	auto ref = From(component);

	if (ref == NULL) {
		return NULL;
	}

	return From<T>(ref);
}

template <typename T>
shared_ptr<T> Component::To() {
	// Try to do a dynamic cast to the requested type
	if (auto cmp = self.lock()) {
		shared_ptr<T> typedCmp = dynamic_pointer_cast<T>(cmp);

		if (!typedCmp) {
			// Dynamic cast failed, so return null
			return NULL;
		}

		return typedCmp;
	}

	// Self was deleted?
	return NULL;
}

template <typename T>
shared_ptr<T> Component::From(CmpRef component) {
	// Passthrough null to allow chaining
	if (component == NULL) {
		return NULL;
	}

	// Try to do a dynamic cast to the requested type
	if (auto cmp = component->self.lock()) {
		return cmp->To<T>();
	}

	// Self was deleted?
	return NULL;
}
