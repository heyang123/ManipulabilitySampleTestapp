
#include "SampleGenerator.h"
#include "SampleGeneratorVisitor_ABC.h"
#include "Sampling/Sample.h"
#include "kinematic/Tree.h"
#include "kinematic/Robot.h"
#include "kinematic/Joint.h"
#include "Pi.h"
#include "Filter_ABC.h"
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

	struct Triangle3Df
	{
		Triangle3Df(const tree::Vector3f& a, const tree::Vector3f& b, const tree::Vector3f& c, const double d)
		: a_(a), b_(b), c_(c), d_(d) {}

		~Triangle3Df(){}

		const tree::Vector3f a_;
		const tree::Vector3f b_;
		const tree::Vector3f c_;
		const double d_;
	};

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

		T_Id select(const Obstacle& obstacle)
		{
			double abcsquared = pow(obstacle.GetA(), 2) + pow(obstacle.GetB(), 2) + pow(obstacle.GetC(), 2);
			double x = (obstacle.GetA()*obstacle.GetD()) / abcsquared;
			double y = (obstacle.GetB()*obstacle.GetD()) / abcsquared;
			double z = (obstacle.GetC()*obstacle.GetD()) / abcsquared;
			Vector3 closestPoint(x, y, z);
			return octree->search(closestPoint, distanceExtrusion);
		}

	private:
		std::vector<Vector3f> points_;
	};

	// those numerics have to be double ( library constraint )
	typedef RegularTree RTree3f;
}

void GenerateJointAngle(Joint* joint)
{
	int minTheta, maxTheta;

	minTheta = (int)(joint->GetMinTheta()*RadiansToDegrees);
	maxTheta = (int)(joint->GetMaxTheta()*RadiansToDegrees);

	joint->SetTheta((rand() % (maxTheta-minTheta + 1) + minTheta) * DegreesToRadians);
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
		for(T_Trees::iterator it = trees_.begin(); it!= trees_.end(); ++it)
		{
			delete (*it);
		}
	}
	
	void GenerateSample(T_IdMatches& samples, tree::RTree3f * rtree, Tree& tree)
	{
		Joint* j = tree.GetRoot();
		while(j)
		{
			GenerateJointAngle(j);
			j = j->pChild_;
		}
		tree.Compute();tree.ComputeJacobian();
		//samples.push_back(Sample(tree));
		// inserting end effector position into tree
		Sample sample(tree);
		matrices::Vector3 tmp(sample.GetPosition());
		tree::Vector3f rTreePos(tmp.x(), tmp.y(), tmp.z());
		tree::EntityId id = rtree->insert(rTreePos);
		T_IdMatches_IT it = samples.find(id);
		if(it == samples.end())
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
	if(pImpl_->allSamples_.size() == id) // TODO this sucks, entries have to be created in sequential order
	{
		pImpl_->allSamples_.push_back(samples);
		tree::RTree3f * rTree = new tree::RTree3f();
		pImpl_->trees_.push_back(rTree);
		Sample init(tree);
		//pImpl_->Reset();
		for(int i = 0; i < nbSamples; ++i)
		{
			pImpl_->GenerateSample(pImpl_->allSamples_[id], rTree, tree);
		}
		init.LoadIntoTree(tree);
		}
}

void SampleGenerator::GenerateSamples(Robot& robot, int nbSamples)
{
	for(unsigned int i = 0; i < robot.GetNumTrees(); ++i)
	{
		GenerateSamples(*(robot.GetTree(i)), nbSamples);
	}
}
void SampleGenerator::Request(const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor, const Filter_ABC& filter) const
{
	for(PImpl::T_IdMatches_IT it = pImpl_->allSamples_[tree.GetTemplateId()].begin(); it != pImpl_->allSamples_[tree.GetTemplateId()].end(); ++it)
	{
		if(filter.ApplyFilter(it->second))
			visitor->Visit(robot, tree, it->second);
	}
}


void SampleGenerator::Request(const Robot& robot, Tree& tree, SampleGeneratorVisitor_ABC* visitor) const
{
	for(PImpl::T_IdMatches_IT it = pImpl_->allSamples_[tree.GetTemplateId()].begin(); it != pImpl_->allSamples_[tree.GetTemplateId()].end(); ++it)
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
	if(filter.ApplyFilter(sample))
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
	tree::T_Id selected = rTree->select(obstacle);
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