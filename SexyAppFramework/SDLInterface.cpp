#include "SDLInterface.h"
#include "Image.h"
#include "SexyAppBase.h"
#include "TriVertex.h"
#include "WidgetManager.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include <iostream>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <typeinfo>

using namespace Sexy;

SDLInterface::SDLInterface(SexyAppBase* theApp)
    : mWindow(nullptr), mRenderer(nullptr), isRunning(true)
{
    mApp = theApp;
}

SDLInterface::~SDLInterface() {
    for (auto& pair : textureCache) {
        SDL_DestroyTexture(pair.second);
    }
    SDL_DestroyTexture(mScreenTexture);
    SDL_DestroyRenderer(mRenderer);
    SDL_DestroyWindow(mWindow);
    SDL_Quit();
    Mix_Quit();
    IMG_Quit();
}

bool SDLInterface::Init() {

    if (!InitWindow() || !InitAudio())
        return false;

    return true;
}


bool SDLInterface::InitAudio() {

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::string aError = Mix_GetError();
        mApp->Popup("SDL_mixer could not initialize! SDL_mixer Error: " + aError);
        return false;
    }

    return true;
}

bool SDLInterface::InitWindow() {

    if (mWindow)
    {
        SDL_DestroyWindow(mWindow);
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::string aError = SDL_GetError();
        mApp->Popup("SDL_Init Error: " + aError);
        return false;
    }
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        std::string aError = SDL_GetError();
        mApp->Popup("SDL_Init Error: " + aError);
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,(mApp->Is3DAccelerated() ?  "1" : "0"));
    mWindow = SDL_CreateWindow(mApp->mTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        mApp->mWidth, mApp->mHeight, SDL_WINDOW_SHOWN);
    if (mWindow == nullptr) {
        std::string aError = SDL_GetError();
        mApp->Popup("SDL_CreateWindow Error: " + aError);
        return false;
    }
    SDL_SetWindowFullscreen(mWindow, !mApp->mIsWindowed ? SDL_WINDOW_FULLSCREEN : 0);

    mRenderer = SDL_CreateRenderer(mWindow, -1, (mApp->Is3DAccelerationSupported() ? SDL_RENDERER_ACCELERATED : SDL_RENDERER_SOFTWARE) | SDL_RENDERER_PRESENTVSYNC);
    if (mRenderer == nullptr) {
        std::string aError = SDL_GetError();
        mApp->Popup("SDL_CreateRenderer Error: " + aError);
        return false;
    }

    mScreenTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mApp->mWidth, mApp->mHeight);
    SDL_SetTextureBlendMode(mScreenTexture, SDL_BLENDMODE_BLEND);

    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG))
    {
        std::string aError = IMG_GetError();
        mApp->Popup("Could not init IMG " + aError);
        return false;
    }

    return true;
}

void SDLInterface::SetRenderMode(bool useHardware)
{
    if (mRenderer != nullptr) {
        SDL_DestroyRenderer(mRenderer);
    }

    int rendererFlags = useHardware ? SDL_RENDERER_ACCELERATED : SDL_RENDERER_SOFTWARE;
    mRenderer = SDL_CreateRenderer(mWindow, -1, rendererFlags | SDL_RENDERER_PRESENTVSYNC);
    mScreenTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, mApp->mWidth, mApp->mHeight);
    SDL_SetTextureBlendMode(mScreenTexture, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, (mApp->Is3DAccelerated() ? "1" : "0"));
    textureCache.clear();
    mIs3D = useHardware;
}

bool shouldDoErrors = false;

