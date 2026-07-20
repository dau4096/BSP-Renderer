/* loader.c */

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


#include "types.h"
#include "graphics.h" //For the geometry datasets




static xmlNode* l_findChildNode(const xmlNode* parent, const char* name) {
	for (xmlNode* node=parent->children; node; node=node->next) {
		if (
			(node->type == XML_ELEMENT_NODE) &&
			(xmlStrEqual(node->name, BAD_CAST name))
		) {
			return node;
		}
	}
	return NULL;
}

static unsigned int l_getNumChildElements(const xmlNode* parent) {
	unsigned int count = 0u;
	for (xmlNode* child=parent->children; child; child=child->next) {
		if (child->type == XML_ELEMENT_NODE) {count++;}
	}
	return count;
}





static const char* l_getStrAttr(const xmlNode* node, const char* name) {
    xmlChar* value = xmlGetProp(node, BAD_CAST name);
    if (!value) {return NULL;}

    char* copy = strdup((const char*)(value));
    xmlFree(value);
    return copy;
}

static int l_getIntAttr(const xmlNode* node, const char* name) {
	xmlChar* value = xmlGetProp(node, BAD_CAST name);
	int result = (int)(strtol((const char*)(value), NULL, 10)); //Accept decimal only
	xmlFree(value);
	return result;
}

static unsigned int l_getUIntAttr(const xmlNode* node, const char* name) {
	xmlChar* value = xmlGetProp(node, BAD_CAST name);
	int result = (unsigned int)(strtoul((const char*)(value), NULL, 0)); //Accept decimal and hexadecimal.
	xmlFree(value);
	return result;
}

static float l_getFloatAttr(const xmlNode* node, const char* name) {
	xmlChar* value = xmlGetProp(node, BAD_CAST name);
	float result = strtof((const char*)(value), NULL);
	xmlFree(value);
	return result;
}

static RGB_t l_getColourAttr(const xmlNode* node, const char* name) {
	const char* strAttr = l_getStrAttr(node, name);

	//Parse individually as R/G/B hex.
	unsigned int r, g, b;
	sscanf(strAttr, "%2x%2x%2x", &r, &g, &b);

	return (RGB_t) { //TBA
		.r = (uint8_t)(r),
		.g = (uint8_t)(g),
		.b = (uint8_t)(b)
	};
}

static unsigned int l_countUInts(const char* strAttr) {
    unsigned int count = 0;
    char* end;

    while (*strAttr) {
        strtoul(strAttr, &end, 10);

        if (strAttr == end) {break;}

        count++;
        strAttr = end;
    }

    return count;
}

static void l_getLineDefsArrayAttr(const xmlNode* node, unsigned int** lineDefsArr, unsigned int* numLineDefs) {
	const char* strAttr = l_getStrAttr(node, "lineDefs");
	char *end;
	*numLineDefs = l_countUInts(strAttr);
	*lineDefsArr = (unsigned int*)calloc(*numLineDefs, sizeof(unsigned int));

	unsigned int* ptr = *lineDefsArr;
	while (*strAttr) {
		unsigned long value = strtoul(strAttr, &end, 10);
		if (strAttr == end) {break; /* no more numbers */}

		*(ptr++) = (unsigned)(value);

		strAttr = end;
	}
}


unsigned int numAssignedTextures;
const char* textureNames[MAX_TEXTURES];
static unsigned int l_assignTextureIndex(const char* filePath) {
	for (unsigned int i=0u; i<numAssignedTextures; i++) {
		//Check if it's already in the dataset. If so, return the index.
		if (strcmp(textureNames[i], filePath) == 0) {return i; /* Found */}
	}

	//Didn't find it, add to the end of the list.
	textureNames[numAssignedTextures] = filePath;
	return numAssignedTextures++;
}





int l_getVertices(const xmlNode* root) {
	const xmlNode* verticesNode = l_findChildNode(root, "vertices");
	if (!verticesNode) {
		//Failiure
		printf("Missing <vertices> XML node.\n");
		return FALSE;
	}


	g_numVertices = l_getNumChildElements(verticesNode);
	g_vertices = calloc(g_numVertices, sizeof(Vec2f_t));

	//Parse each in order.
	Vec2f_t* geoIndex = g_vertices; //Moving ptr
	for (xmlNode* vertNode=verticesNode->children; vertNode; vertNode=vertNode->next) {
		*(geoIndex++) = (Vec2f_t){
			.x=l_getFloatAttr(vertNode, "x"),
			.y=l_getFloatAttr(vertNode, "y")
		};
	}

	return TRUE;
}




