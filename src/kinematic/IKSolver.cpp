
#include "IKSolver.h"
#include "kinematic/Tree.h"
#include "sampling/Sample.h"
#include "kinematic/Jacobian.h"
#include "PartialDerivativeConstraint.h"

#include <vector>
#include <iostream>

using namespace matrices;
using namespace Eigen;
using namespace std;

struct IKPImpl
{
	IKPImpl()
	{
		//TODO
	}

	~IKPImpl()
	{
		for(T_ConstraintIT it = constraints_.begin(); it!= constraints_.end(); ++it)
		{
			delete(*it);
		}
	}
	
	typedef vector<PartialDerivativeConstraint*> T_Constraint;
	typedef T_Constraint::iterator T_ConstraintIT;
	typedef T_Constraint::const_iterator T_ConstraintCIT;
	T_Constraint constraints_;
};

IKSolver::IKSolver(const float espilon, const float treshold)
: epsilon_(espilon)
, treshold_(treshold)
, pImpl_(new IKPImpl)
{
	// NOTHING
}

IKSolver::~IKSolver()
{
	// NOTHING
}


void IKSolver::Register(PartialDerivativeConstraint* constraint)
{
	pImpl_->constraints_.push_back(constraint);
}


bool IKSolver::QuickStepClamping(Tree& tree, const matrices::Vector3& target) const
{
	Jacobian jacobian(tree); 
	Vector3 force = target - tree.GetEffectorPosition(tree.GetNumEffector()-1) ; //TODO we only have one effector  so weird huh ?
	
	if(force.norm() < treshold_) // reached treshold
	{
		return true;
	}
	VectorX velocities;
	MatrixX J = jacobian.GetJacobianCopy(); int colsJ = J.cols(); int rowsJ = J.rows();
	
	Vector3 dX = (force / force.norm()) * treshold_ * 2; // TODO real trajectory please ?
	
	//null space projection
	MatrixX p0 = MatrixX::Identity(colsJ, colsJ);
	MatrixX nullSpace = p0;

	// init all joints to free.
	bool freeJoint[20];// no more than 20 joint ok :)
	for(int i = 0; i < colsJ; ++i)
	{
		freeJoint[i]=true;
	}
		
	bool clamp = false;
	//entering clamping loop
	do
	{
		velocities = jacobian.GetJacobianInverse() * dX;
		// now to the "fun" part
		clamp = false;
		for(int i =0; i < colsJ; ++ i)
		{
			if(freeJoint[i])
			{
				NUMBER overload = tree.GetJoint(i+1)->AddToTheta(velocities(i));
				if (false)//(overload > 0.f) // clamping happened
				{
					freeJoint[i] = false;
					clamp = true;
					dX -= J.col(i) * overload;
					J.col(i) = VectorX::Zero(rowsJ);
					p0(i,i) = 0;
				}
			}
		}
		if(clamp)
			jacobian.SetJacobian(J);
	} while(clamp);
	return false;
}


//REF: Boulic : An inverse kinematics architecture enforcing an arbitrary number of strict priority levels
bool IKSolver::StepClamping(Tree& tree, const matrices::Vector3& target, const Vector3& direction) const
{
	Jacobian jacobian(tree); 
	VectorX postureVariation(VectorX::Zero(jacobian.GetJacobian().cols()));
	PartialDerivatives(tree, direction, postureVariation);

	Vector3 force = target - tree.GetEffectorPosition(tree.GetNumEffector()-1) ; //TODO we only have one effector  so weird huh ?
	
	if(force.norm() < treshold_) // reached treshold
	{
		return true;
	}
	VectorX velocities;
	MatrixX J = jacobian.GetJacobianCopy(); int colsJ = J.cols(); int rowsJ = J.rows();
	
	Vector3 dX = (force / force.norm()) * treshold_ * 2; // TODO real trajectory please ?
	
	//null space projection
	MatrixX p0 = MatrixX::Identity(colsJ, colsJ);
	MatrixX nullSpace = p0;

	// init all joints to free.
	bool freeJoint[20];// no more than 20 joint ok :)
	for(int i = 0; i < colsJ; ++i)
	{
		freeJoint[i]=true;
	}
		
	bool clamp = false;
	//entering clamping loop
	do
	{
		jacobian.GetNullspace(p0, nullSpace); // Pn(j) = P0(j) - Jtr * J

		velocities = jacobian.GetJacobianInverse() * dX + nullSpace * postureVariation;
		// now to the "fun" part
		clamp = false;
		for(int i =0; i < colsJ; ++ i)
		{
			if(freeJoint[i])
			{
				NUMBER overload = tree.GetJoint(i+1)->AddToTheta(velocities(i));
				if(overload > 0.f) // clamping happened
				{
					freeJoint[i] = false;
					clamp = true;
					dX -= J.col(i) * overload;
					J.col(i) = VectorX::Zero(rowsJ);
					p0(i,i) = 0;
				}
			}
		}
		if(clamp)
			jacobian.SetJacobian(J);
	} while(clamp);
	return false;
}

void IKSolver::PartialDerivative(Tree& tree, const Vector3& direction, VectorX& velocities, const int joint) const
{
	Sample save(tree); // saving previous tree
	tree.GetJoint(joint)->AddToTheta(-epsilon_);
	tree.Compute();
	Jacobian jacobMinus(tree);
	save.LoadIntoTree(tree); // loading it

	tree.GetJoint(joint)->AddToTheta(epsilon_);
	tree.Compute();
	Jacobian jacobPlus(tree);
	save.LoadIntoTree(tree); // loading it
	
	for(IKPImpl::T_ConstraintIT it = pImpl_->constraints_.begin(); it!= pImpl_->constraints_.end(); ++it)
	{
		velocities(joint-1) += (*it)->Evaluate(jacobMinus, jacobPlus, epsilon_, direction);
	}
}

void IKSolver::PartialDerivatives(Tree& tree, const Vector3& direction, VectorX& velocities) const
{
	for(int i =1; i<= velocities.rows() ;++i)
	{
		PartialDerivative(tree, direction, velocities, i);
	}
}