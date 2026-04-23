/* bsp.c */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "graphics.h"
#include "maths.h"


#define EPSILON 1.0e-5f
#define MIN_SEGMENTS 3
#define MAX_SPLIT_RATIO 0.75f
#define MAX_SHARE_RATIO 0.95f
#define MAX_DEPTH 20
#define MAX_SPLITS 4096

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))



float bsp_signedDistance(const Split_t split, const Vec2f_t position) {
	return v2f_dot(
		v2f_sub(position, split.position),
		split.forward
	);
}



float bsp_scoringFunction(
	int numberOfSplits, int numberOfFronts, int numberOfBacks, int recursionDepth
) {
	int total = numberOfFronts + numberOfBacks;
	if (total == 0) {return 1.0e9f; /* Heavily disincentivise */}

	float splitCost = (float)(numberOfSplits) / (float)(total);
	float balancePenalty = fabsf((float)(numberOfFronts - numberOfBacks));
	float imbalance = balancePenalty / (float)(total);

	return (
		(splitCost * 2.0f) +
		(imbalance * 5.0f) +
		(balancePenalty * 1.0f) +
		(recursionDepth * 0.2f)
	);
}


Vec2f_t bsp_getDirection(const Segment_t segment) {
	Vec2f_t delta = v2f_sub(segment.vEnd, segment.vStart);
	if (v2f_len(delta) < 1.0e-6f) {return (Vec2f_t){.x=0.0f, .y=1.0f}; /* Arbitrary direction. */}

	delta = v2f_normalise(delta);
	return (Vec2f_t){.x=-delta.y, .y=delta.x};
}


void bsp_pickCandidateSplits(
	const Segment_t* segments, const unsigned int numberOfSegments, //In
	Split_t* splits, unsigned int* numberOfSplits //Out
) {
	for (unsigned int i=0u; i<numberOfSegments; i++) {
		Segment_t segment = segments[i];
		Vec2f_t direction = bsp_getDirection(segment);

		splits[(*numberOfSplits)++] = (Split_t){.position=segment.vStart, .forward=direction};

		Vec2f_t mid = v2f_mul(v2f_add(segment.vStart, segment.vEnd), 0.5f);
		splits[(*numberOfSplits)++] = (Split_t){.position=mid, .forward=direction};

		Vec2f_t normal = (Vec2f_t){.x=-direction.y, .y=direction.x};
		splits[(*numberOfSplits)++] = (Split_t){.position=mid, .forward=normal};
	}
}


void bsp_splitSegments(
	//In
	const Split_t split, const Segment_t* segments, unsigned int  numberOfSegments,

	//Out
	Segment_t* fronts, unsigned int* numberOfFronts,
	Segment_t* coplanars, unsigned int* numberOfCoplanars,
	Segment_t* backs, unsigned int* numberOfBacks
) {
	for (unsigned int i=0u; i<numberOfSegments; i++) {
		Segment_t segment = segments[i];

		float d0 = bsp_signedDistance(split, segment.vStart);
		if (fabsf(d0) <= EPSILON) {d0 = 0.0f;}
		float d1 = bsp_signedDistance(split, segment.vEnd);
		if (fabsf(d1) <= EPSILON) {d1 = 0.0f;}

		//Sort where it should be placed.
		if ((d0 > EPSILON) && (d1 > EPSILON)) {
			fronts[(*numberOfFronts)++] = segment;
		} else if ((d0 < -EPSILON) && (d1 < -EPSILON)) {
			backs[(*numberOfBacks)++] = segment;
		} else if ((fabsf(d0) <= EPSILON) && (fabsf(d1) <= EPSILON)) {
			coplanars[(*numberOfCoplanars)++] = segment;
		} else {
			//Segment crosses plane, split into two.
			float t = d0 / (d0 - d1);

			if (t <= EPSILON) {
				backs[(*numberOfBacks)++] = segment;
				continue;
			}
			if (t >= 1-EPSILON) {
				fronts[(*numberOfFronts)++] = segment;
				continue;
			}

			Vec2f_t intersection = v2f_add(
				segment.vStart, v2f_mul(
					v2f_sub(segment.vEnd, segment.vStart), t
				)
			);

			fronts[(*numberOfFronts)++] = (Segment_t){
				.vStart=segment.vStart, .vEnd=intersection,
				.lineDefIndex=segment.lineDefIndex
			};
			backs[(*numberOfBacks)++] = (Segment_t){
				.vStart=intersection, .vEnd=segment.vEnd,
				.lineDefIndex=segment.lineDefIndex
			};
		}
	}
}


