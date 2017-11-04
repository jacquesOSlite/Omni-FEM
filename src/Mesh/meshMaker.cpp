#include <Mesh/meshMaker.h>
#include <Mesh/GModel.h>
#include <iterator>

std::vector<std::vector<edgeLineShape>> meshMaker::findContours()
{
	std::vector<std::vector<edgeLineShape>> returnPaths; // This is what is filled up with the paths
	std::vector<edgeLineShape> pathContour; // This list contains the initial path
	std::vector<edgeLineShape> branchStack; // the stack of branches
	
	// Find the first line segment that is not visited
	edgeLineShape lineFound;// This line segment is the first one that is found in order to start a new contour path
	bool lineIsFound = false; // Boolean used to indicate if a line or arc was found.
	for(plf::colony<edgeLineShape>::iterator lineIterator = p_lineList->begin(); lineIterator != p_lineList->end(); lineIterator++)
	{
		if(!lineIterator->getVisitedStatus())
		{
			lineFound = *lineIterator;
			lineIsFound = true;
			lineIterator->setVisitedStatus(true);
			p_numberVisited++;
			break;
		}
	}
	
	if(!lineIsFound)
	{
		for(plf::colony<arcShape>::iterator arcIterator = p_arcList->begin(); arcIterator != p_arcList->end(); arcIterator++)
		{
			if(!arcIterator->getVisitedStatus())
			{
				lineFound = *arcIterator;
				lineIsFound = true;
				arcIterator->setVisitedStatus(true);
				p_numberVisited++;
				break;
			}
		}
	}
	
	if(lineIsFound)
		pathContour.push_back(lineFound);
	else
	{
		// Do something if it fails
	}
	
	bool closedContourFormed = false;
	
	do
	{
		std::vector<edgeLineShape> branches = getConnectedPaths(pathContour.back(), pathContour);
		
		// This part will check if any of the branches are already in the path contour vector.
		// If so, then this means a closed contour has been formed
		for(std::vector<edgeLineShape>::iterator listIterator = branches.begin(); listIterator != branches.end(); listIterator++)
		{
			for(std::vector<edgeLineShape>::iterator pathIterator = pathContour.begin(); pathIterator != pathContour.end(); pathIterator++)
			{
				if(*listIterator == *pathIterator)
				{
					closedContourFormed = true;
					break;
				}
			}
			
			if(closedContourFormed)
				break;
		}
		
		if(!closedContourFormed)
		{
			bool lineSet = false;
			pathContour.push_back(branches.at(0));// For now, we are going to assume that the first branch found is the next segment that creates the contour.
			
			branches.erase(branches.begin());// Now that the first element in the branches has been added to the path, we can remove it from the branches
			
			// Next, we will make sure that the branches are not already in another path vector
			// This does not apply to the first "branch" since we are creating another 
			// path and it is possible that 2 paths can intersect.
			// This will mainly prevent a brnach that is within anoth path contour from being added to the stack
			for(std::vector<edgeLineShape>::iterator edgeIterator = branches.begin(); edgeIterator != branches.end();)
			{
				if(edgeIterator->getVisitedStatus())
				{
					if(branches.size() == 1)
					{
						branches.clear();
						break;
					}
					else
					{
						branches.erase(edgeIterator++);
					}
				}
				else
					edgeIterator++;
			}
			
			/* This section will loop through both lists to make sure that the line segment that was added to the 
			 * contour path is updated to be visited
			 */ 
			for(plf::colony<edgeLineShape>::iterator lineIterator = p_lineList->begin(); lineIterator != p_lineList->end(); lineIterator++)
			{
				if(*lineIterator == pathContour.back())
				{
					lineSet = true;
					lineIterator->setVisitedStatus(true);
					p_numberVisited++;
					break;
				}
			}
			
			if(!lineSet)
			{
				for(plf::colony<arcShape>::iterator lineIterator = p_arcList->begin(); lineIterator != p_arcList->end(); lineIterator++)
				{
					if(*lineIterator == pathContour.back())
					{
						lineSet = true;
						lineIterator->setVisitedStatus(true);
						p_numberVisited++;
						break;
					}
				}
			}
			
			// First we will seach through the stack to see if the added branch was already in the stack
			// If so, remove it from the stack
			for(std::vector<edgeLineShape>::iterator stackIterator = branchStack.begin(); stackIterator != branchStack.end();)
			{
				if(pathContour.back() == *stackIterator)
				{
					if(*stackIterator == branchStack.back())
					{
						branchStack.clear();
						break;
					}

					else
						branchStack.erase(stackIterator++);
				}
				else
					stackIterator++;
			}
			
			// Now we are going to check if any of the branches are already in the stack
			// If so, then we do not need to add the branches to the stack later
			if(branches.size() > 0)
			{
				for(std::vector<edgeLineShape>::iterator stackIterator = branchStack.begin(); stackIterator != branchStack.end(); stackIterator++)
				{
					for(std::vector<edgeLineShape>::iterator branchIterator = branches.begin(); branchIterator != branches.end();)
					{
						if(*branchIterator == *stackIterator)
						{
							if(*branchIterator == branches.back())
							{
								branches.clear();
								break;
							}
							else
								branches.erase(branchIterator++);
						}
						else
							branchIterator++;
					}
					
					// If all of the branches are already in the stack, then we can stop checking the stack
					if(branches.size() == 0)
						break;
				}
			}
			
			// If there are any branches left, then go ahead and add them to the stack
			if(branches.size() > 0)
			{
				branchStack.reserve(branchStack.size() + branches.size());// Increase the size of the stack by the number of branches.
				branchStack.insert(branchStack.end(), branches.begin(), branches.end());
			}
		}
	} while(!closedContourFormed);
	
	removeDanglingLines(pathContour);
	
	returnPaths.push_back(pathContour);
	
	while(branchStack.size() > 0)
	{
		closedContourFormed = false;
		
		pathContour.clear();
		
		// Now we have to find the back path of the branch in order to add it to the new path vector
		for(int i = 0; i < returnPaths.size(); i++)
		{
			bool backPathFound = false;
			/*
			 * Starting from the previous path, we are going to loop through all of the lines 
			 * to see if the branch is connected to one of the lines of the previous contour. 
			 * If there is a common node, then this will save time. If not, then it will take a minute to 
			 * complete
			 */ 
			std::vector<edgeLineShape> tempPath = returnPaths[returnPaths.size() - i - 1];
			for(std::vector<edgeLineShape>::iterator pathIterator = tempPath.begin(); pathIterator != tempPath.end(); pathIterator++)
			{
				if(	pathIterator->getFirstNode() == branchStack.begin()->getFirstNode() || 
					pathIterator->getSecondNode() == branchStack.begin()->getFirstNode() || 
					pathIterator->getFirstNode() == branchStack.begin()->getSecondNode() || 
					pathIterator->getSecondNode() == branchStack.begin()->getSecondNode() )
				{
					// If the branch is connected to one of the nodes on the path, then we have found the back path!
					// Next, we add in the back path to the paht contour
					// and from there add in the branch.
					pathContour.reserve(std::distance(tempPath.begin(), pathIterator++));
					pathContour.assign(tempPath.begin(), pathIterator++);
					// Now we pop the first branch from the branch stack 
					pathContour.push_back(*branchStack.begin());
					
					branchStack.erase(branchStack.begin());
					
					backPathFound = true;
					
					break;
				}
			}
			
			if(backPathFound)
				break;
		}
		
		// Now that the back path has been found, we can procced to find the next connected edge
		// To do this, the search will be similiar to above
		// We will be performing the same logic to the branches within the stack
		do
		{
			std::vector<edgeLineShape> branches = getConnectedPaths(pathContour.back(), pathContour);
			
			// This part will check if any of the branches are already in the path contour vector.
			// If so, then this means a closed contour has been formed
			for(std::vector<edgeLineShape>::iterator listIterator = branches.begin(); listIterator != branches.end(); listIterator++)
			{
				for(std::vector<edgeLineShape>::iterator pathIterator = pathContour.begin(); pathIterator != pathContour.end(); pathIterator++)
				{
					if(*listIterator == *pathIterator)
					{
						closedContourFormed = true;
						break;
					}
				}
				
				if(closedContourFormed)
					break;
			}

			if(!closedContourFormed)
			{
				bool lineUpdated = false;
				pathContour.push_back(branches.at(0));// For now, we are going to assume that the first branch found is the next segment that creates the contour.
				
				branches.erase(branches.begin());// Now that the first element in the branches has been added to the path, we can remove it from the branches
			
				// Next, we will make sure that the branches are not already in another path vector
				// This does not apply to the first "branch" since we are creating another 
				// path and it is possible that 2 paths can intersect.
				// This will mainly prevent a brnach that is within anoth path contour from being added to the stack
				for(std::vector<edgeLineShape>::iterator edgeIterator = branches.begin(); edgeIterator != branches.end();)
				{
					if(edgeIterator->getVisitedStatus())
					{
						if(branches.size() == 1)
						{
							branches.clear();
							break;
						}
						else
						{
							branches.erase(edgeIterator++);
						}
					}
					else
						edgeIterator++;
				}
				
				/* This section will loop through both lists to make sure that the line segment that was added to the 
				 * contour path is updated to be visited
				 */ 
				for(plf::colony<edgeLineShape>::iterator lineIterator = p_lineList->begin(); lineIterator != p_lineList->end(); lineIterator++)
				{
					if(*lineIterator == pathContour.back())
					{
						lineUpdated = true;
						lineIterator->setVisitedStatus(true);
						p_numberVisited++;
						break;
					}
				}
				
				if(!lineUpdated)
				{
					for(plf::colony<arcShape>::iterator arcIterator = p_arcList->begin(); arcIterator != p_arcList->end(); arcIterator++)
					{
						if((edgeLineShape)*arcIterator == pathContour.back())
						{
							lineUpdated = true;
							arcIterator->setVisitedStatus(true);
							p_numberVisited++;
							break;
						}
					}
				}
				
				// First we will seach through the stack to see if the added branch was already in the stack
				// If so, remove it from the stack
				for(std::vector<edgeLineShape>::iterator stackIterator = branchStack.begin(); stackIterator != branchStack.end(); stackIterator++)
				{
					if(pathContour.back() == *stackIterator)
					{
						if(*stackIterator == branchStack.back())
						{
							branchStack.clear();
							break;
						}
						else
							branchStack.erase(stackIterator++);
					}
					else
						stackIterator++;
				}
				
				// Now we are going to check if any of the branches are already in the stack
				// If so, then we do not need to add the branches to the stack later
				if(branches.size() > 0)
				{
					for(std::vector<edgeLineShape>::iterator stackIterator = branchStack.begin(); stackIterator != branchStack.end(); stackIterator++)
					{
						for(std::vector<edgeLineShape>::iterator branchIterator = branches.begin(); branchIterator != branches.end();)
						{
							if(*branchIterator == *stackIterator)
							{
								if(*branchIterator == branches.back())
								{
									branches.clear();
									break;
								}
								else
									branches.erase(branchIterator++);
							}
							else
								branchIterator++;
						}
						
						// If all of the branches are already in the stack, then we can stop checking the stack
						if(branches.size() == 0)
							break;
					}
				}
				
				// If there are any branches left, then go ahead and add them to the stack
				if(branches.size() > 0)
				{
					branchStack.reserve(branchStack.size() + branches.size());// Increase the size of the stack by the number of branches.
					branchStack.insert(branchStack.end(), branches.begin(), branches.end());
				}
			}
		}while(!closedContourFormed);
		
		// Once a closed path has been found, we then need to make sure that the path is actually a closed contour
		// by removing any dangling lines which are lines that are in the path but do not make up the closed contour
		// Then, we add the path to the return vector.
		removeDanglingLines(pathContour);
		returnPaths.push_back(pathContour);
	}
	
	return returnPaths;
}



