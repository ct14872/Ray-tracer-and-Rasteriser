#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include "math.h"
#include <omp.h>

using namespace std;
using glm::vec3;
using glm::mat3;

/* ----------------------------------------------------------------------------*/
/* Structures                                                            */
struct aperture{
	float sideLength;
	vec3 cenPos;
};

struct Photon {
vec3 position;
vec3 power;
};

struct Intersection
{
vec3 position;
vec3 normal;
float distance;
int triangleIndex;
};

/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                            */

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
float focalDistance = 100000;
int DOFon = 0;
int photonMapOn=0;
SDL_Surface* screen;
int t;
float f = 500;
vec3 cameraPos( 0, 0, -3 );
vec3 initCamPos = cameraPos;
vec3 lightPos (0,-0.5, -0.7);
vec3 lightColor = (14.f * vec3(1, 1, 1));
vec3 initLightPos = lightPos;
vec3 indirectLight = 0.1f*vec3(1,1,1);
mat3 R;
mat3 Rx;
float yaw=0;
float theta=0;
float buffer[SCREEN_WIDTH*SCREEN_HEIGHT];
int counter = 0;
int neededPhotons=10000;
vector<Photon> allphotons;
int totalphotons=0;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw(const vector<Triangle>& triangles );
bool closestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& cIntersection, bool shadow );
vec3 DirectLight( const Intersection& inter, vector<Triangle> triangles);
vec3 focal(vec3 start, vec3 dir);
void emmitPhotons(vector<Photon>& photons, const vector<Triangle>& triangles);
void findPhoton(vec3 position ,vec3& colour, vector<Photon>& photons){
	colour = vec3(0,0,0);
	for (int i=0;i<(int)photons.size();i++){
		vec3 distance = position- photons[i].position;
		float totaldistance = (distance.x*distance.x)+(distance.y*distance.y)+(distance.z*distance.z);
		totaldistance= sqrt(totaldistance);
		if (totaldistance<0.2&&totaldistance>-0.2){
			totaldistance=totaldistance;
			float distanceScale  = pow((1-totaldistance),10);
			int scale = 12;
			vec3 scaledPower((((photons[i].power.x*scale)/totalphotons)*distanceScale),(((photons[i].power.y*scale)/totalphotons)*distanceScale), (((photons[i].power.z*scale)/totalphotons)*distanceScale));
			colour+=scaledPower;
		}
	}

}
int main( int argc, char* argv[] )
{
	#pragma omp parallel
	{
	#pragma omp master
	printf("Threads: %d\n",omp_get_num_threads());
  }
	R[0] = vec3 (cos(yaw),0,-sin(yaw));
	R[1] = vec3 (0,1,0);
	R[2] = vec3 (sin(yaw),0,cos(yaw) );

	Rx[0] = vec3 (1,0,0);
	Rx[1] = vec3 (0,cos(theta),sin(theta));
	Rx[2] = vec3 (0,-sin(theta),cos(theta));

	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	vector<Triangle> triangles;
	LoadTestModel( triangles );

	while( NoQuitMessageSDL() )
	{
		Update();
		Draw(triangles);
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );

	return 0;
}

