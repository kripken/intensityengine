
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


struct CharacterRendering
{
    static void render(fpsent *entity);
};


/*

#define MAX_ATTACHMENTS 20


//! An attachment to a model, i.e., a sub-visual that is rendered with it

struct Attachment
{
    //! The model name, under packages/models
    std::string name;

    //! The type of attachment, for MD3 attachments to which bone it attaches. For MD5 attachments,
    //! the meshes are simply combined, i.e., they share a skeleton, and the tag is less important.
    //! It is still used to ensure that only a single attachment exists per tag (also for MD3).
    std::string tag;  // The type of attachment, i.e., where it attaches

    Attachment(std::string _name, std::string _tag) : name(_name), tag(_tag) { };

    friend bool operator==(Attachment &a, Attachment &b)
        { return (a.name == b.name) && (a.tag == b.tag); };
};

struct fpsent;

class CharacterInfo
{
    typedef std::vector<Attachment> AttachmentVec;

    //! The dynamic entity whose character info this is, i.e., our parent
    fpsent* data; 

    //! Model name under packages/models
    char*         modelName;

    //! The attachments currently active for this character. From this list we generate a Sauer-
    //! type list of attachments each frame, for rendering.
    AttachmentVec attachments;

public:
    CharacterInfo(fpsent* _data);

    //! Render this character, with the attachments, and with relevant info from the fpsent (movement, etc.)
    void render();

    //! Add an attachment. Must not already have an attachment for that tag
    void addAttachment(Attachment attachment);

    //! Removes an attachment
    void removeAttachment(std::string tag);

    //! Returns the index of a tag inside the vector of attachments, a convenience function. TODO: Should be private,
    //! but sauer_cegui_lua.cpp uses it for convenience
    int  findAttachment(std::string tag);
};

*/
