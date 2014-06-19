

#include "MatrixDefs.h"
#include "Pi.h"

#include "ManipManager.h"
#include "PostureManager.h"
#include "draw/DrawRobot.h"
#include "draw/DrawManager.h"
#include "API/TreeI.h"
#include "API/RobotI.h"
#include "IKSolver/IKSolver.h"

#include "XBOXController/Controller.h"

#include <drawstuff/drawstuff.h> // The drawing library for ODE;

#ifdef WIN32
#include <windows.h>
#endif

#include <iostream>
#include <vector>


namespace matrices
{
	const Vector3 unitx = Vector3(1, 0, 0);
	const Vector3 unity = Vector3(0, 1, 0);
	const Vector3 unitz = Vector3(0, 0, 1);
	const Vector3 zero  = Vector3(0, 0, 0);
}

using namespace matrices;
using namespace Eigen;
using namespace std;
using namespace manip_core;

// main object

void Translate(RobotI* robot, const matrices::Vector3 dir)
{
	double tab [3];
	matrices::vect3ToArray(tab, dir);
	robot->Translate(tab);
}


ManipManager manager;
DrawManager drawManager(manager);
PostureManager* postureManager = manager.GetPostureManager();
DrawRobot* dr;
const IKSolver& solver = manager.GetIkSolver();
xbox::Controller controller(1);
matrices::Vector3 oldDir_(1., 0., 0.);


static float xyz[3] = {-10.0,1,3.0};
static float hpr[3] = {0.0,0.0,0.0};
RobotI* pRobot;


void BuildWorld()
{
	// FORWARD OBSTACLES
	Vector3 p1(-11,6,0.4f);
	Vector3 p2(10,6,0.4f);
	Vector3 p3(10,-1,0.4f);
	Vector3 p4(-11,-1,0.4f);

	/*manager.AddObstacle(p1,p2,p3,p4);

	p1 = Vector3(-7.7,2,0.f);
	p2 = Vector3(-7.5,2, 5.f);
	p3 = Vector3(-7.5,-1, 5.f);
	p4 = Vector3(-7.7,-1,0.f);

	manager.AddObstacle(p1,p2,p3,p4);*/

	Vector3 p11(-5.2, 2, 0.f);
	Vector3 p21( -1, -1, 0.f);
	manager.GenerateStairChess(p11, p21, 0.4, 1.2,  3, 2);

	Vector3 p13(-0.8, 2, 1.7f);
	Vector3 p23( 3, -1, 1.7f);
	manager.GenerateXInclinedPlank(p13, p23);

	Vector3 p131(3.1, 3, 2.0f);
	Vector3 p231( 6, 1, 1.5f);
	manager.GenerateXInclinedPlank(p131, p231);

	Vector3 p132(3.1, 0, 1.5f);
	Vector3 p232( 6, -2, 2.0f);
	manager.GenerateXInclinedPlank(p132, p232);


	Vector3 p136(6.2, 1, 1.5f);
	Vector3 p236( 18, 0.7, 1.5f);
	manager.GenerateXInclinedPlank(p136, p236);
	
	manager.Initialize();
}


void Rotate(RobotI* robot, const matrices::Matrix3& rotation)
{
	matrices::Matrix4 transform;
	double transf[16];
	robot->ToWorldCoordinates(transf);
	matrices::array16ToMatrix4(transf, transform);
	transform.block<3,3>(0,0) = transform.block<3,3>(0,0) * rotation;
	matrices::matrixTo16Array(transf, transform);
	robot->SetTransform(transf);
}

void XboxControl(RobotI * robot)
{
	controller.Update();
	float xL, yL;
	float xR, yR;
	bool transformed = false;
	const float slowDownFactor = 0.01f;
	controller.GetLeftStickMovingVector(xL, yL);
	controller.GetRightStickMovingVector(xR, yR);
	if( xL != 0 || yL != 0)
	{
		double trans[3] = {((double)xL * slowDownFactor*2), 0., ((double)yL * slowDownFactor)};
		robot->Translate(trans);
		oldDir_ = Vector3(xL, 0, yL);
		oldDir_.normalize();
		transformed = true;
	}
	if( yR != 0)
	{
		transformed = true;
		Rotate(pRobot, matrices::Roty3(-yR * slowDownFactor*2));

	}
	if( xR != 0)
	{
		//transformed = true;
		//Rotate(pRobot, matrices::Rotx3(xR * slowDownFactor*3));

	}
	if(transformed)
	{
		matrices::Matrix4 transform;
		double transf[16];
		robot->ToRobotCoordinates(transf);
		matrices::array16ToMatrix4(transf, transform);
		//postureManager->NextPosture(pRobot,matrices::matrix4TimesVect3(transform, oldDir_));
		postureManager->NextPosture(pRobot, oldDir_);
	}
}

static void simLoop (int pause)
{
	XboxControl(pRobot);
	bool ok = true; int i = 0;
	do
	{
		++i;
		ok = solver.QuickStepClampingToTargets(pRobot);
	}
	while(!ok && i < 2);
	dr->Draw();
	drawManager.Draw();
}

void start()
{
    dsSetViewpoint (xyz,hpr);
}

void command(int cmd)   /**  key control function; */
{
	const Vector3 trX(0.03, 0, 0);
	const Vector3 trY(0,0.01,  0);
	switch (cmd)
	{	
		case '+' :
			Translate(pRobot, trX);
			postureManager->NextPosture(pRobot,matrices::unitx);
		break;
		case '-' :
			Translate(pRobot, -trX);
			postureManager->NextPosture(pRobot,matrices::unitx);
			//drawManager.PreviousPosture();
		break;
		case 'a' :
			Rotate(pRobot, matrices::Roty3(-0.1));
			postureManager->NextPosture(pRobot,matrices::unity);
			//drawManager.PreviousPosture();
		break;
	}
}


int main(int argc, char *argv[])
{
	//init robot
	Matrix4 robotBasis(MatrixX::Identity(4,4));
	robotBasis(0,3) = -10;
	robotBasis(1,3) = 0.75;
	robotBasis(2,3) = 1.8;

	BuildWorld();

	pRobot = manager.CreateRobot(enums::robot::Quadruped, robotBasis);
	dr = new DrawRobot(pRobot);
	/* TEST */
	//sg->GenerateSamples(*pRobot, 243);

	Vector3 robotBasis2(0,0,0);

	
/* trajectory */
	postureManager->InitSamples(pRobot,10000);
/*TEST */

	/*
	drawstuff stuff*/
	dsFunctions fn;
    fn.version = DS_VERSION;
    fn.start   = &start;
    fn.step    = &simLoop;
	fn.command = &command;
    fn.stop    = 0;
    fn.path_to_textures = "../textures";
    dsSimulationLoop (argc,argv,800,600,&fn);

    return 0;
}