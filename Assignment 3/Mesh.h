
#define MAXVERTICES 1000
#define MAXQUADS 1000

typedef struct MVertex{
	GLfloat pos[3];
} MVertex;

typedef struct MQuad{
	GLushort vi[4];
} MQuad;

#include "RGBpixmap.h"

typedef struct MNormal{
	GLfloat v[3];
} MNormal;

typedef struct Mesh{
	MVertex  *vertices; 
	MQuad    *quads;
	MNormal  *quadNormals;
	int numVertices;
	int numQuads;

	VECTOR3D center;
	VECTOR3D translation;		// Translatation Deltas
	VECTOR3D scaleFactor;		// X, Y, Z Scale Factors
    VECTOR3D angles;            // Rotation angles around x, y, z axis	
	int   numBaseCopies;        // Number of base quad copies
	float extrudeDelta;			// Distance between base quad copies
	float extrudeDir;           // Extrude in 0 - x, 1 - y, 2 - z directions
	bool selected;

	// Material properties for drawing
	float mat_ambient[4];
    float mat_specular[4];
    float mat_diffuse[4];
	float mat_shininess[1];
    // Material properties if selected
	float highlightMat_ambient[4];
    float highlightMat_specular[4];
    float highlightMat_diffuse[4];
	float highlightMat_shininess[1];

	RGBpixmap personBodyPix[1], personHeadPix[1];
	bool turnHead;
	VECTOR3D headAngle;
} BuildingMesh;

Mesh *createMesh(VECTOR3D scale, VECTOR3D translation, float extrudeDelta, int numBaseCopies);
void drawMesh(Mesh *mesh);
void drawMesh(Mesh *mesh, GLUquadricObj *head);