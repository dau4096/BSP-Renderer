"BSPbuilder.py"
import glm;
from pprint import pprint;


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

	def __repr__(self) -> str:
		return f"<Segment [Start: ({self.v0.x}, {self.v0.y}), End: ({self.v1.x}, {self.v1.y}), LineDef ID: {self.ldID}]>";


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

	def format(self, indentation:str="") -> str:
		if (self.leaf):
			return f"{indentation}<Leaf [NumSegments: {len(self.segments)}]>"
		else:
			childIndent:str = indentation+'        ';
			return (
				f"{indentation}<Node [\n"
				f"{indentation}    Split: {self.split}\n"
				f"{indentation}    Children: [\n"
				f"{self.nodes[0].format(childIndent)}\n"
				f"{self.nodes[1].format(childIndent)}\n"
				f"{indentation}    ]\n"
				f"{indentation}]>"
			);

####### CLASSES ########



def signedDistance(split:Split, position:glm.vec2) -> float:
	return glm.dot(position - split.position, split.forward);



def scoringFunction(splits:int, front:int, back:int, depth:int, total:int) -> float:
	if (total == 0):
		return 1.0e9;

	splitCost:float = splits / total;
	imbalance:float = abs(front - back) / total;
	balancePenalty:float = float(abs(front - back));

	return (
		(splitCost * 2.0) +
		(imbalance * 5.0) +
		(balancePenalty * 1.0) +
		(depth * 0.2)
	);


def getDir(seg:Segment) -> glm.vec2:
	delta:glm.vec2 = seg.v1 - seg.v0;
	if (glm.length(delta) < 1.0e-6):
		return glm.vec2(0.0, 1.0); #Arbirary return.
	delta = glm.normalize(delta);
	return glm.vec2(-delta.y, delta.x);


def pickCandidateSplits(segments:list[Segment]) -> list[Split]:
	splits:list[Split] = [];
	for seg in segments:
		dir:glm.vec2 = getDir(seg);

		#Seg-aligned
		splits.append(Split(seg.v0, dir));

		#Midpoint
		mid:glm.vec2 = (seg.v0 + seg.v1) * 0.5;
		splits.append(Split(mid, dir));

		#Normal-aligned
		normal:glm.vec2 = glm.vec2(-dir.y, dir.x);
		splits.append(Split(mid, normal));
	return splits;



EPSILON:float = 1.0e-5;
def splitSegments(split:Split, segments:list[Segment]) -> tuple[list[Segment], list[Segment], list[Segment]]: #Front, Coplanar, Back
	front:list[Segment] = [];
	back:list[Segment] = [];
	coplanar:list[Segment] = [];


	for seg in segments:
		d0:float = signedDistance(split, seg.v0);
		d1:float = signedDistance(split, seg.v1);
		if (abs(d0) <= EPSILON): d0 = 0
		if (abs(d1) <= EPSILON): d1 = 0

		if ((d0 > EPSILON) and (d1 > EPSILON)):
			front.append(seg);
		elif ((d0 < -EPSILON) and (d1 < -EPSILON)):
			back.append(seg);
		elif ((abs(d0) <= EPSILON) and (abs(d1) <= EPSILON)):
			#Coplanar segment.
			coplanar.append(seg);
		else:
			#Seg crosses plane, split into two.
			t:float = d0 / (d0 - d1);
			if (t <= EPSILON): 
				back.append(seg);
				continue;
			if (t >= 1-EPSILON): 
				front.append(seg);
				continue;
			intersection:glm.vec2 = seg.v0 + t * (seg.v1 - seg.v0);

			front.append(Segment(seg.v0, intersection, seg.ldID));
			back.append(Segment(intersection, seg.v1, seg.ldID));

	return front, coplanar, back;



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



def splitKey(split:Split):
	return (
		round(split.position.x, 3), round(split.position.y, 3),
		round(split.forward.x, 3), round(split.forward.y, 3)
	);



