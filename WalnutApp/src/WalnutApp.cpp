#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include "model.h"
#include "tgaimage.h"
#include <iostream>

using namespace Walnut;
Vec3f light_dir = Vec3f( 1, -1, 1 ).normalize( );
Vec3f eye( 1, 1, 3 );
Vec3f center( 0, 0, 0 );
Matrix ModelView;
Vec3f m2v( Matrix m ) {
	return Vec3f( m[ 0 ][ 0 ] / m[ 3 ][ 0 ], m[ 1 ][ 0 ] / m[ 3 ][ 0 ], m[ 2 ][ 0 ] / m[ 3 ][ 0 ] );
}
const int depth = 255;
Matrix v2m( Vec3f v ) {
	Matrix m( 4, 1 );
	m[ 0 ][ 0 ] = v.x;
	m[ 1 ][ 0 ] = v.y;
	m[ 2 ][ 0 ] = v.z;
	m[ 3 ][ 0 ] = 1.f;
	return m;
}

Matrix viewport( int x, int y, int w, int h ) {
	Matrix m = Matrix::identity( 4 );
	m[ 0 ][ 3 ] = x + w / 2.f;
	m[ 1 ][ 3 ] = y + h / 2.f;
	m[ 2 ][ 3 ] = depth / 2.f;

	m[ 0 ][ 0 ] = w / 2.f;
	m[ 1 ][ 1 ] = h / 2.f;
	m[ 2 ][ 2 ] = depth / 2.f;
	return m;
}

Matrix  lookat( Vec3f eye, Vec3f center, Vec3f up ) {
	Vec3f z = ( eye - center ).normalize( );
	Vec3f x = ( up ^ z ).normalize( );
	Vec3f y = ( z ^ x ).normalize( );
	Matrix res = Matrix::identity( 4 );
	for( int i = 0; i < 3; i++ ) {
		res[ 0 ][ i ] = x[ i ];
		res[ 1 ][ i ] = y[ i ];
		res[ 2 ][ i ] = z[ i ];
		res[ i ][ 3 ] = -center[ i ];
	}
	return res;
}

void DrawLine( int x0, int y0, int x1, int y1, uint32_t* m_ImageData, uint32_t color, uint32_t width ){

	bool steep = false;
	if( std::abs( x0 - x1 ) < std::abs( y0 - y1 ) ) {
		std::swap( x0, y0 );
		std::swap( x1, y1 );
		steep = true;
	}
	if( x0 > x1 ) {
		std::swap( x0, x1 );
		std::swap( y0, y1 );
	}
	int dx = x1 - x0;
	int dy = y1 - y0;

	int derror2 = std::abs( dy ) * 2;
	int error2 = 0;

	int y = y0;
	for( int x = x0; x <= x1; x++ ) {
		if( steep ) {
			m_ImageData[ x * width + y ] = color;
		}
		else {
			m_ImageData[ y * width + x ] = color;
		}
		error2 += derror2;
		if( error2 > dx ) {
			y += ( y1 > y0 ? 1 : -1 );
			error2 -= dx * 2;
		}
	}
}


void DrawTriangle( Vec3i t0, Vec3i t1, Vec3i t2, float ity0, float ity1, float ity2,
	uint32_t* m_ImageData, int* zbuffer, uint32_t width, uint32_t height ){
	if( t0.y == t1.y && t0.y == t2.y ) return; // i dont care about degenerate triangles
	if( t0.y > t1.y ) { std::swap( t0, t1 ); std::swap( ity0, ity1 ); }
	if( t0.y > t2.y ) { std::swap( t0, t2 ); std::swap( ity0, ity2 ); }
	if( t1.y > t2.y ) { std::swap( t1, t2 ); std::swap( ity1, ity2 ); }

	int total_height = t2.y - t0.y;
	for( int i = 0; i < total_height; i++ ) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)( i - ( second_half ? t1.y - t0.y : 0 ) ) / segment_height; // be careful: with above conditions no division by zero here
		Vec3i A = t0 + Vec3f( t2 - t0 ) * alpha;
		Vec3i B = second_half ? t1 + Vec3f( t2 - t1 ) * beta : t0 + Vec3f( t1 - t0 ) * beta;
		float ityA = ity0 + ( ity2 - ity0 ) * alpha;
		float ityB = second_half ? ity1 + ( ity2 - ity1 ) * beta : ity0 + ( ity1 - ity0 ) * beta;
		if( A.x > B.x ) { std::swap( A, B ); std::swap( ityA, ityB ); }
		for( int j = A.x; j <= B.x; j++ ) {
			float phi = B.x == A.x ? 1. : (float)( j - A.x ) / ( B.x - A.x );
			Vec3i    P = Vec3f( A ) + Vec3f( B - A ) * phi;
			float ityP = ityA + ( ityB - ityA ) * phi;
			int idx = P.x + P.y * width;
			if( P.x >= width || P.y >= height || P.x < 0 || P.y < 0 ) continue;
			if( zbuffer[ idx ] < P.z ) {
				zbuffer[ idx ] = P.z;
				int c = ityP * 255;
				m_ImageData[ P.y * width + P.x ] =
					c + ( c << 8 ) + ( c << 16 ) + ( c << 24 );
			}
		}
	}



}