SDL_Texture* SDLInterface::LoadTexture(Image* theImage, const Rect theClipRect, const Color theColor) {
    if (textureCache.find(theImage) != textureCache.end()) {
        auto& aTexture = textureCache[theImage];
        SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
        SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());
        return aTexture;
    }
    SDL_Surface* surface = LoadSurfaceWithExtensions(theImage->mFilePath);
    if (!surface) {
        MemoryImage* theMemoryImage = dynamic_cast<MemoryImage*>(theImage);
        SDL_Texture* aTexture = theMemoryImage->ConvertToSDLTexture();
        if (aTexture)
        {
            textureCache[theImage] = aTexture;
            SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
            SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());
        }
        return aTexture;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(mRenderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        std::string aError = SDL_GetError();
        mApp->Popup("SDL_CreateTexture Error: " + aError);
        return nullptr;
    }
    textureCache[theImage] = texture;
    SDL_SetTextureColorMod(texture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
    SDL_SetTextureAlphaMod(texture, theColor.GetAlpha());
    return texture;
}

void SDLInterface::Render() {
    SDL_Rect aRect;
    aRect.x = 0;
    aRect.y = 0;
    aRect.w = mApp->mWidth;
    aRect.h= mApp->mHeight;

    SDL_RenderClear(mRenderer);
    SDL_RenderCopy(mRenderer, mScreenTexture, NULL, &aRect);
    SDL_RenderPresent(mRenderer);
}

