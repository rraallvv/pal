#ifndef BULLET_PAL_H
#define BULLET_PAL_H
//(c) Adrian Boeing 2006, see liscence.txt (BSD liscence)
/*
	Abstract:
		PAL - Physics Abstraction Layer. Bullet implementation.
		This enables the use of bullet via PAL.
	Author: 
		Adrian Boeing
	Revision History:
	Version 0.0.91: 26/12/07 - Static sphere, capsule
	Version 0.0.9 : 17/12/07 - Base body, compound body position fix, static box, base link support
	Version 0.0.87: 15/12/07 - Body deletion.
	Version 0.0.86: 20/11/07 - PSD fix.
	Version 0.0.85: 10/11/07 - Fixed orientated plane bug
	Version 0.0.84: 09/11/07 - Fixed geometery and body location bugs
	Version 0.0.83: 07/11/07 - Added compound body
	Version 0.0.82: 28/10/07 - Updated for Bullet 2.62RC2
	Version 0.0.81: 19/10/07 - Version number request, new force system
	Version 0.0.8 : 17/10/07 - Added Generic Constraint
	Version 0.0.7 : 15/10/07 - Added PSD sensor
	Version 0.0.6 : 18/08/07 - Convex geom and body and vehicle
	Version 0.0.54: 01/08/07 - Updated for Bullet 2.55
	Version 0.0.53: 25/07/07 - Orientated plane
	Version 0.0.52: 15/07/07 - body sleep
	Version 0.0.51: 22/06/07 - body set velocity linear & angular
	Version 0.0.5 : 10/05/06 - Update for Bullet 2.50
	Version 0.0.4 : 17/11/06 - materials, terrain heightmap
	Version 0.0.3 : 16/11/06 - terrain mesh, spherical, revolute and prismatic link.
	Version 0.0.2 : 14/11/06 - boxgeom fix, sphere geom, cylinder geom, sphere, cylinder, terrainplane
	Version 0.0.1 : 13/11/06 - physics, body, boxgeom, box
	TODO:
		- fix prismatic link config
		- link limits
		- compound bodies
		- sawp terrainplane to use btStaticPlaneShape
	notes:
*/

#include "../pal/palFactory.h"
#include <btBulletDynamicsCommon.h>
#if defined(_MSC_VER)
#ifndef NDEBUG
#pragma comment( lib, "libbulletcollision_d.lib")
#pragma comment( lib, "libbulletdynamics_d.lib")
#pragma comment( lib, "libbulletmath_d.lib")
#else
#pragma comment( lib, "libbulletcollision.lib")
#pragma comment( lib, "libbulletdynamics.lib")
#pragma comment( lib, "libbulletmath.lib")
#endif
#pragma warning(disable : 4250)
#endif



class palBulletPhysics: public palPhysics {
public:
	palBulletPhysics();
	virtual void Init(Float gravity_x, Float gravity_y, Float gravity_z);
	virtual void Cleanup();
	const char* GetVersion();
	//extra methods provided by Bullet abilities:
	btDynamicsWorld* GetDynamicsWorld() {return m_dynamicsWorld;}
protected:
	virtual void Iterate(Float timestep);
	btDynamicsWorld*		m_dynamicsWorld;
	FACTORY_CLASS(palBulletPhysics,palPhysics,Bullet,1)
};


class palBulletBodyBase :virtual public palBodyBase {
public:
	palBulletBodyBase();
	virtual palMatrix4x4& GetLocationMatrix();
	virtual void SetPosition(palMatrix4x4& location);
	virtual void SetMaterial(palMaterial *material);

	btRigidBody *m_pbtBody;
	btDefaultMotionState *m_pbtMotionState;

protected:
	void BuildBody(Float fx, Float fy, Float fz, Float mass, bool dynamic = true, btCollisionShape *btShape = 0);
};

class palBulletBody :  virtual public palBody, virtual public palBulletBodyBase {
public:
	palBulletBody();
	~palBulletBody();
//	virtual void SetPosition(palMatrix4x4& location);
//	virtual palMatrix4x4& GetLocationMatrix();
//	virtual void SetMaterial(palMaterial *material);

//	virtual void SetForce(Float fx, Float fy, Float fz);
//	virtual void GetForce(palVector3& force);
//	virtual void SetTorque(Float tx, Float ty, Float tz);
//	virtual void GetTorque(palVector3& torque);

	virtual void ApplyForce(Float fx, Float fy, Float fz);
	virtual void ApplyTorque(Float tx, Float ty, Float tz);

	virtual void ApplyImpulse(Float fx, Float fy, Float fz);
	virtual void ApplyAngularImpulse(Float fx, Float fy, Float fz);