void bsp_evaluateSplit(
	const Split_t split, const Segment_t* segments, unsigned int numberOfSegments, //In
	unsigned int* numberOfFronts, unsigned int* numberOfSplits, unsigned int* numberOfBacks
) {
	for (unsigned int i=0u; i<numberOfSegments; i++) {
		Segment_t segment = segments[i];
		float side0 = bsp_signedDistance(split, segment.vStart);
		float side1 = bsp_signedDistance(split, segment.vEnd);

		if ((side0 > EPSILON) && (side1 > EPSILON)) {(*numberOfFronts)++;}
		else if ((side0 < -EPSILON) && (side1 < -EPSILON)) {(*numberOfBacks)++;}
		else {(*numberOfSplits)++;}
	}
}


#define SPLIT_SCALE 1.0e3f
SplitKey_t bsp_splitKey(const Split_t split) {
	return (SplitKey_t){
		.p=(Vec2i_t){
			.x=split.position.x*SPLIT_SCALE, .y=split.position.y*SPLIT_SCALE
		},
		.f=(Vec2i_t){
			.x=split.forward.x*SPLIT_SCALE, .y=split.forward.y*SPLIT_SCALE
		}
	};
}

int bsp_splitKeyEQU(const SplitKey_t a, const SplitKey_t b) {
	return (
		((a.p.x == b.p.x) && (a.p.y == b.p.y)) && 
		((a.f.x == b.f.x) && (a.f.y == b.f.y))
	);
}



int bsp_isInUsedSplitsSet(const Split_t split, const SplitKey_t* usedSplits, unsigned int numberOfUsedSplits) {
	if (numberOfUsedSplits==0) {return FALSE;}
	SplitKey_t thisKey = bsp_splitKey(split);
	for (unsigned int i=0u; i<numberOfUsedSplits; i++) {
		SplitKey_t key = usedSplits[i];
		if (bsp_splitKeyEQU(thisKey, key)) {
			return TRUE;
		}
	}
	return FALSE;
}


int bsp_countUniqueSplits(const Segment_t* segments, unsigned int numberOfSegments) {
    SplitKey_t keys[MAX_SPLITS];
    unsigned int count = 0u;

    for (unsigned int i=0u; i<numberOfSegments; i++) {
        Split_t s = {
            .position = segments[i].vStart,
            .forward = bsp_getDirection(segments[i])
        };

        SplitKey_t key = bsp_splitKey(s);

        int found = FALSE;
        for (unsigned int j=0u; j<count; j++) {
            if (bsp_splitKeyEQU(keys[j], key)) {
                found = TRUE;
                break;
            }
        }

        if (!found) {
            keys[count++] = key;
            if (count >= 2) {return count; /* Early exit */}
        }
    }

    return count;
}


int bsp_compareCandidates(const void* a, const void* b) {
    float sa = ((const Candidate_t*)a)->score;
    float sb = ((const Candidate_t*)b)->score;

    if (sa < sb) {return -1;}
    if (sa > sb) {return  1;}
    return 0; //EQU.
}

//////// BSP HELPER FUNCS ////////



//////// BSP INIT/DEL ////////

