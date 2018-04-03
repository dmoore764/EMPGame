#include <fbxsdk.h>

global_variable FbxManager *SDKManager;

void InitMeshLoader()
{
	SDKManager = FbxManager::Create();
}

global_variable int numTabs = 0; 

void PrintTabs()
{
    for(int i = 0; i < numTabs; i++)
        printf("\t");
}

FbxString GetAttributeTypeName(FbxNodeAttribute::EType type) { 
    switch(type) { 
        case FbxNodeAttribute::eUnknown: return "unidentified"; 
        case FbxNodeAttribute::eNull: return "null"; 
        case FbxNodeAttribute::eMarker: return "marker"; 
        case FbxNodeAttribute::eSkeleton: return "skeleton"; 
        case FbxNodeAttribute::eMesh: return "mesh"; 
        case FbxNodeAttribute::eNurbs: return "nurbs"; 
        case FbxNodeAttribute::ePatch: return "patch"; 
        case FbxNodeAttribute::eCamera: return "camera"; 
        case FbxNodeAttribute::eCameraStereo: return "stereo"; 
        case FbxNodeAttribute::eCameraSwitcher: return "camera switcher"; 
        case FbxNodeAttribute::eLight: return "light"; 
        case FbxNodeAttribute::eOpticalReference: return "optical reference"; 
        case FbxNodeAttribute::eOpticalMarker: return "marker"; 
        case FbxNodeAttribute::eNurbsCurve: return "nurbs curve"; 
        case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface"; 
        case FbxNodeAttribute::eBoundary: return "boundary"; 
        case FbxNodeAttribute::eNurbsSurface: return "nurbs surface"; 
        case FbxNodeAttribute::eShape: return "shape"; 
        case FbxNodeAttribute::eLODGroup: return "lodgroup"; 
        case FbxNodeAttribute::eSubDiv: return "subdiv"; 
        default: return "unknown"; 
    } 
}

void PrintAttribute(FbxNodeAttribute* pAttribute) {
    if(!pAttribute) return;
 
    FbxString typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
    FbxString attrName = pAttribute->GetName();
    PrintTabs();
    // Note: to retrieve the character array of a FbxString, use its Buffer() method.
    printf("<attribute type='%s' name='%s'/>\n", typeName.Buffer(), attrName.Buffer());
	if (pAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		FbxMesh *mesh = (FbxMesh *)pAttribute;
		PrintTabs();
		int polyCount = mesh->GetPolygonCount();
		printf("mesh data: polygons: %d\n", polyCount);
		numTabs++;
		for (int i = 0; i < polyCount; i++)
		{
			PrintTabs();
			int polySize = mesh->GetPolygonSize(i);
			printf("poly %d (%d verts)\n", i, polySize);
			numTabs++;
			for (int j = 0; j < polySize; j++)
			{
				PrintTabs();
				int controlPointIndex = mesh->GetPolygonVertex(i,j);
				FbxVector4 v = mesh->GetControlPointAt(controlPointIndex);
				printf("Vert %d (controlP %d): [%f, %f, %f]\n", j, controlPointIndex, v[0], v[1], v[2]);
			}
			numTabs--;
		}
		numTabs--;
	}
}

void PrintFBXNode(FbxNode *pNode)
{
	PrintTabs();
    const char* nodeName = pNode->GetName();
    FbxDouble3 translation = pNode->LclTranslation.Get(); 
    FbxDouble3 rotation = pNode->LclRotation.Get(); 
    FbxDouble3 scaling = pNode->LclScaling.Get();

    // Print the contents of the node.
    printf("<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n", 
        nodeName, 
        translation[0], translation[1], translation[2],
        rotation[0], rotation[1], rotation[2],
        scaling[0], scaling[1], scaling[2]
        );
    numTabs++;

    // Print the node's attributes.
    for(int i = 0; i < pNode->GetNodeAttributeCount(); i++)
        PrintAttribute(pNode->GetNodeAttributeByIndex(i));

    // Recursively print the children.
    for(int j = 0; j < pNode->GetChildCount(); j++)
        PrintFBXNode(pNode->GetChild(j));

    numTabs--;
    PrintTabs();
    printf("</node>\n");
}