	virtual void GetLinearVelocity(palVector3& velocity);
	virtual void GetAngularVelocity(palVector3& velocity_rad);

	virtual void SetLinearVelocity(palVector3 velocity);
	virtual void SetAngularVelocity(palVector3 velocity_rad);

	virtual void SetActive(bool active);

	virtual void SetPosition(palMatrix4x4& location) {
		palBulletBodyBase::SetPosition(location);
	}
protected:
//	void BuildBody(Float fx, Float fy, Float fz, Float mass);

};

class palBulletCompoundBody : public palCompoundBody, public palBulletBody {
public:
	palBulletCompoundBody();
	virtual void SetPosition(palMatrix4x4& location);
	virtual palMatrix4x4& GetLocationMatrix();
	virtual void Finalize();
protected:
	FACTORY_CLASS(palBulletCompoundBody,palCompoundBody,Bullet,1)
};


class palBulletGeometry : virtual public palGeometry {
public:
	palBulletGeometry();
	~palBulletGeometry();
	btCollisionShape* m_pbtShape;
};

class palBulletBoxGeometry : public palBulletGeometry, public palBoxGeometry  {
public:
	palBulletBoxGeometry();
	virtual void Init(palMatrix4x4 &pos, Float width, Float height, Float depth, Float mass);
	btBoxShape *m_pbtBoxShape;
protected:
	FACTORY_CLASS(palBulletBoxGeometry,palBoxGeometry,Bullet,1)
};

class palBulletSphereGeometry : public palSphereGeometry , public palBulletGeometry {
public:
	palBulletSphereGeometry();
	virtual void Init(palMatrix4x4 &pos, Float radius, Float mass);
	btSphereShape *m_btSphereShape;
protected:
	FACTORY_CLASS(palBulletSphereGeometry,palSphereGeometry,Bullet,1)
};

class palBulletCapsuleGeometry : public palCapsuleGeometry , public palBulletGeometry {
public:
	palBulletCapsuleGeometry();
	virtual void Init(palMatrix4x4 &pos, Float radius, Float length, Float mass);
	btCylinderShape *m_btCylinderShape;
protected:
	FACTORY_CLASS(palBulletCapsuleGeometry,palCapsuleGeometry,Bullet,1)
};

class palBulletBox : virtual public palBox, virtual public palBulletBody {
public:
	palBulletBox();
	virtual void Init(Float x, Float y, Float z, Float width, Float height, Float depth, Float mass);
	//extra methods provided by Bullet abilities:
protected:
	FACTORY_CLASS(palBulletBox,palBox,Bullet,1)
};

class palBulletStaticBox : virtual public palStaticBox, virtual public palBulletBodyBase {
public:
	palBulletStaticBox();
	virtual void Init(palMatrix4x4 &pos, Float width, Float height, Float depth);
protected:
	FACTORY_CLASS(palBulletStaticBox,palStaticBox,Bullet,1)
};


class palBulletSphere : public palSphere, public palBulletBody {
public:
	palBulletSphere();
	void Init(Float x, Float y, Float z, Float radius, Float mass);
protected:
	FACTORY_CLASS(palBulletSphere,palSphere,Bullet,1)
};

class palBulletStaticSphere : virtual public palStaticSphere, virtual public palBulletBodyBase {
public:
	palBulletStaticSphere();
	virtual void Init(palMatrix4x4 &pos, Float radius);
protected:
	FACTORY_CLASS(palBulletStaticSphere,palStaticSphere,Bullet,1)
};

class palBulletCapsule : public palCapsule, public palBulletBody {
public:
	palBulletCapsule();
	virtual void Init(Float x, Float y, Float z, Float radius, Float length, Float mass);
	
protected:
	FACTORY_CLASS(palBulletCapsule,palCapsule,Bullet,1)
};

class palBulletStaticCapsule : public palStaticCapsule, public palBulletBodyBase {
public:
	palBulletStaticCapsule();
	virtual void Init(Float x, Float y, Float z, Float radius, Float length);
	
protected:
	FACTORY_CLASS(palBulletStaticCapsule,palStaticCapsule,Bullet,1)
};

class palBulletTerrainPlane : virtual public palTerrainPlane, virtual public palBulletBodyBase  {
public:
	palBulletTerrainPlane();
	virtual void Init(Float x, Float y, Float z, Float min_size);
public:
	btBoxShape *m_pbtBoxShape;
	FACTORY_CLASS(palBulletTerrainPlane,palTerrainPlane,Bullet,1)
};