BSPnode_t bsp_buildRecurse(
	Segment_t* segments, unsigned int numberOfSegments,
	unsigned int depth,
	SplitKey_t* usedSplits, unsigned int numberOfUsedSplits
) {
	if (
		(numberOfSegments < MIN_SEGMENTS) ||
		(depth >= MAX_DEPTH) ||
		(bsp_countUniqueSplits(segments, numberOfSegments) < 2)
	) {
		return (BSPnode_t){
			.split=(Split_t){ //Generic split.
				.position=(Vec2f_t){.x=0.0f, .y=0.0f},
				.forward=(Vec2f_t){.x=0.0f, .y=1.0f}
			},
			.segments=segments, .numSegments=numberOfSegments,
			.isLeaf=TRUE, .isValid=TRUE,
			.forwardNode=(BSPnode_t*)(NULL),
			.behindNode=(BSPnode_t*)(NULL)
		};
	}


	Split_t candidateSplits[MAX_SPLITS];
	unsigned int numberOfSplits = 0u;
	bsp_pickCandidateSplits(
		segments, numberOfSegments,
		candidateSplits, &numberOfSplits
	);
	unsigned int numberOfCandidates = 0u;
	Candidate_t candidates[MAX_SPLITS];
	for (unsigned int i=0; i<numberOfSplits; i++) {
		Split_t split = candidateSplits[i];
		if (bsp_isInUsedSplitsSet(split, usedSplits, numberOfUsedSplits)) {continue; /* Don't re-use */}
		unsigned int front = 0u, splits = 0u, back = 0u;
		bsp_evaluateSplit(
			split, segments, numberOfSegments,
			&front, &splits, &back
		);

		float divisor = (float)(MAX(1, numberOfSegments));
		if (
			(((float)(splits) / divisor) > MAX_SPLIT_RATIO) ||
			(((float)(MAX(front, back)) / divisor) > MAX_SHARE_RATIO)
		) {continue; /* Not worth partitioning by. */}

		float score = bsp_scoringFunction(splits, front, back, depth);
		if (MIN(front, back) == 0) {score += 500; /* Heavily disincentivise empty sides. */}
		candidates[numberOfCandidates++] = (Candidate_t){
			.score=score, .split=split
		};
	}

	if (numberOfCandidates == 0u) {
		//No candidates were found, return a leaf node.
		return (BSPnode_t){
			.split=(Split_t){ //Generic split.
				.position=(Vec2f_t){.x=0.0f, .y=0.0f},
				.forward=(Vec2f_t){.x=0.0f, .y=1.0f}
			},
			.segments=segments, .numSegments=numberOfSegments,
			.isLeaf=TRUE, .isValid=TRUE,
			.forwardNode=(BSPnode_t*)(NULL),
			.behindNode=(BSPnode_t*)(NULL)
		};
	}


	qsort(candidates, numberOfCandidates, sizeof(Candidate_t), bsp_compareCandidates);
	Split_t bestSplit = candidates[0].split;

	Segment_t* fronts = malloc(numberOfSegments * sizeof(Segment_t));
	Segment_t* coplanars = malloc(numberOfSegments * sizeof(Segment_t));
	Segment_t* backs = malloc(numberOfSegments * sizeof(Segment_t));
	unsigned int numberOfFronts = 0u, numberOfCoplanars = 0u, numberOfBacks = 0u;
	bsp_splitSegments(
		bestSplit, segments, numberOfSegments,
		fronts, &numberOfFronts,
		coplanars, &numberOfCoplanars,
		backs, &numberOfBacks
	);


	//Create new dataset of used splits, so that each BRANCH propagates the set, but not the WHOLE TREE.
	unsigned int numberOfUsedSplitsFront = numberOfUsedSplits;
	unsigned int numberOfUsedSplitsBack = numberOfUsedSplits;
	SplitKey_t* splitsFront = malloc(MAX_SPLITS * sizeof(SplitKey_t));
	SplitKey_t* splitsBack  = malloc(MAX_SPLITS * sizeof(SplitKey_t));

	//Add old data to new datasets, if any exists.
	if (numberOfUsedSplits > 0u) {
		memcpy(splitsFront, usedSplits, numberOfUsedSplits*sizeof(SplitKey_t));
		memcpy(splitsBack,  usedSplits, numberOfUsedSplits*sizeof(SplitKey_t));
	}

	//Add this recursion's split to the datasets.
	splitsFront[numberOfUsedSplitsFront++] = bsp_splitKey(bestSplit);
	splitsBack[numberOfUsedSplitsBack++] = bsp_splitKey(bestSplit);

	BSPnode_t newNode = (BSPnode_t){
		.split=bestSplit,
		.segments=coplanars, .numSegments=numberOfCoplanars,
		.isLeaf=FALSE, .isValid=TRUE
	};

	//Recurse for front and back.
	*newNode.forwardNode = bsp_buildRecurse(
		fronts, numberOfFronts, depth+1,
		splitsFront, numberOfUsedSplitsFront
	);
	*newNode.behindNode = bsp_buildRecurse(
		backs, numberOfBacks, depth+1,
		splitsBack, numberOfUsedSplitsBack
	);

	free(splitsFront); free(fronts);
	free(splitsBack);  free(backs);

	return newNode;
}



