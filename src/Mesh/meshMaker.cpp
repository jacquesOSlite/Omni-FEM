#include <Mesh/meshMaker.h>

#include <common/OmniFEMMessage.h>

#include <iterator>


closedPath meshMaker::findContour()
{
	closedPath foundPath;
	std::vector<closedPath> pathsVector;
	
	// Find the first line segment that is not visited
	bool lineIsFound = false; // Boolean used to indicate if a line or arc was found.
	for(plf::colony<edgeLineShape>::iterator lineIterator = p_lineList->begin(); lineIterator != p_lineList->end(); lineIterator++)
	{
		if(!lineIterator->getVisitedStatus() && !lineIterator->getSegmentProperty()->getHiddenState())
		{
			pathsVector.push_back(closedPath(*lineIterator));
			lineIsFound = true;
			break;
		}
	}
	
	if(!lineIsFound)
	{
		for(plf::colony<arcShape>::iterator arcIterator = p_arcList->begin(); arcIterator != p_arcList->end(); arcIterator++)
		{
			if(!arcIterator->getVisitedStatus() && !arcIterator->getSegmentProperty()->getHiddenState())
			{
				pathsVector.push_back(closedPath(*arcIterator));
				lineIsFound = true;
				break;
			}
		}
	}
	
	while(pathsVector.size() != 0)
	{
		// This area could be an area for bugs. 
		// The main reason is that what happens when the program adds a new path to the path vector 
		// and then increments the interator, does this invalid the interator?
		for(std::vector<closedPath>::iterator pathIterator = pathsVector.begin(); pathIterator != pathsVector.end();)
		{
			if(foundPath.getDistance() == 0 || pathIterator->getDistance() < foundPath.getDistance())
			{
				bool closedContourFound = false;
				bool branchInPaths = false;
				
				std::vector<edgeLineShape*> branches = getConnectedPaths(pathIterator->getClosedPath()->back(), pathIterator->getClosedPath());
				
				// Test to see if a closed contour is found
				// A closed contour occurs when one of the branches 
				// is the beginning edge of the closed path
				for(std::vector<edgeLineShape*>::iterator branchIterator = branches.begin(); branchIterator != branches.end(); branchIterator++)
				{
					if(**branchIterator == **(pathIterator->getClosedPath()->begin()))
					{
						closedContourFound = true;
						break;
					}
				}
				
				for(std::vector<edgeLineShape*>::iterator pathEdgeIterator = ++(pathIterator->getClosedPath()->begin()); pathEdgeIterator != pathIterator->getClosedPath()->end(); pathEdgeIterator++)
				{
					for(std::vector<edgeLineShape*>::iterator branchIterator = branches.begin(); branchIterator != branches.end();branchIterator++)
					{
						if(**pathEdgeIterator == **branchIterator)
						{
							// This means that the path has looped back on itself
							branchInPaths = true;
							break;
						}
					}
					
					if(branchInPaths)
						break;
				}
				
				if(branchInPaths)
				{
					pathsVector.erase(pathIterator);
					break;
				}
				else if(closedContourFound)
				{
					// If found, we are going to test if the 
					// found contour's distance is less then the current found path
					// distance
					if(pathIterator->getDistance() < foundPath.getDistance() || foundPath.getDistance() == 0)
					{
						// Replace the found path variable with the pathIterator as the new shortest path
						
						foundPath = *pathIterator;
					}
					
					pathsVector.erase(pathIterator);
					break;
				}
				else
				{

					// If the size of the branches is greater then 1, then we found some additional possible paths
					// Create another closed path object and add them to the paths vector as possible
					// solutions to finding the shortest path.
					if(branches.size() > 1)
					{
						closedPath masterPath = *pathIterator;
						
						pathIterator->addEdgeToPath(*branches.begin());
						branches.erase(branches.begin());
						
						while(branches.size() != 0)
						{
							closedPath addedPath = masterPath;
							
							addedPath.addEdgeToPath(*branches.begin());
							pathsVector.push_back(addedPath);
							branches.erase(branches.begin());
						}
						break;
					}
					else if(branches.size() == 1)
					{
						// Otherwise, add the one branch to the current path
						pathIterator->addEdgeToPath(*branches.begin());
						branches.clear();
					}
					
					pathIterator++;
					continue;
				}
			}
			else
			{
				// Since the path is no the shortest path,
				// remove the path from the vector
				pathsVector.erase(pathIterator);
				break;
			}
		}
	}
	
	return foundPath;
}