std::vector<edgeLineShape> meshMaker::getConnectedPaths(std::vector<edgeLineShape>::reference segment, std::vector<edgeLineShape> &pathVector)
{
	std::vector<edgeLineShape> returnList;
	node *branchNode;
	std::vector<edgeLineShape>::iterator previousAddedSegment; 
	
	if(pathVector.size() >= 2)
		previousAddedSegment = pathVector.end() - 2;
	else
		previousAddedSegment = pathVector.begin();
	
	// Determines what node we should be looking at
	if(pathVector.size() >= 2)
	{
		// FInd out which node is connected to the back
		if((*segment.getFirstNode() == *previousAddedSegment->getFirstNode()) || (*segment.getFirstNode() == *previousAddedSegment->getSecondNode()))
			branchNode = segment.getSecondNode();
		else
			branchNode = segment.getFirstNode();
	}
	else
		branchNode = segment.getFirstNode();// This is the case for the very first segment added
	
	//Find all of the lines that are connected to the segment
	for(plf::colony<edgeLineShape>::iterator lineIterator = p_lineList->begin(); lineIterator != p_lineList->end(); lineIterator++)
	{
		if(*lineIterator == segment || (pathVector.size() > 1 && (*lineIterator == *previousAddedSegment)))
			continue;
		
		// Checks to see if the first node on the line has already been scanned through for branches.
		// If so, then we need to move on to the second node.
		// If both of the nodes for the segment have already been visited then we can go ahead and
		// skip that segment
		if((*branchNode == *lineIterator->getFirstNode()) || (*branchNode == *lineIterator->getSecondNode()))
		{
			returnList.push_back(*lineIterator);
		}
	}
	
	// Find all of the arcs connected to the segment
	for(plf::colony<arcShape>::iterator arcIterator = p_arcList->begin(); arcIterator != p_arcList->end(); arcIterator++)
	{
		if((arcIterator->getArcID() == segment.getArcID()) || (pathVector.size() > 1 && (arcIterator->getArcID() == previousAddedSegment->getArcID())))
			continue;
			
		if((*branchNode == *arcIterator->getFirstNode()) || (*branchNode == *arcIterator->getSecondNode()))
		{
			returnList.push_back(*arcIterator);
		}
	}
	
	return returnList;
}



