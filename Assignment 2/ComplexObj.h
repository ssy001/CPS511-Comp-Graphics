#define PI 3.14159265
#define NUMOBJ 4

//GLuint objList[NUMOBJ];
GLUquadricObj *qobj;     //only need 1 pointer ???

typedef struct ComplexObj
{
	VECTOR3D center;            // center.y is the bottom of object. center.x and center.z are actual object centers. 
	VECTOR3D dim;
	VECTOR3D forward;			// Normalized forward direction vector. Initialized to (0.0, 0.0, 1.0). NOTE: y is always 0.0

	float tx, ty, tz;			// Translatation Deltas
	float sfx, sfy, sfz;		// Scale Factors
	float angle;				// Angle around y-axis of cobj coordinate system

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

	GLuint objList;
	GLuint booster, tank; //for rendering tri-rocket components

} ComplexObj;

ComplexObj *createObj()
{
	ComplexObj *newObj = (ComplexObj*)calloc(1, sizeof(ComplexObj));

	newObj->angle = 0.0;
	newObj->sfx = newObj->sfy = newObj->sfz = 1.0;
	newObj->tx = 0.0;
	newObj->ty = 0.0;
	newObj->tz = 0.0;
	newObj->center.Set(0, 0, 0);
	newObj->dim.Set(2.0f, 2.0f, 2.0f);
	newObj->forward.Set(0.0f, 0.0f, 1.0f);

	newObj->mat_ambient[0] = 0.0;
	newObj->mat_ambient[1] = 0.05;
	newObj->mat_ambient[2] = 0.0;
	newObj->mat_ambient[3] = 1.0;
	newObj->mat_specular[0] = 0.0;
	newObj->mat_specular[1] = 0.0;
	newObj->mat_specular[2] = 0.004;
	newObj->mat_specular[3] = 1.0;
	newObj->mat_diffuse[0] = 0.5;
	newObj->mat_diffuse[1] = 0.5;
	newObj->mat_diffuse[2] = 0.5;
	newObj->mat_diffuse[3] = 1.0;
	newObj->mat_shininess[0] = 0;

	newObj->highlightMat_ambient[0] = 0.0;
	newObj->highlightMat_ambient[1] = 0.00;
	newObj->highlightMat_ambient[2] = 0.0;
	newObj->highlightMat_ambient[3] = 1.0;
	newObj->highlightMat_specular[0] = 0.0;
	newObj->highlightMat_specular[1] = 0.0;
	newObj->highlightMat_specular[2] = 0.0;
	newObj->highlightMat_specular[3] = 1.0;
	newObj->highlightMat_diffuse[0] = 1.0;
	newObj->highlightMat_diffuse[1] = 0.0;
	newObj->highlightMat_diffuse[2] = 0.0;
	newObj->highlightMat_diffuse[3] = 1.0;
	newObj->highlightMat_shininess[0] = 0.0;

	return newObj;
}

// Given a cobj mesh, compute it's current bounding box and return in vectors min and max
// i.e. compute min.x,min.y,min.z,max.x,max.y,max.z
// Use this function for collision detection of cobj and walls/floor
void getBBox(ComplexObj *cobj, VECTOR3D *min, VECTOR3D *max)
{
	float xabs, zabs;
	cobj->center = cobj->center + VECTOR3D(cobj->tx, cobj->ty, cobj->tz);
	xabs = fabs(cobj->dim.x/2.0 * cobj->sfx * cos(cobj->angle*PI / 180.0)) + fabs(cobj->dim.z/2.0 * cobj->sfz*sin(cobj->angle*PI / 180.0));      //cos(angle)*length cobj makes with x-axis + sin(z-dir thickness)
	zabs = fabs(cobj->dim.x/2.0 * cobj->sfx * sin(cobj->angle*PI / 180.0)) + fabs(cobj->dim.z/2.0 * cobj->sfz*cos(cobj->angle*PI / 180.0));      //sin(angle)*length cobj makes with x-axis + cos(z-dir thickness)
	min->x = cobj->center.x - xabs;
	max->x = cobj->center.x + xabs;
	min->z = cobj->center.z - zabs;
	max->z = cobj->center.z + zabs;
	min->y = cobj->center.y;  //center.y is set to y = 0.0 @ initialization
	max->y = cobj->center.y + cobj->dim.y * cobj->sfy;

}

