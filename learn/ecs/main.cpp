#include <iostream>
#include <entt/entt.hpp>

struct Position {
	float x;
	float y;
};

struct Velocity {
	float dx;
	float dy;
};

int main() {
	//tao registry truoc
	entt::registry registry; //so dang ky component
	entt::entity e = registry.create(); //tao entity
	entt::entity e2 = registry.create(); //tao entity
	registry.emplace<Position>(e, 0.0f, 0.0f);
	registry.emplace<Velocity>(e, 0.0f, 0.0f);
	registry.emplace<Position>(e2, 0.0f, 0.0f);
	registry.emplace<Velocity>(e2, 0.0f, 0.0f);
	//entity e 2 component Position va Velocity
	auto view = registry.view<Position, Velocity>();
	for (auto entity : view) {
		auto& vel = view.get<Velocity>(entity);
	}
	auto t = registry.get<Velocity>(e2);
	int a = 3;
	return 0;
}