void Update(){
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;

	cout << "Render time: " << dt << " ms." << endl;
	Uint8* keystate = SDL_GetKeyState( 0 );
	if( keystate[SDLK_r]){//reset cameraPos and orienntation
		yaw=0;
		theta =0;
		cameraPos = initCamPos;
	}
	//turn on or off photon mapping
	if( keystate[SDLK_z]){
		photonMapOn = 1;
	}
	if( keystate[SDLK_x]){
		photonMapOn = 0;
	}
	//turn on or off depth of field
	if( keystate[SDLK_c] ){
		DOFon = 1;
	}
	if( keystate[SDLK_v] ){
		DOFon = 0;
	}
	//adjust the focal distance
	if( keystate[SDLK_b] ){
		focalDistance -= 1000;
	}
	if( keystate[SDLK_n] ){
		focalDistance += 1000;
	}
	// camera movement
	if( keystate[SDLK_w] ){
		cameraPos = cameraPos + vec3 (0,0,0.1)*(Rx*R);
	}
	if( keystate[SDLK_s] ){
		cameraPos = cameraPos + vec3 (0,0,-0.1)*(Rx*R);
	}
	if( keystate[SDLK_a] ){
		cameraPos = cameraPos + vec3 (-0.1,0,0)*(Rx*R);
	}
	if( keystate[SDLK_d] ){
		cameraPos = cameraPos + vec3 (0.1,0,0)*(Rx*R);
	}
	if( keystate[SDLK_q] ){
		cameraPos = cameraPos + vec3 (0,0.1,0)*(Rx*R);
	}
	if( keystate[SDLK_e] ){
		cameraPos = cameraPos + vec3 (0,-0.1,0)*(Rx*R);
	}
	// camera rotation
	if( keystate[SDLK_LEFT]){
		yaw = yaw + 0.1;
	}
	if( keystate[SDLK_RIGHT]){
		yaw = yaw - 0.1;
	}
	if( keystate[SDLK_UP]){
		theta = theta- 0.1;
	}
	if( keystate[SDLK_DOWN]){
		theta = theta + 0.1;
	}
	//light source movement
	if( keystate[SDLK_i] ){
		lightPos = lightPos + vec3 (0,0,0.11);
	}
	if( keystate[SDLK_k] ){
		lightPos = lightPos + vec3 (0,0,-0.11);
	}
	if( keystate[SDLK_j] ){
		lightPos = lightPos + vec3 (-0.11,0,0);
	}
	if( keystate[SDLK_l] ){
		lightPos = lightPos + vec3 (0.11,0,0);
	}
	if( keystate[SDLK_u] ){
		lightPos = lightPos + vec3 (0,-0.11,0);
	}
	if( keystate[SDLK_o] ){
		lightPos = lightPos + vec3 (0,0.11,0);
	}
	if( keystate[SDLK_p] ){
		lightPos = initLightPos;
	}

	R[0] = vec3 (cos(yaw),0,-sin(yaw));
	R[1] = vec3 (0,1,0);
	R[2] = vec3 (sin(yaw),0,cos(yaw) );

	Rx[0] = vec3 (1,0,0);
	Rx[1] = vec3 (0,cos(theta),sin(theta));
	Rx[2] = vec3 (0,-sin(theta),cos(theta));
}

