#ifndef TEST_MODEL_CORNEL_BOX_H
#define TEST_MODEL_CORNEL_BOX_H

// Defines a simple test model: The Cornel Box

#include <glm/glm.hpp>
#include <vector>

// Used to describe a triangular surface:
class Triangle
{
public:
	glm::vec3 v0;
	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 normal;
	glm::vec3 color;
	float s;

	Triangle( glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 color, float s)
		: v0(v0), v1(v1), v2(v2), color(color), s(s)
	{
		ComputeNormal();
	}

	void ComputeNormal()
	{
		glm::vec3 e1 = v1-v0;
		glm::vec3 e2 = v2-v0;
		normal = glm::normalize( glm::cross( e2, e1 ) );
	}
};

// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
void LoadTestModel( std::vector<Triangle>& triangles )
{
	using glm::vec3;

	// Defines colors:
	vec3 red(    0.75f, 0.15f, 0.15f );
	vec3 yellow( 0.75f, 0.75f, 0.15f );
	vec3 green(  0.15f, 0.75f, 0.15f );
	vec3 cyan(   0.15f, 0.75f, 0.75f );
	vec3 blue(   0.15f, 0.15f, 0.75f );
	vec3 purple( 0.75f, 0.15f, 0.75f );
	vec3 white(  0.75f, 0.75f, 0.75f );

	triangles.clear();
	triangles.reserve( 5*2*3 );

	// ---------------------------------------------------------------------------
	// Room

	float L = 555;			// Length of Cornell Box side.
	float behind = -555;
	vec3 A(L,0,0);
	vec3 B(0,0,0);
	vec3 C(L,0,L);
	vec3 D(0,0,L);

	vec3 E(L,L,0);
	vec3 F(0,L,0);
	vec3 G(L,L,L);
	vec3 H(0,L,L);

	vec3 A2(L,0,0);
	vec3 B2(0,0,0);
	vec3 C2(L,0,behind);
	vec3 D2(0,0,behind);

	// Floor:
	triangles.push_back( Triangle( C, B, A, green ,0) );
	triangles.push_back( Triangle( C, D, B, green ,0) );

	// Floor extension:
	triangles.push_back( Triangle( C2, B2, A2, green ,0) );
	triangles.push_back( Triangle( C2, D2, B2, green ,0) );

	// Left wall
	triangles.push_back( Triangle( A, E, C, purple ,0) );
	triangles.push_back( Triangle( C, E, G, purple ,0) );

	// Right wall
	triangles.push_back( Triangle( F, B, D, yellow ,0) );
	triangles.push_back( Triangle( H, F, D, yellow ,0) );

	// Ceiling
	triangles.push_back( Triangle( E, F, G, cyan ,0) );
	triangles.push_back( Triangle( F, H, G, cyan ,0) );

	// Back wall
	triangles.push_back( Triangle( G, D, C, white ,0) );
	triangles.push_back( Triangle( G, H, D, white ,0) );

	// front wall
	// triangles.push_back( Triangle( E, B, A, white ,0) );
	// triangles.push_back( Triangle( E, F, B, white ,0) );
	// ---------------------------------------------------------------------------
	// Short block

	A = vec3(290,0,114);
	B = vec3(130,0, 65);
	C = vec3(240,0,272);
	D = vec3( 82,0,225);

	E = vec3(290,165,114);
	F = vec3(130,165, 65);
	G = vec3(240,165,272);
	H = vec3( 82,165,225);
	vec3 I(180,200,169);

	// Front
	triangles.push_back( Triangle(E,B,A,red,0));
	triangles.push_back( Triangle(E,F,B,red,0));

	// Front
	triangles.push_back( Triangle(F,D,B,red,0) );
	triangles.push_back( Triangle(F,H,D,red,0) );

	// BACK
	triangles.push_back( Triangle(H,C,D,red,0) );
	triangles.push_back( Triangle(H,G,C,red,0) );

	// LEFT
	triangles.push_back( Triangle(G,E,C,red,0) );
	triangles.push_back( Triangle(E,A,C,red,0) );

	//ROOF
	triangles.push_back( Triangle(H,F,I,red,1) );
	triangles.push_back( Triangle(H,G,I,red,1) );
	triangles.push_back( Triangle(G,E,I,red,1) );
	triangles.push_back( Triangle(E,F,I,red,1) );

	// TOP
	// triangles.push_back( Triangle(G,F,E,red,0) );
	// triangles.push_back( Triangle(G,H,F,red,0) );

	// ---------------------------------------------------------------------------
	// Tall block

	A = vec3(423,0,247);
	B = vec3(265,0,296);
	C = vec3(472,0,406);
	D = vec3(314,0,456);

	E = vec3(423,330,247);
	F = vec3(265,330,296);
	G = vec3(472,330,406);
	H = vec3(314,330,456);

	// Front
	triangles.push_back( Triangle(E,B,A,blue,1) );
	triangles.push_back( Triangle(E,F,B,blue,1) );

	// Front
	triangles.push_back( Triangle(F,D,B,blue,1) );
	triangles.push_back( Triangle(F,H,D,blue,1) );

	// BACK
	triangles.push_back( Triangle(H,C,D,blue,1) );
	triangles.push_back( Triangle(H,G,C,blue,1) );

	// LEFT
	triangles.push_back( Triangle(G,E,C,blue,1) );
	triangles.push_back( Triangle(E,A,C,blue,1) );

	// TOP
	triangles.push_back( Triangle(G,F,E,blue,1) );
	triangles.push_back( Triangle(G,H,F,blue,1) );


	// ----------------------------------------------
	// Scale to the volume [-1,1]^3

	for( size_t i=0; i<triangles.size(); ++i )
	{
		triangles[i].v0 *= 2/L;
		triangles[i].v1 *= 2/L;
		triangles[i].v2 *= 2/L;

		triangles[i].v0 -= vec3(1,1,1);
		triangles[i].v1 -= vec3(1,1,1);
		triangles[i].v2 -= vec3(1,1,1);

		triangles[i].v0.x *= -1;
		triangles[i].v1.x *= -1;
		triangles[i].v2.x *= -1;

		triangles[i].v0.y *= -1;
		triangles[i].v1.y *= -1;
		triangles[i].v2.y *= -1;

		triangles[i].ComputeNormal();
	}
}

#endif
