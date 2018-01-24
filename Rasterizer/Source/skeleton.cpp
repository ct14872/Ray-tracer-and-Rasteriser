#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include "math.h"

using namespace std;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using glm::ivec2;
using glm::vec2;
/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                            */
struct Pixel
{
int x;
int y;
float zinv;
vec3 pos3d;
};

typedef struct Vertex
{
	Vertex(){
		outcode = 0;
	}
	vec3 position;
	int outcode;
}Vertex;

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
const int TOP = 8;
const int BOTTOM =4;
const int RIGHT =2;
const int LEFT =1;
int ClipOn = 0;
int seeClip = 0;
float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
SDL_Surface* screen;
int t;
vec3 cameraPos( 0, 0, -3.001 );
float focalLength =500;
float yaw=0;
mat3 R;
float thetax=0;
mat3 Rx;
vec3 currentColor;
vec3 currentNormal;
vec3 currentReflectance;
vec3 lightPos(0,-0.5,-0.7);
vec3 lightPower = 14.f*vec3( 1, 1, 1 );
vec3 indirectLightPowerPerArea = 0.5f*vec3( 1, 1, 1 );
/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw(vector<Triangle> triangles);
void VertexShader( const Vertex& v, Pixel& p );
void PixelShader( const Pixel& p );
void Interpolate( ivec2 a, ivec2 b, vector<ivec2>& result );
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result );
void ComputePolygonRows(const vector<Pixel>& vertexPixels,vector<Pixel>& leftPixels,vector<Pixel>& rightPixels );
void DrawRows(const vector<Pixel>& leftPixels,const vector<Pixel>& rightPixels );
void DrawPolygon( const vector<Vertex>& vertices );
void findNewVertex(Vertex vertex1, Vertex vertex2, float A, float B,Vertex& newVertex);


