#include "cinder/app/AppBasic.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "Kinect.h"
#include "Resources.h"
#include "cinder/Capture.h"

static const int VBO_X_RES  = 1280;
static const int VBO_Y_RES  = 1024;

using namespace ci;
using namespace ci::app;
using namespace std;

class kinectTestApp : public AppBasic {
  public:
	void prepareSettings( Settings* settings );
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void createVbo();
	
	// PARAMS
	params::InterfaceGl	mParams;
	
	// CAMERA
	CameraPersp		mCam;
	Quatf			mSceneRotation;
	Vec3f			mEye, mCenter, mUp;
	float			mCameraDistance;
	float			mKinectTilt;
	
	// KINECT AND TEXTURES
	Kinect			mKinect;
	gl::Texture		mDepthTexture, mColorTexture, newColor;
	Surface8u depthSurf;
	float			mScale;
	float			mXOff, mYOff;
	float mLo, mHi;
	
	// VBO AND SHADER
	gl::VboMesh		mVboMesh;
	gl::GlslProg	mShader;
	
	Capture				mCapture;
	gl::Texture			mTexture;
};

void kinectTestApp::prepareSettings(Settings* settings)
{
	settings->setWindowSize( 1600, 1200 );
}

void kinectTestApp::setup()
{
	// SETUP PARAMS
	mParams = params::InterfaceGl( "KinectPointCloud", Vec2i( 200, 180 ) );
	mParams.addParam( "Scene Rotation", &mSceneRotation, "opened=1" );
	mParams.addParam( "Cam Distance", &mCameraDistance, "min=100.0 max=5000.0 step=100.0 keyIncr=s keyDecr=w" );
	mParams.addParam( "Kinect Tilt", &mKinectTilt, "min=-31 max=31 keyIncr=T keyDecr=t" );
	mParams.addParam( "lo thresh", &mLo, "min=.0 max=1.0 step=.05 keyIncr=L keyDecr=l" );
	mParams.addParam( "hi thresh", &mHi, "min=.0 max=1.0 step=.05 keyIncr=H keyDecr=h" );
	
	// SETUP CAMERA
	mCameraDistance = 1000.0f;
	mEye			= Vec3f( 0.0f, 0.0f, mCameraDistance );
	mCenter			= Vec3f::zero();
	mUp				= Vec3f::yAxis();
	mCam.setPerspective( 75.0f, getWindowAspectRatio(), 1.0f, 8000.0f );
	
	// SETUP KINECT AND TEXTURES
	console() << "There are " << Kinect::getNumDevices() << " Kinects connected." << std::endl;
	mKinect			= Kinect( Kinect::Device() ); // use the default Kinect
	mDepthTexture	= gl::Texture( 640, 480 );
	mColorTexture	= gl::Texture( 640, 480 );
	newColor = gl::Texture(640, 480);
	mLo = 0.3;
	mHi = 0.5;
	
	// SETUP VBO AND SHADER	
	createVbo();
	mShader	= gl::GlslProg( loadResource( RES_VERT_ID ), loadResource( RES_FRAG_ID ) );
	
	// SETUP GL
	gl::enableDepthWrite();
	gl::enableDepthRead();
	
	depthSurf = Surface8u(640, 480, true);
	
	mCapture = Capture( 640, 480 );
	mCapture.start();
}

void kinectTestApp::createVbo()
{
	gl::VboMesh::Layout layout;
	
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticIndices();
	
	std::vector<Vec3f> positions;
	std::vector<Vec2f> texCoords;
	std::vector<uint32_t> indices; 
	
	int numVertices = VBO_X_RES * VBO_Y_RES;
	int numShapes	= ( VBO_X_RES - 1 ) * ( VBO_Y_RES - 1 );
	
	mVboMesh		= gl::VboMesh( numVertices, numShapes, layout, GL_POINTS );
	
	for( int x=0; x<VBO_X_RES; ++x ){
		for( int y=0; y<VBO_Y_RES; ++y ){
			indices.push_back( x * VBO_Y_RES + y );
			
			float xPer	= x / (float)(VBO_X_RES-1);
			float yPer	= y / (float)(VBO_Y_RES-1);
			
			positions.push_back( Vec3f( ( xPer * 2.0f - 1.0f ) * VBO_X_RES, ( yPer * 2.0f - 1.0f ) * VBO_Y_RES, 0.0f ) );
			texCoords.push_back( Vec2f( xPer, yPer ) );			
		}
	}
	
	mVboMesh.bufferPositions( positions );
	mVboMesh.bufferIndices( indices );
	mVboMesh.bufferTexCoords2d( 0, texCoords );
}