void Draw( const vector<Triangle>& triangles )
{
	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);


	if( photonMapOn%2 == 1 ){
		vector<Photon> photons;
		emmitPhotons(photons, triangles);
		totalphotons=totalphotons+neededPhotons;
		cout<<photons.size()<<endl;
		allphotons.insert(allphotons.end(), photons.begin(), photons.end());
		bool q =false;
		#pragma omp parallel for firstprivate(q)
		for( int y=0; y<SCREEN_HEIGHT; ++y )
		{
			for( int x=0; x<SCREEN_WIDTH; ++x )
			{
				Intersection tempInt;
				tempInt.distance = -1;
				vec3 dir (x - SCREEN_WIDTH/2, y - SCREEN_HEIGHT/2, f);
				dir =dir*Rx*R;
				q = closestIntersection(cameraPos, dir, triangles, tempInt, false);

				if(q){

					float reflectivness = triangles[tempInt.triangleIndex].s;

					if(reflectivness==1){
						vec3 n = triangles[tempInt.triangleIndex].normal;
						float perp = 2.0 * glm::dot(dir, n);
						vec3 reflectDir = dir - (perp * n);
						Intersection tempInt2;
						tempInt2.distance = -1;
						bool q2 = false;
						q2 = closestIntersection(tempInt.position, reflectDir, triangles, tempInt2, false);
						if(q2){
							vec3 colour;
							findPhoton(tempInt2.position,colour, allphotons);
							PutPixelSDL( screen, x, y, (indirectLight)*triangles[tempInt.triangleIndex].color + colour );
						}else{
								PutPixelSDL( screen, x, y, vec3 (0,0,0) );
						}

					}else if(reflectivness == 0){
						vec3 colour;
						findPhoton(tempInt.position,colour, allphotons);
						PutPixelSDL( screen, x, y, (indirectLight)*triangles[tempInt.triangleIndex].color + colour );
					}else{
						vec3 colourreflect;
						vec3 colourMat;
						vec3 n = triangles[tempInt.triangleIndex].normal;
						float perp = 2.0 * glm::dot(dir, n);
						vec3 reflectDir = dir - (perp * n);
						Intersection tempInt3;
						tempInt3.distance = -1;
						bool q3 = false;
						q3 = closestIntersection(tempInt.position, reflectDir, triangles, tempInt3, false);
						findPhoton(tempInt.position,colourMat, allphotons);
						if(q3){
							findPhoton(tempInt3.position,colourreflect, allphotons);
							PutPixelSDL( screen, x, y, (indirectLight)*triangles[tempInt.triangleIndex].color + colourreflect*reflectivness + colourMat*(1-reflectivness) );
						}else{
							PutPixelSDL( screen, x, y, (indirectLight)*triangles[tempInt.triangleIndex].color + colourMat*(1-reflectivness) );
						}
					}
				}
			}
		}
	}else if(DOFon%2==1){
		bool q =false;
		#pragma omp parallel for firstprivate(q)
		for( int y=0; y<SCREEN_HEIGHT; ++y )
		{
			for( int x=0; x<SCREEN_WIDTH; ++x )
			{
				vec3 dir (x - SCREEN_WIDTH/2, y - SCREEN_HEIGHT/2, f);
				dir =dir*Rx*R;
				vec3 focalPoint;
				focalPoint = focal(cameraPos, dir);
				vec3 accumColor;
				int randomRaysNum = 20;
				for(int i=0; i<randomRaysNum; i++){

					Intersection tempInt;
					tempInt.distance = -1;
					int randx = cameraPos.x + rand()%251;
					int randy = cameraPos.y + rand()%251;
					vec3 dir1((focalPoint.x-randx) ,(focalPoint.y-randy), focalPoint.z);
					dir1 = dir1*(R)*Rx;

					q = closestIntersection(cameraPos, dir1, triangles, tempInt, false);
					vec3 color;
					if(q){
						vec3 Dtemp = DirectLight(tempInt, triangles);
						color = (Dtemp+indirectLight)*triangles[tempInt.triangleIndex].color ;
					}else{
						color = vec3(0,0,0);
					}
					accumColor += color;
				}
				accumColor = accumColor/vec3(randomRaysNum, randomRaysNum, randomRaysNum);
				PutPixelSDL( screen, x, y, accumColor );
			}
		}

	}else{
		bool q =false;
		#pragma omp parallel for firstprivate(q)
		for( int y=0; y<SCREEN_HEIGHT; ++y )
		{
			for( int x=0; x<SCREEN_WIDTH; ++x )
			{
				Intersection tempInt;
				tempInt.distance = -1;
				vec3 dir (x - SCREEN_WIDTH/2, y - SCREEN_HEIGHT/2, f);
				dir =dir*Rx*R;
				q = closestIntersection(cameraPos, dir, triangles, tempInt, false);
				if(q){
					vec3 Dtemp = DirectLight(tempInt, triangles);
					PutPixelSDL( screen, x, y, (Dtemp+indirectLight)*triangles[tempInt.triangleIndex].color );
				}
				else{
					PutPixelSDL( screen, x, y, vec3 (0,0,0) );
				}
			}
		}

	}

	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

vec3 focal(vec3 start, vec3 dir){
	vec3 focalPoint;
	float interval = focalDistance/dir.z;
	focalPoint.x = start.x + interval*dir.x;
	focalPoint.y = start.y + interval*dir.y;
	focalPoint.z = start.z + focalDistance;
	return focalPoint;
}