int l_getSectors(const xmlNode* root) {
	const xmlNode* sectorsNode = l_findChildNode(root, "sectors");
	if (!sectorsNode) {
		//Failiure
		printf("Missing <sectors> XML node.\n");
		return FALSE;
	}


	g_numSectors = l_getNumChildElements(sectorsNode);
	g_sectors = calloc(g_numSectors, sizeof(Sector_t));

	//Parse each in order.
	Sector_t* geoIndex = g_sectors; //Moving ptr
	for (xmlNode* secNode=sectorsNode->children; secNode; secNode=secNode->next) {
		unsigned int* lineDefsArr; unsigned int numLineDefs;
		l_getLineDefsArrayAttr(secNode, &lineDefsArr, &numLineDefs);

	#ifdef PLANE_SPAN_TEXTURING
		const char* floorTextureName = l_getStrAttr(secNode, "floorTexture");
		const char* ceilTextureName = l_getStrAttr(secNode, "ceilTexture");

		*(geoIndex++) = (Sector_t){
			.floorHeight=l_getFloatAttr(secNode, "floorZ"),
			.floorTexture=l_assignTextureIndex(floorTextureName),
			.floorColour=(RGB_t){.r=0u, .g=0u, .b=0u},

			.ceilingHeight=l_getFloatAttr(secNode, "ceilZ"),
			.ceilingTexture=l_assignTextureIndex(ceilTextureName),
			.ceilingColour=(RGB_t){.r=0u, .g=0u, .b=0u},

			.lineDefs=lineDefsArr, .numLineDefs=numLineDefs,
			.lightLevel=l_getUIntAttr(secNode, "lightLevel")
		};

	#else

		*(geoIndex++) = (Sector_t){
			.floorHeight=l_getFloatAttr(secNode, "floorZ"),
			.floorColour=l_getColourAttr(secNode, "floorC"),
			.floorTexture=-1,

			.ceilingHeight=l_getFloatAttr(secNode, "ceilZ"),
			.ceilingColour=l_getColourAttr(secNode, "ceilC"),
			.ceilingTexture=-1,

			.lineDefs=lineDefsArr, .numLineDefs=numLineDefs,
			.lightLevel=l_getUIntAttr(secNode, "lightLevel")
		};
	#endif
	}

	return TRUE;
}




int l_getLineDefs(const xmlNode* root) {
	//Assumes sectors & vertices have been processed first.
	const xmlNode* lineDefsNode = l_findChildNode(root, "lineDefs");
	if (!lineDefsNode) {
		//Failiure
		printf("Missing <lineDefs> XML node.\n");
		return FALSE;
	}


	g_numLineDefs = l_getNumChildElements(lineDefsNode);
	g_lineDefs = calloc(g_numLineDefs, sizeof(LineDef_t));

	//Parse each in order.
	LineDef_t* geoIndex = g_lineDefs; //Moving ptr
	for (xmlNode* ldNode=lineDefsNode->children; ldNode; ldNode=ldNode->next) {
		const char* textureName = l_getStrAttr(ldNode, "texture");
		unsigned int textureIndex = l_assignTextureIndex(textureName);
		*(geoIndex++) = (LineDef_t){
			.vStart=l_getUIntAttr(ldNode, "v0"),
			.vEnd=l_getUIntAttr(ldNode, "v1"),

			.frontSector=l_getIntAttr(ldNode, "front"), //Eventually replace with autogenerated from sectors (?)
			.backSector=l_getIntAttr(ldNode, "back"),   //Ditto [^^]

			.texture=textureIndex,
			.isValid=TRUE 
		};
	}

	return TRUE; //Success
}




int l_repositionCamera(const xmlNode* root) {
	//Moves and rotates camera to correct starting position.
	//Deals with *r_camera ptr.
	const xmlNode* cameraNode = l_findChildNode(root, "camera");
	if (!cameraNode) {
		//Failiure
		printf("Missing <camera> XML node.\n");
		return FALSE;
	}

	r_camera->position.x = l_getFloatAttr(cameraNode, "x");
	r_camera->position.y = l_getFloatAttr(cameraNode, "y");

	r_camera->yaw = l_getFloatAttr(cameraNode, "yaw");

	//Sector_t * currentSector = p_findCurrentSectorSlow(r_camera);
	Sector_t* currentSector = g_sectors; //Use g_sectors[0] until I figure out a good method it implement the above.
	r_camera->Z = currentSector->floorHeight;

	return TRUE; //Success
}




int l_loadGeo(const char* filePath) {
	//Loads some file into the geometry datasets provided.
	//Returns success

	//Deallocate if already filled.
	if (g_vertices) {free(g_vertices);}
	if (g_lineDefs) {free(g_lineDefs);}
	if (g_sectors) {free(g_sectors);}


	numAssignedTextures = 0u;


	//Read the file
	xmlDocPtr document = xmlReadFile(filePath, NULL, XML_PARSE_NOBLANKS);
	if (!document) {
		//Failed to load document.
		return FALSE;
	}


	xmlNode* root = xmlDocGetRootElement(document);


	//Parse in order
	if (
		!l_getVertices(root) ||
		!l_getSectors(root) ||
		!l_getLineDefs(root) //Front/back sectors depends on processing sectors first
	) {
		return FALSE; //Any of those 3 failed.
	}


	//Read camera start information
	if (!l_repositionCamera(root)) {return FALSE; /* Could not move camera */}


	//Load all textures;
	if (!r_loadTextures(textureNames, numAssignedTextures)) {return FALSE; /* Could not load textures */}


	//Cleanup
	xmlFreeDoc(document);

	return TRUE;
}



