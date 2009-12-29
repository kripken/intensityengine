
/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */

namespace IntensityTexture
{
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

