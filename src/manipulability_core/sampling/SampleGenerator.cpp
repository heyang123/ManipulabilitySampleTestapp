
#include "SampleGenerator.h"
#include "SampleGeneratorVisitor_ABC.h"
#include "Sampling/Sample.h"
#include "kinematic/Tree.h"
#include "kinematic/Robot.h"
#include "kinematic/Joint.h"
#include "Pi.h"
#include "filters\Filter_ABC.h"
#include "world/Obstacle.h"

#include <vector>
#include <time.h>
#include <map>

#include "MatrixDefs.h"

#include "Octree.h"

using namespace manip_core;
using namespace matrices;

// quad-tree related stuff
//#include "Rennes1\SpatialDataStructure\RegularTree.h"
//#include "Rennes1\SpatialDataStructure\Selectors\Triangle3D.h"
//#include <Rennes1/Math/Vector3.h>

//using namespace Rennes1;
//using namespace Rennes1::Math;
//using namespace Rennes1::SpatialDataStructure;
//using namespace Rennes1::SpatialDataStructure::Selectors;

#include <iostream>
using namespace std;
namespace tree
{
	typedef matrices::Vector3 Vector3f;
	typedef size_t EntityId;
	typedef std::vector<EntityId> T_Id;
	typedef T_Id::const_iterator CIT_Id;

	const Vector3f minVals(-10., -10., -10.);
	const Vector3f maxVals(10., 10., 10.);
	const int pointsPerNodes = 10;

	const double distanceExtrusion = 0.02;

	struct RegularTree
	{
		Octree * octree;

		RegularTree() {
			octree = new Octree();
		}
		~RegularTree(){}

		size_t insert(const Vector3f& point)
		{
			return octree->insertToCloud(point);
		}

		void initTree(){
			octree->init();
		}

		T_Id select(const Triangle3Df& obstacle)
		{
			return octree->search(obstacle, distanceExtrusion);
		}

	private:
		std::vector<Vector3f> points_;
	};

	// those numerics have to be double ( library constraint )
	typedef RegularTree RTree3f;
}
// end tree namepsace

void MakeTriangle(const Robot& robot, Tree& tree, const matrices::Vector3& p1, const matrices::Vector3& p2, const matrices::Vector3& p3, vector<Triangle3Df> &triangles)
{
	matrices::Matrix4 robotCoord = robot.ToRobotCoordinates();
	matrices::Vector3 treePos(tree.GetPosition());
	//robotCoord.block(0,3,3,1) = robotCoord.block(0,3,3,1) - treePos;
	matrices::Vector3 p11 = matrices::matrix4TimesVect3(robotCoord, p1) - treePos;
	matrices::Vector3 p21 = matrices::matrix4TimesVect3(robotCoord, p2) - treePos;
	matrices::Vector3 p31 = matrices::matrix4TimesVect3(robotCoord, p3) - treePos;
	// passing vector coordinates into robot coordinates
	tree::Vector3f v1(p11.x(), p11.y(), p11.z());
	tree::Vector3f v2(p21.x(), p21.y(), p21.z());
	tree::Vector3f v3(p31.x(), p31.y(), p31.z());
	triangles.push_back(*(new Triangle3Df(v1, v2, v3)));
}

void MakeTriangles(const Robot& robot, Tree& tree, const Obstacle& obstacle, vector<Triangle3Df> &triangles){
	const matrices::Vector3& p1 = obstacle.GetP1();
	const matrices::Vector3& p2 = obstacle.GetP2();
	const matrices::Vector3& p3 = obstacle.GetP3();
	const matrices::Vector3& p4 = obstacle.GetP4();

	MakeTriangle(robot, tree, p1, p2, p3, triangles);
	MakeTriangle(robot, tree, p1, p4, p3, triangles);
}

void GenerateJointAngle(Joint* joint)
{
	int minTheta, maxTheta;

	minTheta = (int)(joint->GetMinTheta()*RadiansToDegrees);
	maxTheta = (int)(joint->GetMaxTheta()*RadiansToDegrees);

	joint->SetTheta((rand() % (maxTheta - minTheta + 1) + minTheta) * DegreesToRadians);
}

struct PImpl
{
	//typedef std::vector<Sample> LSamples;
	//typedef std::vector<LSamples> LLSamples;
	typedef std::vector<tree::RTree3f*> T_Trees;
	typedef std::map<tree::EntityId, Sample> T_IdMatches;
	typedef std::vector<T_IdMatches> LLSamples;
	typedef T_IdMatches::iterator T_IdMatches_IT;
	typedef T_IdMatches::const_iterator T_IdMatches_CIT;

	PImpl()
	{
		// NOTHING
	}

	~PImpl()
	{
		for (T_Trees::iterator it = trees_.begin(); it != trees_.end(); ++it)
		{
			delete (*it);
		}
	}