void drawObj(ComplexObj *cobj){
	if (cobj->selected){
		// Setup the material and lights used for the cobj
		glMaterialfv(GL_FRONT, GL_AMBIENT, cobj->highlightMat_ambient);
		glMaterialfv(GL_FRONT, GL_SPECULAR, cobj->highlightMat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, cobj->highlightMat_diffuse);
		glMaterialfv(GL_FRONT, GL_SHININESS, cobj->highlightMat_shininess);
	}
	else{
		// Setup the material and lights used for the cobj
		glMaterialfv(GL_FRONT, GL_AMBIENT, cobj->mat_ambient);
		glMaterialfv(GL_FRONT, GL_SPECULAR, cobj->mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, cobj->mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SHININESS, cobj->mat_shininess);
	}

	// Transform and Draw cobj   
	glPushMatrix();                                                    //Here, CTM is V = RT
	// put your transform code here
	VECTOR3D txVec = VECTOR3D(cobj->tx, cobj->ty, cobj->tz);
	cobj->tx = cobj->ty = cobj->tz = 0;
	cobj->center += txVec;
	// update forward vector of model
	cobj->forward.x = sin(cobj->angle * PI / 180.0);
	cobj->forward.z = cos(cobj->angle * PI / 180.0);

	glTranslatef(cobj->center.x, cobj->center.y, cobj->center.z);     //CTM is V * T(x,y,z)
	glRotatef(cobj->angle, 0.0, 1.0, 0.0);                            //CTM is V * T(x,y,z) * R_y(angle)
	glScalef(cobj->sfx, cobj->sfy, cobj->sfz);                        //CTM is V * T(x,y,z) * R_y(angle) * S(x,y,z) 
	glCallList(cobj->objList);
	glPopMatrix();
}

void createMageHead(ComplexObj *cobj){
	//Create a display list(s) for a glu quadric
	cobj->dim.Set(3.0, 3.8, 3.0);  //set dimensions of MageHead
	cobj->objList = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);

	glNewList(cobj->objList, GL_COMPILE);
	//create object 1 - clown head (cone (hat) + sphere (head) + disk (collar) + cylinder (nose)
		glPushMatrix();     //hat base
		glTranslatef(0.0, 1.5, 0.0);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		gluCylinder(qobj, 1.5, 0.0, 0.75, 20, 10);
		glPopMatrix();
		glPushMatrix();     //hat top
		glTranslatef(0.0, 1.8, 0.0);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		gluCylinder(qobj, 0.7, 0.0, 2.0, 20, 10);
		glPopMatrix();
		glPushMatrix();     //head
		glTranslatef(0.0, 1.0, 0.0);
		gluSphere(qobj, 1.0, 20, 10);
		glPopMatrix();
		glPushMatrix();     //collar
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		gluDisk(qobj, 0.5, 1.5, 20, 10);
		glPopMatrix();
		glPushMatrix();     //eye right
		glTranslatef(0.35, 1.25, 0.5);
		gluCylinder(qobj, 0.1, 0.3, 0.5, 20, 10);
		glPopMatrix();
		glPushMatrix();     //eye left
		glTranslatef(-0.35, 1.25, 0.5);
		gluCylinder(qobj, 0.1, 0.3, 0.5, 20, 10);
		glPopMatrix();
		glPushMatrix();     //mouth
		glTranslatef(0.0, 1.5, 0.8);
		glRotatef(-67.5, 0.0, 0.0, 1.0);
		gluPartialDisk(qobj, 0.3, 0.7, 20, 10, 0, 225);
		glPopMatrix();
		glPushMatrix();     //left ear
		glTranslatef(0.8, 1.9, 0.0);
		glRotatef(-50, 0.0, 0.0, 1.0);
		glScalef(0.75, 1.0, 1.0);
		glRotatef(-19.47, 0.0, 0.0, 1.0);
		glScalef(0.6, 0.6, 0.6);
		glutSolidTetrahedron();
		glPopMatrix();
		glPushMatrix();     //right ear
		glTranslatef(-0.8, 1.9, 0.0);
		glRotatef(50, 0.0, 0.0, 1.0);
		glRotatef(-90, 0.0, 1.0, 0.0);
		glRotatef(-90, 0.0, 1.0, 0.0);
		glScalef(0.75, 1.0, 1.0);
		glRotatef(-19.47, 0.0, 0.0, 1.0);
		glScalef(0.6, 0.6, 0.6);
		glutSolidTetrahedron();
		glPopMatrix();
	glEndList();

	gluDeleteQuadric(qobj);
}