void DrawTriangle( Vec2i t0, Vec2i t1, Vec2i t2, uint32_t* m_ImageData, uint32_t color, uint32_t width ){
	// 首先，填充三角形就相当于在边框内部逐行或逐列地画线
	// 我们从低到高，分两段来进行画线
	//if( t0.y == t1.y && t1.y == t2.y ) return;

	//// 先保证最高的点在t2，最低的点在t0
	//if( t0.y > t1.y ) std::swap( t0, t1 );
	//if( t0.y > t2.y ) std::swap( t0, t2 );
	//if( t1.y > t2.y ) std::swap( t1, t2 );
	//int totalHeight = t2.y - t0.y;
	//for( int i = 0; i < totalHeight; i ++ ){
	//	// 先区分第一段还是第二段
	//	bool isSecond = i > t1.y - t0.y || t0.y == t1.y;
	//	// 最长斜边上的点
	//	float alpha = (float)i / totalHeight;
	//	Vec2i A = t0 + ( t2 - t0 ) * alpha;

	//	if( isSecond ){
	//		float beta = (float)( i - t1.y + t0.y ) / ( t2.y - t1.y );
	//		Vec2i B = t1 + ( t2 - t1 ) * beta;
	//		if( A.x > B.x ) std::swap( A, B );
	//		for( int j = A.x; j <= B.x; j++ ) {
	//			m_ImageData[ i * width + j ] = color;
	//			//image.set( j, y, color ); // attention, due to int casts t0.y+i != A.y 
	//		}
	//		

	//		std::cout << B.x << " " << B.y << " " << A.x << " " << A.y << std::endl;
	//	}
	//	else{
	//		float beta = (float)i / ( t1.y - t0.y );
	//		Vec2i B = t0 + ( t1 - t0 ) * beta;
	//		if( A.x > B.x ) std::swap( A, B );
	//		for( int j = A.x; j <= B.x; j++ ) {
	//			m_ImageData[ i * width + j ] = color;
	//		}
	//		std::cout << B.x << " " << B.y << " " << A.x << " " << A.y << std::endl;
	//	}
	//}
	//三角形面积为0
	//if( t0.y == t1.y && t0.y == t2.y ) return;
	////根据y的大小对坐标进行排序
	//if( t0.y > t1.y ) std::swap( t0, t1 );
	//if( t0.y > t2.y ) std::swap( t0, t2 );
	//if( t1.y > t2.y ) std::swap( t1, t2 );
	//int total_height = t2.y - t0.y;
	////以高度差作为循环控制变量，此时不需要考虑斜率，因为着色完后每行都会被填充
	//for( int i = 0; i < total_height; i++ ) {
	//	//根据t1将三角形分割为上下两部分
	//	bool second_half = i > t1.y - t0.y || t1.y == t0.y;
	//	int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
	//	float alpha = (float)i / total_height;
	//	float beta = (float)( i - ( second_half ? t1.y - t0.y : 0 ) ) / segment_height;
	//	//计算A,B两点的坐标
	//	Vec2i A = t0 + ( t2 - t0 ) * alpha;
	//	Vec2i B = second_half ? t1 + ( t2 - t1 ) * beta : t0 + ( t1 - t0 ) * beta;
	//	if( A.x > B.x ) std::swap( A, B );
	//	//根据A,B和当前高度对tga着色
	//	for( int j = A.x; j <= B.x; j++ ) {
	//		m_ImageData[ (i + t0.y) * width + j ] = color;
	//	}
	//}

	//if( t0.y > t1.y ) std::swap( t0, t1 );
	//if( t0.y > t2.y ) std::swap( t0, t2 );
	//if( t1.y > t2.y ) std::swap( t1, t2 );
	//int total_height = t2.y - t0.y;
	//for( int y = t0.y; y <= t1.y; y++ ) {
	//	int segment_height = t1.y - t0.y + 1;
	//	float alpha = (float)( y - t0.y ) / total_height;
	//	float beta = (float)( y - t0.y ) / segment_height; // be careful with divisions by zero 
	//	Vec2i A = t0 + ( t2 - t0 ) * alpha;
	//	Vec2i B = t0 + ( t1 - t0 ) * beta;
	//	if( A.x > B.x ) std::swap( A, B );
	//	for( int j = A.x; j <= B.x; j++ ) {
	//		image.set( j, y, color ); // attention, due to int casts t0.y+i != A.y 
	//	}
	//}
	//for( int y = t1.y; y <= t2.y; y++ ) {
	//	int segment_height = t2.y - t1.y + 1;
	//	float alpha = (float)( y - t0.y ) / total_height;
	//	float beta = (float)( y - t1.y ) / segment_height; // be careful with divisions by zero 
	//	Vec2i A = t0 + ( t2 - t0 ) * alpha;
	//	Vec2i B = t1 + ( t2 - t1 ) * beta;
	//	if( A.x > B.x ) std::swap( A, B );
	//	for( int j = A.x; j <= B.x; j++ ) {
	//		image.set( j, y, color ); // attention, due to int casts t0.y+i != A.y 
	//	}
	//}

}
Vec3f world2screen( Vec3f v, uint32_t width, uint32_t height ) {
	return Vec3f( int( ( v.x + 1. ) * width / 2. + .5 ), int( ( v.y + 1. ) * height / 2. + .5 ), v.z );
}
class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender( ) override
	{
		ImGui::Begin( "Hello" );
		ImGui::Text( "Last render: %.3fms", m_LastRenderTime );
		if( ImGui::Button( "Render" ) ){
			Render( );
		}
		ImGui::End( );


		ImGui::Begin( "Viewport" );

		m_ViewportWidth = ImGui::GetContentRegionAvail( ).x;
		m_ViewportHeight = ImGui::GetContentRegionAvail( ).y;

		if( m_Image )
			ImGui::Image( m_Image->GetDescriptorSet( ), { (float)m_Image->GetWidth( ), (float)m_Image->GetHeight( ) } );

		ImGui::End( );

		//Render();
	}

	void Render( ){
		Timer timer;

		if( !m_Image || m_ViewportWidth != m_Image->GetWidth( ) || m_ViewportHeight != m_Image->GetHeight( ) )
		{
			m_Image = std::make_shared<Image>( m_ViewportWidth, m_ViewportHeight, ImageFormat::RGBA );
			delete[ ] m_ImageData;
			m_ImageData = new uint32_t[ ( m_ViewportWidth + 10 ) * ( m_ViewportHeight + 10 ) ];
		}

		// 1. init z-buffer
		int* zbuffer = new int[ ( m_ViewportWidth + 10 ) * ( m_ViewportHeight + 10 ) ];
		for( int i = m_ViewportWidth * m_ViewportHeight; i--; zbuffer[ i ] = -std::numeric_limits<float>::max( ) );

		// draw model
		Matrix ModelView = lookat( eye, center, Vec3f( 0, 1, 0 ) );
		Matrix Projection = Matrix::identity( 4 );
		Matrix ViewPort = viewport( m_ViewportWidth / 8, m_ViewportHeight / 8, m_ViewportWidth * 3 / 4, m_ViewportHeight * 3 / 4 );
		Projection[ 3 ][ 2 ] = -1.f / ( eye - center ).norm( );


		m_Model = new Model( "obj/african_head.obj" );
		Vec3f light_dir( 0, 0, -1 ); // define light_dir

		for( int i = 0; i < m_Model->nfaces( ); i++ ) {
			std::vector<int> face = m_Model->face( i );

			Vec3i screen_coords[ 3 ];
			Vec3f world_coords[ 3 ];
			float intensity[ 3 ];
			for( int j = 0; j < 3; j++ ) {
				Vec3f v = m_Model->vert( face[ j ] );
				screen_coords[ j ] = Vec3f( ViewPort * Projection * ModelView * Matrix( v ) );
				world_coords[ j ] = v;
				//std::cout << screen_coords[j] << std::endl;
				intensity[ j ] = m_Model->norm( i, j ) * light_dir;
			};

			DrawTriangle( screen_coords[ 0 ], screen_coords[ 1 ], screen_coords[ 2 ],
				intensity[ 0 ], intensity[ 1 ], intensity[ 2 ], m_ImageData, zbuffer,
				m_ViewportWidth, m_ViewportHeight );
		}


		m_Image->SetData( m_ImageData );

		m_LastRenderTime = timer.ElapsedMillis( );
	}


private:
	std::shared_ptr<Image> m_Image;

	uint32_t* m_ImageData = nullptr;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	float m_LastRenderTime = 0.0f;

	Model* m_Model;
};

Walnut::Application* Walnut::CreateApplication( int argc, char** argv )
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Walnut Example";

	Walnut::Application* app = new Walnut::Application( spec );
	app->PushLayer<ExampleLayer>( );
	app->SetMenubarCallback( [app]( )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Exit" ) )
				{
					app->Close( );
				}
				ImGui::EndMenu( );
			}
		} );
	return app;
}