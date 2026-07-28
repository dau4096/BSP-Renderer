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


static int l_getIntAttr(const xmlNode* node, const char* name, const int defaultValue) {
	xmlChar* value = xmlGetProp(node, BAD_CAST name);
    if (!value) {return defaultValue;}
	int result = (int)(strtol((const char*)(value), NULL, 10)); //Accept decimal only
	xmlFree(value);
	return result;
}


static unsigned int l_getUIntAttr(const xmlNode* node, const char* name, const unsigned int defaultValue) {
	xmlChar* value = xmlGetProp(node, BAD_CAST name);
    if (!value) {return defaultValue;}
	int result = (unsigned int)(strtoul((const char*)(value), NULL, 0)); //Accept decimal and hexadecimal.
	xmlFree(value);
	return result;
}


static float l_getFloatAttr(const xmlNode* node, const char* name, const float defaultValue) {
	xmlChar* value = xmlGetProp(node, BAD_CAST name);
    if (!value) {return 0.0f;}
	float result = strtof((const char*)(value), NULL);
	xmlFree(value);
	return result;
}


static RGB_t l_getColourAttr(const xmlNode* node, const char* name, const RGB_t defaultValue) {
	const char* strAttr = l_getStrAttr(node, name);
	if (!strAttr) {return defaultValue;}

	//Parse individually as R/G/B hex.
	unsigned int r, g, b;
	sscanf(strAttr, "%2x%2x%2x", &r, &g, &b);

	return (RGB_t) { //TBA
		.r = (uint8_t)(r),
		.g = (uint8_t)(g),
		.b = (uint8_t)(b)
	};
}


static void l_getLineDefsArrayAttr(
	const unsigned int sectorID,
	unsigned int** lineDefsArr, unsigned int* numLineDefs
) {
	//Loop through all, find NUMBER of linedefs that point to THIS sector. Add them to temp array.
	//Assumes that linedefs were read and processed FIRST.
	*numLineDefs = 0u;
	unsigned int* tempLDArray = calloc(g_numLineDefs, sizeof(unsigned int));
	for (unsigned int ldIndex=0u; ldIndex<g_numLineDefs; ldIndex++) {
		LineDef_t* thisLineDef = g_lineDefs + ldIndex;
		if ((thisLineDef->frontSector == sectorID) || (thisLineDef->backSector == sectorID)) {
			//Matches this sector.
			tempLDArray[(*numLineDefs)++] = ldIndex; //Add to temporary array.
		}
	}

	*lineDefsArr = calloc(*numLineDefs, sizeof(unsigned int));
	memcpy(*lineDefsArr, tempLDArray, (*numLineDefs) * sizeof(unsigned int)); //Copy into correctly sized array.
	free(tempLDArray);
}


unsigned int numAssignedTextures;
const char* textureNames[MAX_TEXTURES];
static unsigned int l_assignTextureIndex(const char* filePath) {
	if (!filePath) {return fallbackTextureIndex;}

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
			.x=l_getFloatAttr(vertNode, "x", 0.0f),
			.y=l_getFloatAttr(vertNode, "y", 0.0f)
		};
	}

	return TRUE;
}




