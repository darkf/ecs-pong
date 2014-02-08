#include <vector>
#include <iostream>
#include <memory>
#include <ecs.hpp>
#include "event.hpp"
#include "renderer.hpp"

const float AI_SPEED = 0.15f; // in percentage of distance/s

EventMap event::eventMap;

using EntityPtr = std::shared_ptr<Entity>;

struct PositionComponent : Component {
	int x, y;
	PositionComponent(int x, int y) : x(x), y(y) {}
};

struct VelocityComponent : Component {
	int vx, vy;
	VelocityComponent(int vx, int vy) : vx(vx), vy(vy) {}
};

struct VelocitySystem : System<PositionComponent, VelocityComponent> {
	VelocitySystem() {}

	void logic(Entity& e) {
		auto pos = e.GetComponent<PositionComponent>();
		auto vel = e.GetComponent<VelocityComponent>();
		pos->x += vel->vx;
		pos->y += vel->vy;
	}
};

struct RectComponent : Component {
	int w, h;
	RectComponent(int w, int h) : w(w), h(h) {}
};

class RectRenderingSystem : public System<PositionComponent, RectComponent> {
private:
	Renderer& r;

public:
	RectRenderingSystem(Renderer& renderer) : r(renderer) {}

	void logic(Entity& e) {
		auto pos = e.GetComponent<PositionComponent>();
		auto rect = e.GetComponent<RectComponent>();
		r.drawRect(pos->x, pos->y, rect->w, rect->h, r.red);
	}
};

struct UserInputComponent : Component {};

class InputSystem : public System<UserInputComponent, PositionComponent> {
	private:
		Renderer& r;
	public:
		InputSystem(Renderer& r) : r(r) {}

		void logic(Entity& e) {
			e.GetComponent<PositionComponent>()->y = r.mouseY;
		}
};

struct Collidable : Component {};

struct BounceComponent : Component {
	int w, h;
	BounceComponent(int w, int h) : w(w), h(h) {}
};

struct BounceSystem : System<BounceComponent, PositionComponent, RectComponent, VelocityComponent> {
	public:
		BounceSystem() {
			event::on<CollisionEvent>(std::bind(&BounceSystem::onCollision, this, std::placeholders::_1));
		}

		void onCollision(const Event& e) {
			auto col = static_cast<const CollisionEvent&>(e);
			if(col.a.GetComponent<BounceComponent>() != nullptr) {
				// col.a is bouncy, col.b will be whatever it collided with
				auto vel = col.a.GetComponent<VelocityComponent>();
				vel->vx *= -1;
				vel->vy *= -1;
			}
		}

		void logic(Entity& e) {
			auto bounds = e.GetComponent<BounceComponent>();
			auto pos = e.GetComponent<PositionComponent>();
			auto vel = e.GetComponent<VelocityComponent>();
			auto rect = e.GetComponent<RectComponent>();

			if(bounds->w != -1)
				if(pos->x < 0 || pos->x+rect->w >= bounds->w) vel->vx *= -1;
			if(bounds->h != -1)
				if(pos->y < 0 || pos->y+rect->h >= bounds->h) vel->vy *= -1;
		}
};

struct BallComponent : Component {};
struct Ball : Entity {
	Ball(int x, int y, int boundW, int boundH) : Entity() {
		AddComponent<BallComponent>();
		AddComponent<PositionComponent>(x, y);
		AddComponent<RectComponent>(8, 8);
		AddComponent<VelocityComponent>(8, 8);
		AddComponent<BounceComponent>(-1, boundH);
		AddComponent<Collidable>();
	}

	Ball(int boundW, int boundH) : Ball(boundW/2, boundH/2, boundW, boundH) {}
};

struct BallSystem : System<BallComponent, PositionComponent, RectComponent> {
	int boundsW, boundsH;
	BallSystem(int boundsW, int boundsH) : boundsW(boundsW), boundsH(boundsH) {}

	void logic(Entity& e) {
		auto pos = e.GetComponent<PositionComponent>();
		auto vel = e.GetComponent<VelocityComponent>();
		auto rect = e.GetComponent<RectComponent>();

		if(pos->x <= 0 || pos->x+rect->w >= boundsW) {
			// ball hit left/right edge
			bool left = (pos->x <= 0);
			event::emit(EdgeCollisionEvent(e, left));

			// reset us to the middle of the screen
			pos->x = boundsW/2;
			pos->y = boundsH/2;

			// going towards which player lost
			if(left) {
				vel->vx = -std::abs(vel->vx);
				//vel->vy = -std::abs(vel->vy);
			} else vel->vx = std::abs(vel->vx);
		}
	}
};