void SDLInterface::Blit(Image* theImage, float theX, float theY, float theScaleX, float theScaleY, const Rect theClipRect, const Color theColor, int aBlendMode) {
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Texture* aTexture = LoadTexture(theImage, theClipRect, theColor);

    SDL_Rect destRect = { theX, theY, theImage->mWidth * theScaleX, theImage->mHeight * theScaleY };

    SDL_Rect clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };

    SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(aBlendMode));
    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderCopyEx(mRenderer, aTexture, NULL, &destRect, 0, NULL, SDL_FLIP_NONE);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::Blit(Image* theImage, float theX, float theY, const Rect theSrcRect, double theRot, float theCenterRotX, float theCenterRotY, const Rect theClipRect, const Color theColor, int aBlendMode)
{
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Texture* aTexture = LoadTexture(theImage, theClipRect, theColor);

    SDL_Rect destRect = { theX - theCenterRotX, theY - theCenterRotY, theImage->mWidth, theImage->mHeight};
    SDL_Rect srcRect = { theSrcRect.mX, theSrcRect.mY, theSrcRect.mWidth, theSrcRect.mHeight};

    SDL_Rect clipRect;
    if (theClipRect.mWidth > 0 || theClipRect.mHeight > 0)
        clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };
    else
        clipRect = { theClipRect.mX, theClipRect.mY , mApp->mWidth, mApp->mHeight };

    SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(aBlendMode));
    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderCopyEx(mRenderer, aTexture, &srcRect, &destRect, theRot, NULL, SDL_FLIP_NONE);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::Blit(Image* theImage, const Rect theDestRect, const Rect theSrcRect, const Rect theClipRect, const Color theColor, int aBlendMode)
{
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Texture* aTexture = LoadTexture(theImage, theClipRect, theColor);

    SDL_Rect destRect = { theDestRect.mX, theDestRect.mY, theDestRect.mWidth, theDestRect.mHeight};
    SDL_Rect srcRect = { theSrcRect.mX, theSrcRect.mY, theSrcRect.mWidth, theSrcRect.mHeight};

    SDL_Rect clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };

    SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(aBlendMode));
    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderCopyEx(mRenderer, aTexture, &srcRect, &destRect, 0, NULL, SDL_FLIP_NONE);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::BlitMirror(Image* theImage, const Rect theDestRect, const Rect theSrcRect, const Rect theClipRect, const Color theColor, int aBlendMode)
{
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Texture* aTexture = LoadTexture(theImage, theClipRect, theColor);

    SDL_Rect destRect = { theDestRect.mX, theDestRect.mY, theDestRect.mWidth, theDestRect.mHeight};
    SDL_Rect srcRect = { theSrcRect.mX, theSrcRect.mY, theSrcRect.mWidth, theSrcRect.mHeight};

    SDL_Rect clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };

    SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(aBlendMode));
    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderCopyEx(mRenderer, aTexture, &srcRect, &destRect, 0, NULL, SDL_FLIP_HORIZONTAL);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::Blit(Image* theImage, int theX, int theY, SexyMatrix3 theMatrix, const Rect theSrcRect,const Rect theClipRect, const Color theColor, int aBlendMode)
{
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Texture* aTexture = LoadTexture(theImage, theClipRect, theColor);
    SDL_Rect clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };

    SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(aBlendMode));

    // From here down its just pure magic that handles all the Reanimation Transformations.
    SDL_Vertex vertices[4];
    float width = theImage->GetWidth();
    float height = theImage->GetHeight();

    SDL_Color aColor = { theColor.GetRed() , theColor.GetGreen(), theColor.GetBlue(), theColor.GetAlpha() };

    // Top-left vertex
    vertices[0].position.x = theMatrix.m00 * -width / 2 + theMatrix.m01 * -height / 2 + theMatrix.m02 + theSrcRect.mX;
    vertices[0].position.y = theMatrix.m10 * -width / 2 + theMatrix.m11 * -height / 2 + theMatrix.m12 + theSrcRect.mY;
    vertices[0].tex_coord = { 0.0f, 0.0f };
    vertices[0].color = aColor;

    // Top-right vertex
    vertices[1].position.x = theMatrix.m00 * width / 2 + theMatrix.m01 * -height / 2 + theMatrix.m02 + theSrcRect.mX;
    vertices[1].position.y = theMatrix.m10 * width / 2 + theMatrix.m11 * -height / 2 + theMatrix.m12 + theSrcRect.mY;
    vertices[1].tex_coord = { 1.0f, 0.0f };
    vertices[1].color = aColor;

    // Bottom-left vertex
    vertices[2].position.x = theMatrix.m00 * -width / 2 + theMatrix.m01 * height / 2 + theMatrix.m02 + theSrcRect.mX;
    vertices[2].position.y = theMatrix.m10 * -width / 2 + theMatrix.m11 * height / 2 + theMatrix.m12 + theSrcRect.mY;
    vertices[2].tex_coord = { 0.0f, 1.0f };
    vertices[2].color = aColor;

    // Bottom-right vertex
    vertices[3].position.x = theMatrix.m00 * width / 2 + theMatrix.m01 * height / 2 + theMatrix.m02 + theSrcRect.mX;
    vertices[3].position.y = theMatrix.m10 * width / 2 + theMatrix.m11 * height / 2 + theMatrix.m12 + theSrcRect.mY;
    vertices[3].tex_coord = { 1.0f, 1.0f };
    vertices[3].color = aColor;

    // Indices for two triangles
    int indices[] = { 0, 1, 2, 1, 2, 3 };

    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderGeometry(mRenderer, aTexture, vertices, 4, indices, 6);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::BlitTriangle(Image* theImage, const TriVertex theVertices[][3], int theNumTriangles, const Rect theSrcRect, const Rect theClipRect, const Color theColor, int aBlendMode)
{//!!! EARLY RETURN: This code is very broken,do not use yet.
    return;
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Texture* aTexture = NULL;
    if (theImage != nullptr)
    {
        MemoryImage* theMemoryImage = dynamic_cast<MemoryImage*>(theImage);
        aTexture = theMemoryImage->ConvertToSDLTexture();

        SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(aBlendMode));

    }

    SDL_Vertex vertices[4];

    // Top-left vertex
    vertices[0].position.x = theVertices[0]->x;
    vertices[0].position.y = theVertices[0]->y;
    vertices[0].tex_coord = { 0.0f, 0.0f };
    vertices[0].color = { 255, 255, 255, 255 };

    vertices[1].position.x = theVertices[1]->x;
    vertices[1].position.y = theVertices[1]->y;
    vertices[1].tex_coord = { 0.0f, 0.0f };
    vertices[1].color = { 255, 255, 255, 255 };

    vertices[2].position.x = theVertices[2]->x;
    vertices[2].position.y = theVertices[2]->y;
    vertices[2].tex_coord = { 0.0f, 0.0f };
    vertices[2].color = { 255, 255, 255, 255 };

    vertices[3].position.x = theVertices[3]->x;
    vertices[3].position.y = theVertices[3]->y;
    vertices[3].tex_coord = { 0.0f, 0.0f };
    vertices[3].color = { 255, 255, 255, 255 };

    int indices[] = { 0, 1, 2, 1, 2, 3 };
    // Render the triangles
    SDL_RenderGeometry(mRenderer, aTexture, vertices, 4, indices, 6);
    SDL_SetRenderTarget(mRenderer, nullptr);
}


