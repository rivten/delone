#include <SDL2/SDL.h>

#define ArrayCount(x) (sizeof((x))/(sizeof((x)[0])))
#define Assert(x) do{if(!(x)){*(int*)0=0;}}while(0)
#define MAX_POINT_COUNT 100

static bool Running = true;

struct vertex
{
	int x;
	int y;

	int Neighbours[MAX_POINT_COUNT];
	int NeighbourCount;
};

void PushVertex(vertex* Vertices, vertex V, int* VertexCount)
{
	Vertices[*VertexCount] = V;
	(*VertexCount)++;
}

int main(int ArgumentCount, char** Arguments)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* Window = SDL_CreateWindow("Delaunay Triangulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600, 0);
	Assert(Window);

	SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);
	Assert(Renderer);

	// NOTE(hugo) : Init graph
	vertex Vertices[MAX_POINT_COUNT];
	int VertexCount = 0;

	vertex V = {20, 20};
	PushVertex(Vertices, V, &VertexCount);

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
		SDL_SetRenderDrawColor(Renderer, 255, 0, 0, 255);

		for(int VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
		{
			SDL_Rect VertexRect;
			VertexRect.x = Vertices[VertexIndex].x;
			VertexRect.y = Vertices[VertexIndex].y;
			VertexRect.w = 3;
			VertexRect.h = 3;
			SDL_RenderDrawRect(Renderer, &VertexRect);
		}

		SDL_RenderPresent(Renderer);
	}

	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	return(0);
}