struct AIComponent : Component {};

struct AISystem : System<AIComponent, PositionComponent> {
	private:
		std::shared_ptr<Ball> ball;
		float speed;
	public:
		AISystem(std::shared_ptr<Ball> ball, float speed) : ball(ball), speed(speed) {}

		void logic(Entity& e) {
			auto pos = e.GetComponent<PositionComponent>();
			int ballY = ball->GetComponent<PositionComponent>()->y;
			//std::cout << "ball: " << ballY << std::endl;
			pos->y += static_cast<int>( (ballY - pos->y) * speed );
		}
};

struct CollisionSystem : System<Collidable, PositionComponent, RectComponent> {
	private:
		std::vector<EntityPtr>& entities;
	public:
		CollisionSystem(std::vector<EntityPtr>& entities) : entities(entities) {}

		void logic(Entity& a) {
			// todo: when collision is checked with (a,b) then we don't need to check (b,a)
			for(EntityPtr b : entities) {
				if(&a == b.get()) continue; // don't check collision with itself

				if(collides(a, *b))
					event::emit(CollisionEvent(a, *b));
			}
		}

		bool collides(const Entity& a, const Entity& b) const {
			auto& posA = *a.GetComponent<PositionComponent>();
			auto& posB = *b.GetComponent<PositionComponent>();

			auto& rectA = *a.GetComponent<RectComponent>();
			auto& rectB = *b.GetComponent<RectComponent>();

			if ((posA.y+rectA.h <= posB.y) || (posA.y >= posB.y+rectB.h)) return false;
			if ((posA.x+rectA.w <= posB.x) || (posA.x >= posB.x+rectB.w)) return false;
			return true;
		}
};

class Game {
	private:
	Renderer& renderer;
	std::vector<EntityPtr> entities;

	VelocitySystem velSystem;
	BounceSystem bounceSystem;
	RectRenderingSystem rectRenderSystem;
	InputSystem inputSystem;
	CollisionSystem collisionSystem;
	BallSystem ballSystem;

	std::shared_ptr<Ball> ball_;
	AISystem aiSystem;

	int scoreL, scoreR;

	public:
	Game(Renderer& renderer) : renderer(renderer), rectRenderSystem(renderer),
	                           inputSystem(renderer), collisionSystem(entities), ballSystem(renderer.screenWidth, renderer.screenHeight),
	                           ball_(new Ball(renderer.screenWidth, renderer.screenHeight)), aiSystem(ball_, AI_SPEED),
	                           scoreL(0), scoreR(0) {
		entities.push_back(ball_);

		EntityPtr leftPaddle = EntityPtr(new Entity);
		leftPaddle->AddComponent(new PositionComponent(5, 10));
		leftPaddle->AddComponent(new RectComponent(16, 16*4));
		leftPaddle->AddComponent(new UserInputComponent());
		leftPaddle->AddComponent<Collidable>();
		entities.push_back(leftPaddle);

		EntityPtr rightPaddle = EntityPtr(new Entity);
		rightPaddle->AddComponent(new PositionComponent(600 - 5, 10));
		rightPaddle->AddComponent(new RectComponent(16, 16*4));
		rightPaddle->AddComponent(new AIComponent());
		rightPaddle->AddComponent<Collidable>();
		entities.push_back(EntityPtr(rightPaddle));


		event::on<EdgeCollisionEvent>(std::bind(&Game::scoreChanged, this, std::placeholders::_1));
	}

	void scoreChanged(const Event& e) {
		// edge hit, update score
		auto edgeCol = static_cast<const EdgeCollisionEvent&>(e);
		if(edgeCol.left) scoreL++;
		else scoreR++;
		std::cout << "Player " << (edgeCol.left ? "Left" : "Right") <<
				  " scores! Their score is now " << (edgeCol.left ? scoreL : scoreR) << "." << std::endl;
	}

	void run() {
		// mainloop
		while(renderer.pollEvents()) {
			renderer.clear();
			for(EntityPtr& entity : entities) {
				inputSystem.process(*entity);
				velSystem.process(*entity);
				bounceSystem.process(*entity);
				ballSystem.process(*entity);
				aiSystem.process(*entity);
				collisionSystem.process(*entity);
				rectRenderSystem.process(*entity);
			}
			renderer.flip();
			SDL_Delay(1000 / 30);
		}
	}
};

int main(int argc, char *argv[]) {
	Renderer renderer(800, 600);
	Game game(renderer);
	game.run();
	return 0;
}