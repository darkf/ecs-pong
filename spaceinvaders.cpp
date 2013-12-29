#include <SDL/SDL.h>
#include <vector>
#include <ecs.hpp>

class Renderer {
public:
	unsigned int screenWidth, screenHeight;
	SDL_Surface *screen;
	Uint32 black, red;
	Renderer(const unsigned int screenWidth, const unsigned int screenHeight)
		: screenWidth(screenWidth), screenHeight(screenHeight)
		{
			if(SDL_Init(SDL_INIT_VIDEO) > 0)
				throw "Couldn't initialize SDL";
			screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, 0);
			if(screen == nullptr)
				throw "Couldn't initialize screen";

			black = SDL_MapRGB(screen->format, 0, 0, 0);
			red = SDL_MapRGB(screen->format, 255, 0, 0);
		}

	bool pollEvents() {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) return false;
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

class PositionComponent : public Component {
	public:
	int x, y;
	PositionComponent(int x, int y) : x(x), y(y) {}
};

class VelocityComponent : public Component {
	public:
	int vx, vy;
	VelocityComponent(int vx, int vy) : vx(vx), vy(vy) {}
};

class VelocitySystem : public System<PositionComponent, VelocityComponent> {
	public:
	VelocitySystem() {}

	void logic(Entity& e) {
		auto pos = e.GetComponent<PositionComponent>();
		auto vel = e.GetComponent<VelocityComponent>();
		pos->x += vel->vx;
		pos->y += vel->vy;
	}
};

class RenderingSystem : public System<PositionComponent> {
private:
	Renderer& r;

public:
	RenderingSystem(Renderer& renderer) : r(renderer) {}

	void logic(Entity& e) {
		auto pos = e.GetComponent<PositionComponent>();
		SDL_Rect rect {(short)pos->x, (short)pos->y, 8, 8};
		SDL_FillRect(r.screen, &rect, r.red);
	}
};

class Game {
	private:
	Renderer renderer;
	std::vector<Entity> entities;
	VelocitySystem velSystem;
	RenderingSystem renderSystem;

	public:
	Game(Renderer& renderer) : renderer(renderer), renderSystem(renderer) {
		Entity e;
		e.AddComponent(new PositionComponent(32, 32));
		e.AddComponent(new VelocityComponent(1, 1));
		entities.push_back(e);
	}

	void run() {
		// mainlop
		while(renderer.pollEvents()) {
			renderer.clear();
			velSystem.process(entities);
			renderSystem.process(entities);
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