void SDLInterface::BlitMemoryImage(MemoryImage* theImage, float theX, float theY, float theScaleX, float theScaleY, const Rect theClipRect, const Color theColor, int aBlendMode) {
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Texture* aTexture;

    if (theImage->mSDL_Texture != nullptr)
        aTexture = theImage->mSDL_Texture;
    else
    {
        aTexture = theImage->ConvertToSDLTexture();
        textureMemoryImageCache[theImage] = aTexture;
    }
    SDL_Rect destRect = { theX, theY, theImage->mWidth * theScaleX, theImage->mHeight * theScaleY };

    SDL_Rect clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };

    SDL_SetTextureColorMod(aTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
    SDL_SetTextureAlphaMod(aTexture, theColor.GetAlpha());
    SDL_SetTextureBlendMode(aTexture, ChooseBlendMode(aBlendMode));
    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderCopyEx(mRenderer, aTexture, NULL, &destRect, 0, NULL, SDL_FLIP_NONE);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::BlitMemoryImageToSDLTexture(MemoryImage* theImage, SDL_Texture* theTexture, float theX, float theY, float theScaleX, float theScaleY, const Rect theClipRect, const Color theColor, int aBlendMode) {

    SDL_Texture* aNewTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, theImage->mWidth, theImage->mHeight);
    SDL_SetTextureBlendMode(aNewTexture, SDL_BLENDMODE_BLEND);

    theTexture = aNewTexture;

    SDL_SetRenderTarget(mRenderer, theTexture);

    SDL_Rect destRect = { theX, theY, theImage->mWidth * theScaleX, theImage->mHeight * theScaleY };

    SDL_Rect clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };

    SDL_SetTextureColorMod(aNewTexture, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue());
    SDL_SetTextureAlphaMod(aNewTexture, theColor.GetAlpha());
    SDL_SetTextureBlendMode(aNewTexture, ChooseBlendMode(aBlendMode));
    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderCopyEx(mRenderer, aNewTexture, NULL, &destRect, 0, NULL, SDL_FLIP_NONE);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}


void SDLInterface::BlitSDLTexture(SDL_Texture* theTexture, float theX, float theY, float theScaleX, float theScaleY, const Rect theClipRect, const Color theColor, int aBlendMode) {
    SDL_SetRenderTarget(mRenderer, mScreenTexture);

    SDL_Point size;
    SDL_QueryTexture(theTexture, NULL, NULL, &size.x, &size.y);

    SDL_Rect destRect = { theX, theY, size.x * theScaleX, size.y * theScaleY };

    SDL_Rect clipRect = { theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight };

    SDL_SetTextureBlendMode(theTexture, ChooseBlendMode(aBlendMode));
    SDL_RenderSetClipRect(mRenderer, &clipRect);
    SDL_RenderCopyEx(mRenderer, theTexture, NULL, &destRect, 0, NULL, SDL_FLIP_NONE);
    SDL_RenderSetClipRect(mRenderer, NULL);
    SDL_SetRenderTarget(mRenderer, nullptr);
}