bool meshMaker::contourRecombination(std::vector<edgeLineShape> &contourPath1, std::vector<edgeLineShape> &contourPath2)
{
	bool returnSuccess = true;
	
	if(shareCommonEdge(contourPath1, contourPath2))
	{
		std::vector<edgeLineShape> tempPath;
		std::vector<edgeLineShape> localPath2Copy = contourPath2;
		
		/*
		 * First we will scan through the two lists in order to determine where the common edges are located.
		 * If a common edge is found, then we do not need to add it to the tempPath
		 * If the contourPath1 iterator is not in the second list, then we can go ahead and add it to the 
		 * tempPath vector.
		 */ 
		for(auto pathIterator1 : contourPath1)
		{
			bool segmentFound = false;
			for(auto pathIterator2 = localPath2Copy.begin(); pathIterator2 != localPath2Copy.end();)
			{
				if(pathIterator1 == *pathIterator2)
				{
					localPath2Copy.erase(pathIterator2++);
					segmentFound = true;
					break;
				}
				else
					pathIterator2++;
			}
			
			if(!segmentFound)
				tempPath.push_back(pathIterator1);
		}
		
		/*
		 * Any remaining size from the second path can now be added to the 
		 * tempPath vector
		 */ 
		if(localPath2Copy.size() > 0)
		{
			tempPath.reserve(tempPath.size() + localPath2Copy.size());
			tempPath.insert(tempPath.end(), localPath2Copy.begin(), localPath2Copy.end());
		}
		
		// Now we need to rearrange the temp path vector to make sure all of the 
		// edgeLineShapes are in connecting order
		for(auto pathIterator = tempPath.begin(); pathIterator != tempPath.end();)
		{
			edgeLineShape switchShape;
			// If we are already at the back, then we need to end the loop
			if(*pathIterator == tempPath.back())
				break;
				
			std::vector<edgeLineShape>::iterator oneNext = pathIterator + 1;
			// If the next iterator is connected to the current on, then continue on to the next iterator
			if(*pathIterator->getFirstNode() == *oneNext->getFirstNode() ||
				*pathIterator->getFirstNode() == *oneNext->getSecondNode() ||
				*pathIterator->getSecondNode() == *oneNext->getFirstNode() ||
				*pathIterator->getSecondNode() == *oneNext->getSecondNode())
			{
				pathIterator++;
			}
			else
			{
				for(std::vector<edgeLineShape>::iterator pathIterator2 = pathIterator + 2; pathIterator2 != tempPath.end(); pathIterator2++)
				{
					if(	*pathIterator->getFirstNode() == *pathIterator2->getFirstNode() ||
						*pathIterator->getFirstNode() == *pathIterator2->getSecondNode() ||
						*pathIterator->getSecondNode() == *pathIterator2->getFirstNode() ||
						*pathIterator->getSecondNode() == *pathIterator2->getSecondNode())
					{
						std::iter_swap(pathIterator, pathIterator2);
						break;
					}
					
				}
				pathIterator++;
			}
			
		}
		
		// The final checks
		
		if(isClosedContour(tempPath) && (tempPath.size() <= contourPath1.size() && tempPath.size() <= contourPath2.size()))
		{
			if(contourPath1.size() > contourPath2.size())
				contourPath1 = tempPath;
			else
				contourPath2 = tempPath;
				
			returnSuccess = true;
		}
		else
			returnSuccess = false;
	}
	else
		returnSuccess = false;
		
	return returnSuccess;
}



