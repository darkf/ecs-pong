#pragma once
#include <functional>
#include <map>
#include <typeinfo>
#include <iostream>

struct Event {
	virtual ~Event() {}
};

struct TestEvent : Event {
	std::string msg;
	TestEvent(std::string msg) : msg(msg) {}
};

class Entity;
struct CollisionEvent : Event {
	const Entity& a;
	const Entity& b;
	CollisionEvent(const Entity& a, const Entity& b) : a(a), b(b) {}
};


// Has the limitation that on<T> will not catch subtypes of T, only T.
// That may or may not be a problem for your usecase.
//
// It also doesn't (yet) let you use the subtype as an argument.

typedef std::multimap<const std::type_info*,
					  const std::function<void(const Event&)>> EventMap;

class event {
	private:
		static EventMap eventMap;

	public:
		template<typename EventWanted>
		static void on(const std::function<void(const Event&)>& fn) {
			event::eventMap.emplace(&typeid(EventWanted), fn);
		}

		static void emit(const Event& event) {
			auto range = eventMap.equal_range(&typeid(event));
			for(auto it = range.first; it != range.second; ++it)
				it->second(event);

		}
};
