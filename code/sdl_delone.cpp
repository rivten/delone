#include <SDL2/SDL.h>

#define ArrayCount(x) (sizeof((x))/(sizeof((x)[0])))
#define Assert(x) do{if(!(x)){*(int*)0=0;}}while(0)
#define MAX_POINT_COUNT 1000

static bool Running = true;

struct vertex
{
	int x;
	int y;
	bool IsFakePoint;
};

struct edge
{
	int Vertex0Index;
	int Vertex1Index;
};

struct triangle
{
	int Edge0Index;
	int Edge1Index;
	int Edge2Index;
};

struct triangulation
{
	vertex Vertices[MAX_POINT_COUNT];
	int VertexCount;

	edge Edges[MAX_POINT_COUNT];
	int EdgeCount;

	triangle Triangles[MAX_POINT_COUNT];
	int TriangleCount;
};

void PushVertex(triangulation* T, vertex V)
{
	Assert(T->VertexCount < ArrayCount(T->Vertices));
	T->Vertices[T->VertexCount] = V;
	T->VertexCount++;
}

void PushEdge(triangulation* T, edge E)
{
	Assert(T->EdgeCount < ArrayCount(T->Edges));
	T->Edges[T->EdgeCount] = E;
	T->EdgeCount++;
}

void PushTriangle(triangulation* T, triangle F)
{
	Assert(T->TriangleCount < ArrayCount(T->Triangles));
	T->Triangles[T->TriangleCount] = F;
	T->TriangleCount++;
}

void ComputeDelaunay(triangulation* T)
{
	vertex S = T->Vertices[T->VertexCount];
	Assert(!S.IsFakePoint);
}

int main(int ArgumentCount, char** Arguments)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* Window = SDL_CreateWindow("Delaunay Triangulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600, 0);
	Assert(Window);

	SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);
	Assert(Renderer);

	// NOTE(hugo) : Init graph
	triangulation T = {};
	T.VertexCount = 0;
	T.EdgeCount = 0;
	T.TriangleCount = 0;

	vertex FakePoint0 = {-10, -100, true};
	vertex FakePoint1 = {-10, 5000, true};
	vertex FakePoint2 = {5000, 5000, true};
	edge E01 = {0, 1};
	edge E12 = {1, 2};
	edge E20 = {2, 0};
	triangle F = {0, 1, 2};
	PushVertex(&T, FakePoint0);
	PushVertex(&T, FakePoint1);
	PushVertex(&T, FakePoint2);
	PushEdge(&T, E01);
	PushEdge(&T, E12);
	PushEdge(&T, E20);
	PushTriangle(&T, F);

	bool DirtyTriangulation = false;

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
				case SDL_MOUSEBUTTONDOWN:
					{
						if(Event.button.button == SDL_BUTTON_LEFT)
						{
							vertex V = {Event.button.x, Event.button.y, false};
							PushVertex(&T, V);
							DirtyTriangulation = true;
						}
					} break;
			}
		}

		// NOTE(hugo) : Triangulation processing
		if(DirtyTriangulation)
		{
			ComputeDelaunay(&T);
			DirtyTriangulation = false;
		}

		// NOTE(hugo) : Rendering !
		SDL_SetRenderDrawColor(Renderer, 119, 136, 153, 255);
		SDL_RenderClear(Renderer);
		SDL_SetRenderDrawColor(Renderer, 20, 20, 20, 255);

		for(int VertexIndex = 0; VertexIndex < T.VertexCount; ++VertexIndex)
		{
			vertex V = T.Vertices[VertexIndex];
			if(!V.IsFakePoint)
			{
				SDL_Rect VertexRect;
				VertexRect.x = V.x - 2;
				VertexRect.y = V.y - 2;
				VertexRect.w = 5;
				VertexRect.h = 5;
				SDL_RenderDrawRect(Renderer, &VertexRect);
			}
		}

		for(int EdgeIndex = 0; EdgeIndex < T.EdgeCount; ++EdgeIndex)
		{
			edge E = T.Edges[EdgeIndex];
			vertex V = T.Vertices[E.Vertex0Index];
			vertex W = T.Vertices[E.Vertex1Index];
			if((!V.IsFakePoint) && (!W.IsFakePoint))
			{
				SDL_RenderDrawLine(Renderer, V.x, V.y, W.x, W.y);
			}
		}

		SDL_RenderPresent(Renderer);
	}

	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	return(0);
}