bool meshMaker::isClosedContour(std::vector<edgeLineShape> contour)
{
	if(	*contour.begin()->getFirstNode() == *contour.back().getFirstNode() ||
		*contour.begin()->getFirstNode() == *contour.back().getSecondNode() ||
		*contour.begin()->getSecondNode() == *contour.back().getFirstNode() ||
		*contour.begin()->getSecondNode() == *contour.back().getSecondNode()
		)
		return true;
	else
		return false;
}



void meshMaker::removeDanglingLines(std::vector<edgeLineShape> &contour)
{
	unsigned int numberConnections = 0;
	std::vector<edgeLineShape>::iterator temp = contour.end() - 2;
	for(std::vector<edgeLineShape>::iterator contourIterator = contour.begin(); contourIterator != contour.end(); contourIterator++)
	{
		/*
		 * Basically, the line itself and the line segment that the back is attached to
		 * Doesn't count since the previouse line segment in the list will always be
		 * attached to the one in back
		 */ 
		
		if(*contourIterator == contour.back() || (*contourIterator == *(temp)))
			continue;
		else
		{
			if(	*contourIterator->getFirstNode() == *contour.back().getFirstNode() || 
				*contourIterator->getSecondNode() == *contour.back().getFirstNode() || 
				*contourIterator->getFirstNode() == *contour.back().getSecondNode() || 
				*contourIterator->getSecondNode() == *contour.back().getSecondNode() )
			{
				numberConnections++;
			}
			
			if(numberConnections == 2)// Dangling lines found!
			{
				contour.erase(contour.begin(), contourIterator);
				break;
			}
		}
	}
}



