#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
//#include <gl/glut.h>
#include "Dependencies\freeglut\freeglut.h"
#include <string.h>
#include <math.h>
#include <utility>
#include <vector>
#include "VECTOR3D.h"

#include "TerrainGrid.h"

TerrainGrid::TerrainGrid(int maxGridSize, float gridDim){
	minGridSize =1;
	numVertices = 0;
	vertices = NULL;
	numQuads = 0;
	quads = NULL;
	numFacesDrawn = 0;
	
	this->maxGridSize = maxGridSize < minGridSize ? minGridSize : maxGridSize;
	this->gridDim = gridDim;
	CreateMemory();

	// Setup the material and lights used for the grid
	mat_ambient[0] = 0.0;
	mat_ambient[1] = 0.0;
	mat_ambient[2] = 0.0;
	mat_ambient[3] = 1.0;
	mat_specular[0] = 0.0;
	mat_specular[1] = 0.0;
	mat_specular[2] = 0.0;
	mat_specular[3] = 1.0;
	mat_diffuse[0] = 0.9;
	mat_diffuse[1] = 0.5;
	mat_diffuse[2] = 0.0;
	mat_diffuse[3] = 1.0;
	mat_shininess[0] = 0.0;
}

bool TerrainGrid::CreateMemory(){
	vertices = new TerrainGridVertex[(maxGridSize+1)*(maxGridSize+1)];
	if(!vertices)	{
		return false;
	}
	quads = new TerrainGridQuad[maxGridSize*maxGridSize];
	if(!quads) {
		return false;
	}
	return true;
}
		
bool TerrainGrid::InitGrid(int gridSize, VECTOR3D origin, double gridLength, double gridWidth) {
	VECTOR3D o;
	int currentVertex = 0; 	  
	double sf1,sf2; 
    
	VECTOR3D v1,v2; // size of each grid in x,z dir
	
	v1.x = gridLength; // 32
	v1.y = 0;
	v1.z = 0;
	sf1 = 1.0/gridSize; // 1/16
	v1 *= sf1; // (2.0, 0, 0)

	v2.x = 0;
	v2.y = 0;
	v2.z = -gridWidth; // -32
	sf2 = 1.0/gridSize;
	v2 *= sf2; // (0, 0, -2.0)
    
	VECTOR3D gridpt;
	
	// VERTICES
	numVertices=(gridSize+1)*(gridSize+1);
	// Starts at front left corner of grid 
	o.Set(origin.x,origin.y,origin.z);
	for(int i=0; i< gridSize+1; i++){
		for(int j=0; j< gridSize+1; j++){
			// compute vertex position along grid row (along x direction)
			gridpt.x = o.x + j * v1.x;
			gridpt.y = o.y + j * v1.y;
			gridpt.z = o.z + j * v1.z;
			vertices[currentVertex].position.Set(gridpt.x,gridpt.y,gridpt.z);
			currentVertex++;
		}
		// go to next row in grid (negative z direction)
		o += v2;
	}
	
	// Build Quad Polygons
	numQuads=(gridSize)*(gridSize);
	int currentQuad=0;
	for(int j=0; j < gridSize; j++)	{
		for(int k=0; k < gridSize; k++)	{
			// Counterclockwise order
			quads[currentQuad].vertices[0]=&vertices[j*    (gridSize+1) + k];   // vertices[0]    - j=0, k=0, gridsize=16
			quads[currentQuad].vertices[1]=&vertices[j*    (gridSize+1) + k+1]; // vertices[1]
			quads[currentQuad].vertices[2]=&vertices[(j+1)*(gridSize+1) + k+1]; // vertices[17+1]
			quads[currentQuad].vertices[3]=&vertices[(j+1)*(gridSize+1) + k];   // vertices[17]
			currentQuad++;
		}
	}

    ResetGridHeight();
	this->ComputeNormals();
	return true;
}

