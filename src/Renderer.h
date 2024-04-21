#include <Platform.h>
#include "NetworkSettings.h"

/* custom scancode for Fn keys */
#define KEY_F1 282
#define KEY_F2 283
#define KEY_F3 284
#define KEY_F4 285
#define KEY_F5 286
#define KEY_F6 287
#define KEY_F7 288
#define KEY_F8 289
#define KEY_F9 290
#define KEY_F10 291
#define KEY_F11 292
#define KEY_F12 293

typedef enum {
  RENDERER_WINDOWMODE_WINDOWED = 0,
  RENDERER_WINDOWMODE_FULLSCREEN,
  RENDERER_WINDOWMODE_BORDERLESS
} RENDERER_WINDOWMODE;

typedef struct 
{
  int nWidth;
  int nHeight;
  RENDERER_WINDOWMODE windowMode;
  bool ResizableWindow;
  bool bVsync;
} RENDERER_SETTINGS;

namespace Renderer
{
  extern const char * defaultShaderFilename;
  extern const char * defaultShaderExtention;
  extern const char defaultShader[65536];

  extern int nWidth;
  extern int nHeight;
  extern bool nSizeChanged;

  bool OpenSetupDialog( RENDERER_SETTINGS * settings, NETWORK_SETTINGS* netSettings );
  bool Open( RENDERER_SETTINGS * settings, std::string windowName );
  
  void StartFrame();
  void EndFrame();
  bool WantsToQuit();

  void RenderFullscreenQuad();

  bool ReloadShader( const char * szShaderCode, int nShaderCodeSize, char * szErrorBuffer, int nErrorBufferSize );
  void SetShaderConstant( const char * szConstName, float x );
  void SetShaderConstant( const char * szConstName, float x, float y );
  void SetShaderConstantInt( const char * szConstName, int x );

  void StartTextRendering();
  void SetTextRenderingViewport( Scintilla::PRectangle rect );
  void EndTextRendering();

  bool GrabFrame( void * pPixelBuffer ); // input buffer must be able to hold w * h * 4 bytes of 0xAABBGGRR data

  void Close();

  enum TEXTURETYPE
  {
    TEXTURETYPE_1D = 1,
    TEXTURETYPE_2D = 2,
  };

  struct Texture
  {
    int width;
    int height;
    TEXTURETYPE type;
  };

  Texture * CreateR32UIntTexture();
  Texture * CreateRGBA8Texture();
  Texture * CreateRGBA8TextureFromFile( const char * szFilename );
  Texture * CreateA8TextureFromData( int w, int h, const unsigned char * data );
  Texture * Create1DR32Texture( int w );
  bool UpdateR32Texture( Texture * tex, float * data );
  void SetShaderTexture( const char * szTextureName, Texture * tex );
  void BindTextureAsImage( unsigned int unit, Texture * tex );
  void BindTexture( Texture * tex ); // temporary function until all the quad rendering is moved to the renderer
  void ReleaseTexture( Texture * tex );
  void ClearTexture(Texture * tex);

  void CopyBackbufferToTexture(Texture * tex);

  struct Vertex
  {
    Vertex( float _x, float _y, unsigned int _c = 0xFFFFFFFF, float _u = 0.0, float _v = 0.0) : 
      x(_x), y(_y), c(_c), u(_u), v(_v) {}
    float x, y;
    unsigned int c;
    float u, v;
  };
  void RenderQuad( const Vertex & a, const Vertex & b, const Vertex & c, const Vertex & d );
  void RenderLine( const Vertex & a, const Vertex & b );

  struct KeyEvent
  {
    int character;
    int scanCode;
    bool ctrl;
    bool shift;
    bool alt;
  };
  extern KeyEvent keyEventBuffer[512];
  extern int keyEventBufferCount;

  enum MOUSEEVENTTYPE
  {
    MOUSEEVENTTYPE_DOWN = 0,
    MOUSEEVENTTYPE_MOVE,
    MOUSEEVENTTYPE_UP,
    MOUSEEVENTTYPE_SCROLL
  };
  enum MOUSEBUTTON
  {
    MOUSEBUTTON_LEFT = 0,
    MOUSEBUTTON_RIGHT,
    MOUSEBUTTON_MIDDLE,
  };
  struct MouseEvent
  {
    MOUSEEVENTTYPE eventType;
    float x;
    float y;
    MOUSEBUTTON button;
  };
  extern MouseEvent mouseEventBuffer[512];
  extern int mouseEventBufferCount;
}