int l_getLineDefs(const xmlNode* root) {
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
		
		//If either of these fail, they will default to -1. This can this be taken as a faliure to load.
		int start = l_getIntAttr(ldNode, "v0", -1);
		int end = l_getIntAttr(ldNode, "v1", -1);

		if ((start < 0) || (end < 0)) {
			//Was not provided.
			printf(
				"Linedef %lu is missing a vertex: [v0: %s | v1: %s]",
				geoIndex - g_lineDefs,
				(start < 0) ? "MISSING" : "FOUND",
				(end < 0) ? "MISSING" : "FOUND"
			);
			return FALSE;
		} else if ((start >= (int)(g_numVertices)) || (end >= (int)(g_numVertices))) {
			//Uses invalid vertex index.
			printf(
				"Linedef %lu uses invalid vertex/vertices: [v0: %i | v1: %i] (Max valid index is %u)",
				geoIndex - g_lineDefs, start, end, g_numVertices
			);
			return FALSE;
		}

		int front = l_getIntAttr(ldNode, "front", 0);
		int back = l_getIntAttr(ldNode, "back", 0);
		if (front < 0) {
			if (back < 0) {
				printf("Linedef %lu is missing front-sector index", geoIndex - g_lineDefs);
				return FALSE;
			} else {
				front = back;
				back = -1; //Swap.
			}
		}

		*(geoIndex++) = (LineDef_t){
			.vStart=(unsigned int)(start), .vEnd=(unsigned int)(end),

			.frontSector=front, .backSector=back,

			.texture=textureIndex,
			.isValid=TRUE 
		};
	}

	return TRUE; //Success
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
		l_getLineDefsArrayAttr(geoIndex-g_sectors, &lineDefsArr, &numLineDefs);


	#ifdef PLANE_SPAN_TEXTURING
		const char* floorTextureName = l_getStrAttr(secNode, "floorTexture");
		const char* ceilTextureName = l_getStrAttr(secNode, "ceilTexture");

		int floorSuccess, ceilSuccess;
		const RGB_t floorColour = l_getColourAttr(secNode, "floorColour", RGB_MAGENTA);
		const RGB_t ceilColour = l_getColourAttr(secNode, "ceilColour", RGB_CYAN);
		const uint8_t flags =  (
			((floorTextureName) ? 1u : 0u) | //Bit 0: floor colour/texture
			((ceilTextureName ? 1u : 0u) << 1) //Bit 1: ceil colour/texture
		);

		*(geoIndex++) = (Sector_t){
			.floorHeight=l_getFloatAttr(secNode, "floorZ", -1.0f),
			.floorTexture=l_assignTextureIndex(floorTextureName),
			.floorColour=floorColour,

			.ceilingHeight=l_getFloatAttr(secNode, "ceilZ", 1.0f),
			.ceilingTexture=l_assignTextureIndex(ceilTextureName),
			.ceilingColour=ceilColour,

			.flags=flags,

			.lineDefs=lineDefsArr, .numLineDefs=numLineDefs,
			.lightLevel=l_getUIntAttr(secNode, "lightLevel", 255u)
		};

	#else

		*(geoIndex++) = (Sector_t){
			.floorHeight=l_getFloatAttr(secNode, "floorZ", -1.0f),
			.floorColour=l_getColourAttr(secNode, "floorColour", RGB_MAGENTA),
			.floorTexture=fallbackTextureIndex,

			.ceilingHeight=l_getFloatAttr(secNode, "ceilZ", 1.0f),
			.ceilingColour=l_getColourAttr(secNode, "ceilColour", RGB_CYAN),
			.ceilingTexture=fallbackTextureIndex,

			.flags=0b0000000,

			.lineDefs=lineDefsArr, .numLineDefs=numLineDefs,
			.lightLevel=l_getUIntAttr(secNode, "lightLevel", 255u)
		};
	#endif
	}

	return TRUE;
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

	r_camera->position.x = l_getFloatAttr(cameraNode, "x", 0.0f);
	r_camera->position.y = l_getFloatAttr(cameraNode, "y", 0.0f);

	r_camera->yaw = l_getFloatAttr(cameraNode, "yaw", 0.0f);

	//Sector_t * currentSector = p_findCurrentSectorSlow(r_camera);
	Sector_t* currentSector = g_sectors; //Use g_sectors[0] until I figure out a good method to implement the above.
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
	fallbackTextureIndex = l_assignTextureIndex(FALLBACK_TEXTURE_PATH);


	//Read the file
	xmlDocPtr document = xmlReadFile(filePath, NULL, XML_PARSE_NOBLANKS);
	if (!document) {
		//Failed to load document.
		return FALSE;
	}


	//XML root. <geometry> in the current format.
	xmlNode* root = xmlDocGetRootElement(document);

	//Parse in order
	if (
		!l_getVertices(root) ||
		!l_getLineDefs(root) ||
		!l_getSectors(root)
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