void kinectTestApp::mouseDown( MouseEvent event )
{
}

void kinectTestApp::update()
{
	
	bool newDepth = mKinect.checkNewDepthFrame();
	if( newDepth )
	{
		mDepthTexture = mKinect.getDepthImage();
		depthSurf = Surface8u(mKinect.getDepthImage());
	}

	if( mKinect.checkNewColorFrame() )
		mColorTexture = mKinect.getColorImage();
	
	
	/*
	
	//Surface depthSurf = Surface(ImageSourceRef(mDepthTexture));
	//Surface8u depthSurf( mKinect.getDepthImage() );
	Surface colorSurf = Surface(ImageSourceRef(mColorTexture));
	Surface::Iter iter = depthSurf.getIter( Area( 0, 0, depthSurf.getWidth(), depthSurf.getHeight() ) );
	int r = 0, c = 0;
	
	//newColor = gl::Texture(800, 600);
	
	Surface newColorSurf = Surface(640, 480, true);
	Surface::Iter iter2 = newColorSurf.getIter( Area( 0, 0, newColorSurf.getWidth(), newColorSurf.getHeight() ) );
	
	while( iter.line() && iter2.line() ) 
	{
		while( iter.pixel() && iter2.pixel() ) 
		{
			float depth = iter.b();
			
			if(depth != 0)
			{
				int x = 0;
			}
			
			Vec3f v(float(r), float(c), depth * 1000);
			
			Vec3f vec(	(v.x - 3.3930780975300314e+02) * depth / 5.9421434211923247e+02,
					  (v.y - 2.4273913761751615e+02) * depth / 5.9104053696870778e+02,
					  depth);
			
			
			Matrix44<float> m(9.9984628826577793e-01, 1.2635359098409581e-03, -1.7487233004436643e-02, 0,
					   -1.4779096108364480e-03, 9.9992385683542895e-01, -1.2251380107679535e-02, 0,
					   1.7470421412464927e-02, 1.2275341476520762e-02, 9.9977202419716948e-01, 0,
					   1.9985242312092553e-02, -7.4423738761617583e-04, -1.0916736334336222e-02, 1);
			
			float fx_rgb = 5.2921508098293293e+02;
			float fy_rgb = 5.2556393630057437e+02;
			float cx_rgb = 3.2894272028759258e+02;
			float cy_rgb = 2.6748068171871557e+02;
			
			Vec3f vec_new = m * vec;
			
			Vec2f projected(.0f, .0f);
			projected.x = vec_new.x * fx_rgb / vec_new.z + cx_rgb;
			projected.y = vec_new.y * fy_rgb / vec_new.z + cy_rgb;
			
			Vec2i projected_i = Vec2i((int) projected.x, (int) projected.y);
			
			ColorA color = colorSurf.getPixel(projected_i);
			
			iter2.r() = color.r * 100;
			iter2.g() = color.g * 100;
			iter2.b() = color.b * 100;
			
			//newColorSurf.setPixel(Vec2i(r, c), ColorA(1.0f, depth, depth, 1.0f));//color);
			
			c++;
		}
		
		r++;
	}
	*/

	//newColor = gl::Texture(colorSurf);
	
	if( mKinectTilt != mKinect.getTilt() )
		mKinect.setTilt( mKinectTilt );
	
	
	mEye = Vec3f( 0.0f, 0.0f, mCameraDistance );
	mCam.lookAt( mEye, mCenter, mUp );
	gl::setMatrices( mCam );
	 
}

void kinectTestApp::draw()
{
	gl::clear( Color( 0.0f, 0.0f, 0.0f ), true );
	
	
	gl::pushMatrices();
	gl::scale( Vec3f( -1.0f, -1.0f, 1.0f ) );
	gl::rotate( mSceneRotation );
	mDepthTexture.bind( 0 );
	mColorTexture.bind( 1 );
	mShader.bind();
	mShader.uniform( "depthTex", 0 );
	mShader.uniform( "colorTex", 1 );
	mShader.uniform( "lo", mLo);
	mShader.uniform( "hi", mHi);
	gl::draw( mVboMesh );
	mShader.unbind();
	gl::popMatrices();
	 
	
	/*
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	if( mDepthTexture )
		gl::draw( mDepthTexture );
	 */
	
	params::InterfaceGl::draw();
}


CINDER_APP_BASIC( kinectTestApp, RendererGl )
