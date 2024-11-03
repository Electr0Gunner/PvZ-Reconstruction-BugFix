#ifndef __SDLINTERFACE_H__
#define __SDLINTERFACE_H__

#include <SDL.h>
#include <string>
#include <map>
#include "Rect.h"
#include "Color.h"
#include "SexyMatrix.h"
namespace Sexy
{
    class Image;
    class SexyAppBase;
    class TriVertex;

// A simple 2D Renderer class using SDL2
class SDLInterface {
public:
    SDLInterface(SexyAppBase* theApp);
    ~SDLInterface();

    bool Init();
    bool InitAudio();
    bool InitWindow();
    void SetRenderMode(bool useHardware);

    // Load texture from a file
    SDL_Texture* LoadTexture(Image* theImage, const Rect theClipRect, const Color theColor);

    void Render();

    // Poll for events
    void PollEvents();

    // Blit to screen texture
    void Blit(Image* theImage, float theX, float theY, float theScaleX, float theScaleY, const Rect theClipRect, const Color theColor, int aBlendMode);
    void Blit(Image* theImage, float theX, float theY, const Rect theSrcRect, double theRot, float theCenterRotX, float theCenterRotY, const Rect theClipRect, const Color theColor, int aBlendMode);
    void Blit(Image* theImage, const Rect theDestRect, const Rect theSrcRect, const Rect theClipRect, const Color theColor, int aBlendMode);
    void BlitMirror(Image* theImage, const Rect theDestRect, const Rect theSrcRect, const Rect theClipRect, const Color theColor, int aBlendMode);
    void Blit(Image* theImage, SexyMatrix3 theMatrix, const Rect theSrcRect, const Rect theClipRect, const Color theColor, int aBlendMode);
    void BlitTriangle(Image* theImage, const TriVertex theVertices[][3], int theNumTriangles, const Rect theSrcRect, const Rect theClipRect, const Color theColor, int aBlendMode);
    void DrawRect(int theX, int theY, int theWidth, int theHeight, const Color theColor, int aBlendMode);
    void DrawRectFilled(int theX, int theY, int theWidth, int theHeight, const Color theColor, int aBlendMode);

    //Update the Interface
    void Update();

    //Mouse Movement
    void MouseMovement();

    //Mouse Down
    void MouseDown(SDL_MouseButtonEvent& theButton, bool isPressed);

    //Play a sound
    void PlaySDLSound(const std::string& file);

    // Try to load a image using a list of extensions
    SDL_Surface* LoadSurfaceWithExtensions(const std::string& fileNameBase);

    // Try to load a alpha mask to a surface
    SDL_Surface* ApplyAlphaMask(SDL_Surface* baseSurface, SDL_Surface* alphaSurface);

    SDL_BlendMode ChooseBlendMode(int theBlendMode);

public:
    SexyAppBase* mApp;
    int mMouseX;
    int mMouseY;
    bool mIs3D;
    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    SDL_Texture* mScreenTexture;

private:
    bool isRunning;
    std::map<std::string, SDL_Texture*> textureCache;
};
}
#endif