void createRocketTank(void){
	//Create subobject - tri-rocket tank (cone (top) + cylinder (body) + inverted half cone + half cone 
	double clipEq[4] = { 0.0, -1.0, 0.0, 0.0 };

	glPushMatrix();     //base hemisphere
		glTranslatef(0.0, 0.4, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		gluSphere(qobj, 0.4, 20, 10);
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //body
		glTranslatef(0.0, 0.4, 0.0);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		gluCylinder(qobj, 0.4, 0.4, 2.0, 20, 10);
	glPopMatrix();
	glPushMatrix();     //top cone
		glTranslatef(0.0, 2.4, 0.0);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		gluCylinder(qobj, 0.4, 0.05, 0.9, 20, 10);
	glPopMatrix();
	glPushMatrix();     //top cone
		glTranslatef(0.0, 3.3, 0.0);
		gluSphere(qobj, 0.05, 20, 10);
	glPopMatrix();
}

void createRocketBooster(void){

	//Create subobject - tri-rocket booster
	glPushMatrix();     //base half cone
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(qobj, 0.25, 0.0, 0.375, 20, 10);
	glPopMatrix();
	glPushMatrix();     //base torus
	glTranslatef(0.0, 0.165, 0.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	glutSolidTorus(0.05, 0.2, 20, 50);
	glPopMatrix();
	glPushMatrix();     //skirt half cone
	glTranslatef(0.0, 0.19, 0.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(qobj, 0.3, 0.0, 0.5, 20, 10);
	glPopMatrix();
	glPushMatrix();     //body
	glTranslatef(0.0, 0.35, 0.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(qobj, 0.2, 0.2, 1.8, 20, 10);
	glPopMatrix();
	glPushMatrix();     //cap
	glTranslatef(0.0, 2.15, 0.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(qobj, 0.2, 0.0, 0.45, 20, 10);
	glPopMatrix();
}

void createTriRocket(ComplexObj *cobj){
	//Create a display list(s) for a glu quadric
	cobj->dim.Set(1.8, 3.35, 0.8);  //set dimensions of TriRocket
	cobj->objList = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);

	cobj->booster = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glNewList(cobj->booster, GL_COMPILE);
		createRocketBooster();
	glEndList();

	cobj->tank = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glNewList(cobj->tank, GL_COMPILE);
		createRocketTank();
	glEndList();

	glNewList(cobj->objList, GL_COMPILE);
		glTranslatef(0.0, 0.4, 0.0);  //main tank
		glCallList(cobj->tank);
		glTranslatef(0.0, -0.4, 0.0);

		glTranslatef(0.6, 0.0, 0.0);  //left booster
		glCallList(cobj->booster);
		glTranslatef(-0.6, 0.0, 0.0);

		glTranslatef(-0.6, 0.0, 0.0); //right booster
		glCallList(cobj->booster);
		glTranslatef(0.6, 0.0, 0.0);
	glEndList();

	gluDeleteQuadric(qobj);
}


void createBracelet(ComplexObj *cobj){
	//Create a display list(s) for a glu quadric
	double clipEq[4] = { 0.0, 0.0, 1.0, 0.0 };

	cobj->dim.Set(2.4, 0.71, 2.4);  //set dimensions of Bracelet
	cobj->objList = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);

	glNewList(cobj->objList, GL_COMPILE);
	//Create and init object 3 - bracelet (cylinder + disk x2 + icosahedron, dodecahedron, tetrahedron, octahedron,
	glPushMatrix();     //base torus
		glTranslatef(0.0, 0.65, 0.0);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glutSolidTorus(0.06, 1.0, 20, 50);
	glPopMatrix();
	glPushMatrix();     //cylinder
		glTranslatef(0.0, 0.1, 0.0);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		gluCylinder(qobj, 1.0, 1.0, 0.5, 20, 10);
	glPopMatrix();
	glPushMatrix();     //top torus
		glTranslatef(0.0, 0.05, 0.0);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glutSolidTorus(0.06, 1.0, 20, 50);
	glPopMatrix();

	glPushMatrix();     //jewel 1 - octahedron
		glTranslatef(0.0, 0.35, 1.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glScalef(0.25, 0.25, 0.25);
		glutSolidOctahedron();
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //jewel 2 - octahedron
		glTranslatef(0.0, 0.35, -1.0);
		glRotatef(180, 0.0, 1.0, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glScalef(0.25, 0.25, 0.25);
		glutSolidOctahedron();
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //jewel 3 - dodecahedron
		glTranslatef(0.707, 0.35, -0.707);
		glRotatef(135, 0.0, 1.0, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glScalef(0.125, 0.125, 0.125);
		glutSolidDodecahedron();
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //jewel 4 - dodecahedron
		glTranslatef(-0.707, 0.35, 0.707);
		glRotatef(-45, 0.0, 1.0, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glScalef(0.125, 0.125, 0.125);
		glutSolidDodecahedron();
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //jewel 5 - icosahedron
		glTranslatef(0.707, 0.35, 0.707);
		glRotatef(45, 0.0, 1.0, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glScalef(0.25, 0.25, 0.25);
		glutSolidIcosahedron();
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //jewel 6 - icosahedron
		glTranslatef(-0.707, 0.35, -0.707);
		glRotatef(-135, 0.0, 1.0, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glScalef(0.25, 0.25, 0.25);
		glutSolidIcosahedron();
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //jewel 7 - sphere
		glTranslatef(1.0, 0.35, 0.0);
		glRotatef(90, 0.0, 1.0, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glutSolidSphere(0.2, 20, 10);
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //jewel 8 - sphere
		glTranslatef(-1.0, 0.35, 0.0);
		glRotatef(-90, 0.0, 1.0, 0.0);
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);
		glutSolidSphere(0.2, 20, 10);
		glDisable(GL_CLIP_PLANE0);
	glPopMatrix();

	glEndList();

	gluDeleteQuadric(qobj);
}