void TerrainGrid::DrawGrid(int gridSize){
	int currentQuad=0;

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	for(int j=0; j< gridSize; j++){
		for(int k=0; k< gridSize; k++){
			glBegin(GL_QUADS);
			
			glNormal3f(quads[currentQuad].vertices[0]->normal.x,
				       quads[currentQuad].vertices[0]->normal.y,
					   quads[currentQuad].vertices[0]->normal.z);
			glTexCoord2f(0.0f, 0.0f); 
			glVertex3f(quads[currentQuad].vertices[0]->position.x,
				       quads[currentQuad].vertices[0]->position.y,
					   quads[currentQuad].vertices[0]->position.z);
			
			glNormal3f(quads[currentQuad].vertices[1]->normal.x,
				       quads[currentQuad].vertices[1]->normal.y,
					   quads[currentQuad].vertices[1]->normal.z);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(quads[currentQuad].vertices[1]->position.x,
				       quads[currentQuad].vertices[1]->position.y,
					   quads[currentQuad].vertices[1]->position.z);
			
			glNormal3f(quads[currentQuad].vertices[2]->normal.x,
				       quads[currentQuad].vertices[2]->normal.y,
					   quads[currentQuad].vertices[2]->normal.z);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(quads[currentQuad].vertices[2]->position.x,
				       quads[currentQuad].vertices[2]->position.y,
					   quads[currentQuad].vertices[2]->position.z);
			
			glNormal3f(quads[currentQuad].vertices[3]->normal.x,
				       quads[currentQuad].vertices[3]->normal.y,
					   quads[currentQuad].vertices[3]->normal.z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(quads[currentQuad].vertices[3]->position.x,
				       quads[currentQuad].vertices[3]->position.y,
					   quads[currentQuad].vertices[3]->position.z);
			glEnd();
			currentQuad++;
		}
	}
}

void TerrainGrid::ResetGridHeight() {
	for(int i=0; i<this->numVertices; i++) {
		this->vertices[i].height     = 0.0f;
		this->vertices[i].position.y = this->vertices[i].height; 
	}
}

void TerrainGrid::UpdateGrid() {
	ResetGridHeight();
}

void TerrainGrid::FreeMemory() {
	if(vertices)
		delete [] vertices;
	vertices=NULL;
	numVertices=0;

	if(quads)
		delete [] quads;
	quads=NULL;
	numQuads=0;
}

void TerrainGrid::ComputeNormals() {
	int currentQuad=0;

	for(int j=0; j< this->maxGridSize; j++) {
		for(int k=0; k< this->maxGridSize; k++) {
			VECTOR3D n0,n1,n2,n3,e0,e1,e2,e3,ne0,ne1,ne2,ne3;
			
			quads[currentQuad].vertices[0]->normal.LoadZero();
			quads[currentQuad].vertices[1]->normal.LoadZero();
			quads[currentQuad].vertices[2]->normal.LoadZero();
			quads[currentQuad].vertices[3]->normal.LoadZero();
			e0 = quads[currentQuad].vertices[1]->position - quads[currentQuad].vertices[0]->position; 
			e1 = quads[currentQuad].vertices[2]->position - quads[currentQuad].vertices[1]->position; 
			e2 = quads[currentQuad].vertices[3]->position - quads[currentQuad].vertices[2]->position; 
			e3 = quads[currentQuad].vertices[0]->position - quads[currentQuad].vertices[3]->position; 
			e0.Normalize();
			e1.Normalize();
			e2.Normalize();
			e3.Normalize();
			
			n0 = e0.CrossProduct(-e3);
			n0.Normalize();
			quads[currentQuad].vertices[0]->normal += n0;
			
			n1 = e1.CrossProduct(-e0);
			n1.Normalize();
			quads[currentQuad].vertices[1]->normal += n1;

			n2 = e2.CrossProduct(-e1);
			n2.Normalize();
			quads[currentQuad].vertices[2]->normal += n2;

			n3 = e3.CrossProduct(-e2);
			n3.Normalize();
			quads[currentQuad].vertices[3]->normal += n3;
			
			quads[currentQuad].vertices[0]->normal.Normalize();
			quads[currentQuad].vertices[1]->normal.Normalize();
			quads[currentQuad].vertices[2]->normal.Normalize();
			quads[currentQuad].vertices[3]->normal.Normalize();

			currentQuad++;
		}
	}
}
