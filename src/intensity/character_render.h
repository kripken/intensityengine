
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

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
