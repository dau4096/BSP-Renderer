"BSPbuilder.py"
import glm;


####### CLASSES ########
class LineDef:
	#Generic linedef class. Has vertices and such.
	def __init__(self, v0:glm.vec2, v1:glm.vec2, ID:int, colour:glm.ivec3=glm.ivec3(255, 0, 255)):
		self.v0:glm.vec2 = v0; #Presently stores the vertices directly, will index into some vertex list later.
		self.v1:glm.vec2 = v1; #^^^
		self.ID:int = ID; #Index of this linedef
		self.colour:glm.ivec3 = colour;


class Segment:
	#Used for the BSP building function, instead of LineDefs. Has less data (In this case it's nearly identical, but in the actual system it would not be.)
	def __init__(self, v0:glm.vec2, v1:glm.vec2, ldID:int):
		self.v0:glm.vec2 = v0;
		self.v1:glm.vec2 = v1;
		self.ldID:int = ldID; #ID of the parent linedef it was formed from.


class Split:
	#Infinite line (bidirectional).
	def __init__(self, position:glm.vec2=glm.vec2(0.0, 0.0), forward:glm.vec2=glm.vec2(0.0, 1.0)):
		self.position:glm.vec2 = position; #Origin
		self.forward:glm.vec2 = forward;   #Direction of "Forward" (All positions in the forward half are dot-positive.)

	def __str__(self) -> str:
		return f"<Split [Origin: ({self.position.x}, {self.position.y}), forwardDirection: ({self.forward.x}, {self.forward.y})]>";


class BSPnode:
	#Used in the BSP tree, to store segments and the splitting plane.
	def __init__(self, split:Split, segments:list[Segment], nodes:tuple[any, any]|None=None):
		self.split:Split = split; #Plane it was split along
		self.segments:list[Segment] = segments; #List of included segments.
		self.leaf:bool = nodes is None;
		self.nodes:tuple[any, any]|None = nodes; #The 2 child nodes.

	def getChildNode(self, position:glm.vec2) -> tuple[any, bool]:
		if (self.leaf): return self, True; #Is a leaf, end here.
		if (positionInFrontOf(self.split, position)): #Front node
			return self.nodes[0];
		else: return self.nodes[1];

	def format(self, indentation:str="") -> str:
		if (self.leaf):
			return f"{indentation}<Leaf [NumSegments: {len(self.segments)}]>"
		else:
			childIndent:str = indentation+'    ';
			return (
				f"{indentation}<Node [\n"
				f"{childIndent}Split: {self.split}\n"
				f"{childIndent}Children: [\n"
				f"{self.nodes[0].format(childIndent)}\n"
				f"{self.nodes[1].format(childIndent)}\n"
				f"{childIndent}]\n"
				f"{indentation}]>"
			);

####### CLASSES ########


def recurseTreeGetNode(startNode:BSPnode, position:glm.vec2) -> BSPnode:
	foundLeaf:bool = False;
	node:BSPnode = startNode;
	while (not foundLeaf):
		node, foundLeaf = node.getChildNode(position);
	return node;



def signedDistance(split:Split, position:glm.vec2) -> float:
	return glm.dot(position - split.position, split.forward);



def scoringFunction(numberOfSplits:int, numberOfFronts:int, numberOfBacks:int) -> int:
	#Takes in the number of splits and so on, to evaluate how good a split is.
	return (numberOfSplits * 20) + (abs(numberOfFronts - numberOfBacks));


def getDir(seg:Segment) -> glm.vec2:
	delta:glm.vec2 = seg.v1 - seg.v0;
	if (glm.length(delta) < 1.0e-6):
	    return glm.vec2(0.0, 1.0); #Arbirary return.
	delta = glm.normalize(delta);
	return glm.vec2(-delta.y, delta.x);


def pickCandidateSplits(segments:list[Segment]) -> list[Split]:
	return [
		Split(seg.v0, getDir(seg))
		for seg in segments
	]