BSPnode_t bsp_build(
	const Vec2f_t* vertices, unsigned int numberOfVertices,
	const LineDef_t* lineDefs, unsigned int numberOfLineDefs,
	int* success
) {
	//Build the BSP tree from a set of linedefs & vertices.
	//Check number of segments.
	unsigned int numberOfSegments = numberOfLineDefs; //Equal to start with.
	if ((numberOfSegments * 3u) >= MAX_SPLITS) {
		//Too many segments, won't fit in the datasets.
		*success = FALSE;
		return (BSPnode_t){
			.split=(Split_t){ //Generic split.
				.position=(Vec2f_t){.x=0.0f, .y=0.0f},
				.forward=(Vec2f_t){.x=0.0f, .y=1.0f}
			},
			.segments=NULL, .numSegments=0,
			.isLeaf=TRUE, .isValid=FALSE,
			.forwardNode=(BSPnode_t*)(NULL),
			.behindNode=(BSPnode_t*)(NULL)
		};
	}

	//Convert linedefs and vertices to segments.
	Segment_t* segments = calloc(numberOfSegments, sizeof(Segment_t));
	for (unsigned int i=0; i<numberOfLineDefs; i++) {
		const LineDef_t* ld = lineDefs+i;
		if ((ld->vStart >= numberOfVertices) || (ld->vEnd >= numberOfVertices)) {
			*success = FALSE;
			return (BSPnode_t){
				.split=(Split_t){ //Generic split.
					.position=(Vec2f_t){.x=0.0f, .y=0.0f},
					.forward=(Vec2f_t){.x=0.0f, .y=1.0f}
				},
				.segments=NULL, .numSegments=0,
				.isLeaf=TRUE, .isValid=FALSE,
				.forwardNode=(BSPnode_t*)(NULL),
				.behindNode=(BSPnode_t*)(NULL)
			};
		}

		segments[i] = (Segment_t){
			.vStart=vertices[ld->vStart],
			.vEnd=vertices[ld->vEnd],
			.lineDefIndex=i
		};
	}


	BSPnode_t rootNode = bsp_buildRecurse(
		segments, numberOfSegments, 0, //Starting depth of 0
		NULL, 0u //Don't bother creating a used splits dataset yet, length is 0. Always checked before access.
	);
	*success = TRUE; //Successfully created the tree.
	return rootNode;
}



void bsp_free(BSPnode_t* node) {
	if (!node || !(node->isValid)) {return; /* No need to process. */}
	if (!(node->isLeaf)) {
		//Recursively free nodes.
		bsp_free(node->forwardNode);
		bsp_free(node->behindNode);
	}
	free(node->segments); //Free this node's dataset.
	node->numSegments = 0u;
	node->isValid = FALSE; //Can no longer be used.
}

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
) {
	if (!(node->isValid)) {return; /* Do not try to process. */}

	BSPnode_t* firstNode;
	BSPnode_t* lastNode;
	if (!(node->isLeaf)) {
		//Find which node is closer.
		float dist = bsp_signedDistance(node->split, cameraPosition);
		if (dist > 0.0f) {
			//Front first, then back.
			firstNode = node->forwardNode;
			lastNode = node->behindNode;
		} else {
			//Back first, then front.
			firstNode = node->behindNode;
			lastNode = node->forwardNode;
		}

		//Recurse into the first node.
		bsp_walk(
			firstNode, lineDefs, cameraPosition, render
		);
	}

	//Process every segment within this node specifically.
	//If not leaf, then these are coplanar segments.
	for (unsigned int segmentIndex=0u; segmentIndex<node->numSegments; segmentIndex++) {
		render(node->segments[segmentIndex], lineDefs);
	}

	if (!(node->isLeaf)) {
		//Recurse into the other node.
		bsp_walk(
			lastNode, lineDefs, cameraPosition, render
		);
	}
}

//////// WALKING THE BSP ////////
