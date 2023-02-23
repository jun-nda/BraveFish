#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include "model.h"
#include "tgaimage.h"
#include <iostream>

using namespace Walnut;

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

Vec3f barycentric( Vec3f A, Vec3f B, Vec3f C, Vec3f P ) {
	Vec3f s[ 2 ];
	for( int i = 2; i--; ) {
		s[ i ][ 0 ] = C[ i ] - A[ i ];
		s[ i ][ 1 ] = B[ i ] - A[ i ];
		s[ i ][ 2 ] = A[ i ] - P[ i ];
	}
	Vec3f u = cross( s[ 0 ], s[ 1 ] );
	if( std::abs( u[ 2 ] ) > 1e-2 ) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return Vec3f( 1.f - ( u.x + u.y ) / u.z, u.y / u.z, u.x / u.z );
	return Vec3f( -1, 1, 1 ); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}


void DrawTriangle( Vec3f* pts, uint32_t* m_ImageData, uint32_t color, uint32_t width, uint32_t height, float* zbuffer){
	// 重心坐标 或  向量叉乘版本
	// 无论哪个版本  都要先求三角形的最小包围盒
	Vec2f bboxmin( std::numeric_limits<float>::max( ), std::numeric_limits<float>::max( ) );
	Vec2f bboxmax( -std::numeric_limits<float>::max( ), -std::numeric_limits<float>::max( ) );
	Vec2f clamp( width, height);
	for( int i = 0; i < 3; i++ ) {
		for( int j = 0; j < 2; j++ ) {
			bboxmin[ j ] = std::max( 0.f, std::min( bboxmin[ j ], pts[ i ][ j ] ) );
			bboxmax[ j ] = std::min( clamp[ j ], std::max( bboxmax[ j ], pts[ i ][ j ] ) );
		}
	}

	Vec3f P;
	for( P.x = bboxmin.x; P.x <= bboxmax.x; P.x++ ) {
		for( P.y = bboxmin.y; P.y <= bboxmax.y; P.y++ ) {
			Vec3f bc_screen = barycentric( pts[ 0 ], pts[ 1 ], pts[ 2 ], P );
			if( bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 ) continue;
			P.z = 0;
			for( int i = 0; i < 3; i++ ) P.z += pts[ i ][ 2 ] * bc_screen[ i ];
			if( zbuffer[ int( P.x + P.y * width ) ] < P.z ) {
				zbuffer[ int( P.x + P.y * width ) ] = P.z;
				m_ImageData[(int) (P.x + P.y * width) ] = color;
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

Vec3f world2screen( Vec3f v , uint32_t width, uint32_t height) {
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

		//for( int i = 0; i < 1000; ++i )
		//{
		//	DrawLine( 13, 20, 80, 40, m_ImageData, 0xff000000, m_ViewportWidth );
		//	DrawLine( 20, 13, 40, 80, m_ImageData, 0xf1202200, m_ViewportWidth );
		//	DrawLine( 80, 40, 13, 20, m_ImageData, 0xf0333000, m_ViewportWidth );
		//}
		//Vec2i A( 450, 450 );
		//Vec2i B( 400, 400 );
		//Vec2i C( 500, 400 );

		//DrawTriangle( A, B, C, m_ImageData, 0xf0333000, m_ViewportWidth );

		//Vec2i pts[ 3 ] = { Vec2i( 10,10 ), Vec2i( 100, 30 ), Vec2i( 190, 160 ) };
		//DrawTriangle( pts, m_ImageData, 0xf0333000, m_ViewportWidth, m_ViewportHeight);

		//m_Model = new Model("obj/african_head.obj");
		//for( int i = 0; i < m_Model->nfaces( ); i++ ) {
		//	std::vector<int> face = m_Model->face( i );
		//	for( int j = 0; j < 3; j++ ) {
		//		Vec3f v0 = m_Model->vert( face[ j ] );
		//		Vec3f v1 = m_Model->vert( face[ ( j + 1 ) % 3 ] );
		//		int x0 = ( v0.x + 1. ) * m_ViewportWidth / 2.;
		//		int y0 = ( v0.y + 1. ) * m_ViewportHeight / 2.;
		//		int x1 = ( v1.x + 1. ) * m_ViewportWidth / 2.;
		//		int y1 = ( v1.y + 1. ) * m_ViewportHeight / 2.;
		//		DrawLine( x0, y0, x1, y1, m_ImageData, 0xf0333800, m_ViewportWidth);
		//	}
		//}

		// z-buffer
		float* zbuffer = new float[ ( m_ViewportWidth + 10 ) * ( m_ViewportHeight + 10 ) ];
		for( int i = m_ViewportWidth * m_ViewportHeight; i--; zbuffer[ i ] = -std::numeric_limits<float>::max( ) );

		m_Model = new Model( "obj/african_head.obj" );
		Vec3f light_dir( 0, 0, -1 ); // define light_dir

		for( int i = 0; i < m_Model->nfaces( ); i++ ) {
			std::vector<int> face = m_Model->face( i );

			Vec3f pts[ 3 ];
			for( int i = 0; i < 3; i++ ) pts[ i ] = world2screen( m_Model->vert( face[ i ] ), m_ViewportWidth, m_ViewportHeight );
			
			
			Vec3f screen_coords[ 3 ];
			Vec3f world_coords[ 3 ];
			for( int j = 0; j < 3; j++ ) {
				Vec3f v = m_Model->vert( face[ j ] );
				screen_coords[ j ] = Vec3f( ( v.x + 1. ) * m_ViewportWidth / 2., ( v.y + 1. ) * m_ViewportHeight / 2., v.z );
				world_coords[ j ] = v;
			};
			Vec3f n = cross( ( world_coords[ 2 ] - world_coords[ 0 ] ), ( world_coords[ 1 ] - world_coords[ 0 ] ) );
			n.normalize( );
			float intensity = n * light_dir;
			int c = intensity * 255;
			
			DrawTriangle( pts,
				m_ImageData,
				c + ( c << 8 ) + ( c << 16 ) + ( c << 24 ),
				m_ViewportWidth,
				m_ViewportHeight,
				zbuffer
			);

		
			//std::cout << intensity << std::endl;
			//if( intensity > 0 ){
			//	DrawTriangle( screen_coords,
			//		m_ImageData,
			//		c + ( c << 8 ) + ( c << 16 ) + ( c << 24 ),
			//		m_ViewportWidth,
			//		m_ViewportHeight,
			//		zbuffer
			//	);
			//}

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