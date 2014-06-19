
#include "CollisionHandlerColdet.h"
#include "kinematic/Robot.h"
#include "kinematic/Tree.h"
#include "kinematic/Joint.h"
#include "world/Obstacle.h"

#include <ozcollide\ozcollide.h>
#include <ozcollide\aabbtree_poly.h>
#include <ozcollide\aabbtreepoly_builder.h>
#include <ozcollide\Vec3f.h>

// Collision handler based on
// ozCollide 1.1.1 library

using namespace std;
using namespace ozcollide;
using namespace manip_core::enums;

struct CollisionHandlerPImpl
{
	CollisionHandlerPImpl()
		:instantiated_(false)
	{
		// TODO
	}

	~CollisionHandlerPImpl()
	{
		if(instantiated_) // free tris and vertices memory
		{
			verts.clear();
			tris.clear();
		}
	}

	void AddVertice(const matrices::Vector3& point)
	{
		verts.add(Vec3f((float)point(0),(float)point(1),(float)point(2)));
	}

	void BuildCollisionTree()
	{
		instantiated_ = true;
		AABBTreePolyBuilder builder;
		BaseTree = builder.buildFromPolys(tris.mem(),//polygons
                                          tris.size(),//polygon count
                                          verts.mem(),//vertices
                                          verts.size());//vertices count
	}

	bool CollideJoint(matrices::Matrix4& currentTransform, Joint* joint)
	{
		const matrices::Vector3 zero(0, 0, 0);
		matrices::Matrix4 jointTransform = matrices::Translate(joint->GetR()); // translation of current joint
		if(joint->GetR() != zero )
		{
			switch(joint->GetRotation())
			{
				case rotation::X:
					jointTransform = jointTransform + matrices::Rotx4(joint->GetTheta());
					break;
				case rotation::Y:
					jointTransform = jointTransform + matrices::Roty4(joint->GetTheta());
					break;
				case rotation::Z:
					jointTransform = jointTransform + matrices::Rotz4(joint->GetTheta());
					break;
			}

			Vec3f from((float)currentTransform(0, 3), (float)currentTransform(1, 3), (float)currentTransform(2, 3));
			currentTransform = currentTransform * jointTransform;
			Vec3f to((float)currentTransform(0, 3), (float)currentTransform(1, 3), (float)currentTransform(2, 3));

			if (BaseTree->isCollideWithSegment(from, to))
			{
				return true;
			}
		}
		return (joint->pChild_ != 0) ? CollideJoint(currentTransform, joint->pChild_) : false;
	}

	bool instantiated_;
	AABBTreePoly *BaseTree;//this is the basic triangle BSP
	ozcollide::Vector<Vec3f> verts, normals;
	ozcollide::Vector<ozcollide::Polygon> tris;
	AABBTreePoly::SegmentColResult result; // <-------THIS IS VERY IMPORTANT!!!!!!!
};

CollisionHandlerColdet::CollisionHandlerColdet()
	: CollisionHandler_ABC()
	, pImpl_(new CollisionHandlerPImpl())
{
	// NOTHING
}

CollisionHandlerColdet::~CollisionHandlerColdet()
{
	// NOTHING
}

void CollisionHandlerColdet::AddObstacle(const Obstacle* obstacle)
{
	assert(!(pImpl_->instantiated_));
	ozcollide::Polygon *p = new ozcollide::Polygon();//we must build each face before add it to the list
    int *indx = new int[4];
	ozcollide::Vec3f tmpNormal((float)(obstacle->GetA()), (float)(obstacle->GetB()), (float)(obstacle->GetC()));

	int nextIndice = pImpl_->verts.size();
	// adding Vertices and associating polygon indices
	pImpl_->AddVertice(obstacle->GetP1());pImpl_->AddVertice(obstacle->GetP2());pImpl_->AddVertice(obstacle->GetP3());pImpl_->AddVertice(obstacle->GetP4());
	for(int i=0; i<4; ++i)
	{
		indx[i] = nextIndice++;
	}
    p->setIndicesMemory(4,&indx[0]);//setting up indices

    p->setNormal(tmpNormal);//adding normals (previously readed)
    pImpl_->tris.add(p[0]);//adding the polygon to the polygon list
}

void CollisionHandlerColdet::Instantiate()
{
	// TODO
	pImpl_->BuildCollisionTree();
}

bool CollisionHandlerColdet::IsColliding(const Robot& robot, const Tree& tree)
{
	// TODO
	matrices::Matrix4 m (robot.ToWorldCoordinates());
	assert(pImpl_->instantiated_);
	Joint* j = tree.GetRoot();
	assert(j);
	matrices::Vector3 attach(j->GetR());
	m.block(0,3,3,1) = matrices::matrix4TimesVect3(m, attach);
	return (j->pChild_ != 0) ? pImpl_->CollideJoint(m,j->pChild_)  : false;
}

// TESTS

//using namespace matrices;
//#include "MatrixDefs.h"
//#include "kinematic/TreeFactory.h"
//
//int main(int argc, char *argv[])
//{
//	Vector3 p1(-1,1,1.f);
//	Vector3 p2(1,1,1.f);
//	Vector3 p3(1,-1,1.f);
//	Vector3 p4(-1,-1,1.f);
//	Obstacle* obs = new Obstacle(p1,p2,p3,p4); // first test
//
//	Vector3 p11(-1,1,2.2f);
//	Vector3 p12(1,1,2.2f);
//	Vector3 p13(1,-1,2.2f);
//	Vector3 p14(-1,-1,2.2f);
//
//	Obstacle* obs1 = new Obstacle(p11,p12,p13,p14); // third test
//
//	CollisionHandler cHandler;
//	cHandler.AddObstacle(obs);
//	cHandler.AddObstacle(obs1);
//	cHandler.Instantiate();
//
//	factories::TreeFactory factory;
//	matrices::Matrix4 m(matrices::Matrix4::Identity());
//
//	Tree * testTree = factory.CreateTree(manip_core::enums::robot::QuadrupedLegRight, Vector3(0.0, 0.0, 1.8), 0);
//	
//	//checking collision with first obstacle
//	Robot robot(m, testTree);
//	assert(cHandler.IsColliding(robot, *testTree));
//
//	m = matrices::Rotx4(1.57);
//	Tree * testTree2 = factory.CreateTree(manip_core::enums::robot::QuadrupedLegRight, Vector3(0.0, 0.0, 1.8), 0);
//	Robot robot2(m, testTree2);
//	assert(!cHandler.IsColliding(robot2, *testTree2));
//
//	
//	m = matrices::Roty4(3.14) + matrices::Translate(Vector3(0,0,3.6));
//	Tree * testTree3 = factory.CreateTree(manip_core::enums::robot::QuadrupedLegRight, Vector3(0.0, 0.0, 1.8), 0);
//	Robot robot3(m, testTree3);
//	assert(cHandler.IsColliding(robot3, *testTree3));
//	return 0;
//}