MIN_SEGMENTS = 3;
MAX_SPLIT_RATIO = 0.75;
MAX_DEPTH = 20;
WORST_ACCEPTABLE_SCORE = 20;
def buildBSP(segments:list[Segment], depth:int=0, usedSplits:set=None) -> BSPnode:
	if (usedSplits is None): usedSplits = set();
	if (
		(len(segments) <= MIN_SEGMENTS) or
		(depth >= MAX_DEPTH) or
		(len(set(splitKey(Split(seg.v0, getDir(seg))) for seg in segments)) < 2)
	):
		#Less than some minimum number.
		return BSPnode(
			Split(), #Arbitrary split.
			segments #All segments.
		);

	candidateSplits:list[Split] = pickCandidateSplits(segments);
	candidates = [];
	for split in candidateSplits:
		if (splitKey(split) in usedSplits): continue;
		splits, front, back = evaluateSplit(split, segments);

		if (
			((splits / max(1, len(segments))) > MAX_SPLIT_RATIO) or
			(max(front, back) / max(1, len(segments)) > 0.95)
		):
			#Not worth partitioning by.
			continue;

		score = scoringFunction(splits, front, back, depth, front+back);
		if (min(front, back) == 0): score += 500;
		candidates.append((score, split));


	if (not candidates):
		return BSPnode(
			Split(), #Arbitrary split.
			segments #All segments.
		);

	candidates.sort(key=lambda x: x[0]);
	"""if (candidates[0] > WORST_ACCEPTABLE_SCORE):
		return BSPnode(
			Split(),
			segments
		); #Not worth persuing."""
	bestSplit = candidates[0][1];

	front, coplanar, back = splitSegments(bestSplit, segments);
	usedSplits.add(splitKey(bestSplit));

	return BSPnode(
		bestSplit, coplanar,
		[
			buildBSP(front, depth+1, usedSplits.copy()), #Recurse
			buildBSP(back, depth+1, usedSplits.copy())   #^^^
		]
	);





def convertLineDefsToSegments(lineDefs:list[LineDef]) -> list[Segment]:
	segments:list[Segment] = [];
	for ld in lineDefs:
		segments.append(Segment(ld.v0, ld.v1, ld.ID));
	return segments;



def walkTree(node:BSPnode, position:glm.vec2, segments:list[Segment]) -> list[Segment]:
	if (node.leaf):
		segments.extend(node.segments);
		return segments;


	side:float = signedDistance(node.split, position);
	if (side >= 0): #"Front" first
		segments = walkTree(node.nodes[0], position, segments); #Front
		segments.extend(node.segments); #Coplanar
		segments = walkTree(node.nodes[1], position, segments); #Back
	else: #"Back" first
		segments = walkTree(node.nodes[1], position, segments); #Back
		segments.extend(node.segments); #Coplanar
		segments = walkTree(node.nodes[0], position, segments); #Front
	return segments;



#Example linedefs.
lineDefs:list[LineDef] = [
	LineDef(glm.vec2(-10.0,  10.0), glm.vec2( 10.0,  10.0), 0),
	LineDef(glm.vec2( 10.0,  10.0), glm.vec2( 15.0, -10.0), 1),
	LineDef(glm.vec2( 15.0, -10.0), glm.vec2(-17.5, -10.0), 2),
	LineDef(glm.vec2(-17.5, -10.0), glm.vec2(-20.0,   0.0), 3),
	LineDef(glm.vec2(-20.0,   0.0), glm.vec2(-10.0,  10.0), 4),
	LineDef(glm.vec2(-17.5, -10.0), glm.vec2(-30.0, -10.0), 5),
	LineDef(glm.vec2(-30.0, -10.0), glm.vec2(-25.0,   0.0), 6),
	LineDef(glm.vec2(-25.0,   0.0), glm.vec2(-20.0,   0.0), 7)
];

segments:list[Segment] = convertLineDefsToSegments(lineDefs);
BSPtreeRootNode:BSPnode = buildBSP(segments); #Create the tree.


#Display the final tree.
print(BSPtreeRootNode.format());


#Try walking through it like the renderer would.
pprint(walkTree(BSPtreeRootNode, glm.vec2(0.0, 0.0), []));
