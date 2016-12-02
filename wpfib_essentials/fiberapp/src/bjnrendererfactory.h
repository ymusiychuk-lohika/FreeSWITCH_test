#ifndef H_BJNRENDERER_FACTORY_H
#define H_BJNRENDERER_FACTORY_H

#ifdef FB_WIN
	#ifdef FB_WIN_D3D
		#include "win/D3D/DxBjnRenderer.h"
	#else
		#include "win/winbjnrenderer.h"
	#endif
#elif FB_MACOSX
#include "mac/macbjnrenderer.h"
#elif FB_X11
#include "X11/x11bjnrenderer.h"
#endif

#include "bjnrenderer.h"


class BjnRendererFactory
{
public:
	static BjnRenderer* CreateBjnRenderer(int index)
	{
#if(defined FB_WIN && defined FB_WIN_D3D)
		return new DxBjnRenderer(index);
#else
		//This factory is used for D3D renderer only
		assert(false);
		return new BjnRenderer();
#endif
	}

    static BjnRenderer* CreateBjnRenderer(bool windowless = false) {
#ifdef FB_WIN
        return new WinBjnRenderer(windowless);
#elif FB_MACOSX
        return new MacBjnRenderer();
#elif FB_X11
        return new X11BjnRenderer();
#else
        return new BjnRenderer();
#endif
    }
};

#endif