void meshMaker::mesh()
{
	bool canMakeMesh = true;
	
	while((p_numberofLines != p_numberVisited) || (p_numberofLines > p_numberVisited))
	{
		std::vector<std::vector<edgeLineShape>> temp = findContours();
		
		p_closedContours.reserve(p_closedContours.size() + temp.size());
		p_closedContours.insert(p_closedContours.end(), temp.begin(), temp.end());
		
		for(auto contourIterator = p_closedContours.begin(); contourIterator != p_closedContours.end(); contourIterator++)
		{
			if((contourIterator + 1) != p_closedContours.end())
				contourRecombination(*contourIterator, *(contourIterator + 1));
			else
				break;
		}
	}
	
	// Another check here to ensure that all of the contours saved are actually closed contours
	// Must be done before making the mesh and failing
	for(auto contourIterator : p_closedContours)
	{
		if(!isClosedContour(contourIterator))
		{
			canMakeMesh = false;
			break;
		}
	}
	
	if(canMakeMesh)
	{
		std::vector<GVertex*> vertexModelList;
		std::vector<std::vector<std::vector<GEdge*>>> compeleteLineLoop;
		
		vertexModelList.reserve(p_nodeList->size());
		GModel meshModel;
		GmshInitialize();
		
		meshModel.setFactory("Gmsh");
		
		for(auto nodeIterator : *p_nodeList)
		{
			double test1 = nodeIterator.getCenterXCoordinate();
			double test2 = nodeIterator.getCenterYCoordinate();
			vertexModelList.push_back(meshModel.addVertex(test1, test2, 0.0, 0.1));
		}
		
		for(auto contourIterator : p_closedContours)
		{
			std::vector<GEdge*> contourLoop;
			
			std::vector<edgeLineShape> contour = contourIterator;
			for(auto lineIterator : contour)
			{
				GVertex *firstNode = nullptr;
				GVertex *secondNode = nullptr;
				
				//GVertex test;
				
				
				for(auto vertexIterator : vertexModelList)
				{
					double xValueVertex = vertexIterator->x();
					double yValueVertex = vertexIterator->y();
					double firstNodeCenterX = lineIterator.getFirstNode()->getCenterXCoordinate();
					double firstNodeCenterY = lineIterator.getFirstNode()->getCenterYCoordinate();
					double secondNodeCenterX = lineIterator.getSecondNode()->getCenterXCoordinate();
					double secondNodeCenterY = lineIterator.getSecondNode()->getCenterYCoordinate();
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
				
				if(lineIterator.isArc())
				{
					// Throw in the code here in order to create a Bspline within the mesh model
					//GEdge *test = meshModel->addBezier()
				}
				else
				{
					GEdge *temp = meshModel.addLine(firstNode, secondNode);
					//temp->meshAttributes.meshSize = 1000;
					temp->resetMeshAttributes();
				//	temp->meshAttributes.meshSize
					contourLoop.push_back(temp);
				}
				//delete firstNode;
				//delete secondNode;
			}
			
			std::vector<std::vector<GEdge*>> test;
			test.push_back(contourLoop);
			// Now, we need to test if the contour is a hole to another contour
			// Or, if the contour lies outside of a set of contours
			// Or if the contour is not related to any of the already added contours
			// And add it in the correct place of the complete line loop
			// For now, we will just add it to the model directly
			GFace *testFace = meshModel.addPlanarFace(test);
			//testFace->meshAttributes.meshSize = 1000;
			testFace->resetMeshAttributes();
			double temp = testFace->getMeshSize();
			//testFace->meshSize = 0.001;
			testFace->setMeshingAlgo(ALGO_2D_DELAUNAY); // Found in GmshDefines.h
			
		}
		
		CTX::instance()->mesh.recombineAll = 1;
		CTX::instance()->mesh.algo2d = ALGO_2D_DELAUNAY;
		CTX::instance()->mesh.algoSubdivide = 1;
		
		//CTX::instance()->mesh.algoRecombine = 1; // Setting to one will cause the program to perform the blossom algorthim for recombination
		
		meshModel.mesh(2);
		
		
		//meshModel.writeMSH("/home/phillip/Desktop/test.msh");
		
		meshModel.writeVTK("/home/phillip/Desktop/test.vtk");
		
		//meshModel.Addv
		
	}
	else
	{
		// Error out and let the user know that the mesh
		// can not be made
	}
	
	// TODO: Check for an errors?
	// TODO: Add in the interface to the GMSH API
	// TODO: Mesh the model
	// TODO: Return the mesh. The function return is void for now
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