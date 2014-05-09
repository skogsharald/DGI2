#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include "math.h"

using namespace std;
using glm::vec3;
using glm::mat3;
using glm::vec3;
using glm::mat3;


// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;
int t;
vector<Triangle> triangles;
float f = SCREEN_HEIGHT/2;
float m = std::numeric_limits<float>::max();
vec3 cameraPos = vec3(0, 0 , -2);
mat3 R;
float yaw = 0;
vec3 lightPos( 0, -0.5, -0.7 );
vec3 lightColor = 20.f * vec3( 1, 1, 1 );
vec3 indirectLight = 0.5f*vec3( 1, 1, 1 );



// ----------------------------------------------------------------------------
// FUNCTIONS
struct Intersection
{
	vec3 position;
	float distance;
	int triangleIndex;
};


void Update();
void Draw();
bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection );
void UpdateR();
vec3 DirectLight( const Intersection& i );
vec3 IndirectLight( const Intersection& i);


int main( int argc, char* argv[] )
{
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.
	LoadTestModel( triangles );
	UpdateR();

	while( NoQuitMessageSDL() )
	{
		Update();
		Draw();
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void UpdateR(){
	vec3 a(cos(yaw), 0, -sin(yaw));
	vec3 b(0, 1, 0);
	vec3 c(sin(yaw), 0, cos(yaw));
	R = mat3(a, b, c);
}

void Update()
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;

	vec3 forward( R[2][0], R[2][1], R[2][2] );
	vec3 right( R[0][0], R[0][1], R[0][2] );
	vec3 down( R[1][0], R[1][1], R[1][2] );
	Uint8* keystate = SDL_GetKeyState( 0 );

	if( keystate[SDLK_UP] )
	{
		cameraPos += vec3(0, 0, 0.1f);
	}
	if( keystate[SDLK_DOWN] )
	{
		cameraPos -= vec3(0, 0, 0.1f);
	}
	if( keystate[SDLK_LEFT] )
	{
		yaw += 0.03f;
		UpdateR();
	}
	if( keystate[SDLK_RIGHT] )
	{
		//cameraPos += vec3(0.1f, 0, 0);
		yaw -= 0.03f;
		UpdateR();

	}
	if( keystate[SDLK_w]) {
		lightPos += forward;
	}
	if( keystate[SDLK_s]) {
		lightPos -= forward;
	}
	if( keystate[SDLK_a]) {
		lightPos -= right;
	}
	if( keystate[SDLK_d]) {
		lightPos += right;
	}
	if( keystate[SDLK_q]) {
		lightPos -= down;
	}
	if( keystate[SDLK_e]) {
		lightPos += down;
	}

}

void Draw()
{
	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);

	for( int y=0; y<SCREEN_HEIGHT; ++y )
	{
		for( int x=0; x<SCREEN_WIDTH; ++x )
		{	
			vec3 d = vec3(x - SCREEN_WIDTH/2, y - SCREEN_HEIGHT / 2, f);
			d = R*d;
			Intersection closestIntersection;
			vec3 dColor(0,0,0);
			vec3 lColor(0,0,0);
			vec3 iColor(0,0,0);
			if(ClosestIntersection(cameraPos, d, triangles, closestIntersection)){
				lColor = DirectLight(closestIntersection);
				iColor = IndirectLight(closestIntersection);
				dColor = triangles[closestIntersection.triangleIndex].color * iColor;
			}
			PutPixelSDL( screen, x, y, dColor );
		}
	}

	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

vec3 DirectLight( const Intersection& i ){
	vec3 n_hatt = triangles[i.triangleIndex].normal;
	vec3 r_hatt = glm::normalize(lightPos - i.position);

	float r = glm::length(lightPos - i.position);

	// Shadow
	Intersection shadow;
	vec3 shadowDir = glm::normalize(i.position - lightPos);
	if(ClosestIntersection(lightPos, shadowDir, triangles, shadow)){
		// Compensate for rounding error
		if(shadow.distance < r - 0.00001f){
			return vec3(0,0,0);
		}
	}
    // Get the direction that the light travels.
    // vec3 dir = glm::normalize(i.position - lightPos);


	float dot_product = glm::dot(r_hatt, n_hatt);

	float divisor = 4 * M_PI * r * r;

	vec3 returnVec = lightColor * std::max(dot_product, 0.0f)/divisor;
	return returnVec;
	
}

vec3 IndirectLight( const Intersection& i){

	return DirectLight(i) + indirectLight;
}

bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection ){
	float distance = m;
	bool found = false;
	for(int i = 0; i < triangles.size(); ++i){
		Triangle triangle = triangles[i];

		vec3 v0 = triangle.v0;
		vec3 v1 = triangle.v1;
		vec3 v2 = triangle.v2;
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;
		vec3 b = start - v0;
		mat3 A( -dir, e1, e2 );
		vec3 x = glm::inverse( A ) * b;

		// According to the equations given, this is the distance
		float t = x.x;
		// Have to see if it even is in the triangle
		if(t>=0 && t<=distance && 0 <= x.y && 0 <= x.z && (x.y+x.z)<=1){
			closestIntersection.position = start + t*dir;
			closestIntersection.distance = glm::distance(start + t*dir, start);
			closestIntersection.triangleIndex = i;

			found = true;
			distance = t;
		}
	}
	return found;
}