int main( int argc, char* argv[] )
{
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.
	vector<Triangle> triangles;
	LoadTestModel(triangles);

	R[0]=vec3(cos(yaw),0,-sin(yaw));
	R[1]=vec3(0,1,0);
	R[2]=vec3(sin(yaw),0,cos(yaw));
	Rx[0]=vec3(1,0,0);
	Rx[1]=vec3(0,cos(thetax),sin(thetax));
	Rx[2]=vec3(0,-sin(thetax),cos(thetax));
	while( NoQuitMessageSDL() )
	{
		Update();
		Draw(triangles);
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update()
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;
	Uint8* keystate = SDL_GetKeyState( 0 );
	if( keystate[SDLK_c]){
		ClipOn = 1;
	}
	if( keystate[SDLK_v]){
		ClipOn = 0;
	}
	if( keystate[SDLK_z]){
		seeClip = 1;
	}
	if( keystate[SDLK_x]){
		seeClip = 0;
	}
	if( keystate[SDLK_w] )
	{
		// Move camera forward
		cameraPos= cameraPos + vec3(0,0,0.05)*R;
	}
	if( keystate[SDLK_s] )
	{
		// Move camera backward
		cameraPos= cameraPos + vec3(0,0,-0.05)*R;
	}
	if( keystate[SDLK_a] )
	{
	// Move camera to the left
		cameraPos= cameraPos + vec3(-0.05,0,0)*R;
	}
	if( keystate[SDLK_d] )
	{
	// Move camera to the right
		cameraPos= cameraPos + vec3(0.05,0,0)*R;
	}
	if( keystate[SDLK_q] )
	{
	// Move camera to the left
		cameraPos= cameraPos + vec3(0,-0.05,0)*R;
	}
	if( keystate[SDLK_e] )
	{
	// Move camera to the right
		cameraPos= cameraPos + vec3(0,0.05,0)*R;
	}
	if( keystate[SDLK_i] )
	{
		// Move light forward
		lightPos.z +=0.1;
	}
	if( keystate[SDLK_k] )
	{
		// Move light backward
		lightPos.z += -0.1;
	}
	if( keystate[SDLK_j] )
	{
	// Move light to the left
		lightPos.x += -0.1;
	}
	if( keystate[SDLK_l] )
	{
	// Move light to the right
		lightPos.x += 0.1;
	}
	if( keystate[SDLK_u] )
	{
	// Move light to the up
		lightPos.y += -0.1;
	}
	if( keystate[SDLK_o] )
	{
	// Move light to the down
		lightPos.y += 0.1;
	}
	if( keystate[SDLK_LEFT] )
	{
		// rotate Camera Left
		yaw += 0.008;
	}
	if( keystate[SDLK_RIGHT] )
	{
		// rotate Camera Right
		yaw -=0.008;
	}
	if( keystate[SDLK_UP] )
	{
		// rotate Camera Left
		thetax -= 0.008;
	}
	if( keystate[SDLK_DOWN] )
	{
		// rotate Camera Right
		thetax +=0.008;
	}
	if( keystate[SDLK_r] )
	{
		// reset button
		thetax =0;
		yaw=0;
		cameraPos=vec3(0,0,-2.6);
	}
	R[0]=vec3(cos(yaw),0,-sin(yaw));
	R[1]=vec3(0,1,0);
	R[2]=vec3(sin(yaw),0,cos(yaw));
	Rx[0]=vec3(1,0,0);
	Rx[1]=vec3(0,cos(thetax),sin(thetax));
	Rx[2]=vec3(0,-sin(thetax),cos(thetax));
}

void Draw(vector<Triangle> triangles)
{
	SDL_FillRect( screen, 0, 0 );
	if( SDL_MUSTLOCK(screen) )
	SDL_LockSurface(screen);
	for( int y=0; y<SCREEN_HEIGHT; ++y )
		for( int x=0; x<SCREEN_WIDTH; ++x )
			depthBuffer[y][x] = 0;
			mat4 perspective;
			perspective[0]=vec4(1.0,0.0,0.0,0.0);
			perspective[1]=vec4(0.0,1.0,0.0,0.0);
			perspective[2]=vec4(0.0,0.0,1.0,0);
			perspective[3]=vec4(0.0,0.0,0.5,0);
			bool candraw;

	for( int i=0; i<(int)triangles.size(); ++i )
	{
		candraw = true;
		currentColor = triangles[i].color;
		currentNormal = triangles[i].normal;
		currentReflectance = vec3(1.0,1.0,1.0);
		vector<Vertex> vertices;
		vertices.resize(3);

		vertices[0].position = triangles[i].v0;
		vertices[1].position = triangles[i].v1;
		vertices[2].position = triangles[i].v2;
		vector<vec4> hVertices;

		if(ClipOn ==0 ){
			DrawPolygon(vertices);
		}else{
			hVertices.resize(3);
			for(int v=0; v<3; ++v)
			{
				vec3 newv = vertices[v].position-cameraPos;
				newv=R*newv;
				hVertices[v].x = newv.x;
				hVertices[v].y = newv.y;
				hVertices[v].z = newv.z;
				hVertices[v].w =1;
				hVertices[v] = hVertices[v]*perspective;
				hVertices[v].x = hVertices[v].x/hVertices[v].w;
				hVertices[v].y = 	hVertices[v].y/hVertices[v].w;
				hVertices[v].z = hVertices[v].z/hVertices[v].w;
				hVertices[v].w = hVertices[v].w/hVertices[v].w;

				//set the outcode
				vertices[v].outcode=0;
				if(hVertices[v].x>1.001){
					vertices[v].outcode += RIGHT;
				}else if(hVertices[v].x<-1.001){
					vertices[v].outcode += LEFT;
				}
				if(hVertices[v].y>1.001){
					vertices[v].outcode += BOTTOM;
				}else if(hVertices[v].y<-1.001){
					vertices[v].outcode += TOP;
				}
			}

			if((vertices[1].outcode&vertices[0].outcode)!=0 && (vertices[1].outcode&vertices[2].outcode)!=0 && (vertices[2].outcode&vertices[0].outcode)!=0){
			}else if((vertices[1].outcode|vertices[0].outcode)==0 && (vertices[1].outcode|vertices[2].outcode)==0 && (vertices[2].outcode|vertices[0].outcode)==0){
				if(isnan(vertices[0].position.z)==false||isnan(vertices[1].position.z)==false||isnan(vertices[2].position.z)==false) DrawPolygon( vertices );
			}else{
					for(int k =0; k<(int)vertices.size();k++){
							if((vertices[k].outcode&BOTTOM)!=0&&(vertices[(k+1)%3].outcode&BOTTOM)!=0){
									//clip 1
									float w = 1;//hVertices[1].w;
									//Find interval.
									float interval1 = (w-hVertices[k].y)/(hVertices[(k+2)%3].y-hVertices[k].y);
									float A = hVertices[k].x +interval1*(hVertices[(k+2)%3].x-hVertices[k].x);
									float B = w;
									findNewVertex(vertices[k], vertices[(k+2)%3],A, B,vertices[k]);
									float interval2 = (w-hVertices[(k+1)%3].y)/(hVertices[(k+2)%3].y-hVertices[(k+1)%3].y);
									float A2 = hVertices[(k+1)%3].x +interval2*(hVertices[(k+2)%3].x-hVertices[(k+1)%3].x);
									float B2 = w;
									findNewVertex(vertices[(k+1)%3], vertices[(k+2)%3],A2, B2,vertices[(k+1)%3]);
									hVertices[k].y = B;
									hVertices[k].x = A;
									hVertices[(k+1)%3].y = B2;
									hVertices[(k+1)%3].x = A2;
									vertices[k].outcode-=BOTTOM;
									vertices[(k+1)%3].outcode-=BOTTOM;
									if(A<=1 && A>=-1 && A2<=1 && A2>=-1){
										vertices[(k+1)%3].outcode = 0;
										vertices[k].outcode = 0;
									}else{
										vertices[(k+1)%3].outcode = 0;
										vertices[k].outcode = 0;
										if(A>1){
												vertices[k].outcode += RIGHT;
										}else if(A<-1){
												vertices[k].outcode += LEFT;
										}

										if(A2>1){
												vertices[(k+1)%3].outcode += RIGHT;
										}else if(A2<-1){
												vertices[(k+1)%3].outcode += LEFT;
										}
									}

							}else if((vertices[k].outcode&TOP)!=0 && (vertices[(k+1)%3].outcode&TOP)!=0){
								//clip 1
								float w = -1;//hVertices[1].w;
								//Find interval.
								float interval1 = (w-hVertices[k].y)/(hVertices[(k+2)%3].y-hVertices[k].y);
								float A = hVertices[k].x +interval1*(hVertices[(k+2)%3].x-hVertices[k].x);
								float B = w;
								findNewVertex(vertices[k], vertices[(k+2)%3],A, B,vertices[k]);
								float interval2 = (w-hVertices[(k+1)%3].y)/(hVertices[(k+2)%3].y-hVertices[(k+1)%3].y);
								float A2 = hVertices[(k+1)%3].x +interval2*(hVertices[(k+2)%3].x-hVertices[(k+1)%3].x);
								float B2 = w;

								findNewVertex(vertices[(k+1)%3], vertices[(k+2)%3],A2, B2,vertices[(k+1)%3]);
								vertices[k].outcode-=TOP;
								vertices[(k+1)%3].outcode-=TOP;
								hVertices[k].y = B;
								hVertices[k].x = A;
								hVertices[(k+1)%3].y = B2;
								hVertices[(k+1)%3].x = A2;
								if(A<=-w && A>=w && A2<=-w && A2>=w){
									vertices[(k+1)%3].outcode = 0;
									vertices[k].outcode = 0;
								}else{
									vertices[(k+1)%3].outcode = 0;
									vertices[k].outcode = 0;
									if(A>1){
											vertices[k].outcode += RIGHT;
									}else if(A<-1){
											vertices[k].outcode += LEFT;
									}

									if(A2>1){
											vertices[(k+1)%3].outcode += RIGHT;
									}else if(A2<-1){
											vertices[(k+1)%3].outcode += LEFT;
									}
								}

							}
							if((vertices[k].outcode&LEFT)!=0 && (vertices[(k+1)%3].outcode&LEFT)!=0){
								//clip 1
								float w = -1;//hVertices[1].w;
								//Find interval.
								float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
								float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
								float A = w;
								findNewVertex(vertices[k], vertices[(k+2)%3],A, B,vertices[k]);
								float interval2 = (w-hVertices[(k+1)%3].x)/(hVertices[(k+2)%3].x-hVertices[(k+1)%3].x);
								float B2 = hVertices[(k+1)%3].y +interval2*(hVertices[(k+2)%3].y-hVertices[(k+1)%3].y);
								float A2 = w;

								findNewVertex(vertices[(k+1)%3], vertices[(k+2)%3],A2, B2,vertices[(k+1)%3]);
								vertices[k].outcode-=LEFT;
								vertices[(k+1)%3].outcode-=LEFT;
								hVertices[k].y = B;
								hVertices[k].x = A;
								hVertices[(k+1)%3].y = B2;
								hVertices[(k+1)%3].x = A2;
								if(B<=-w && B>=w && B2<=-w && B2>=w){
									vertices[(k+1)%3].outcode = 0;
									vertices[k].outcode = 0;
								}else{
									vertices[(k+1)%3].outcode = 0;
									vertices[k].outcode = 0;
									if(B>-w){
											vertices[k].outcode += BOTTOM;
									}else if(B<w){
											vertices[k].outcode += TOP;
									}

									if(B2>-w){
											vertices[(k+1)%3].outcode += BOTTOM;
									}else if(B2<w){
											vertices[(k+1)%3].outcode += TOP;
									}
								}

							}else if((vertices[k].outcode&RIGHT)!=0 && (vertices[(k+1)%3].outcode&RIGHT)!=0){
								//clip 1
								float w = 1;//hVertices[1].w;
								//Find interval.
								float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
								float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
								float A = w;
								findNewVertex(vertices[k], vertices[(k+2)%3],A, B,vertices[k]);
								float interval2 = (w-hVertices[(k+1)%3].x)/(hVertices[(k+2)%3].x-hVertices[(k+1)%3].x);
								float B2 = hVertices[(k+1)%3].y +interval2*(hVertices[(k+2)%3].y-hVertices[(k+1)%3].y);
								float A2 = w;
								findNewVertex(vertices[(k+1)%3], vertices[(k+2)%3],A2, B2,vertices[(k+1)%3]);
								vertices[k].outcode-=RIGHT;
								vertices[(k+1)%3].outcode-=RIGHT;
								hVertices[k].y = B;
								hVertices[k].x = A;
								hVertices[(k+1)%3].y = B2;
								hVertices[(k+1)%3].x = A2;
								if(B<w && B>-w && B2<w && B2>-w){
									vertices[(k+1)%3].outcode = 0;
									vertices[k].outcode = 0;
								}else{
									vertices[(k+1)%3].outcode = 0;
									vertices[k].outcode = 0;
									if(B>w){
											vertices[k].outcode += BOTTOM;
									}else if(B<-w){
											vertices[k].outcode += TOP;
									}

									if(B2>w){
											vertices[(k+1)%3].outcode += BOTTOM;
									}else if(B2<-w){
											vertices[(k+1)%3].outcode += TOP;
									}
								}
							}
					}
					for(int k =0; k<3;k++){
						vector<Vertex> newTriangle;
						newTriangle.resize(3);
						if((vertices[k].outcode&RIGHT)!=0 && (vertices[k].outcode&BOTTOM)!=0){
								float w = 1;//hVertices[1].w;
								//Find interval.
								float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
								float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
								float A = w;
								Vertex ktok1;
								Vertex ktok2;
								findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
								float interval2 = (w-hVertices[k].x)/(hVertices[(k+1)%3].x-hVertices[k].x);
								float B2 = hVertices[k].y +interval2*(hVertices[(k+1)%3].y-hVertices[k].y);
								float A2 = w;
								findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
								newTriangle[k]=ktok1;
								newTriangle[(k+1)%3]=vertices[(k+1)%3];
								newTriangle[(k+2)%3]=ktok2;
								triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));
								vertices[k]= ktok2;
								vertices[k].outcode-=RIGHT;
								candraw = false;
									triangles.push_back(Triangle(vertices[0].position,vertices[1].position,vertices[2].position,triangles[i].color));
						}else if((vertices[k].outcode&LEFT)!=0 && (vertices[k].outcode&BOTTOM)!=0){
							float w = -1;//hVertices[1].w;
							//Find interval.
							float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
							float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
							float A = w;
							Vertex ktok1;
							Vertex ktok2;
							findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
							float interval2 = (w-hVertices[k].x)/(hVertices[(k+1)%3].x-hVertices[k].x);
							float B2 = hVertices[k].y +interval2*(hVertices[(k+1)%3].y-hVertices[k].y);
							float A2 = w;
							findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
							newTriangle[k]=ktok1;
							newTriangle[(k+1)%3]=vertices[(k+1)%3];
							newTriangle[(k+2)%3]=ktok2;
							triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));
							vertices[k]= ktok2;
							vertices[k].outcode-=RIGHT;
							candraw = false;
								triangles.push_back(Triangle(vertices[0].position,vertices[1].position,vertices[2].position,triangles[i].color));
						}else if((vertices[k].outcode&LEFT)!=0 && (vertices[k].outcode&TOP)!=0){
							float w = -1;//hVertices[1].w;
							//Find interval.
							float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
							float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
							float A = w;
							Vertex ktok1;
							Vertex ktok2;
							findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
							float interval2 = (w-hVertices[k].x)/(hVertices[(k+1)%3].x-hVertices[k].x);
							float B2 = hVertices[k].y +interval2*(hVertices[(k+1)%3].y-hVertices[k].y);
							float A2 = w;
							findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
							newTriangle[k]=ktok1;
							newTriangle[(k+1)%3]=vertices[(k+1)%3];
							newTriangle[(k+2)%3]=ktok2;
							triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));
							vertices[k]= ktok2;
							vertices[k].outcode-=RIGHT;
							candraw = false;
								triangles.push_back(Triangle(vertices[0].position,vertices[1].position,vertices[2].position,triangles[i].color));
						}else if((vertices[k].outcode&RIGHT)!=0 && (vertices[k].outcode&TOP)!=0){
							float w = 1;//hVertices[1].w;
							//Find interval.
							float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
							float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
							float A = w;
							Vertex ktok1;
							Vertex ktok2;
							findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
							float interval2 = (w-hVertices[k].x)/(hVertices[(k+1)%3].x-hVertices[k].x);
							float B2 = hVertices[k].y +interval2*(hVertices[(k+1)%3].y-hVertices[k].y);
							float A2 = w;
							findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
							newTriangle[k]=ktok1;
							newTriangle[(k+1)%3]=vertices[(k+1)%3];
							newTriangle[(k+2)%3]=ktok2;
							triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));
							vertices[k]= ktok2;
							vertices[k].outcode-=RIGHT;
							candraw = false;
								triangles.push_back(Triangle(vertices[0].position,vertices[1].position,vertices[2].position,triangles[i].color));
						}else if((vertices[k].outcode&BOTTOM)!=0){
							float w = 1;//hVertices[1].w;
							//Find interval.
							float interval1 = (w-hVertices[k].y)/(hVertices[(k+2)%3].y-hVertices[k].y);
							float A = hVertices[k].x +interval1*(hVertices[(k+2)%3].x-hVertices[k].x);
							float B = w;
							Vertex ktok1;
							Vertex ktok2;
							findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
							float interval2 = (w-hVertices[k].y)/(hVertices[(k+1)%3].y-hVertices[k].y);
							float A2 = hVertices[k].x +interval2*(hVertices[(k+1)%3].x-hVertices[k].x);
							float B2 = w;
							findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
							newTriangle[k]=ktok1;
							newTriangle[(k+1)%3]=vertices[(k+1)%3];
							newTriangle[(k+2)%3]=ktok2;

							vertices[k].position = ktok2.position;
							vertices[k].outcode-=BOTTOM;
							triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));

						}else if((vertices[k].outcode&TOP)!=0){

							float w = -1;//hVertices[1].w;
							//Find interval.
							float interval1 = (w-hVertices[k].y)/(hVertices[(k+2)%3].y-hVertices[k].y);
							float A = hVertices[k].x +interval1*(hVertices[(k+2)%3].x-hVertices[k].x);
							float B = w;
							Vertex ktok1;
							Vertex ktok2;
							findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
							float interval2 = (w-hVertices[k].y)/(hVertices[(k+1)%3].y-hVertices[k].y);
							float A2 = hVertices[k].x +interval2*(hVertices[(k+1)%3].x-hVertices[k].x);
							float B2 = w;
							findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
							newTriangle[k]=ktok1;
							newTriangle[(k+1)%3]=vertices[(k+1)%3];
							newTriangle[(k+2)%3]=ktok2;
							vertices[k].position= ktok2.position;
							vertices[k].outcode-=TOP;

							triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));
						}else if((vertices[k].outcode&LEFT)!=0){
							float w = -1;//hVertices[1].w;
							//Find interval.
							float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
							float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
							float A = w;
							Vertex ktok1;
							Vertex ktok2;
							findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
							float interval2 = (w-hVertices[k].x)/(hVertices[(k+1)%3].x-hVertices[k].x);
							float B2 = hVertices[k].y +interval2*(hVertices[(k+1)%3].y-hVertices[k].y);
							float A2 = w;
							findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
							newTriangle[k]=ktok1;
							newTriangle[(k+1)%3]=vertices[(k+1)%3];
							newTriangle[(k+2)%3]=ktok2;
							vertices[k].position= ktok2.position;
							vertices[k].outcode-=LEFT;

							triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));

						}else if((vertices[k].outcode&RIGHT)!=0){
							float w = 1;//hVertices[1].w;
							//Find interval.
							float interval1 = (w-hVertices[k].x)/(hVertices[(k+2)%3].x-hVertices[k].x);
							float B = hVertices[k].y +interval1*(hVertices[(k+2)%3].y-hVertices[k].y);
							float A = w;
							Vertex ktok1;
							Vertex ktok2;
							findNewVertex(vertices[k], vertices[(k+2)%3],A, B,ktok2);
							float interval2 = (w-hVertices[k].x)/(hVertices[(k+1)%3].x-hVertices[k].x);
							float B2 = hVertices[k].y +interval2*(hVertices[(k+1)%3].y-hVertices[k].y);
							float A2 = w;
							findNewVertex(vertices[k], vertices[(k+1)%3],A2, B2,ktok1);
							newTriangle[k]=ktok1;
							newTriangle[(k+1)%3]=vertices[(k+1)%3];
							newTriangle[(k+2)%3]=ktok2;
							vertices[k].position = ktok2.position;
							vertices[k].outcode-=RIGHT;
							triangles.push_back(Triangle(newTriangle[0].position,newTriangle[1].position,newTriangle[2].position,triangles[i].color));

						}
					}
					if(candraw){
						if(isnan(vertices[0].position.z)==false)	DrawPolygon( vertices );
					}
				}
			}
		}
		if ( SDL_MUSTLOCK(screen) )
			SDL_UnlockSurface(screen);
		SDL_UpdateRect( screen, 0, 0, 0, 0 );
	}

	void findNewVertex(Vertex vertex1, Vertex vertex2, float A, float B,Vertex& newVertex){
		if(((vertex2.position.y-vertex1.position.y)- (B/A)*(vertex2.position.x-vertex1.position.x))==0){
			float z = (((vertex1.position.y)-cameraPos.y)*2)/B;
			newVertex.position.z = z+cameraPos.z;
			newVertex.position.y = vertex1.position.y;
			newVertex.position.x = vertex1.position.x;
			if(isnan(newVertex.position.z)){
				cerr<<"z is zero in 1"<<endl;
			}
		}else{
			float worldInterval = ((B/A)*(vertex1.position.x - cameraPos.x) - vertex1.position.y + cameraPos.y)/((vertex2.position.y-vertex1.position.y)- (B/A)*(vertex2.position.x-vertex1.position.x));
			newVertex.position.x = vertex1.position.x + worldInterval*(vertex2.position.x-vertex1.position.x);
			newVertex.position.y = vertex1.position.y + worldInterval*(vertex2.position.y-vertex1.position.y);
			newVertex.position.z = vertex1.position.z + worldInterval*(vertex2.position.z-vertex1.position.z);
			if(isnan(newVertex.position.z)){
				cerr<<"z is zero in 2"<<endl;
				cerr<<"A is"<<A<<" B is "<<B<<endl;
				newVertex.position.x = 0;
				newVertex.position.y = 0;
				newVertex.position.z = 0;
			}
		}

	}
	void VertexShader( const Vertex& v, Pixel& p ){
			vec3 newv1 = v.position;
			if(seeClip == 1)
				newv1.z = newv1.z+0.1;
			vec3 newv = newv1-cameraPos;
			newv=R*newv;
			p.x = (focalLength*newv.x)/newv.z + SCREEN_WIDTH/2;
			p.y = (focalLength*newv.y)/newv.z+ SCREEN_HEIGHT/2;
			p.zinv = 1/newv.z;
			p.pos3d = newv1/newv.z;
	}

	void PixelShader( const Pixel& p )
	{
		int x = p.x;
		int y = p.y;
		if( p.zinv > depthBuffer[y][x] )
		{
		depthBuffer[y][x] = p.zinv;
		vec3 pos= p.pos3d * (1/p.zinv);
		vec3 rhat = lightPos - pos;
		vec3 normal = currentNormal;
		float r = glm::length(rhat);
		rhat= rhat/r;
		float A = 4*(float)M_PI*r*r;
		vec3 B = lightPower/A;
		vec3 D= B*max(dot(normal,rhat),(float)0);
		vec3 Rlight = currentReflectance*(D+indirectLightPowerPerArea);
		PutPixelSDL( screen, x, y, Rlight*currentColor);
	  }

	}
	void Interpolate( Pixel a, Pixel b, vector<Pixel>& result ){
		int N = result.size();
		vec3 step = vec3(b.x-a.x,b.y-a.y,b.zinv-a.zinv) / float(max(N-1,1));
		vec3 posStep = vec3(b.pos3d - a.pos3d)/float(max(N-1,1));
		vec3 current( a.x,a.y,a.zinv);
		vec3 currentPos = a.pos3d;
		for( int i=0; i<N; ++i )
		{
			result[i].x=current.x;
			result[i].y=(current.y+0.5);
			result[i].zinv=current.z;
			result[i].pos3d= currentPos;
			current += step;
			currentPos+=posStep;
		}
	}
	// void DrawLineSDL( SDL_Surface* surface, ivec2 a, ivec2 b, vec3 color ){
	// 	ivec2 delta = glm::abs( a - b );
	// 	int pixels = glm::max( delta.x, delta.y ) + 1;
	// 	vector<ivec2> line( pixels );
	// 	Interpolate( a, b, line );
	// 	for(int i=0;i<pixels;i++){
	// 		vec3 color(1,1,1);
	// 		PutPixelSDL( screen, line[i].x, line[i].y, color );
	// 	}
	//
	// }

	// void DrawPolygonEdges( const vector<vec3>& vertices )
	// {
	// 	int V = vertices.size();
	// 	// Transform each vertex from 3D world position to 2D image position:
	// 	vector<ivec2> projectedVertices( V );
	// 	for( int i=0; i<V; ++i )
	// 	{
	// 		VertexShader( vertices[i], projectedVertices[i] );
	// 	}
	// 	// Loop over all vertices and draw the edge from it to the next vertex:
	// 	for( int i=0; i<V; ++i )
	// 	{
	// 		int j = (i+1)%V; // The next vertex
	// 		vec3 color( 1, 1, 1 );
	// 		DrawLineSDL( screen, projectedVertices[i], projectedVertices[j], color );
	// 	}
	// }
	void ComputePolygonRows(const vector<Pixel>& vertexPixels,vector<Pixel>& leftPixels,vector<Pixel>& rightPixels )
	{
		// 1. Find max and min y-value of the polygon
		//    and compute the number of rows it occupies.
		int maxy=vertexPixels[0].y;
		int miny=vertexPixels[0].y;
		for(int i=1;i< (int)vertexPixels.size();i++){
			if(vertexPixels[i].y>maxy){
				maxy=vertexPixels[i].y;
			}
			if(vertexPixels[i].y<miny){
				miny=vertexPixels[i].y;
			}
		}
		int rows = maxy-miny+1;
		// 2. Resize leftPixels and rightPixels
		//    so that they have an element for each row.
		leftPixels.resize(rows);
    rightPixels.resize(rows);
		// 3. Initialize the x-coordinates in leftPixels
		//    to some really large value and the x-coordinates
		//    in rightPixels to some really small value.
		for( int i=0; i<rows; ++i )
		{
				leftPixels[i].x  = +numeric_limits<int>::max();
				rightPixels[i].x = -numeric_limits<int>::max();
		}
		// 4. Loop through all edges of the polygon and use
		//    linear interpolation to find the x-coordinate for
		//    each row it occupies. Update the corresponding
		//    values in rightPixels and leftPixels.
		for( int i=0; i<(int)vertexPixels.size(); ++i )
		{
			int j = (i+1)%(int)vertexPixels.size(); // The next vertex
			vec3 delta = glm::abs( vec3(vertexPixels[i].x-vertexPixels[j].x,vertexPixels[i].y-vertexPixels[j].y, vertexPixels[i].zinv-vertexPixels[j].zinv));
			int pixels = glm::max( delta.x, delta.y) + 1;
			vector<Pixel> line(pixels);
			Interpolate( vertexPixels[i], vertexPixels[j], line );
			for(int k=0; k<pixels;++k){

					if( leftPixels[line[k].y-miny].x> line[k].x){
						leftPixels[line[k].y-miny] = line[k];
					}
					if( rightPixels[line[k].y-miny].x< line[k].x){
						rightPixels[line[k].y-miny] = line[k];
					}
				//}

			}
		}
	}

	void DrawRows(const vector<Pixel>& leftPixels,const vector<Pixel>& rightPixels ){
		for(int i=0;i<(int)leftPixels.size();i++){
					vec3 delta = glm::abs( vec3( leftPixels[i].x - rightPixels[i].x, leftPixels[i].y - rightPixels[i].y, leftPixels[i].zinv - rightPixels[i].zinv));
					int pixels = glm::max( delta.x, delta.y ) + 1;
					if(pixels>1){
						vector<Pixel> line;
						line.resize(pixels);
						Interpolate( leftPixels[i], rightPixels[i], line );
						for(int j=0; j<pixels;j++){
							if(line[j].x>=0&&line[j].x<SCREEN_WIDTH && line[j].y>=0 && line[j].y<SCREEN_HEIGHT){
								PixelShader(line[j]);

							}
						}
					}
		}
	}
	void DrawPolygon( const vector<Vertex>& vertices )
	{
		int V = vertices.size();
		vector<Pixel> vertexPixels;
		vertexPixels.resize(V);
		for( int i=0; i<V; ++i ){
			VertexShader( vertices[i], vertexPixels[i] );
		}
		// int a =3;
		vector<Pixel> leftPixels;
		vector<Pixel> rightPixels;
		ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
		if(leftPixels.size()>1){
			DrawRows(leftPixels,rightPixels);
		}else{
			// printf("%d\t",leftPixels.size());
			// printf("%d\n",rightPixels.size());

		}
}