	void GenerateSample(T_IdMatches& samples, tree::RTree3f * rtree, Tree& tree)
	{
		Joint* j = tree.GetRoot();
		while (j)
		{
			GenerateJointAngle(j);
			j = j->pChild_;
		}
		tree.Compute(); tree.ComputeJacobian();
		//samples.push_back(Sample(tree));
		// inserting end effector position into tree
		Sample sample(tree);
		matrices::Vector3 tmp(sample.GetPosition());
		tree::Vector3f rTreePos(tmp.x(), tmp.y(), tmp.z());
		tree::EntityId id = rtree->insert(rTreePos);
		T_IdMatches_IT it = samples.find(id);
		if (it == samples.end())
		{
			samples.insert(std::make_pair(id, sample));
		}
	}
	LLSamples allSamples_;
	T_Trees trees_;
};

SampleGenerator *SampleGenerator::instance = 0;

SampleGenerator::SampleGenerator()
: pImpl_(new PImpl)
{
	srand((unsigned int)(time(0))); //Init Random generation
}

SampleGenerator::~SampleGenerator()
{
	// NOTHING
}

void SampleGenerator::GenerateSamples(Tree& tree, int nbSamples)
{
	assert(nbSamples > 0);
	PImpl::T_IdMatches samples;
	Tree::TREE_ID id = tree.GetTemplateId();
	if (pImpl_->allSamples_.size() == id) // TODO this sucks, entries have to be created in sequential order
	{
		pImpl_->allSamples_.push_back(samples);
		tree::RTree3f * rTree = new tree::RTree3f();
		pImpl_->trees_.push_back(rTree);
		Sample init(tree);
		//pImpl_->Reset();
		for (int i = 0; i < nbSamples; ++i)
		{
			pImpl_->GenerateSample(pImpl_->allSamples_[id], rTree, tree);
		}

		rTree->initTree();
		init.LoadIntoTree(tree);
	}
}

void SampleGenerator::GenerateSamples(Robot& robot, int nbSamples)
{
	for (unsigned int i = 0; i < robot.GetNumTrees(); ++i)
	{
		GenerateSamples(*(robot.GetTree(i)), nbSamples);
	}
}
void SampleGenerator::Request(const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor, const Filter_ABC& filter) const
{
	for (PImpl::T_IdMatches_IT it = pImpl_->allSamples_[tree.GetTemplateId()].begin(); it != pImpl_->allSamples_[tree.GetTemplateId()].end(); ++it)
	{
		if (filter.ApplyFilter(it->second))
			visitor->Visit(robot, tree, it->second);
	}
}


void SampleGenerator::Request(const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor) const
{
	for (PImpl::T_IdMatches_IT it = pImpl_->allSamples_[tree.GetTemplateId()].begin(); it != pImpl_->allSamples_[tree.GetTemplateId()].end(); ++it)
	{
		visitor->Visit(robot, tree, it->second);
	}
}
/*
void TriangleRequest(const SampleGenerator& generator, const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor, const Filter_ABC& filter, const tree::RTree3f* rTree, tree::T_Id& selected, PImpl::T_IdMatches& concerned, tree::Triangle3Df* selector, const Obstacle& obstacle)
{
rTree->select(*selector, selected);
PImpl::T_IdMatches_IT itMatches;
for(tree::CIT_Id it = selected.begin(); it != selected.end(); ++ it)
{
itMatches = concerned.find(*it);
if(itMatches != concerned.end())
{
generator.Request(robot, tree, visitor, filter, itMatches->second, obstacle);
}
}
}*/

void SampleGenerator::Request(const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor, const Filter_ABC& filter, Sample& sample, const Obstacle& obstacle) const
{
	if (filter.ApplyFilter(sample))
		visitor->Visit(robot, tree, sample, obstacle);
}

/*
void SampleGenerator::Request(const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor, const Filter_ABC& filter, const Obstacle& obstacle) const
{
tree::RTree3f* rTree = pImpl_->trees_[tree.GetTemplateId()];
tree::T_Id selected;
PImpl::T_IdMatches& concerned = pImpl_->allSamples_[tree.GetTemplateId()];
TriangleRequest(*this, robot, tree, visitor, filter, rTree, selected, concerned, tree::MakeTriangle(robot, tree, obstacle.GetP1(), obstacle.GetP2(), obstacle.GetP3()), obstacle);
TriangleRequest(*this, robot, tree, visitor, filter, rTree, selected, concerned, tree::MakeTriangle(robot, tree, obstacle.GetP1(), obstacle.GetP4(), obstacle.GetP3()), obstacle);
}*/


void SampleGenerator::Request(const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor, const Filter_ABC& filter, const Obstacle& obstacle) const
{
	tree::RTree3f* rTree = pImpl_->trees_[tree.GetId()];
	vector<Triangle3Df> triangles;
	MakeTriangles(robot, tree, obstacle, triangles);

	tree::T_Id selected;

	for (int i = 0; i < triangles.size(); i++){
		tree::T_Id cur = rTree->select(triangles[i]);
		selected.insert(selected.end(), cur.begin(), cur.end());
	}

	PImpl::T_IdMatches& concerned = pImpl_->allSamples_[tree.GetId()];
	PImpl::T_IdMatches_IT itMatches;
	for (tree::CIT_Id it = selected.begin(); it != selected.end(); ++it)
	{
		itMatches = concerned.find(*it);
		if (itMatches != concerned.end())
		{
			this->Request(robot, tree, visitor, filter, itMatches->second, obstacle);
		}
	}
}