class palBulletOrientatedTerrainPlane : virtual public palOrientatedTerrainPlane, virtual public palBulletBodyBase  {
public:
	palBulletOrientatedTerrainPlane();
	virtual void Init(Float x, Float y, Float z, Float nx, Float ny, Float nz, Float min_size);
	virtual palMatrix4x4& GetLocationMatrix() {
		return palOrientatedTerrainPlane::GetLocationMatrix();
	}
public:
	btStaticPlaneShape *m_pbtPlaneShape;
	FACTORY_CLASS(palBulletOrientatedTerrainPlane,palOrientatedTerrainPlane,Bullet,1)
};

class palBulletTerrainMesh : virtual public palTerrainMesh, virtual public palBulletBodyBase  {
public:
	palBulletTerrainMesh();
	virtual void Init(Float x, Float y, Float z, const Float *pVertices, int nVertices, const int *pIndices, int nIndices);
protected:
	btBvhTriangleMeshShape *m_pbtTriMeshShape;
	FACTORY_CLASS(palBulletTerrainMesh,palTerrainMesh,Bullet,1)
};

class palBulletTerrainHeightmap : virtual public palTerrainHeightmap, private palBulletTerrainMesh {
public:
	palBulletTerrainHeightmap();
	virtual void Init(Float x, Float y, Float z, Float width, Float depth, int terrain_data_width, int terrain_data_depth, const Float *pHeightmap);
protected:
	FACTORY_CLASS(palBulletTerrainHeightmap,palTerrainHeightmap,Bullet,1)
};


class palBulletSphericalLink : public palSphericalLink {
public:
	palBulletSphericalLink();
	virtual void Init(palBodyBase *parent, palBodyBase *child, Float x, Float y, Float z);
	virtual void SetLimits(Float cone_limit_rad, Float twist_limit_rad);

	btTypedConstraint *m_btp2p;
protected:
	FACTORY_CLASS(palBulletSphericalLink,palSphericalLink,Bullet,1)
};

class palBulletRevoluteLink: public palRevoluteLink {
public:
	palBulletRevoluteLink();
	virtual void Init(palBodyBase *parent, palBodyBase *child, Float x, Float y, Float z, Float axis_x, Float axis_y, Float axis_z);
	virtual void SetLimits(Float lower_limit_rad, Float upper_limit_rad); 

	btHingeConstraint *m_btHinge;
protected:
	FACTORY_CLASS(palBulletRevoluteLink,palRevoluteLink,Bullet,1)
};

class palBulletPrismaticLink:  public palPrismaticLink {
public:
	palBulletPrismaticLink();
	virtual void Init(palBodyBase *parent, palBodyBase *child, Float x, Float y, Float z, Float axis_x, Float axis_y, Float axis_z); 

	btGeneric6DofConstraint* m_btSlider;
protected:
	FACTORY_CLASS(palBulletPrismaticLink,palPrismaticLink,Bullet,1)
};


class palBulletConvexGeometry : public palBulletGeometry, public palConvexGeometry  {
public:
	palBulletConvexGeometry() {};
	~palBulletConvexGeometry() {};
	virtual void Init(palMatrix4x4 &pos, const Float *pVertices, int nVertices, Float mass);
	btConvexHullShape *m_pbtConvexShape;
protected:
	FACTORY_CLASS(palBulletConvexGeometry,palConvexGeometry,Bullet,1)
};


class palBulletConvex : public palBulletBody, public palConvex {
public:
	palBulletConvex();
	virtual void Init(Float x, Float y, Float z, const Float *pVertices, int nVertices, Float mass);
protected:
	FACTORY_CLASS(palBulletConvex,palConvex,Bullet,1)
};

class palBulletPSDSensor : public palPSDSensor {
public:
	palBulletPSDSensor();
	void Init(palBody *body, Float x, Float y, Float z, Float dx, Float dy, Float dz, Float range); //position, direction
	Float GetDistance();
protected:
	Float m_fRelativePosX;
	Float m_fRelativePosY;
	Float m_fRelativePosZ;
	FACTORY_CLASS(palBulletPSDSensor,palPSDSensor,Bullet,1)
};


class palBulletGenericLink : public palGenericLink {
public:
	palBulletGenericLink();
	void Init(palBody *parent, palBody *child, palMatrix4x4& parentFrame, palMatrix4x4& childFrame,
		palVector3 linearLowerLimits,
		palVector3 linearUpperLimits,
		palVector3 angularLowerLimits,
		palVector3 angularUpperLimits);
protected:
	FACTORY_CLASS(palBulletGenericLink,palGenericLink,Bullet,1)
};

#endif