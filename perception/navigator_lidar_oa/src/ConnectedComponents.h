////////////////////////////////////////////////////////////
//
// Connected components for ROS
//
////////////////////////////////////////////////////////////
#ifndef CONNECTEDCOMPONENTS_H
#define CONNECTEDCOMPONENTS_H

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include "OccupancyGrid.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct objectStats
{
	void update(int row, int col, cell z)
	{
		//std::cout << "Stats: " << z.min << "," << z.max << std::endl;
		/*if (first) {
			minRow = maxRow = row;
			minCol = maxCol = col;
			minHeight = z.min;
			maxHeight = z.max;
		} else {
			minRow = std::min(row,minRow);
			minCol = std::min(col,minCol);
			maxRow = std::max(row,maxRow);
			maxCol = std::max(col,maxCol);
			minHeight = std::min(z.min,minHeight);
			maxHeight = std::max(z.max.maxHeight);
			//if (row < minRow) { minRow = row; }
			//if (row > maxRow) { maxRow = row; }
			//if (col < minCol) { minCol = col; }
			//if (col > maxCol) { maxCol = col; }
			//if (z.min < minHeight) { minHeight = z.min; }
			//if (z.max > maxHeight) { maxHeight = z.max; }
		}
		first = false;*/
	}
	void insertPersist(const std::deque<LidarBeam> &strikes) {
		geometry_msgs::Point32 p32;
		for (auto strike : strikes) {
			p32.x = strike.x;
			p32.y = strike.y;
			p32.z = strike.z;
			strikesPersist.push_back(p32);
			intensityPersist.push_back(strike.i);
		}
	}
	void insertFrame(const std::vector<LidarBeam> &strikes) {
		geometry_msgs::Point32 p32;
		for (auto strike : strikes) {
			p32.x = strike.x;
			p32.y = strike.y;
			p32.z = strike.z;
			strikesFrame.push_back(p32);
			intensityFrame.push_back(strike.i);
		}		
	}
	bool first = true;
	float minRow, maxRow, minCol, maxCol, minHeight, maxHeight;
	//std::vector<LidarBeam> beams;
	std::vector<geometry_msgs::Point32> strikesPersist;
	std::vector<geometry_msgs::Point32> strikesFrame;
	std::vector<uint32_t> intensityPersist;
	std::vector<uint32_t> intensityFrame;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct objectMessage
{
	geometry_msgs::Point position;
	geometry_msgs::Vector3 scale;
	std::vector<geometry_msgs::Point32> strikesPersist;
	std::vector<geometry_msgs::Point32> strikesFrame;
	std::vector<uint32_t> intensityPersist;
	std::vector<uint32_t> intensityFrame;
	std_msgs::ColorRGBA color;
	int id = -1;
	std::string name = "unknown";
	uint32_t pclInliers = 0;
	geometry_msgs::Vector3 normal;
	float minHeightFromLidar,maxHeightFromLidar;
	bool current = false;
	bool locked = false;
	bool real = true;
	ros::Time age;
    std::array<size_t,5> confidence{ {0,0,0,0,0} };
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RCLabel
{
	RCLabel(int r, int c, int l) : row(r), col(c), label(l) {}
	int row,col,label;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector< std::vector<int> > ConnectedComponents(OccupancyGrid &ogrid, std::vector<objectMessage> &objects, const double MIN_OBJECT_SEPERATION_DISTANCE)
{
	//std::cout << "STARTING CONNECTED COMPONENTS" << std::endl;

	std::vector<std::vector<int> > cc(ogrid.ROI_SIZE, std::vector<int>(ogrid.ROI_SIZE,0));
	const int NEIGHBORS = 4;
	int row_neighbor[NEIGHBORS] = {0,-1,1,-1};
	int col_neighbor[NEIGHBORS] = {-1,0,-1,-1};

	int label = 0;

	std::map<int,int> labelMap;

	for (int row = 1; row < ogrid.ROI_SIZE-1; ++row) {
		for (int col = 1; col < ogrid.ROI_SIZE-1; ++col) {
			//Is this a good pixel?
			if (ogrid.ogridBinary[row][col]) {
				//Check neighbors to the right and down
				std::vector<RCLabel> neighbors;
				for (int kk = 0; kk < NEIGHBORS; ++kk) {
					int nrow = row_neighbor[kk]+row;
					int ncol = col_neighbor[kk]+col;					
					//if (nrow >= 0 && ncol >= 0) {
						if ( cc[nrow][ncol] ) {
							neighbors.push_back(RCLabel(nrow,ncol,cc[nrow][ncol]));
						}
					//}
				}
				
				//Was this the first neighbor?
				if (neighbors.size() == 0) {
					++label;
					//std::cout << "New label at " << row << "," << col << " with label " << label << std::endl;
					cc[row][col] = label;
					labelMap[label] = label;
				} else {
					RCLabel min_label = *min_element(neighbors.begin(),neighbors.end(),[](RCLabel &left, RCLabel &right) { return left.label < right.label; });
					//int max_label = *max_element(neighbors.begin(),neighbors.end());
					//std::cout << "Choosing minimum label of " << min_label << " - " << max_label << " at " << row << "," << col << std::endl;
					cc[row][col] = min_label.label;
					//4 way
					////if ( labelMap.count(max_label) == 0 || labelMap[max_label] > min_label) { 
					for (auto ii : neighbors) {
						if (labelMap[ii.label] > min_label.label) {
							labelMap[ii.label] = min_label.label; 
						}
					}
				}
			}
		}
	}

	//Organize labels
	std::set<int> ids;
	for (auto ii : labelMap)  {
		int index = ii.first, match = ii.second;
		//std::cout << "old label " << ii.first << " becomes " << ii.second << std::endl;
		while (true) {
			if (labelMap[match] != match && labelMap[match] > 0) {
				labelMap[index] = labelMap[match]; match = labelMap[match];
			} else {
				break;
			}
		}
		ids.insert(match);
		//std::cout << "new label " << index << " becomes " << match << std::endl;
	}

	//Pass 2
	std::map<int,objectStats> mapObjects;
	for (int row = 0; row < ogrid.ROI_SIZE; ++row) {
		for (int col = 0; col < ogrid.ROI_SIZE; ++col) {
			if (cc[row][col]) { 
				cc[row][col] = labelMap[cc[row][col]]; 
				mapObjects[cc[row][col]].update(row,col,ogrid.ogrid[row + ogrid.boatRow - ogrid.ROI_SIZE/2][col + ogrid.boatCol - ogrid.ROI_SIZE/2]);
				int r = row + ogrid.boatRow - ogrid.ROI_SIZE/2, c = col + ogrid.boatCol - ogrid.ROI_SIZE/2;
				mapObjects[cc[row][col]].insertPersist(ogrid.pointCloudTable[r*ogrid.GRID_SIZE+c].q);
				mapObjects[cc[row][col]].insertFrame(ogrid.pointCloudTable_Uno[r*ogrid.GRID_SIZE+c]);
			}
			//std::cout << cc[ii][jj] << " ";
		}
		//std::cout << std::endl;
	}

	//Re-organize obstacles, how many connections to count as an obstacle?
	objects.clear();
	for (auto ii : mapObjects)  {
		if (ii.second.strikesPersist.size() >= 3) {
			bool isNewObject = true;
			objectMessage obj;
			float dx = (ii.second.maxCol-ii.second.minCol)+0.001; obj.scale.x = dx*ogrid.VOXEL_SIZE_METERS;
			float dy = (ii.second.maxRow-ii.second.minRow)+0.001; obj.scale.y = dy*ogrid.VOXEL_SIZE_METERS;
			float dz = (ii.second.maxHeight - ii.second.minHeight)+0.001; obj.scale.z = dz;
			obj.position.x = (dx/2+ii.second.minCol - ogrid.ROI_SIZE/2)*ogrid.VOXEL_SIZE_METERS + ogrid.lidarPos.x;
			obj.position.y = (dy/2+ii.second.minRow - ogrid.ROI_SIZE/2)*ogrid.VOXEL_SIZE_METERS + ogrid.lidarPos.y;
			obj.position.z =  dz/2 + ii.second.minHeight;
			obj.strikesPersist = ii.second.strikesPersist;
			obj.strikesFrame = ii.second.strikesFrame;
			obj.intensityFrame = ii.second.intensityFrame;
			obj.intensityPersist = ii.second.intensityPersist;
			obj.minHeightFromLidar = ii.second.minHeight-ogrid.lidarPos.z;
			obj.maxHeightFromLidar = ii.second.maxHeight-ogrid.lidarPos.z;
			obj.age = ros::Time::now();
			//obj.color = ii.second.color; //Eventually work with color

			//Is this object really part of another one?
			for (auto &jj : objects) {
				auto distance = sqrt(pow(obj.position.x-jj.position.x,2) + pow(obj.position.y-jj.position.y,2)  );
				if (distance <= MIN_OBJECT_SEPERATION_DISTANCE) {
					ROS_INFO_STREAM("LIDAR | Merging object together: " << jj.position.x << "," << jj.position.y << "," << jj.position.z << " vs " << obj.position.x << "," << obj.position.y << "," << obj.position.z );
					ROS_INFO_STREAM("LIDAR | Merging object together: " << jj.strikesPersist.size() << "," << jj.strikesFrame.size() << " vs " << obj.strikesPersist.size() << "," << obj.strikesFrame.size() );
					//ros::Duration(5).sleep();
					jj.position.x = (obj.position.x+jj.position.x)/2;
					jj.position.y = (obj.position.y+jj.position.y)/2;
					jj.position.z = (obj.position.z+jj.position.z)/2;
					jj.strikesPersist.insert(jj.strikesPersist.end(),obj.strikesPersist.begin(),obj.strikesPersist.end() );
					jj.strikesFrame.insert(jj.strikesFrame.end(),obj.strikesFrame.begin(),obj.strikesFrame.end() );
					jj.intensityPersist.insert(jj.intensityPersist.end(),obj.intensityPersist.begin(),obj.intensityPersist.end() );
					jj.intensityFrame.insert(jj.intensityFrame.end(),obj.intensityFrame.begin(),obj.intensityFrame.end() );
					jj.minHeightFromLidar = std::min(jj.minHeightFromLidar,obj.minHeightFromLidar);
					jj.maxHeightFromLidar = std::max(jj.maxHeightFromLidar,obj.maxHeightFromLidar);
					isNewObject = false;
					break;
				}
			}

			if (obj.scale.z >= ogrid.objectMinHeight && isNewObject) {
				objects.push_back(obj);
			}
			//ROS_INFO_STREAM(newId << " -> " << ob.position.x << "," << ob.position.y << "," << ob.position.z << "|" << ob.scale.x << "," << ob.scale.y << "," << ob.scale.z);
		}
	}

	//std::cout << "FINISHED CONNECTED COMPONENTS" << std::endl;
	return cc;
}
#endif