std::vector<edgeLineShape*> meshMaker::getConnectedPaths(std::vector<edgeLineShape*>::reference currentSegment, std::vector<edgeLineShape*> *pathVector)
{
	std::vector<edgeLineShape*> returnList;
	node *branchNode;
	std::vector<edgeLineShape*>::iterator previousAddedSegment;
	
	edgeLineShape *previouseSegmentPointer;
	edgeLineShape *currentSegmentPointer = currentSegment; // This part may not be necessary but it helps with the thinking
	
	if(pathVector->size() >= 2)
		previousAddedSegment = pathVector->end() - 2;
	else
		previousAddedSegment = pathVector->begin();
		
	previouseSegmentPointer = *previousAddedSegment;
	
	// Determines what node we should be looking at
	if(pathVector->size() >= 2)
	{
		// FInd out which node is connected to the back
		if((*(currentSegmentPointer->getFirstNode()) == *(previouseSegmentPointer->getFirstNode())) || (*(currentSegmentPointer->getFirstNode()) == *(previouseSegmentPointer->getSecondNode())))
			branchNode = currentSegmentPointer->getSecondNode();
		else
			branchNode = currentSegmentPointer->getFirstNode();
	}
	else
		branchNode = currentSegmentPointer->getFirstNode();// This is the case for the very first segment added
	
	//Find all of the lines that are connected to the segment
	for(plf::colony<edgeLineShape>::iterator lineIterator = p_lineList->begin(); lineIterator != p_lineList->end(); lineIterator++)
	{
		// Perform check to make sure that the segment and the previously added segment are not included
		// Perform check to make sure that the segment is not hidden
		if(*lineIterator == *currentSegmentPointer || (pathVector->size() > 1 && (*lineIterator == *previouseSegmentPointer)) && !lineIterator->getSegmentProperty()->getHiddenState())
			continue;
		
		// Checks to see if the first node on the line has already been scanned through for branches.
		// If so, then we need to move on to the second node.
		// If both of the nodes for the segment have already been visited then we can go ahead and
		// skip that segment
		if((*branchNode == *lineIterator->getFirstNode()) || (*branchNode == *lineIterator->getSecondNode()))
		{
			returnList.push_back(&(*lineIterator));
		}
	}
	
	// Find all of the arcs connected to the segment
	for(plf::colony<arcShape>::iterator arcIterator = p_arcList->begin(); arcIterator != p_arcList->end(); arcIterator++)
	{
		if((arcIterator->getArcID() == currentSegmentPointer->getArcID()) || (pathVector->size() > 1 && (arcIterator->getArcID() == previouseSegmentPointer->getArcID())) && !arcIterator->getSegmentProperty()->getHiddenState())
			continue;
			
		if((*branchNode == *arcIterator->getFirstNode()) || (*branchNode == *arcIterator->getSecondNode()))
		{
			returnList.push_back(&(*arcIterator));
		}
	}
	
	return returnList;
}



