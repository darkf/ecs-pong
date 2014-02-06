#include <SDL/SDL.h>
#include <vector>
#include <ecs.hpp>
#include <iostream>
#include <memory>

using EntityPtr = std::shared_ptr<Entity>;

class Renderer {
public:
	unsigned int screenWidth, screenHeight;
	SDL_Surface *screen;
	Uint32 black, red;
	Uint16 mouseX, mouseY;
	Renderer(const Renderer&) = delete;
	Renderer(const unsigned int screenWidth, const unsigned int screenHeight)
		: screenWidth(screenWidth), screenHeight(screenHeight),
		  mouseX(0), mouseY(0)
		{
			if(SDL_Init(SDL_INIT_VIDEO) > 0)
				throw "Couldn't initialize SDL";
			screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, 0);
			if(screen == nullptr)
				throw "Couldn't initialize screen";

			black = SDL_MapRGB(screen->format, 0, 0, 0);
			red = SDL_MapRGB(screen->format, 255, 0, 0);
		}

	void drawRect(const int x, const int y, const unsigned int w, const unsigned int h, Uint32 color) {
		SDL_Rect rect {(Sint16)x, (Sint16)y, (Uint16) w, (Uint16) h};
		SDL_FillRect(screen, &rect, color);
	}

	bool pollEvents() {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) return false;
			else if(event.type == SDL_MOUSEMOTION) {
				mouseX = event.motion.x;
				mouseY = event.motion.y;
			}
		}
		return true;
	}

	void clear() {
		SDL_FillRect(screen, NULL, black);
	}

	void flip() {
		SDL_Flip(screen);
	}
};

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

struct BounceComponent : Component {
	int w, h;
	BounceComponent(int w, int h) : w(w), h(h) {}
};

struct BounceSystem : System<BounceComponent, PositionComponent, RectComponent, VelocityComponent> {
	public:
		BounceSystem() {}

		void logic(Entity& e) {
			auto bounds = e.GetComponent<BounceComponent>();
			auto pos = e.GetComponent<PositionComponent>();
			auto vel = e.GetComponent<VelocityComponent>();
			auto rect = e.GetComponent<RectComponent>();

			if(pos->x < 0 || pos->x+rect->w >= bounds->w) vel->vx *= -1;
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
		AddComponent<BounceComponent>(boundW, boundH);
	}
};

struct AIComponent : Component {};

struct AISystem : System<AIComponent, PositionComponent> {
	private:
		std::shared_ptr<Ball> ball;
	public:
		AISystem(std::shared_ptr<Ball> ball) : ball(ball) {}

		void logic(Entity& e) {
			auto pos = e.GetComponent<PositionComponent>();
			int ballY = ball->GetComponent<PositionComponent>()->y;
			//std::cout << "ball: " << ballY << std::endl;
			pos->y += static_cast<int>( (ballY - pos->y) * 0.01f );
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

	std::shared_ptr<Ball> ball_;
	AISystem aiSystem;

	public:
	Game(Renderer& renderer) : renderer(renderer), rectRenderSystem(renderer),
	                           inputSystem(renderer),
	                           ball_(new Ball(32, 32, renderer.screenWidth, renderer.screenHeight)), aiSystem(ball_) {
		entities.push_back(ball_);

		EntityPtr leftPaddle = EntityPtr(new Entity);
		leftPaddle->AddComponent(new PositionComponent(5, 10));
		leftPaddle->AddComponent(new RectComponent(16, 16*4));
		leftPaddle->AddComponent(new UserInputComponent());
		entities.push_back(leftPaddle);

		EntityPtr rightPaddle = EntityPtr(new Entity);
		rightPaddle->AddComponent(new PositionComponent(600 - 5, 10));
		rightPaddle->AddComponent(new RectComponent(16, 16*4));
		rightPaddle->AddComponent(new AIComponent());
		entities.push_back(EntityPtr(rightPaddle));
	}

	void run() {
		// mainloop
		while(renderer.pollEvents()) {
			renderer.clear();
			for(EntityPtr& entity : entities) {
				inputSystem.process(*entity);
				bounceSystem.process(*entity);
				velSystem.process(*entity);
				rectRenderSystem.process(*entity);
				aiSystem.process(*entity);
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