EPSILON:float = 1.0e-5;
def splitSegments(split:Split, segments:list[Segment]) -> tuple[list[Segment], list[Segment]]: #Front, Back
	front:list[Segment] = [];
	back:list[Segment] = [];


	for seg in segments:
		d0:float = signedDistance(split, seg.v0);
		d1:float = signedDistance(split, seg.v1);

		if ((d0 > EPSILON) and (d1 > EPSILON)):
			front.append(seg);
		elif ((d0 < -EPSILON) and (d1 < -EPSILON)):
			back.append(seg);
		elif ((abs(d0) <= EPSILON) and (abs(d1) <= EPSILON)):
		    #Coplanar, pick front arbitrarily.
		    front.append(seg)
		    continue;
		else:
			#Seg crosses plane, split into two.
			t:float = d0 / (d0 - d1);
			intersection:glm.vec2 = seg.v0 + t * (seg.v1 - seg.v0);

			front.append(Segment(seg.v0, intersection, seg.ldID));
			back.append(Segment(intersection, seg.v1, seg.ldID));

	return front, back;



def evaluateSplit(split, segments):
	splits:int = 0;
	front:int = 0;
	back:int = 0;

	for seg in segments:
		side0:float = signedDistance(split, seg.v0);
		side1:float = signedDistance(split, seg.v1);

		if ((side0 > EPSILON) and (side1 > EPSILON)):
			front += 1;
		elif ((side0 < -EPSILON) and (side1 < -EPSILON)):
			back += 1;
		else:
			splits += 1;

	return splits, front, back;



def buildBSP(segments:list[Segment]) -> BSPnode:
	if (len(segments) <= 3):
		#Less than some minimum number.
		return BSPnode(
			Split(), #Arbitrary split.
			segments #All segments.
		);

	candidateSplits:list[Split] = pickCandidateSplits(segments);
	candidates = [];
	for split in candidateSplits:
		splits, front, back = evaluateSplit(split, segments);
		score = scoringFunction(splits, front, back);
		candidates.append((score, split));


	if (not candidates):
		return BSPnode(
			Split(), #Arbitrary split.
			segments #All segments.
		);

	candidates.sort(key=lambda x: x[0]);
	bestSplit = candidates[0][1];

	front, back = splitSegments(bestSplit, segments);

	return BSPnode(
		bestSplit, [], #Empty dataset.
		[
			buildBSP(front), #Recurse
			buildBSP(back)   #^^^
		]
	);





def convertLineDefsToSegments(lineDefs:list[LineDef]) -> list[Segment]:
	segments:list[Segment] = [];
	for ld in lineDefs:
		segments.append(Segment(ld.v0, ld.v1, ld.ID));
	return segments;



lineDefs:list[LineDef] = [
	LineDef(glm.vec2(-10.0, 10.0), glm.vec2( 10.0, 10.0), 0),
	LineDef(glm.vec2( 10.0, 10.0), glm.vec2( 15.0,-10.0), 1),
	LineDef(glm.vec2( 15.0,-10.0), glm.vec2(-17.5,-10.0), 2),
	LineDef(glm.vec2(-17.5,-10.0), glm.vec2(-20.0,  0.0), 3),
	LineDef(glm.vec2(-20.0,  0.0), glm.vec2(-10.0, 10.0), 4),
	LineDef(glm.vec2(-17.5,-10.0), glm.vec2(-30.0,-10.0), 5),
	LineDef(glm.vec2(-30.0,-10.0), glm.vec2(-25.0,  0.0), 6),
	LineDef(glm.vec2(-25.0,  0.0), glm.vec2(-20.0,  0.0), 7)
];

segments:list[Segment] = convertLineDefsToSegments(lineDefs);
BSPtreeRootNode:BSPnode = buildBSP(segments);

print(BSPtreeRootNode.format());