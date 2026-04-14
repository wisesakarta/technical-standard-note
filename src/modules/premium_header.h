/*
  Solum
  
  Premium header component using Direct2D for hardware-accelerated 
  visual effects and animations (Liquid Logo Reveal).
*/

#pragma once
#include "graphics_engine.h"
#include "animation_controller.h"
#include <string>

namespace Premium
{
    class Header
    {
    public:
        Header() = default;
        ~Header();

        bool Initialize(Graphics::Engine* pEngine);
        void Update();
        void Render(const RECT& rect);
        void StartReveal();

    private:
        Graphics::Engine* m_pEngine = nullptr;
        Animation::Transition m_revealTransition;
        ID2D1SolidColorBrush* m_pAccentBrush = nullptr;
        ID2D1SolidColorBrush* m_pTextBrush = nullptr;
        IDWriteTextFormat* m_pTextFormat = nullptr;
        
        bool m_isInitialized = false;
        void CreateResources();
    };
}
