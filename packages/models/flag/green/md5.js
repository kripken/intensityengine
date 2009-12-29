Model.md5Load("gk_syn_flag_04.md5mesh", "flag");

for (var i = 1; i <= 5; i++) {
    Model.md5Skin("Mesh" + i, "../gk_syn_flag_cc.jpg", "../gk_syn_flag_sc.jpg");
    Model.md5Bumpmap("Mesh" + i, "../gk_syn_flag_nm.jpg");
}

Model.md5Anim("idle", "gk_syn_flag_04.md5anim", 60, 1);

//mdlspec("175");
Model.scale("166");
Model.yaw("90");
//mdltrans("0", "0", -0.25);

