//////////////////////////////////////////////////////////////////////////////////////////
//	TerrainGrid.h
//	Class declaration for grid of cubes for "marching cubes" algorithm
//	Downloaded from: www.paulsprojects.net
//	Created:	20th July 2002
//
//	Copyright (c) 2006, Paul Baker
//	Distributed under the New BSD Licence. (See accompanying file License.txt or copy at
//	http://www.paulsprojects.net/NewBSDLicense.txt)
//////////////////////////////////////////////////////////////////////////////////////////	

struct TerrainGridVertex {
	VECTOR3D	position;
	VECTOR3D    normal;
	float		height;	//the height of the scalar field at this point
};

struct TerrainGridQuad {
	// pointers to vertices of each quad
	TerrainGridVertex *vertices[4];	
};

class TerrainGrid {

private:
	int maxGridSize;
	int minGridSize;
	float gridDim;

	int numVertices;
	TerrainGridVertex *vertices;

	int numQuads;
	TerrainGridQuad *quads;

	int numFacesDrawn;
	
	GLfloat mat_ambient[4];
    GLfloat mat_specular[4];
    GLfloat mat_diffuse[4];
	GLfloat mat_shininess[1];
	
private:
	bool CreateMemory();
	void FreeMemory();

public:
	//std::vector<Metaball> metaballs;
	typedef std::pair<int, int> TMaxGridDim;
	TerrainGrid(int maxGridSize = 40, float gridDim = 1.0f);	
  
	~TerrainGrid() { FreeMemory(); }

	TMaxGridDim GetMaxGridDimentions(){
		return TMaxGridDim(minGridSize, maxGridSize);
	}
	
	bool InitGrid(int gridSize, VECTOR3D origin, double gridLength, double gridWidth);
	void DrawGrid(int gridSize);
	void UpdateGrid();
	void ResetGridHeight();
	void ComputeNormals();
	
	/*
	void AddMetaball(VECTOR3D position, float squaredRadius, bool isNegative = false) {
		this->AddMetaball(Metaball(position, squaredRadius, isNegative));
	}

	void AddMetaball(const Metaball &metaball) {
		this->metaballs.push_back(metaball);
	}

	void UndoLastMetaball()	{
		this->metaballs.pop_back();
		this->UpdateGrid();
	}

	void ClearMetaballs()	{
		this->metaballs.clear();
	};
	*/
};

