
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

namespace IntensityTexture
{
    void resetBackgroundLoading();

    void doBackgroundLoading(bool all=false);

#ifdef USE_JPEG2000
    void convertJP2toPNG(std::string source, std::string dest);
#endif

    void convertPNGtoDDS(std::string source, std::string dest);

    void combineImages(std::string primary, std::string secondary, std::string dest);

    //! Texture should have been created before, with something like
    //!     textureload(name, 0, false);  (false - do not mip it)
    //! And of course the pixels must be big enough for the texture data
    //!
    //! Note: x,y,w,h are in the OpenGL texture, which may differ from the original. For
    //! example, a 1024x768 original JPG will become a 1024x1024 image, so you should
    //! give coordinates for that
    void uploadTextureData(std::string name, int x, int y, int w, int h, void *pixels);
}

