#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define ArrayCount(x) (sizeof((x))/(sizeof((x)[0])))
#define Assert(x) do{if(!(x)){*(int*)0=0;}}while(0)
#define MAX_POINT_COUNT 1000

/*
 * TODO(hugo)
 *   - make the program robust 
 *		+ no Assert popping
 *		+ handling degenerate cases (3 vertices on one line, 4 vertices on one circle)
 *		+ disabling edge flip osscilation (rare but can happen)
 *	 - improve the algorithm (real incremental, not naive incremental)
 *	 - clean the render stuff I did for debugging purposes
 *	 - consider improving the data structure for effiency purposes
 */

static bool Running = true;
static int ScreenWidth = 600;
static int ScreenHeight = 600;

/* ------------------------------
 *           mat4 
 * ------------------------------ */
struct mat4
{
    float Data_[16];
};

inline float GetValue(mat4 M, int i, int j)
{
    Assert(i >= 0);
    Assert(j >= 0);
    Assert(i < 4);
    Assert(j < 4);
    return(M.Data_[4 * j + i]);
}

inline void SetValue(mat4* M, int i, int j, float Value)
{
    Assert(i >= 0);
    Assert(j >= 0);
    Assert(i < 4);
    Assert(j < 4);
    M->Data_[4 * j + i] = Value;
}

float Det(mat4 M)
{
	float Result = 0.0f;

	Result += GetValue(M, 0, 0) * GetValue(M, 1, 1) * GetValue(M, 2, 2) * GetValue(M, 3, 3);
	Result += GetValue(M, 0, 0) * GetValue(M, 1, 2) * GetValue(M, 2, 3) * GetValue(M, 3, 1);
	Result += GetValue(M, 0, 0) * GetValue(M, 1, 3) * GetValue(M, 2, 1) * GetValue(M, 3, 2);

	Result += GetValue(M, 0, 1) * GetValue(M, 1, 0) * GetValue(M, 2, 3) * GetValue(M, 3, 2);
	Result += GetValue(M, 0, 1) * GetValue(M, 1, 2) * GetValue(M, 2, 0) * GetValue(M, 3, 3);
	Result += GetValue(M, 0, 1) * GetValue(M, 1, 3) * GetValue(M, 2, 2) * GetValue(M, 3, 0);

	Result += GetValue(M, 0, 2) * GetValue(M, 1, 0) * GetValue(M, 2, 1) * GetValue(M, 3, 3);
	Result += GetValue(M, 0, 2) * GetValue(M, 1, 1) * GetValue(M, 2, 3) * GetValue(M, 3, 0);
	Result += GetValue(M, 0, 2) * GetValue(M, 1, 3) * GetValue(M, 2, 0) * GetValue(M, 3, 1);

	Result += GetValue(M, 0, 3) * GetValue(M, 1, 0) * GetValue(M, 2, 2) * GetValue(M, 3, 1);
	Result += GetValue(M, 0, 3) * GetValue(M, 1, 1) * GetValue(M, 2, 0) * GetValue(M, 3, 2);
	Result += GetValue(M, 0, 3) * GetValue(M, 1, 2) * GetValue(M, 2, 1) * GetValue(M, 3, 0);

	Result -= GetValue(M, 0, 0) * GetValue(M, 1, 1) * GetValue(M, 2, 3) * GetValue(M, 3, 2);
	Result -= GetValue(M, 0, 0) * GetValue(M, 1, 2) * GetValue(M, 2, 1) * GetValue(M, 3, 3);
	Result -= GetValue(M, 0, 0) * GetValue(M, 1, 3) * GetValue(M, 2, 2) * GetValue(M, 3, 1);
	
	Result -= GetValue(M, 0, 1) * GetValue(M, 1, 0) * GetValue(M, 2, 2) * GetValue(M, 3, 3);
	Result -= GetValue(M, 0, 1) * GetValue(M, 1, 2) * GetValue(M, 2, 3) * GetValue(M, 3, 0);
	Result -= GetValue(M, 0, 1) * GetValue(M, 1, 3) * GetValue(M, 2, 0) * GetValue(M, 3, 2);

	Result -= GetValue(M, 0, 2) * GetValue(M, 1, 0) * GetValue(M, 2, 3) * GetValue(M, 3, 1);
	Result -= GetValue(M, 0, 2) * GetValue(M, 1, 1) * GetValue(M, 2, 0) * GetValue(M, 3, 3);
	Result -= GetValue(M, 0, 2) * GetValue(M, 1, 3) * GetValue(M, 2, 1) * GetValue(M, 3, 0);

	Result -= GetValue(M, 0, 3) * GetValue(M, 1, 0) * GetValue(M, 2, 1) * GetValue(M, 3, 2);
	Result -= GetValue(M, 0, 3) * GetValue(M, 1, 1) * GetValue(M, 2, 2) * GetValue(M, 3, 0);
	Result -= GetValue(M, 0, 3) * GetValue(M, 1, 2) * GetValue(M, 2, 0) * GetValue(M, 3, 1);

	return(Result);
}