void SDLInterface::DrawRect(int theX, int theY, int theWidth, int theHeight, const Color theColor, int aBlendMode)
{
    SDL_SetRenderTarget(mRenderer, mScreenTexture);
    SDL_SetRenderDrawBlendMode(mRenderer, ChooseBlendMode(aBlendMode));
    SDL_SetRenderDrawColor(mRenderer, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue(), theColor.GetAlpha());
    SDL_Rect aRect = { theX, theY, theWidth, theHeight };
    SDL_RenderDrawRect(mRenderer, &aRect);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::DrawRectFilled(int theX, int theY, int theWidth, int theHeight, const Color theColor, int aBlendMode)
{
    SDL_SetRenderTarget(mRenderer, mScreenTexture);
    SDL_SetRenderDrawBlendMode(mRenderer, ChooseBlendMode(aBlendMode));
    SDL_SetRenderDrawColor(mRenderer, theColor.GetRed(), theColor.GetGreen(), theColor.GetBlue(), theColor.GetAlpha());
    SDL_Rect aRect = { theX, theY, theWidth, theHeight };
    SDL_RenderFillRect(mRenderer, &aRect);
    SDL_SetRenderTarget(mRenderer, nullptr);
}

void SDLInterface::Update() {
    PollEvents();
}

void SDLInterface::MouseMovement()
{
    mApp->mWidgetManager->MouseMove(mMouseX, mMouseY);
}

void SDLInterface::MouseDown(SDL_MouseButtonEvent& theButton, bool isPressed)
{
    mApp->mWidgetManager->MouseDown(mMouseX, mMouseY, 0);
}

void SDLInterface::PlaySDLSound(const std::string& file, float aPitch)
{
    return;
    Mix_Chunk* sound = Mix_LoadWAV(file.c_str());

    int aOriginalFrequency;
    Uint16 aFormat;
    int channels;
    Mix_QuerySpec(&aOriginalFrequency, &aFormat, &channels);

    int adjustedFrequency = static_cast<int>(aOriginalFrequency * aPitch);
    Mix_CloseAudio();
    if (Mix_OpenAudio(adjustedFrequency, aFormat, channels, 4096) == -1) {
        SDL_Log("Could not open audio: %s", Mix_GetError());
        return;
    }

    Mix_PlayChannel(-1, sound, 0);
}

void SDLInterface::PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        SDL_GetMouseState(&mMouseX, &mMouseY);
        float logicalMouseX, logicalMouseY;
        SDL_RenderWindowToLogical(mRenderer, mMouseX, mMouseY, &logicalMouseX, &logicalMouseY);
        switch (event.type) {
        case SDL_QUIT:
            isRunning = false;
            exit(0);
            break;
        case SDL_MOUSEBUTTONDOWN:
            {

            mApp->mWidgetManager->MouseDown(mMouseX, mMouseY, event.button.clicks);
        }
            break;
        case SDL_MOUSEBUTTONUP:
            {

            mApp->mWidgetManager->MouseUp(mMouseX, mMouseY, event.button.clicks);
        }
            break;

        case SDL_TEXTINPUT:
            mApp->mLastUserInputTick = mApp->mLastTimerTime;
            mApp->mWidgetManager->KeyChar((SexyChar)event.text.text[0]);
            break;
        case SDL_MOUSEMOTION:
            MouseMovement();
            break;
        case SDL_KEYDOWN:
            mApp->mWidgetManager->KeyDown((KeyCode)event.key.keysym.sym);
            break;
        case SDL_WINDOWEVENT:

            switch (event.window.event) {

            case SDL_WINDOWEVENT_RESIZED:
                mApp->mWidgetManager->Resize(Rect(0, 0, mApp->mWidth, mApp->mHeight), Rect(0, 0, mApp->mWidth, mApp->mHeight));
                break;
            case SDL_WINDOWEVENT_CLOSE:
                exit(0);
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
            case SDL_WINDOWEVENT_FOCUS_LOST:
                mApp->mActive = event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED;
                mApp->RehupFocus();
                break;
            }
            break;
        }
    }
}