void TestLoadMesh(basic_mesh *m)
{
	char *meshFile = "meshes/ship.fbx";
	char filePath[1024];
	CombinePath(filePath, BaseFolder, meshFile);
	
	//create an io settings object
	FbxIOSettings *ios = FbxIOSettings::Create(SDKManager, IOSROOT);
    SDKManager->SetIOSettings(ios);

	//create an importer
	FbxImporter *importer = FbxImporter::Create(SDKManager, "");

	if (!importer->Initialize(filePath, -1, SDKManager->GetIOSettings()))
	{
		DBG("Call to FbxImporter::Initialize() failed");
	}
	else
	{
		DBG("Imported! %s", meshFile);
	}

	FbxScene* scene = FbxScene::Create(SDKManager,"testScene");

    // Import the contents of the file into the scene.
    importer->Import(scene);
    importer->Destroy();

	FbxNode* rootNode = scene->GetRootNode();
	/*
    if(rootNode) {
        for(int i = 0; i < rootNode->GetChildCount(); i++)
            PrintFBXNode(rootNode->GetChild(i));
    }
	*/
	
	if (rootNode)
	{
		for (int i = 0; i < rootNode->GetChildCount(); i++)
		{
			FbxNode *pNode = rootNode->GetChild(i);
			for(int j = 0; j < pNode->GetNodeAttributeCount(); j++)
			{
				FbxNodeAttribute *pAttribute = pNode->GetNodeAttributeByIndex(j);
				if (pAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
				{

					FbxDouble3 translation = pNode->LclTranslation.Get(); 
					FbxDouble3 rotation = pNode->LclRotation.Get(); 
					FbxDouble3 scaling = pNode->LclScaling.Get();

					glm::mat4 transformMat = glm::translate(glm::mat4(1.0f), glm::vec3(translation[0],translation[1],translation[2]));
					transformMat = glm::rotate(transformMat, (float)rotation[0]*DEGREES_TO_RADIANS, glm::vec3(1.0f,0.0f,0.0f));
					transformMat = glm::rotate(transformMat, (float)rotation[1]*DEGREES_TO_RADIANS, glm::vec3(0.0f,1.0f,0.0f));
					transformMat = glm::rotate(transformMat, (float)rotation[2]*DEGREES_TO_RADIANS, glm::vec3(0.0f,0.0f,1.0f));
					transformMat = glm::scale(transformMat, glm::vec3(scaling[0], scaling[1], scaling[2]));

					FbxMesh *mesh = (FbxMesh *)pAttribute;
					int polyCount = mesh->GetPolygonCount();
					for (int k = 0; k < polyCount; k++)
					{
						assert(mesh->GetPolygonSize(k) == 3);
					}
					assert(polyCount*3 < 5000);

					//save current VAO
					GLint currentVao;
					glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);

					vert3d verts[5000];

					FbxAMatrix& globalTransformRef = pNode->EvaluateGlobalTransform();
					FbxAMatrix& localTransformRef = pNode->EvaluateLocalTransform();
					FbxMatrix globalTransform(globalTransformRef);
					FbxMatrix localTransform(localTransformRef);
					FbxMatrix transform = globalTransform*localTransform;

					uint32_t c = MAKE_COLOR(230,130,30,255);
					for (int k = 0; k < polyCount; k++)
					{
						int ctrP0 = mesh->GetPolygonVertex(k,0);
						int ctrP1 = mesh->GetPolygonVertex(k,1);
						int ctrP2 = mesh->GetPolygonVertex(k,2);
						FbxVector4 vp0 = mesh->GetControlPointAt(ctrP0);
						FbxVector4 vp1 = mesh->GetControlPointAt(ctrP1);
						FbxVector4 vp2 = mesh->GetControlPointAt(ctrP2);

						glm::vec4 glmVec0(vp0[0],vp0[1],vp0[2],1.0f);
						glm::vec4 glmVec1(vp1[0],vp1[1],vp1[2],1.0f);
						glm::vec4 glmVec2(vp2[0],vp2[1],vp2[2],1.0f);
						glmVec0 = transformMat*glmVec0;
						glmVec1 = transformMat*glmVec1;
						glmVec2 = transformMat*glmVec2;

						FbxVector4 vn0, vn1, vn2;
						mesh->GetPolygonVertexNormal(k,0,vn0);
						mesh->GetPolygonVertexNormal(k,1,vn1);
						mesh->GetPolygonVertexNormal(k,2,vn2);

						glm::vec4 glmNorm0(vn0[0],vn0[1],vn0[2],1.0f);
						glm::vec4 glmNorm1(vn1[0],vn1[1],vn1[2],1.0f);
						glm::vec4 glmNorm2(vn2[0],vn2[1],vn2[2],1.0f);
						glmNorm0 = transformMat*glmNorm0;
						glmNorm1 = transformMat*glmNorm1;
						glmNorm2 = transformMat*glmNorm2;
						vp0 = transform.MultNormalize(vp0);
						vp1 = transform.MultNormalize(vp1);
						vp2 = transform.MultNormalize(vp2);
						vn0 = transform.MultNormalize(vn0);
						vn1 = transform.MultNormalize(vn1);
						vn2 = transform.MultNormalize(vn2);

						/*
						verts[k*3 + 0] = {{glmVec0.x, glmVec0.y, glmVec0.z},{glmNorm0.x, glmNorm0.y, glmNorm0.z},c};
						verts[k*3 + 1] = {{glmVec1.x, glmVec1.y, glmVec1.z},{glmNorm1.x, glmNorm1.y, glmNorm1.z},c};
						verts[k*3 + 2] = {{glmVec2.x, glmVec2.y, glmVec2.z},{glmNorm2.x, glmNorm2.y, glmNorm2.z},c};
						*/

						verts[k*3 + 0] = {{(float)vp0[0], (float)vp0[1], (float)vp0[2]},{(float)vn0[0], (float)vn0[1], (float)vn0[2]},c};
						verts[k*3 + 1] = {{(float)vp1[0], (float)vp1[1], (float)vp1[2]},{(float)vn1[0], (float)vn1[1], (float)vn1[2]},c};
						verts[k*3 + 2] = {{(float)vp2[0], (float)vp2[1], (float)vp2[2]},{(float)vn2[0], (float)vn2[1], (float)vn2[2]},c};
					}

					uint32_t vao, vbo;
					glGenVertexArrays(1, &vao);
					glGenBuffers(1, &vbo);

					glBindVertexArray(vao);
					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBufferData(GL_ARRAY_BUFFER, sizeof(vert3d)*polyCount*3, verts, GL_STATIC_DRAW);

					glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);
					glEnableVertexAttribArray(0);
					glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(3 * sizeof(float)));
					glEnableVertexAttribArray(1);
					glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, 7 * sizeof(float), (void *)(6 * sizeof(float)));
					glEnableVertexAttribArray(2);

					glBindBuffer(GL_ARRAY_BUFFER, 0);

					m->vaoHandle = vao;
					m->numVerts = polyCount*3;
					m->bufferHandle = vbo;

					glBindVertexArray(currentVao);
					return;
				}
			}
		}
	}
	
}