bool meshMaker::lineIntersectsLine(Vector P1, Vector P2, Vector P3, Vector P4)
{
	    /* This code was adapted from FEMM from FEmmeDoc.cpp line 728 BOOL CFemmeDoc::GetIntersection*/
    Vector pNode0, pNode1, iNode0, iNode1;
    Vector tempNode0, tempNode1;

    pNode0 = P1;
    pNode1 = P2;
    iNode0 = P3;
    iNode1 = P4;
    
    tempNode0 = iNode0;
    tempNode1 = iNode1;
    
    double ee = min(Vabs(pNode1 - pNode0), Vabs(iNode1 - iNode0)) * 1.0e-8;
    
    iNode0 = (iNode0 - pNode0) / (pNode1 - pNode0);
    iNode1 = (iNode1 - pNode0) / (pNode1 - pNode0);
    
    if(iNode0.getXComponent() <= 0 && iNode1.getXComponent() <= 0)
        return false;
    else if(iNode0.getXComponent() >= 1.0 && iNode1.getXComponent() >= 1.0)
        return false;
    else if(iNode0.getYComponent() <= 0 && iNode1.getYComponent() <= 0)
        return false;
    else if(iNode0.getYComponent() >= 0 && iNode1.getYComponent() >= 0)
        return false;
        
    double z = iNode0.getYComponent() / (iNode0 - iNode1).getYComponent();
    
    double x = ((1.0 - z) * iNode0 + z * iNode1).getXComponent();
    if((x < ee) || (x > (1.0 - ee)))
        return false;
        
  //  pNode0 = (1.0 - z) * tempNode0 + z * tempNode1;
    
 //   intersectionXPoint = pNode0.getXComponent();
 //   intersectionYPoint = pNode0.getYComponent();
    
    return true;
}

int meshMaker::lineIntersectsArc(Vector P1, Vector P2, arcShape intersectingArc)
{
	    /* Note: this function has not yet been verified to be working. Logical bugs could still exist */
    // This function was ported from CbeladrawDoc::GetLineArcIntersection
    
	Vector arcSegVec1, arcSegVec2, unitVec1, tempVec2, arcCenterPoint;
	Vector *pointVec;
	double distance, length, radius, z;
	int intersectionCounter = 0;
	
	arcSegVec1.Set(intersectingArc.getFirstNode()->getCenterXCoordinate(), intersectingArc.getFirstNode()->getCenterYCoordinate());
	arcSegVec2.Set(intersectingArc.getSecondNode()->getCenterXCoordinate(), intersectingArc.getSecondNode()->getCenterYCoordinate());
	
	distance = Vabs(arcSegVec2 - arcSegVec1);
	
    radius = intersectingArc.getRadius();
    
    arcCenterPoint.Set(intersectingArc.getCenterXCoordinate(), intersectingArc.getCenterYCoordinate());
    
    // Determining the distance between the line and the circle's center
	distance = Vabs(P2 - P1);
	unitVec1 = (P2 - P1) / distance;
	tempVec2 = (arcCenterPoint - P1) / unitVec1;
	
	if(fabs(tempVec2.getYComponent()) > radius)
		return 0;
		
	length = sqrt(pow(radius, 2) - pow(tempVec2.getYComponent(), 2));
	// If the line is a tangent, make it a tangent
	if((length / radius) < 1.0e-05)
	{
		pointVec[intersectionCounter] = P1 + tempVec2.getXComponent() * unitVec1;
		radius = ((pointVec[intersectionCounter] - P1) / unitVec1).getXComponent();
		z = Varg((pointVec[intersectionCounter] - arcCenterPoint) / (arcSegVec1 - arcCenterPoint));
		if((radius > 0) && (radius < distance) && (z > 0.0) && (z < (intersectingArc.getArcAngle() * PI / 180.0)))
			intersectionCounter++;
		delete pointVec;
		return intersectionCounter;
	}
	
    // First intersection
	pointVec[intersectionCounter] = P1 + (tempVec2.getXComponent() + length) * unitVec1;
	radius = ((pointVec[intersectionCounter] - P1) / unitVec1).getXComponent();
	z = Varg((pointVec[intersectionCounter] - arcCenterPoint) / (arcSegVec1 - arcCenterPoint));
	if((radius > 0) && (radius < distance) && (z > 0.0) && (z < (intersectingArc.getArcAngle() * PI / 180.0)))
    {
		intersectionCounter++;
    }
    
    // Second intersection
	pointVec[intersectionCounter] = P1 + (tempVec2.getXComponent() - length) * unitVec1;
	radius = ((pointVec[intersectionCounter] - P1) / unitVec1).getXComponent();
	z = Varg((pointVec[intersectionCounter] - arcCenterPoint) / (arcSegVec1 - arcCenterPoint));
	if((radius > 0) && (radius < distance) && (z > 0.0) && (z < (intersectingArc.getArcAngle() * PI / 180.0)))
    {
		intersectionCounter++;
    }
	
	delete pointVec;	
	return intersectionCounter;
}


