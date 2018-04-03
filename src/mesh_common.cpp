
collada_file *MeshManagerLoadCollada(const char *collada)
{
	char colladaFileName[1024];
	CombinePath(colladaFileName, App.baseFolder, collada);
	char *colladaFile = ReadFileIntoString(colladaFileName);
	collada_file *dae = PUSH_ITEM(&Arena, collada_file);
	*dae = ReadColladaFile(colladaFile);
	return dae;
}

bone_model MeshManagerLoadBoneModel(collada_file *dae, const char *name, const char *spriteName)
{
	game_sprite *s = NULL;
	if (spriteName)
	{
		s = (game_sprite *)GetFromHash(&Sprites, spriteName);
	}
	bone_model model = BoneModelGetFromColladaByName(dae, name, s);
	return model;
}

basic_mesh MeshManagerLoadBasicMesh(collada_file *dae, const char *name, const char *spriteName = NULL)
{
	game_sprite *s = NULL;
	if (spriteName)
	{
		s = (game_sprite *)GetFromHash(&Sprites, spriteName);
	}
	basic_mesh mesh = BasicMeshGetFromColladaByName(dae, name, s);
	return mesh;
}
