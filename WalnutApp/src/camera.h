#ifndef CAMERA_H
#define CAMERA_H


#include <glm/glm.hpp>

#include "geometry.h"

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// camera Attributes
	Vec3f Position;
	Vec3f Front;
	Vec3f Up;
	Vec3f Right;
	Vec3f WorldUp;
	// euler Angles
	float Yaw;
	float Pitch;
	// camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	// constructor with vectors
	Camera( Vec3f position = Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f up = Vec3f( 0.0f, 1.0f, 0.0f ), float yaw = YAW, float pitch = PITCH ) : Front( Vec3f( 0.0f, 0.0f, -1.0f ) ), MovementSpeed( SPEED ), MouseSensitivity( SENSITIVITY ), Zoom( ZOOM )
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors( );
	}
	// constructor with scalar values
	Camera( float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch ) : Front( Vec3f( 0.0f, 0.0f, -1.0f ) ), MovementSpeed( SPEED ), MouseSensitivity( SENSITIVITY ), Zoom( ZOOM )
	{
		Position = Vec3f( posX, posY, posZ );
		WorldUp = Vec3f( upX, upY, upZ );
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors( );
	}

	// returns the view matrix calculated using Euler Angles and the LookAt Matrix
	Matrix GetViewMatrix( )
	{
		Vec3f z = ( Front - Position ).normalize( );
		Vec3f x = cross( Up, z ).normalize( );
		Vec3f y = cross( z, x ).normalize( );
		Matrix Minv = Matrix::identity( );
		Matrix Tr = Matrix::identity( );
		for( int i = 0; i < 3; i++ ) {
			Minv[ 0 ][ i ] = x[ i ];
			Minv[ 1 ][ i ] = y[ i ];
			Minv[ 2 ][ i ] = z[ i ];
			Tr[ i ][ 3 ] = -Front[ i ];
		}

		return Minv * Tr;
	}


	Matrix GetViewport( int x, int y, int w, int h ) {
		//Matrix m = Matrix::identity();
		//m[ 0 ][ 3 ] = x + w / 2.f;
		//m[ 1 ][ 3 ] = y + h / 2.f;
		//m[ 2 ][ 3 ] = depth / 2.f;

		//m[ 0 ][ 0 ] = w / 2.f;
		//m[ 1 ][ 1 ] = h / 2.f;
		//m[ 2 ][ 2 ] = depth / 2.f;
		//return m;
	}

	// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard( Camera_Movement direction, float deltaTime )
	{
		float velocity = MovementSpeed * deltaTime;
		if( direction == FORWARD )
			Position = Position + Front * velocity;
		if( direction == BACKWARD )
			Position = Position - Front * velocity;
		if( direction == LEFT )
			Position = Position - Right * velocity;
		if( direction == RIGHT )
			Position = Position + Right * velocity;
	}

	// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement( float xoffset, float yoffset, bool constrainPitch = true )
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if( constrainPitch )
		{
			if( Pitch > 89.0f )
				Pitch = 89.0f;
			if( Pitch < -89.0f )
				Pitch = -89.0f;
		}

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors( );
	}

	// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll( float yoffset )
	{
		Zoom -= (float)yoffset;
		if( Zoom < 1.0f )
			Zoom = 1.0f;
		if( Zoom > 45.0f )
			Zoom = 45.0f;
	}

private:
	// calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors( )
	{
		// calculate the new Front vector
		Vec3f front;
		front.x = cos( glm::radians( Yaw ) ) * cos( glm::radians( Pitch ) );
		front.y = sin( glm::radians( Pitch ) );
		front.z = sin( glm::radians( Yaw ) ) * cos( glm::radians( Pitch ) );
		Front = front.normalize( );
		// also re-calculate the Right and Up vector
		Right = cross( Front, WorldUp ).normalize( );  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = cross( Right, Front ).normalize( );
	}
};
#endif