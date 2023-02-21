#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include "model.h"
#include "tgaimage.h"
#include <iostream>

using namespace Walnut;

void DrawLine( int x0, int y0, int x1, int y1, uint32_t* m_ImageData, uint32_t color, uint32_t width){

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

void DrawTriangle(Vec2i t0, Vec2i t1, Vec2i t2, uint32_t* m_ImageData, uint32_t color, uint32_t width ){
	// 首先，填充三角形就相当于在边框内部逐行或逐列地画线
	// 我们从低到高，分两段来进行画线

	// 先保证最高的点在t2，最低的点在t0
	if( t0.y > t2.y ) std::swap( t0, t2 );
	if( t0.y > t1.y ) std::swap( t0, t1 );
	if( t1.y > t2.y ) std::swap( t1, t2 );




}


class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Hello");
		ImGui::Text( "Last render: %.3fms", m_LastRenderTime );
		if( ImGui::Button( "Render" ) ){
			Render( );
		}
		ImGui::End();

		
		ImGui::Begin( "Viewport" );

		m_ViewportWidth = ImGui::GetContentRegionAvail( ).x;
		m_ViewportHeight = ImGui::GetContentRegionAvail( ).y;

		if( m_Image )
			ImGui::Image( m_Image->GetDescriptorSet( ), { (float)m_Image->GetWidth( ), (float)m_Image->GetHeight( ) } );
		
		ImGui::End();

		//Render();
	}

	void Render( ){
		Timer timer;

		if( !m_Image || m_ViewportWidth != m_Image->GetWidth( ) || m_ViewportHeight != m_Image->GetHeight( ) )
		{
			m_Image = std::make_shared<Image>( m_ViewportWidth, m_ViewportHeight, ImageFormat::RGBA );
			delete[ ] m_ImageData;
			m_ImageData = new uint32_t[ m_ViewportWidth * (m_ViewportHeight + 10) ];
		}

		//for( int i = 0; i < 1000; ++i )
		//{
		//	DrawLine( 13, 20, 80, 40, m_ImageData, 0xff000000, m_ViewportWidth );
		//	DrawLine( 20, 13, 40, 80, m_ImageData, 0xf1202200, m_ViewportWidth );
		//	DrawLine( 80, 40, 13, 20, m_ImageData, 0xf0333000, m_ViewportWidth );
		//}
		TGAImage tgaImage(m_ViewportWidth, m_ViewportHeight, TGAImage::RGB);
		m_Model = new Model("obj/african_head.obj");
		for( int i = 0; i < m_Model->nfaces( ); i++ ) {
			std::vector<int> face = m_Model->face( i );
			for( int j = 0; j < 3; j++ ) {
				Vec3f v0 = m_Model->vert( face[ j ] );
				Vec3f v1 = m_Model->vert( face[ ( j + 1 ) % 3 ] );
				int x0 = ( v0.x + 1. ) * m_ViewportWidth / 2.;
				int y0 = ( v0.y + 1. ) * m_ViewportHeight / 2.;
				int x1 = ( v1.x + 1. ) * m_ViewportWidth / 2.;
				int y1 = ( v1.y + 1. ) * m_ViewportHeight / 2.;
				DrawLine( x0, y0, x1, y1, m_ImageData, 0xf0333800, m_ViewportWidth);
			}
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

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Walnut Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}