bool closestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& cIntersection, bool shadow ){
	bool validIntersection = false;
	for(size_t i=0; i<triangles.size();i++){
		Triangle triangle = triangles[i];
		vec3 v0 = triangle.v0;
		vec3 v1 = triangle.v1;
		vec3 v2 = triangle.v2;
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;
		vec3 b = start - v0;
		mat3 A( -dir, e1, e2 );
		vec3 x = glm::inverse( A ) * b;// where x.x = t and x.y = u and x.z = v
		if(x.x > 0.00001 && x.y>=0 && x.z>=0 && x.y+x.z<=1 && x.x*dir!=vec3(0,0,0)){
			if(cIntersection.distance == -1 || x.x <= cIntersection.distance){
				validIntersection = true;
				cIntersection.distance = x.x;
				cIntersection.position = start + x.x * dir;
				cIntersection.normal = triangles[i].normal;
				cIntersection.triangleIndex = i;
				if (shadow) return validIntersection;
			}
		}
	}
	return validIntersection;
}

float rand_FloatRange(float a, float b)
{
return ((b-a)*((float)rand()/RAND_MAX))+a;
}


void tracePhoton(vec3 start, vec3 dir, int depth,Photon& newPhoton, const vector<Triangle>& triangles, bool& coloured){

		float t;                               // distance to intersection
		Intersection tempInt;
		tempInt.distance = -1;
		bool inter  = closestIntersection(start, dir, triangles, tempInt, false);
		if (!inter){
			coloured = false;
			return;
		} // if miss, return black


		const Triangle tri = triangles[tempInt.triangleIndex];        // the hit object
		if (depth==5) return; //R.R.
		t = tempInt.distance;
		vec3 newStart = start + dir*t;
		vec3 n = tri.normal;
		vec3 f = tri.color;
		float decision=tri.s;
		float x = rand_FloatRange(0,1.0);
		if (decision<=x){
			decision = 0;
		}else{
			decision =1;
		}
		if(decision ==0.0){
			//absorb photon
		 		newPhoton.power = vec3(f.x*lightColor.x,f.y*lightColor.y,f.z*lightColor.z);
				newPhoton.position= newStart;
				return;
		}else if(decision ==1.0 ){
			//specular reflection
			float perp = 2.0 * glm::dot(dir, n);
			vec3 reflectDir = dir - (perp * n);
			tracePhoton(newStart,reflectDir,depth+1,newPhoton, triangles, coloured);
			coloured = true;
			return;
		}

}



void emmitPhotons(vector<Photon>& photons, const vector<Triangle>& triangles){

	int noOfPhotons =0;

	while(noOfPhotons<neededPhotons){
		Photon newPhoton;
		float x;
		float y;
		float z;
		do{
			x = rand_FloatRange(-1.0,1.0);
			y = rand_FloatRange(-1.0,1.0);
			z = rand_FloatRange(-1.0,1.0);
		}while((x*x + y*y + z*z) > 1);
		vec3 dir(x-lightPos.x,y-lightPos.y,z-lightPos.z);
		float rlength = glm::length(dir);
		dir = dir/rlength;
		bool coloured;
		tracePhoton(lightPos, dir,0, newPhoton, triangles, coloured);
		if(coloured){
			photons.push_back(newPhoton);
			noOfPhotons++;
		}

	}
}
vec3 DirectLight( const Intersection& inter, vector<Triangle> triangles){
	vec3 nHat = inter.normal;// a unit vector describing the normal pointing out from the surface
	vec3 rHat(lightPos.x-inter.position.x, lightPos.y-inter.position.y, lightPos.z-inter.position.z);
	float rlength = glm::length(rHat);
	rHat = rHat/rlength;
	float vSphere = 4*(float)M_PI*rlength*rlength;
	vec3 B(lightColor.x/vSphere,lightColor.y/vSphere,lightColor.z/vSphere);
	vec3 D = B*max(glm::dot(nHat,rHat),(float)0);
	Intersection temp;
	temp.distance = rlength;
	bool blocked = closestIntersection(inter.position, rHat,triangles,temp, true);

	if( blocked )
		return vec3 (0,0,0);
	else
		return D;
}
