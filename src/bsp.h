/* bsp.h */
#ifndef BSP_H
#define BSP_H



//////// BSP INIT/DEL ////////
BSPnode_t bsp_build( //Process geometry into a BSP tree.
	const Vec2f_t* vertices, unsigned int numberOfVertices,
	const LineDef_t* lineDefs, unsigned int numberOfLineDefs,
	int* success
);

void bsp_free(BSPnode_t* node); //Clear a BSP tree and free all memory used.
//////// BSP INIT/DEL ////////



//////// WALKING THE BSP ////////
void bsp_walk(
	const BSPnode_t* node, //Node to process
	const LineDef_t* lineDefs, //Dataset of linedefs
	const Vec2f_t cameraPosition, //Used to decide where to recurse into first.
	void (*render)( //Function to call on every segment in every node.
		const Segment_t, //This segment
		const LineDef_t* //Dataset of linedefs.
	)
);
//////// WALKING THE BSP ////////


#endif