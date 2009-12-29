Model.md5Load("stromar.md5mesh", "stromar");

Library.include("models/stromar/ragdoll.js", true);

Library.include("models/stromar/tags.js", true);

Model.md5Skin("Cube.037", "<dds>upper_attacker_04_a.jpg", "<dds>upper_attacker_01_sc_A.jpg", 0.6, 0.2);
Model.md5Bumpmap("Cube.037", "upper_norm.jpg");

Model.md5Skin("Cube", "<dds>lower_attacker_04_a.jpg", "<dds>lower_attacker_01_sc_A.jpg", 0.6, 0.2);
Model.md5Bumpmap("Cube", "lower_norm.jpg");

Model.md5Skin("Health", "health/1.png");
Model.md5Bumpmap("Health", "<normal>health/1.png"); // In theory, sauer models need ALL meshes to have or not have
                                                    // bumpmaps. This is a simple way to make a 'dummy' bumpmap.
                                                    // But, recent skelmodel.h versions (sauer rev. )
                                                    // fix that. Still, putting it here so people using the old
                                                    // code are ok.

Library.include("models/stromar/anims.js", true);

Model.scale("500");