/* ------------------------------
 *        triangulation 
 * ------------------------------ */

struct vertex
{
	int x;
	int y;
	bool IsRealPoint;
};

struct edge
{
	int Vertex0Index;
	int Vertex1Index;
};

struct triangle
{
	union
	{
		struct
		{
			int Edge0Index;
			int Edge1Index;
			int Edge2Index;
		};
		int EdgeIndices[3];
	};

	union
	{
		struct
		{
			int Vertex0Index;
			int Vertex1Index;
			int Vertex2Index;
		};
		int VertexIndices[3];
	};
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


bool IsVertexInTriangle(triangulation* T, int VIndex, int FIndex)
{
	triangle F = T->Triangles[FIndex];
	for(int i = 0; i < ArrayCount(F.VertexIndices); ++i)
	{
		if(VIndex == F.VertexIndices[i])
		{
			return(true);
		}
	}

	return(false);
}

bool IsEdgeInTriangle(triangulation* T, int AIndex, int BIndex, int FIndex)
{
	triangle F = T->Triangles[FIndex];
	for(int i = 0; i < ArrayCount(F.EdgeIndices); ++i)
	{
		edge E = T->Edges[F.EdgeIndices[i]];
		int UIndex = E.Vertex0Index;
		int VIndex = E.Vertex1Index;
		if(((UIndex == AIndex) && (VIndex == BIndex)) || ((UIndex == BIndex) && (VIndex == AIndex)))
		{
			return(true);
		}
	}

	return(false);
}

bool IsTriangleValid(triangulation* T, int FIndex)
{
	triangle F = T->Triangles[FIndex];

	if((F.Edge0Index == F.Edge1Index) || (F.Edge0Index == F.Edge2Index) || (F.Edge1Index == F.Edge2Index))
	{
		return(false);
	}
	if((F.Vertex0Index == F.Vertex1Index) || (F.Vertex0Index == F.Vertex2Index) || (F.Vertex1Index == F.Vertex2Index))
	{
		return(false);
	}

	for(int i = 0; i < ArrayCount(F.EdgeIndices); ++i)
	{
		int EdgeIndex = F.EdgeIndices[i];
		int AIndex = T->Edges[EdgeIndex].Vertex0Index;

		if(!IsVertexInTriangle(T, AIndex, FIndex))
		{
			return(false);
		}

		int BIndex = T->Edges[EdgeIndex].Vertex1Index;
		if(!IsVertexInTriangle(T, BIndex, FIndex))
		{
			return(false);
		}
	}

	if(!IsEdgeInTriangle(T, F.Vertex0Index, F.Vertex1Index, FIndex))
	{
		return(false);
	}
	if(!IsEdgeInTriangle(T, F.Vertex1Index, F.Vertex2Index, FIndex))
	{
		return(false);
	}
	if(!IsEdgeInTriangle(T, F.Vertex0Index, F.Vertex2Index, FIndex))
	{
		return(false);
	}


	return(true);
}

bool IsTriangulationValid(triangulation* T)
{
	for(int TriangleIndex = 0; TriangleIndex < T->TriangleCount; ++TriangleIndex)
	{
		if(!IsTriangleValid(T, TriangleIndex))
		{
			return(false);
		}
	}
	return(true);
}

int PushVertex(triangulation* T, vertex V)
{
	Assert(T->VertexCount < ArrayCount(T->Vertices));
	T->Vertices[T->VertexCount] = V;
	T->VertexCount++;

	return(T->VertexCount - 1);
}

int PushEdge(triangulation* T, edge E)
{
	Assert(T->EdgeCount < ArrayCount(T->Edges));
	T->Edges[T->EdgeCount] = E;
	T->EdgeCount++;

	return(T->EdgeCount - 1);
}

int PushTriangle(triangulation* T, triangle F)
{
	Assert(T->TriangleCount < ArrayCount(T->Triangles));
	T->Triangles[T->TriangleCount] = F;
	T->TriangleCount++;

	return(T->TriangleCount - 1);
}

void DeleteTriangle(triangulation* T, int TriangleIndex)
{
	Assert(T->TriangleCount > 0);
	T->Triangles[TriangleIndex] = T->Triangles[T->TriangleCount - 1];
	T->TriangleCount--;
}

bool AreTwoTrianglesIdentical(triangulation* T, int* F0Index, int* F1Index)
{
	for(int FirstTriangleIndex = 0; FirstTriangleIndex < (T->TriangleCount - 1); ++FirstTriangleIndex)
	{
		for(int SecondTriangleIndex = FirstTriangleIndex + 1; SecondTriangleIndex < T->TriangleCount; ++SecondTriangleIndex)
		{
			triangle FirstTriangle = T->Triangles[FirstTriangleIndex];
			bool AreIdentical = true;
			for(int i = 0; i < ArrayCount(FirstTriangle.VertexIndices); ++i)
			{
				if(!IsVertexInTriangle(T, FirstTriangle.VertexIndices[i], SecondTriangleIndex))
				{
					AreIdentical = false;
					break;
				}
			}
			if(AreIdentical)
			{
				*F0Index = FirstTriangleIndex;
				*F1Index = SecondTriangleIndex;
				return(true);
			}
		}
	}
	return(false);
}

bool GetTrianglesOfEdge(triangulation* T, int EdgeIndex, int* F0Index, int* F1Index)
{
	// TODO(hugo) : This function could be considerably improved if I changed my data structure
	// and if an edge would reference its incident triangles
	bool F0Found = false;
	bool F1Found = false;
	for(int TriangleIndex = 0; TriangleIndex < T->TriangleCount; ++TriangleIndex)
	{
		triangle F = T->Triangles[TriangleIndex];
		for(int i = 0; i < ArrayCount(F.EdgeIndices); ++i)
		{
			if(EdgeIndex == F.EdgeIndices[i])
			{
				if(!F0Found)
				{
					*F0Index = TriangleIndex;
					F0Found = true;
					break;
				}
				else if(!F1Found)
				{
					*F1Index = TriangleIndex;
					F1Found = true;
					break;
				}
				else
				{
					Assert(false); //NOTE(hugo) : more than two triangles incident to a single edge ???
				}
			}
		}
		if(F0Found && F1Found)
		{
			return(true);
		}
	}

	Assert(F0Found);
	Assert(!F1Found);
	return(false);
}

bool IsCounterClockWise(vertex A, vertex B, vertex C)
{
	float CrossProductZ = (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
	bool IsCCW = (CrossProductZ > 0);

	return(IsCCW);
}

struct barycentric_coords
{
	float Alpha;
	float Beta;
	float Gamma;
};

barycentric_coords FindBarycentricCoordsOfPointInTriangle(triangulation* T, triangle F, vertex V)
{
	barycentric_coords Result = {};
	vertex A = T->Vertices[F.Vertex0Index];
	vertex B = T->Vertices[F.Vertex1Index];
	vertex C = T->Vertices[F.Vertex2Index];
	float TriangleArea = 0.5f * (-B.y * C.x + A.y * (-B.x + C.x) + A.x * (B.y - C.y) + B.x * C.y);
	//float Sign = TriangleArea > 0 ? 1.0f : -1.0f;
	float s = (1.0f / (2.0f * TriangleArea)) * (A.y * C.x - A.x * C.y + (C.y - A.y) * V.x + (A.x - C.x) * V.y);
	float t = (1.0f / (2.0f * TriangleArea)) * (A.x * B.y - A.y * B.x + (A.y - B.y) * V.x + (B.x - A.x) * V.y);

	Result.Beta = s;
	Result.Gamma = t;
	Result.Alpha = 1.0f - Result.Beta - Result.Gamma;

	return(Result);
}

bool IsInTriangle(triangulation* T, triangle F, vertex V)
{
	barycentric_coords Bar = FindBarycentricCoordsOfPointInTriangle(T, F, V);
	bool IsInside = (Bar.Alpha > 0 && Bar.Beta > 0 && Bar.Gamma > 0);

	return(IsInside);
}

int FindEdgeIndexLinkingVertices(triangulation* T, int PIndex, int QIndex, triangle F)
{
	for(int EdgeIndex = 0; EdgeIndex < ArrayCount(F.EdgeIndices); ++EdgeIndex)
	{
		edge E = T->Edges[F.EdgeIndices[EdgeIndex]];
		if(((E.Vertex0Index == PIndex) && (E.Vertex1Index == QIndex))
			||  ((E.Vertex0Index == QIndex) && (E.Vertex1Index == PIndex)))
		{
			return(F.EdgeIndices[EdgeIndex]);
		}
	}

	SDL_Log("FindEdgeIndexLinkingVertices : edge not found.");
	Assert(false);
	return(0);
}

int FindVertexIndexNotInEdgeInTriangle(triangulation* T, int EdgeIndex, int FIndex)
{
	// NOTE(hugo) : We assume that the triangle is ABC and the edge is BC. We are therefore looking for A.
	int BIndex = T->Edges[EdgeIndex].Vertex0Index;
	int CIndex = T->Edges[EdgeIndex].Vertex1Index;

	triangle F = T->Triangles[FIndex];
	for(int i = 0; i < ArrayCount(F.VertexIndices); ++i)
	{
		int VertexIndex = F.VertexIndices[i];
		if((VertexIndex != BIndex) && (VertexIndex != CIndex))
		{
			return(VertexIndex);
		}
	}

	Assert(false);
	return(0);
}

struct common_edge_result
{
	int AIndex;
	int BCIndex;
	int DIndex;
};

common_edge_result FindEdgeIndexInCommonBetweenTriangles(triangulation* T, int F0Index, int F1Index)
{
	common_edge_result Result = {};
	triangle F0 = T->Triangles[F0Index];
	triangle F1 = T->Triangles[F1Index];
	for(int I0 = 0; I0 < ArrayCount(F0.EdgeIndices); ++I0)
	{
		for(int I1 = 0; I1 < ArrayCount(F1.EdgeIndices); ++I1)
		{
			if(F0.EdgeIndices[I0] == F1.EdgeIndices[I1])
			{
				Result.BCIndex = F0.EdgeIndices[I0];
				// NOTE(hugo) : We have found the common edge. Let's now look for the last point in each triangle. A is for F0 and D is for F1
				Result.AIndex = FindVertexIndexNotInEdgeInTriangle(T, Result.BCIndex, F0Index);
				Result.DIndex = FindVertexIndexNotInEdgeInTriangle(T, Result.BCIndex, F1Index);

				return(Result);
			}
		}
	}

	Assert(false);
	return(Result);
}

bool IsDelaunay(triangulation* T, int TriangleIndex)
{
	triangle F = T->Triangles[TriangleIndex];
	vertex A = T->Vertices[F.Vertex0Index];
	vertex B = T->Vertices[F.Vertex1Index];
	vertex C = T->Vertices[F.Vertex2Index];

	if(!IsCounterClockWise(A, B, C))
	{
		vertex Temp = B;
		B = C;
		C = Temp;
		Assert(IsCounterClockWise(A, B, C));
	}

	mat4 M = {};
	SetValue(&M, 0, 0, A.x);
	SetValue(&M, 0, 1, A.y);
	SetValue(&M, 0, 2, A.x * A.x + A.y * A.y);
	SetValue(&M, 0, 3, 1);

	SetValue(&M, 1, 0, B.x);
	SetValue(&M, 1, 1, B.y);
	SetValue(&M, 1, 2, B.x * B.x + B.y * B.y);
	SetValue(&M, 1, 3, 1);

	SetValue(&M, 2, 0, C.x);
	SetValue(&M, 2, 1, C.y);
	SetValue(&M, 2, 2, C.x * C.x + C.y * C.y);
	SetValue(&M, 2, 3, 1);

	for(int VertexIndex = 0; VertexIndex < T->VertexCount; ++VertexIndex)
	{
		vertex D = T->Vertices[VertexIndex];
		if(D.IsRealPoint)
		{
			if((VertexIndex !=  F.Vertex0Index) && (VertexIndex != F.Vertex1Index) && (VertexIndex != F.Vertex2Index))
			{

				SetValue(&M, 3, 0, D.x);
				SetValue(&M, 3, 1, D.y);
				SetValue(&M, 3, 2, D.x * D.x + D.y * D.y);
				SetValue(&M, 3, 3, 1);

				float Determinant = Det(M);
				if(Determinant > 0)
				{
					// TODO(hugo) : There is a big optim to be done. If we found the vertex that is inside the circle, we directly have the other triangle that we need to flip the edge with.
					return(false);
				}
			}
		}
	}

	return(true);

}

void PerformLawsonFlip(triangulation* T, int F0Index, int F1Index)
{
	int FId0;
	int FId1;
	Assert(!AreTwoTrianglesIdentical(T, &FId0, &FId1));
	Assert(IsTriangleValid(T, F0Index));
	Assert(IsTriangleValid(T, F1Index));

	// NOTE(hugo):  At first we have : F0 (ABC) and F1 (BCD) so that BC is the edge to be flipped
	common_edge_result CommonEdgeResult = FindEdgeIndexInCommonBetweenTriangles(T, F0Index, F1Index);
	int BCIndex = CommonEdgeResult.BCIndex;
	int BIndex = T->Edges[BCIndex].Vertex0Index;
	int CIndex = T->Edges[BCIndex].Vertex1Index;
	int AIndex = CommonEdgeResult.AIndex;
	int DIndex = CommonEdgeResult.DIndex;
	Assert(AIndex != BIndex);
	Assert(AIndex != CIndex);
	Assert(AIndex != DIndex);
	Assert(BIndex != CIndex);
	Assert(BIndex != DIndex);
	Assert(CIndex != DIndex);

	int ACIndex = FindEdgeIndexLinkingVertices(T, AIndex, CIndex, T->Triangles[F0Index]);
	int ABIndex = FindEdgeIndexLinkingVertices(T, AIndex, BIndex, T->Triangles[F0Index]);

	int DCIndex = FindEdgeIndexLinkingVertices(T, DIndex, CIndex, T->Triangles[F1Index]);
	int DBIndex = FindEdgeIndexLinkingVertices(T, DIndex, BIndex, T->Triangles[F1Index]);
	Assert(ABIndex != ACIndex);
	Assert(ABIndex != DCIndex);
	Assert(ABIndex != DBIndex);
	Assert(ACIndex != DCIndex);
	Assert(ACIndex != DBIndex);
	Assert(DCIndex != DBIndex);

	T->Edges[BCIndex].Vertex0Index = AIndex;
	T->Edges[BCIndex].Vertex1Index = DIndex;
	int ADIndex = BCIndex;
	
	T->Triangles[F0Index].Vertex0Index = AIndex;
	T->Triangles[F0Index].Vertex1Index = BIndex;
	T->Triangles[F0Index].Vertex2Index = DIndex;

	T->Triangles[F0Index].Edge0Index = ABIndex;
	T->Triangles[F0Index].Edge1Index = DBIndex;
	T->Triangles[F0Index].Edge2Index = ADIndex;


	T->Triangles[F1Index].Vertex0Index = AIndex;
	T->Triangles[F1Index].Vertex1Index = CIndex;
	T->Triangles[F1Index].Vertex2Index = DIndex;

	T->Triangles[F1Index].Edge0Index = ACIndex;
	T->Triangles[F1Index].Edge1Index = DCIndex;
	T->Triangles[F1Index].Edge2Index = ADIndex;


	Assert(IsTriangleValid(T, F0Index));
	Assert(IsTriangleValid(T, F1Index));

	Assert(!AreTwoTrianglesIdentical(T, &FId0, &FId1));
}

void Render(SDL_Renderer* Renderer, triangulation* T, TTF_Font* Font)
{
	// NOTE(hugo) : Rendering !
	SDL_SetRenderDrawColor(Renderer, 119, 136, 153, 255);
	SDL_RenderClear(Renderer);
	SDL_SetRenderDrawColor(Renderer, 20, 20, 20, 255);

	for(int VertexIndex = 0; VertexIndex < T->VertexCount; ++VertexIndex)
	{
		vertex V = T->Vertices[VertexIndex];
		if(V.IsRealPoint)
		{
			SDL_Rect VertexRect;
			VertexRect.x = V.x - 2;
			VertexRect.y = ScreenHeight - V.y - 2;
			VertexRect.w = 5;
			VertexRect.h = 5;
			SDL_RenderDrawRect(Renderer, &VertexRect);
		}
	}

	for(int EdgeIndex = 0; EdgeIndex < T->EdgeCount; ++EdgeIndex)
	{
		edge E = T->Edges[EdgeIndex];
		vertex V = T->Vertices[E.Vertex0Index];
		vertex W = T->Vertices[E.Vertex1Index];
		if(V.IsRealPoint && W.IsRealPoint)
		{
			SDL_RenderDrawLine(Renderer, V.x, ScreenHeight - V.y, W.x, ScreenHeight - W.y);
		}
	}

#if 0
	for(int TriangleIndex = 0; TriangleIndex < T->TriangleCount; ++TriangleIndex)
	{
		triangle F = T->Triangles[TriangleIndex];
		char Buffer[4];
		sprintf(Buffer, "%i", TriangleIndex);
		SDL_Surface* SurfaceMessage = TTF_RenderText_Solid(Font, Buffer, {255, 255, 255});
		SDL_Texture* Message = SDL_CreateTextureFromSurface(Renderer, SurfaceMessage);
		SDL_Rect MessageRect = {0, 0, 25, 25};

		float FakeWeight = 0.1f;
		float RealWeightForTwo = 1.0f - 2.0f * FakeWeight;
		float RealWeightForOne = 0.5f * (1.0f - FakeWeight);

		if((!T->Vertices[F.Vertex0Index].IsRealPoint) && (!T->Vertices[F.Vertex1Index].IsRealPoint))
		{
			MessageRect.x = FakeWeight * T->Vertices[F.Vertex0Index].x + FakeWeight * T->Vertices[F.Vertex1Index].x + RealWeightForTwo * T->Vertices[F.Vertex2Index].x;
			MessageRect.y = FakeWeight * T->Vertices[F.Vertex0Index].y + FakeWeight * T->Vertices[F.Vertex1Index].y + RealWeightForTwo * T->Vertices[F.Vertex2Index].y;
		}
		else if((!T->Vertices[F.Vertex2Index].IsRealPoint) && (!T->Vertices[F.Vertex1Index].IsRealPoint))
		{
			MessageRect.x = RealWeightForTwo * T->Vertices[F.Vertex0Index].x + FakeWeight * T->Vertices[F.Vertex1Index].x + FakeWeight * T->Vertices[F.Vertex2Index].x;
			MessageRect.y = RealWeightForTwo * T->Vertices[F.Vertex0Index].y + FakeWeight * T->Vertices[F.Vertex1Index].y + FakeWeight * T->Vertices[F.Vertex2Index].y;
		}
		else if((!T->Vertices[F.Vertex2Index].IsRealPoint) && (!T->Vertices[F.Vertex0Index].IsRealPoint))
		{
			MessageRect.x = FakeWeight * T->Vertices[F.Vertex0Index].x + RealWeightForTwo * T->Vertices[F.Vertex1Index].x + FakeWeight * T->Vertices[F.Vertex2Index].x;
			MessageRect.y = FakeWeight * T->Vertices[F.Vertex0Index].y + RealWeightForTwo * T->Vertices[F.Vertex1Index].y + FakeWeight * T->Vertices[F.Vertex2Index].y;
		}
		else if(!T->Vertices[F.Vertex0Index].IsRealPoint)
		{
			MessageRect.x = FakeWeight * T->Vertices[F.Vertex0Index].x + RealWeightForOne * T->Vertices[F.Vertex1Index].x + RealWeightForOne * T->Vertices[F.Vertex2Index].x;
			MessageRect.y = FakeWeight * T->Vertices[F.Vertex0Index].y + RealWeightForOne * T->Vertices[F.Vertex1Index].y + RealWeightForOne * T->Vertices[F.Vertex2Index].y;
		}
		else if(!T->Vertices[F.Vertex1Index].IsRealPoint)
		{
			MessageRect.x = RealWeightForOne * T->Vertices[F.Vertex0Index].x + FakeWeight * T->Vertices[F.Vertex1Index].x + RealWeightForOne * T->Vertices[F.Vertex2Index].x;
			MessageRect.y = RealWeightForOne * T->Vertices[F.Vertex0Index].y + FakeWeight * T->Vertices[F.Vertex1Index].y + RealWeightForOne * T->Vertices[F.Vertex2Index].y;
		}
		else if(!T->Vertices[F.Vertex2Index].IsRealPoint)
		{
			MessageRect.x = RealWeightForOne * T->Vertices[F.Vertex0Index].x + FakeWeight * T->Vertices[F.Vertex1Index].x + RealWeightForOne * T->Vertices[F.Vertex2Index].x;
			MessageRect.y = RealWeightForOne * T->Vertices[F.Vertex0Index].y + FakeWeight * T->Vertices[F.Vertex1Index].y + RealWeightForOne * T->Vertices[F.Vertex2Index].y;
		}
		else
		{
			MessageRect.x = 0.333 * T->Vertices[F.Vertex0Index].x + 0.333 * T->Vertices[F.Vertex1Index].x + 0.333 * T->Vertices[F.Vertex2Index].x;
			MessageRect.y = 0.333 * T->Vertices[F.Vertex0Index].y + 0.333 * T->Vertices[F.Vertex1Index].y + 0.333 * T->Vertices[F.Vertex2Index].y;
		}
		MessageRect.y = ScreenHeight - MessageRect.y;

		if(MessageRect.x < 0)
		{
			MessageRect.x = 0;
		}
		if(MessageRect.x + MessageRect.w > ScreenWidth)
		{
			MessageRect.x = ScreenWidth - MessageRect.w;
		}
		if(MessageRect.y < 0)
		{
			MessageRect.y = 0;
		}
		if(MessageRect.y + MessageRect.h > ScreenHeight)
		{
			MessageRect.y = ScreenWidth - MessageRect.h;
		}

		SDL_RenderCopy(Renderer, Message, 0, &MessageRect);
		SDL_FreeSurface(SurfaceMessage);
		SDL_DestroyTexture(Message);
	}
#endif

	SDL_RenderPresent(Renderer);
}


void ComputeDelaunay(triangulation* T, SDL_Renderer* Renderer, TTF_Font* Font)
{
	int SIndex = T->VertexCount - 1;
	vertex S = T->Vertices[SIndex];
	Assert(S.IsRealPoint);

	int TriangleToBeSplitIndex = -1;
	for(int TriangleIndex = 0; TriangleIndex < T->TriangleCount; ++TriangleIndex)
	{
		triangle F = T->Triangles[TriangleIndex];
		if(IsInTriangle(T, F, S))
		{
			TriangleToBeSplitIndex = TriangleIndex;
			break;
		}
	}
	
	// NOTE(hugo) : Creating new triangles
	Assert(TriangleToBeSplitIndex != -1);
	triangle TriangleToBeSplit = T->Triangles[TriangleToBeSplitIndex];

	int PIndex = TriangleToBeSplit.Vertex0Index;
	int QIndex = TriangleToBeSplit.Vertex1Index;
	int RIndex = TriangleToBeSplit.Vertex2Index;

	edge SP = {SIndex, PIndex};
	edge SQ = {SIndex, QIndex};
	edge SR = {SIndex, RIndex};

	int SPIndex = PushEdge(T, SP);
	int SQIndex = PushEdge(T, SQ);
	int SRIndex = PushEdge(T, SR);

	int PQIndex = FindEdgeIndexLinkingVertices(T, PIndex, QIndex, TriangleToBeSplit);
	int QRIndex = FindEdgeIndexLinkingVertices(T, QIndex, RIndex, TriangleToBeSplit);
	int PRIndex = FindEdgeIndexLinkingVertices(T, RIndex, PIndex, TriangleToBeSplit);
	DeleteTriangle(T, TriangleToBeSplitIndex);

	triangle QSP = {SQIndex, SPIndex, PQIndex, QIndex, SIndex, PIndex};
	triangle QSR = {SQIndex, SRIndex, QRIndex, QIndex, SIndex, RIndex};
	triangle SRP = {SRIndex, SPIndex, PRIndex, SIndex, RIndex, PIndex};

	PushTriangle(T, QSP);
	PushTriangle(T, QSR);
	PushTriangle(T, SRP);

	Render(Renderer, T, Font);

	// NOTE(hugo) : Performing Lawson flips
	bool NeedNewFlipCheck = true;
	while(NeedNewFlipCheck)
	{
		NeedNewFlipCheck = false;
		for(int EdgeIndex = 0; EdgeIndex < T->EdgeCount; ++EdgeIndex)
		{
			int F0Index = 0;
			int F1Index = 1;
			bool GotTriangles = GetTrianglesOfEdge(T, EdgeIndex, &F0Index, &F1Index);
			if(GotTriangles && ((!IsDelaunay(T, F0Index)) && (!IsDelaunay(T, F1Index))))
			{
				Render(Renderer, T, Font);
				PerformLawsonFlip(T, F0Index, F1Index);
				Render(Renderer, T, Font);
				NeedNewFlipCheck = true;
			}
		}
	}

}

int main(int ArgumentCount, char** Arguments)
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* Window = SDL_CreateWindow("Delaunay Triangulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ScreenWidth, ScreenHeight, 0);
	Assert(Window);

	SDL_Renderer* Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED);
	Assert(Renderer);

	TTF_Init();
	TTF_Font* Font = TTF_OpenFont("/usr/share/fonts/truetype/lao/Phetsarath_OT.ttf", 15);
	//TTF_Font* Font = TTF_OpenFont("Lato.ttf", 24);
	if(!Font)
	{
		printf("TTF Font : %s\n", TTF_GetError());
	}

	// NOTE(hugo) : Init graph
	triangulation T = {};
	T.VertexCount = 0;
	T.EdgeCount = 0;
	T.TriangleCount = 0;

	int FarAwayCoordinate = 8000;
	vertex FakePoint0 = {-100, ScreenHeight - (-FarAwayCoordinate), false};
	vertex FakePoint1 = {-100, ScreenHeight - FarAwayCoordinate, false};
	vertex FakePoint2 = {FarAwayCoordinate, ScreenHeight - FarAwayCoordinate, false};
	edge E01 = {0, 1};
	edge E12 = {1, 2};
	edge E20 = {2, 0};
	triangle F = {0, 1, 2, 0, 1, 2};
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
							// NOTE(hugo) : Putting the point in normal coordinates (not the screen coordinates which is not correctly oriented)
							vertex V = {Event.button.x, ScreenHeight - Event.button.y, true};
							PushVertex(&T, V);
							DirtyTriangulation = true;
						}
					} break;
			}
		}

		// NOTE(hugo) : Triangulation processing
		if(DirtyTriangulation)
		{
			ComputeDelaunay(&T, Renderer, Font);
			DirtyTriangulation = false;
		}

		Render(Renderer, &T, Font);

	}

	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
	return(0);
}
