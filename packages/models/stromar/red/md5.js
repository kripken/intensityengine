Model.md5Dir("stromar");
Model.md5Load("stromar.md5mesh", "stromar");

Library.include("models/stromar/ragdoll.js", true);

Library.include("models/stromar/tags.js", true);

Model.md5Skin("Cube.037", "<dds>upper_attacker_04_a.jpg", "<dds>upper_attacker_01_sc_A.jpg", 0.6, 0.2);
Model.md5Bumpmap("Cube.037", "upper_norm.jpg");

Model.md5Skin("Cube", "<dds>lower_attacker_04_a.jpg", "<dds>lower_attacker_01_sc_A.jpg", 0.6, 0.2);
Model.md5Bumpmap("Cube", "lower_norm.jpg");

Model.md5Skin("Health", "health/1.png");

Library.include("models/stromar/anims.js", true);

Model.scale("500");


