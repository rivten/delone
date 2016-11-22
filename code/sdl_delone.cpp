#include <SDL2/SDL.h>

#define Assert(x) do{if(!(x)){*(int*)0=0;}}while(0)

static bool Running = true;

int main(int ArgumentCount, char** Arguments)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* Window = SDL_CreateWindow("Delaunay Triangulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600, 0);
	Assert(Window);

	SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);
	Assert(Renderer);

	while(Running)
	{
		// NOTE(hugo) : Event handling
		SDL_Event Event;
		while(SDL_PollEvent(&Event))
		{
			switch(Event.type)
			{
				case SDL_QUIT:
					{
						Running = false;
					} break;
			}
		}

		SDL_SetRenderDrawColor(Renderer, 119, 136, 153, 255);
		SDL_RenderClear(Renderer);
		SDL_RenderPresent(Renderer);
	}

	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	return(0);
}