void meshMaker::mesh(GModel *meshModel, meshSettings settings)
{
	bool meshCreated = false;

	GmshInitialize();
	
	OmniFEMMsg::instance()->MsgStatus("Creating GMSH Geometry from Omni-FEM geometry");
	OmniFEMMsg::instance()->MsgStatus("Finding contours");
	
	while(p_numberVisited < p_numberofLines)
	{
		closedPath contourPath = findContour();
		
		if(contourPath.getDistance() == 0)
		{
			//TODO: Add in error handling
		}
		else
			p_closedContourPaths.push_back(contourPath);
		
		for(std::vector<edgeLineShape*>::iterator edgeIterator = contourPath.getClosedPath()->begin(); edgeIterator != contourPath.getClosedPath()->end(); edgeIterator++)
		{
			if(!(*edgeIterator)->getVisitedStatus())
			{
				(*edgeIterator)->setVisitedStatus(true);
				p_numberVisited++;
			}
		}
	}
	
	OmniFEMMsg::instance()->MsgStatus("Contours found");
	
	CTX::instance()->mesh.recombinationTestNewStrat = 0;
	CTX::instance()->mesh.nProc = 0;
	CTX::instance()->mesh.nbProc = 0;
//	CTX::instance()->lc = 2.3323807574381199;
	CTX::instance()->lc = 2.5;
	CTX::instance()->mesh.optimizeLloyd = 5;
	CTX::instance()->mesh.multiplePasses = 1;
	
	/* These are settings that will remain constant */
	
	CTX::instance()->mesh.algoSubdivide = 1; // The 1 here is to specify that we are only concerned with quads
	CTX::instance()->mesh.secondOrderIncomplete = 0; // Always use complete meshes
	CTX::instance()->mesh.lcExtendFromBoundary = 1; // By setting to 1 the mesh size will extend from the boundary. This needs more research. This maybe an option that the user speficies
	CTX::instance()->mesh.recombineAll = 1; // Make sure to recombine all triangles into quads
	
	CTX::instance()->mesh.algo3d = 1; // Not really necessary?
	CTX::instance()->mesh.lcFromPoints = 0;// Obtain the characteristic length from the nodes (This doesn't work too well)
	CTX::instance()->mesh.lcFromCurvature = 1;// Obtain the characteristic length (mesh size) from the edges of the face (This works better)
	
	/* These are the settings that are set by the user */
	
	switch(settings.getRemeshParameter())
	{
		case MeshParametrization::MESH_PARAM_HARMONIC:
			CTX::instance()->mesh.remeshParam = 0;
			break;
		case MeshParametrization::MESH_PARAM_CONFORMAL:
			CTX::instance()->mesh.remeshParam = 1;
			break;
		case MeshParametrization::MESH_PARAM_RBFHARMONIC:
			CTX::instance()->mesh.remeshParam = 2;
			break;
	}
	
	if(settings.getAutoRemeshingState())
		CTX::instance()->mesh.remeshAlgo = 1;
	else
		CTX::instance()->mesh.remeshAlgo = 0;
		
	switch(settings.getMeshAlgorithm())
	{
		case MeshAlgorthim::MESH_ALGO_AUTOMATIC:
			CTX::instance()->mesh.algo2d = ALGO_2D_AUTO;
			break;
		case MeshAlgorthim::MESH_ALGO_MESHADAPT:
			CTX::instance()->mesh.algo2d = ALGO_2D_MESHADAPT;
			break;
		case MeshAlgorthim::MESH_ALGO_DELAUNAY:
			CTX::instance()->mesh.algo2d = ALGO_2D_DELAUNAY;
			break;
		case MeshAlgorthim::MESH_ALGO_FRONTAL:
			CTX::instance()->mesh.algo2d = ALGO_2D_FRONTAL;
			break;
	}
	
	if(settings.getBlossomRecombinationState())
		CTX::instance()->mesh.algoRecombine = 1; // Setting to one will cause the program to perform the blossom algorthim for recombination
	else
		CTX::instance()->mesh.algoRecombine = 0;
		
	CTX::instance()->mesh.nbSmoothing = (int)settings.getSmoothingSteps();
	
	CTX::instance()->mesh.lcFactor = settings.getElementSizeFactor();
	CTX::instance()->mesh.lcMin = settings.getMinElementSize();
	CTX::instance()->mesh.lcMax = settings.getMaxElementSize();
	
	CTX::instance()->mesh.order = settings.getElementOrder();
	
	contextMeshOptions testMesh = CTX::instance()->mesh;
	
	/* Now we go to conver Omni-FEM's geometry data structure into the data structure for GMSH */
	// lcFactor->0.29999999999 lcExtendFromBoundary->1 algo3D-> 1
	OmniFEMMsg::instance()->MsgStatus("Verfied Mesh");
	std::vector<GVertex*> vertexModelList;
	std::vector<std::vector<std::vector<GEdge*>>> compeleteLineLoop;
	
	vertexModelList.reserve(p_nodeList->size());

	meshModel->setFactory("Gmsh");
	
	OmniFEMMsg::instance()->MsgStatus("Adding in GMSH Vertices");
	for(auto nodeIterator : *p_nodeList)
	{
		double test1 = nodeIterator.getCenterXCoordinate();
		double test2 = nodeIterator.getCenterYCoordinate();
		GVertex *temp = meshModel->addVertex(test1, test2, 0.0, 1.0);
		vertexModelList.push_back(temp);
	//	vertexModelList
	}
	
	/* Now we create the faces */
	OmniFEMMsg::instance()->MsgStatus("Adding in GMSH faces");
	
	for(auto contourIterator : p_closedContourPaths)
	{
		/* This section here will transfer the found closed contours into a vector 
		 * of GEdges
		 */ 
		std::vector<GEdge*> contourLoop;
		
		closedPath contour = contourIterator;
		
		for(std::vector<edgeLineShape*>::iterator lineIterator = contour.getClosedPath()->begin(); lineIterator != contour.getClosedPath()->end(); lineIterator++)
		{
			GVertex *firstNode = nullptr;
			GVertex *secondNode = nullptr;
			//numberOfIntersections = 0;
	
			for(auto vertexIterator : vertexModelList)
			{
				double xValueVertex = vertexIterator->x();
				double yValueVertex = vertexIterator->y();
				double firstNodeCenterX = (*lineIterator)->getFirstNode()->getCenterXCoordinate();
				double firstNodeCenterY = (*lineIterator)->getFirstNode()->getCenterYCoordinate();
				double secondNodeCenterX = (*lineIterator)->getSecondNode()->getCenterXCoordinate();
				double secondNodeCenterY = (*lineIterator)->getSecondNode()->getCenterYCoordinate();
				if(((xValueVertex == firstNodeCenterX) && (yValueVertex == firstNodeCenterY)) ||
					((xValueVertex == secondNodeCenterX) && (yValueVertex == secondNodeCenterY)))
					{
						if(xValueVertex == firstNodeCenterX && yValueVertex == firstNodeCenterY)
							firstNode = vertexIterator;
						else if (xValueVertex == secondNodeCenterX && yValueVertex == secondNodeCenterY)
							secondNode = vertexIterator;
						
						if(firstNode && secondNode)
							break;
					}
			}
			
			if((*lineIterator)->isArc())
			{
				// Throw in the code here in order to create a Bspline within the mesh model
				// look at the code: bool GEO_Internals::addBezier(int &tag, const std::vector<int> &vertexTags)
				// located in GMODELIO_GEO.cpp file on added in a Bezier curve to the model
				//GEdge *test = meshModel->addBezier()
			}
			else
			{
				GEdge *temp = meshModel->addLine(firstNode, secondNode);
			//	meshModel->
		//		temp->meshAttributes.meshSize = 1.0;
		//		temp->meshAttributes.meshSize = p_blockLabelList->begin()->getProperty()->getMeshSize();
				if(!(*lineIterator)->getSegmentProperty()->getMeshAutoState())
				{
					temp->meshAttributes.meshSize = (*lineIterator)->getSegmentProperty()->getElementSizeAlongLine();
				}
				
				contourLoop.push_back(temp);
			}
		}
		
		std::vector<std::vector<GEdge*>> test;
		
		// Now, we need to test if the contour is a hole to another contour
		// Or, if the contour lies outside of a set of contours
		// Or if the contour is not related to any of the already added contours
		// And add it in the correct place of the complete line loop
		// For now, we will just add it to the model directly
		test.push_back(contourLoop);
		
		// Add the contour to the face selection
		GFace *testFace = meshModel->addPlanarFace(test);
		
		if(settings.getStructuredState())
		{
			testFace->meshAttributes.method = 1;// Sets the face to be transfinite
			
			switch(settings.getMeshArrangment())
			{
				case StructuredArrangement::ARRANGMENT_LEFT:
					testFace->meshAttributes.transfiniteArrangement = -1;
					break;
				case StructuredArrangement::ARRANGMENT_RIGHT:
					testFace->meshAttributes.transfiniteArrangement = 1;
					break;
				case StructuredArrangement::ARRANGMENT_ALTERNATED:
					testFace->meshAttributes.transfiniteArrangement = 2;
					break;
				default:
					testFace->meshAttributes.transfiniteArrangement = -1;
					break;
			}
		}
		else
		{
			testFace->meshAttributes.method = 2;
			testFace->meshAttributes.transfiniteArrangement = 0;
		}
			  
		
		/* TODO: Next, we need to create an algorthim that will determine which block labels lie
		 * within the boundary of the face and from there, extra the mesh size (or no Mesh) 
		 */ 
		SBoundingBox3d aBox = testFace->bounds();
		
		/* Now we enter into the ray casting algorthim.
		 * The idea behind this is that a point from far awa is "drawn". A 
		 * line connecting the blocklabel and this far point is "created". 
		 * The algorithm will then determine the number of times that the 
		 * line intersects with a line of the face. If it is an odd number
		 * of times, then this means that the block label exists within the face
		 * if it is an even number of times, then this means that the block label 
		 * exists outside of the face
		 */ 
		for(auto blockIterator : *p_blockLabelList)
		{
			int numberOfIntersections = 0;
			
			for(auto rayContourIterator : test)
			{
				std::vector<GEdge*> temp = rayContourIterator;
				bool isConnectedToCommonEdgeAndAccounted = false; 
				for(auto lineSegmentIterator : temp)
				{
					bool test = false;
					Vector blockPoint = Vector(blockIterator.getCenterXCoordinate(), blockIterator.getCenterYCoordinate());
					Vector maxBBPoint = Vector(Vector(aBox.max().x(), aBox.max().y()));// The factor here will ensure that the point is not on the same point as a vertex
					Vector beginPoint = Vector(lineSegmentIterator->getBeginVertex()->x(), lineSegmentIterator->getBeginVertex()->y());
					Vector endPoint = Vector(lineSegmentIterator->getEndVertex()->x(), lineSegmentIterator->getEndVertex()->y());
					
					if(	blockPoint.getYComponent() < std::max(lineSegmentIterator->getBeginVertex()->y(), lineSegmentIterator->getEndVertex()->y()) &&
						blockPoint.getYComponent() > std::min(lineSegmentIterator->getBeginVertex()->y(), lineSegmentIterator->getEndVertex()->y()))
					{
						maxBBPoint.Set(maxBBPoint.getXComponent() + 0.1, blockPoint.getYComponent());
						
						test = lineIntersectsLine( blockPoint, maxBBPoint, beginPoint, endPoint);
						if(!test)
							test = lineIntersectsLine(beginPoint, endPoint, blockPoint, maxBBPoint);
					}
					
					if(test)
						numberOfIntersections++;
				}
			}
			
			/* Or we can throw in a loop here to loop through all of the arcs to see if there is an intersection here
			 */ 
			 
			if(numberOfIntersections % 2 == 1)
			{
				meshSize testEnum = blockIterator.getProperty()->getMeshsizeType();
				if(blockIterator.getProperty()->getMeshsizeType() != meshSize::MESH_NONE_)
				{
					
					double checkMesh = blockIterator.getProperty()->getMeshSize();
					for(int i = 0; i < contourLoop.size(); i++)
					{
						if(contourLoop[i]->meshAttributes.meshSize == MAX_LC)
							contourLoop[i]->meshAttributes.meshSize = blockIterator.getProperty()->getMeshSize();
					}
				//	testFace->meshAttributes.meshSize = blockIterator.getProperty()->getMeshSize();	
					testFace->meshAttributes.meshSize = 1.0;	
				}
				else
					testFace->meshAttributes.method = MESH_NONE;
				
				break;
			}
		}
	}
	
	/* As a side note,this current method for constructing the GMSH geometry will not work.
	 * The order needs to be this:
	 * 1) Create the list of vertices. Convert all Omni-FEM nodes to GMSH vertices
	 * 2) Convert all Omni-FEM lines/arcs into GMSH edges. Save in a list which groups them based on the face that they are in
	 * At the end of step 2, we should have a vector will all of the closed contours containing the GMSH edges
	 * 3) Construct the faces of the model by indentifying if the contour lies within the another contour and construct the face vector
	 * 4) Add face to the GMSH geometry
	 */ 
	
	OmniFEMMsg::instance()->MsgStatus("Meshing GMSH geometry");
	
	for(int i = 0; i < CTX::instance()->mesh.multiplePasses; i++)
	{
		OmniFEMMsg::instance()->MsgStatus("Performing pass " + std::to_string(i + 1) + " of " + std::to_string(CTX::instance()->mesh.multiplePasses));
		meshModel->mesh(2);
	}
	
	// Next set any output mesh options
	// such as different files to output the mesh. Be it VTK or some other format
	OmniFEMMsg::instance()->MsgStatus("Saving Mesh file");
	meshModel->writeVTK("/home/phillip/Desktop/test.vtk");
	//meshModel.getMesh
	
	// TODO: Check for an errors?
	// TODO: Add in the interface to the GMSH API
	// TODO: Mesh the model
	// TODO: Return the mesh. The function return is void for now
	
	// No matter what happens, we need to reset the visited state for arcs and lines 
	// lines visited state back to false!
	for(plf::colony<edgeLineShape>::iterator lineIterator = p_lineList->begin(); lineIterator != p_lineList->end(); lineIterator++)
	{
		lineIterator->setVisitedStatus(false);
	}
	
	for(plf::colony<arcShape>::iterator arcIterator = p_arcList->begin(); arcIterator != p_arcList->end(); arcIterator++)
	{
		arcIterator->setVisitedStatus(false);
	}

	if(meshModel->getNumMeshVertices() > 0)
		meshModel->indexMeshVertices(true, 0, true);
	
	OmniFEMMsg::instance()->MsgStatus("Meshing Finished");
}



bool meshMaker::shareCommonEdge(std::vector<edgeLineShape> path1, std::vector<edgeLineShape> path2)
{
	bool commonFound = false;
	
	for(auto pathIterator1 : path1)
	{
		for(auto pathIterator2 : path2)
		{
			if(pathIterator2 == pathIterator1)
			{
				commonFound = true;
				break;
			}
		}
		
		if(commonFound)
			break;
	}
	
	return commonFound;
}