const std::vector<std::string> extensions = { ".png", ".jpg", ".jpeg", ".gif" };

SDL_Surface* SDLInterface::LoadSurfaceWithExtensions(const std::string& fileNameBase) {
    // Try to load the base image with any of the extensions
    for (const std::string& ext : extensions) {
        std::string fullPath = fileNameBase + ext;
        SDL_Surface* surface = IMG_Load(fullPath.c_str());
        if (surface) {
            SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
            SDL_SetSurfaceBlendMode(convertedSurface, SDL_BLENDMODE_BLEND);
            SDL_FreeSurface(surface);

            for (const std::string& ext : extensions) {
                std::string fullPath_alpha = fileNameBase + "_" + ext;
                SDL_Surface* surface_alpha = IMG_Load(fullPath_alpha.c_str());
                if (surface_alpha) {
                    SDL_Surface* convertedAlphaSurface = SDL_ConvertSurfaceFormat(surface_alpha, SDL_PIXELFORMAT_RGBA8888, 0);
                    ApplyAlphaMask(convertedSurface, convertedAlphaSurface);
                    SDL_FreeSurface(surface_alpha);
                    SDL_FreeSurface(convertedAlphaSurface);
                }
            }

            return convertedSurface;
        }
    }

    // Return nullptr if no surface could be loaded
    return nullptr;
}

SDL_Surface* SDLInterface::ApplyAlphaMask(SDL_Surface* baseSurface, SDL_Surface* alphaSurface) {
    if (baseSurface->w != alphaSurface->w || baseSurface->h != alphaSurface->h) {
        std::cerr << "Alpha mask and base image sizes do not match!" << std::endl;
        return nullptr;
    }

    SDL_Surface* convertedMask = SDL_ConvertSurface(alphaSurface, baseSurface->format, 0);

    SDL_LockSurface(baseSurface);
    SDL_LockSurface(convertedMask);

    // Access pixels and blend alpha from the mask
    Uint32* normalPixels = (Uint32*)baseSurface->pixels;
    Uint32* maskPixels = (Uint32*)convertedMask->pixels;

    for (int y = 0; y < baseSurface->h; y++) {
        for (int x = 0; x < baseSurface->w; x++) {
            // Get the pixel index
            int index = y * baseSurface->pitch / 4 + x;

            // Extract color channels from normal image
            Uint8 r, g, b, a;
            SDL_GetRGBA(normalPixels[index], baseSurface->format, &r, &g, &b, &a);

            // Extract the alpha from the mask (assuming grayscale mask, so R=G=B)
            Uint8 maskR, maskG, maskB, maskA;
            SDL_GetRGBA(maskPixels[index], convertedMask->format, &maskR, &maskG, &maskB, &maskA);

            // Apply the alpha from the mask (you can modulate or replace)
            a = (a * maskR) / 255;  // Modulate the alpha based on mask's grayscale value

            // Store the modified pixel back into the normal image
            normalPixels[index] = SDL_MapRGBA(baseSurface->format, r, g, b, a);
        }
    }

    // Unlock surfaces
    SDL_UnlockSurface(baseSurface);
    SDL_UnlockSurface(convertedMask);
}

SDL_BlendMode SDLInterface::ChooseBlendMode(int theBlendMode)
{
    SDL_BlendMode theSDLBlendMode;
    switch (theBlendMode)
    {
    case Graphics::DRAWMODE_ADDITIVE:
        theSDLBlendMode = SDL_BLENDMODE_ADD;
        break;
    default:
    case Graphics::DRAWMODE_NORMAL:
        theSDLBlendMode = SDL_BLENDMODE_BLEND;
        break;
    }
    return theSDLBlendMode;
}
