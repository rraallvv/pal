#ifdef MICROSOFT_VC
#pragma warning( disable : 4786 ) // ident trunc to '255' chars in debug info
#endif
//(c) Adrian Boeing 2004, see liscence.txt (BSD liscence)
#include "ode_pal.h"
/*
	Abstract:
		PAL - Physics Abstraction Layer. ODE implementation.
		This enables the use of ODE via PAL.

		Implementaiton
	Author: 
		Adrian Boeing
	Revision History:
		Version 0.5 : 04/06/04 -
	TODO:
		-get to 1.0 (ie: same as pal.h)
*/

#ifndef NDEBUG
 #ifdef MICROSOFT_VC
#ifdef MEMDEBUG
 #include <crtdbg.h>
 #define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif
 #endif
#endif

FACTORY_CLASS_IMPLEMENTATION_BEGIN_GROUP;
//FACTORY_CLASS_IMPLEMENTATION(palODEMaterial);
FACTORY_CLASS_IMPLEMENTATION(palODEPhysics);

FACTORY_CLASS_IMPLEMENTATION(palODEBoxGeometry);
FACTORY_CLASS_IMPLEMENTATION(palODESphereGeometry);
FACTORY_CLASS_IMPLEMENTATION(palODECapsuleGeometry);
FACTORY_CLASS_IMPLEMENTATION(palODEConvexGeometry);

FACTORY_CLASS_IMPLEMENTATION(palODECompoundBody);
FACTORY_CLASS_IMPLEMENTATION(palODEConvex);
FACTORY_CLASS_IMPLEMENTATION(palODEBox);
FACTORY_CLASS_IMPLEMENTATION(palODESphere);
FACTORY_CLASS_IMPLEMENTATION(palODECylinder);

FACTORY_CLASS_IMPLEMENTATION(palODESphericalLink);
FACTORY_CLASS_IMPLEMENTATION(palODERevoluteLink);
FACTORY_CLASS_IMPLEMENTATION(palODEPrismaticLink);

FACTORY_CLASS_IMPLEMENTATION(palODEOrientatedTerrainPlane);
FACTORY_CLASS_IMPLEMENTATION(palODETerrainPlane);
FACTORY_CLASS_IMPLEMENTATION(palODETerrainMesh);
FACTORY_CLASS_IMPLEMENTATION(palODETerrainHeightmap);

FACTORY_CLASS_IMPLEMENTATION(palODEMaterials);
FACTORY_CLASS_IMPLEMENTATION_END_GROUP;

MAP <dGeomID, ODE_MATINDEXLOOKUP> palODEMaterials::g_IndexMap;
std_matrix<palMaterial *> palODEMaterials::g_Materials;
VECTOR<STRING> palODEMaterials::g_MaterialNames;

static dWorldID g_world;
static dSpaceID g_space;
static dJointGroupID g_contactgroup;

/*
palODEMaterial::palODEMaterial() {
};

void palODEMaterial::Init(Float static_friction, Float kinetic_friction, Float restitution) {
	palMaterial::Init(static_friction,kinetic_friction,restitution);
}
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_CONTACTS 256 // maximum number of contact points per body

/* this is called by dSpaceCollide when two objects in space are
 * potentially colliding.
 */

static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{

	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
			{
				// Colliding a space with either a geom or another space.
				dSpaceCollide2(o1, o2, data, &nearCallback);

				if (dGeomIsSpace(o1))
				{
					// Colliding all geoms internal to the space.
					dSpaceCollide((dSpaceID) o1, data,
					               &nearCallback);
				}

				if (dGeomIsSpace(o2))
				{
					// Colliding all geoms internal to the space.
					dSpaceCollide((dSpaceID) o2, data,
					               &nearCallback);
				}
				return;
			}

		int i=0;
		dBodyID b1=dGeomGetBody(o1);
		dBodyID b2=dGeomGetBody(o2);

//		ODE_MATINDEXLOOKUP *sm1=palODEMaterials::GetMaterial(o1);
//		ODE_MATINDEXLOOKUP *sm2=palODEMaterials::GetMaterial(o2);
//		printf("material interaction: [%d %d][%d %d]",b1,b2,o1,o2);
//		printf("indexs::%d %d\n",*sm1,*sm2);
/*		if (sm1)
			printf("%s",sm1->c_str());
		printf(" with ");
		if (sm2)
			printf("%s",sm2->c_str());
		printf("\n");*/

		palMaterial *pm = palODEMaterials::GetODEMaterial(o1,o2);
				
		if(b1 && b2 && dAreConnectedExcluding(b1,b2,dJointTypeContact))return;

		dContact contact[MAX_CONTACTS];
		for(i=0;i<MAX_CONTACTS;i++){
			#pragma message("todo: fix ode flags to allow friction AND restitution")
				contact[i].surface.mode =dContactBounce |
				dContactSoftERP | dContactSoftCFM | dContactApprox1;
				//remove dContactSoftCFM | dContactApprox1 for bounce..
			if (pm) {
				
				contact[i].surface.mu = pm->m_fStatic;
				contact[i].surface.bounce= pm->m_fRestitution;
				contact[i].surface.mode|=dContactMu2; 
				contact[i].surface.mu2 = pm->m_fKinetic;
			} else {
				contact[i].surface.mu = dInfinity;
				contact[i].surface.bounce= 0.1f;
			}
//			const real minERP=(real)0.01;
//			const real maxERP=(real)0.99;
			//contact[i].surface.slip1 = 0.1; // friction
			//contact[i].surface.slip2 = 0.1;
			contact[i].surface.bounce_vel = 1;			
			contact[i].surface.soft_erp = 0.5f;
			contact[i].surface.soft_cfm = 0.01f;
		}
		int numc=dCollide(o1,o2,MAX_CONTACTS,&contact[0].geom,sizeof(dContact));

		if(numc>0){
			for(i=0;i<numc;i++){		
				dJointID c=dJointCreateContact(g_world,g_contactgroup,&contact[i]);
				dJointAttach(c,b1,b2);
			}
		}

}

dGeomID CreateTriMesh(const Float *pVertices, int nVertices, const int *pIndices, int nIndices) {
	dGeomID odeGeom;
	int i;
	dVector3 *spacedvert = new dVector3[nVertices];
	int *dIndices = new int[nIndices];

	for (i=0;i<nVertices;i++) {
		spacedvert[i][0]=pVertices[i*3+0];
		spacedvert[i][1]=pVertices[i*3+1];
		spacedvert[i][2]=pVertices[i*3+2];
	}

	for (i=0;i<nIndices;i++) {
		dIndices[i] = pIndices[i];
	}

	
	// build the trimesh data
	dTriMeshDataID data=dGeomTriMeshDataCreate();
	dGeomTriMeshDataBuildSimple(data,(dReal*)spacedvert,nVertices,dIndices,nIndices);
	// build the trimesh geom 
	odeGeom=dCreateTriMesh(g_space,data,0,0,0);
	return odeGeom;
}

palODEPhysics::palODEPhysics() {
}

const char* palODEPhysics::GetVersion() {
	static char verbuf[256];
	sprintf(verbuf,"ODE V.UNKOWN");
	return verbuf;
}

void palODEPhysics::Init(Float gravity_x, Float gravity_y, Float gravity_z) {
	g_world = dWorldCreate();
	g_space = dHashSpaceCreate (0);
	g_contactgroup = dJointGroupCreate (0); //0 happparently
	SetGravity(gravity_x,gravity_y,gravity_z);
};

void palODEPhysics::SetGravity(Float gravity_x, Float gravity_y, Float gravity_z) {
	dWorldSetGravity (g_world,gravity_x,gravity_y,gravity_z);
}
/*
void palODEPhysics::SetGroundPlane(bool enabled, Float size) {
	if (enabled) 
//		dCreatePlane (g_space,0,0,1,0);
		dCreatePlane (g_space,0,1,0,0);
};
*/

void palODEPhysics::Iterate(Float timestep) {
	dSpaceCollide (g_space,0,&nearCallback);//evvvil
    dWorldStep (g_world,timestep);

    dJointGroupEmpty (g_contactgroup);
};

void palODEPhysics::Cleanup() {
  dJointGroupDestroy (g_contactgroup);
  dSpaceDestroy (g_space);
  dWorldDestroy (g_world);
};



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

palODEBody::palODEBody() {
	odeBody=0;
}

palODEBody::~palODEBody() {
	//printf("now deleteing %d, with %d,%d\n",this,odeBody,odeGeom);
	Cleanup();
	if(odeBody) { dBodyDestroy(odeBody); odeBody=0; }
}

void palODEBody::SetPosition(Float x, Float y, Float z) {
	//palBody::SetPosition(x,y,z);
	dBodySetPosition (odeBody,x, y, z);
}

void convODEFromPAL(dReal pos[3], dReal R[12], const palMatrix4x4& location) {
	R[0]=location._mat[0];
	R[4]=location._mat[1];
	R[8]=location._mat[2];
	
	R[1]=location._mat[4];
	R[5]=location._mat[5];
	R[9]=location._mat[6];
	
	R[2]=location._mat[8];
	R[6]=location._mat[9];
	R[10]=location._mat[10];
	
	pos[0]=location._mat[12];
	pos[1]=location._mat[13];
	pos[2]=location._mat[14];
}

void convODEToPAL(const dReal *pos, const dReal *R, palMatrix4x4& m_mLoc) {
	mat_identity(&m_mLoc);
	//this code is correct!
	//it just looks wrong, because R is a padded SSE structure!
	m_mLoc._mat[0]=R[0];
	m_mLoc._mat[1]=R[4];
	m_mLoc._mat[2]=R[8];
	m_mLoc._mat[3]=0;
	m_mLoc._mat[4]=R[1];
	m_mLoc._mat[5]=R[5];
	m_mLoc._mat[6]=R[9];
	m_mLoc._mat[7]=0;
	m_mLoc._mat[8]=R[2];
	m_mLoc._mat[9]=R[6];
	m_mLoc._mat[10]=R[10];
	m_mLoc._mat[11]=0;
	m_mLoc._mat[12]=pos[0];
	m_mLoc._mat[13]=pos[1];
	m_mLoc._mat[14]=pos[2];
	m_mLoc._mat[15]=1;
}

void palODEBody::SetPosition(palMatrix4x4& location) {
	if (odeBody) {
	dReal pos[3];
	dReal R[12];
	convODEFromPAL(pos,R,location);

	dBodySetPosition(odeBody,pos[0],pos[1],pos[2]);
	dBodySetRotation(odeBody,R);
	}
	palBody::SetPosition(location);
}

palMatrix4x4& palODEBody::GetLocationMatrix() {
	if (odeBody) {
	const dReal *pos = dBodyGetPosition    (odeBody);
	const dReal *R = dBodyGetRotation    (odeBody);
	//memset(m_mLoc._mat,0,sizeof(palMatrix4x4));
	convODEToPAL(pos,R,m_mLoc);
	}
	return m_mLoc;
}

void palODEBody::SetActive(bool active) {
	if (active)
		dBodySetAutoDisableFlag(odeBody,0);
	else
		dBodySetAutoDisableFlag(odeBody,1);
}


#if 0
void palODEBody::SetForce(Float fx, Float fy, Float fz) {
	dBodySetForce (odeBody,fx,fy,fz);
}
void palODEBody::GetForce(palVector3& force) {
	const dReal *pf=dBodyGetForce(odeBody);
	force.x=pf[0];
	force.y=pf[1];
	force.z=pf[2];
}



void palODEBody::SetTorque(Float tx, Float ty, Float tz) {
	dBodySetTorque(odeBody,tx,ty,tz);
}

void palODEBody::GetTorque(palVector3& torque) {
	const dReal *pt=dBodyGetTorque(odeBody);
	torque.x=pt[0];
	torque.y=pt[1];
	torque.z=pt[2];
}
#endif

void palODEBody::ApplyForce(Float fx, Float fy, Float fz) {
	dBodyAddForce(odeBody,fx,fy,fz);
}

void palODEBody::ApplyTorque(Float tx, Float ty, Float tz) {
	dBodyAddTorque(odeBody,tx,ty,tz);
}
/*
void palODEBody::ApplyImpulse(Float fx, Float fy, Float fz) {
	dReal *pv = (dReal *)dBodyGetLinearVel(odeBody);
	dBodySetLinearVel(odeBody,pv[0]+fx/m_fMass,pv[1]+fy/m_fMass,pv[2]+fz/m_fMass);
	//	m_kVelocity        += rkImpulse * m_fInvMass;
}

void palODEBody::ApplyAngularImpulse(Float fx, Float fy, Float fz) {
	dReal *pv = (dReal *)dBodyGetAngularVel(odeBody);
	dBodySetAngularVel(odeBody,pv[0]+fx/m_fMass,pv[1]+fy/m_fMass,pv[2]+fz/m_fMass);
}
*/
void palODEBody::GetLinearVelocity(palVector3& velocity) {
	const dReal *pv=dBodyGetLinearVel(odeBody);
	velocity.x=pv[0];
	velocity.y=pv[1];
	velocity.z=pv[2];
}

void palODEBody::GetAngularVelocity(palVector3& velocity) {
	const dReal *pv=dBodyGetAngularVel(odeBody);	
	velocity.x=pv[0];
	velocity.y=pv[1];
	velocity.z=pv[2];
}

void palODEBody::SetLinearVelocity(palVector3 vel) {
	dBodySetLinearVel(odeBody,vel.x,vel.y,vel.z);
}
void palODEBody::SetAngularVelocity(palVector3 vel) {
	dBodySetAngularVel(odeBody,vel.x,vel.y,vel.z);
}

/////////////////
palODEMaterials::palODEMaterials() {
}

palMaterial *palODEMaterials::GetODEMaterial(dGeomID odeGeomA,dGeomID odeGeomB) {
	ODE_MATINDEXLOOKUP *a=GetMaterialIndex(odeGeomA);
	ODE_MATINDEXLOOKUP *b=GetMaterialIndex(odeGeomB);
	if (!a) return NULL;
	if (!b) return NULL;
	return g_Materials.Get(*a,*b);
}

void palODEMaterials::NewMaterial(STRING name, Float static_friction, Float kinetic_friction, Float restitution) {
		if (GetIndex(name)!=-1) //error
		return;

		int size,check;
		g_Materials.GetDimensions(size,check); 
		g_Materials.Resize(size+1,size+1);

		palMaterials::NewMaterial(name,static_friction,kinetic_friction,restitution);
}

void palODEMaterials::SetIndex(int posx, int posy, palMaterial *pm) {
//	printf("palODEMATERIALS setindex\n");
	g_Materials.Set(posx,posy,pm);
	palMaterials::SetIndex(posx, posy, pm);
}

void palODEMaterials::SetNameIndex(STRING name) {
	g_MaterialNames.push_back(name);
	palMaterials::SetNameIndex(name);
}


void palODEMaterials::InsertIndex(dGeomID odeBody, palMaterial *mat) {
	palMaterialUnique *pmu = dynamic_cast<palMaterialUnique *> (mat);

	int index=-1;
	for (unsigned int i=0;i<g_MaterialNames.size();i++)
		if (g_MaterialNames[i] == pmu->m_Name)
			index=i;

	if (index<0) {
		STATIC_SET_ERROR("Could not insert index for material %s\n",pmu->m_Name.c_str());
	}
	
	g_IndexMap.insert(std::make_pair(odeBody,index));
}

ODE_MATINDEXLOOKUP* palODEMaterials::GetMaterialIndex(dGeomID odeBody) {
	MAP <dGeomID, ODE_MATINDEXLOOKUP> ::iterator itr;
	itr = g_IndexMap.find(odeBody);
	if (itr == g_IndexMap.end()) {		
		return NULL;
	}
	return &itr->second;
	//return m_IndexMap[odeBody];
}

////////////////
void palODEBody::SetMaterial(palMaterial *material) {
	for (unsigned int i=0;i<m_Geometries.size();i++) {
		palODEGeometry *poG = dynamic_cast<palODEGeometry *> (m_Geometries[i]);
		if (poG)
			poG->SetMaterial(material);
	}
	palBody::SetMaterial(material);
}


palODEGeometry::palODEGeometry() {
	odeGeom = 0;
}

palODEGeometry::~palODEGeometry() {
	if(odeGeom) { dGeomDestroy(odeGeom); odeGeom=0; }
}

palMatrix4x4& palODEGeometry::GetLocationMatrix() {
	if (odeGeom) {
		const dReal *pos = dGeomGetPosition (odeGeom);
		const dReal *R = dGeomGetRotation (odeGeom);
		convODEToPAL(pos,R,m_mLoc);
	}
	return m_mLoc;
}

void palODEGeometry::SetMaterial(palMaterial *material) {
	palODEMaterials::InsertIndex(odeGeom, material);
}

void palODEGeometry::SetPosition(palMatrix4x4 &loc) {
	dReal pos[3];
	dReal R[12];

	convODEFromPAL(pos,R,loc);

	dGeomSetPosition(odeGeom,pos[0],pos[1],pos[2]);
	dGeomSetRotation(odeGeom,R);
}

palODEBoxGeometry::palODEBoxGeometry() {
}

void palODEBoxGeometry::Init(palMatrix4x4 &pos, Float width, Float height, Float depth, Float mass) {
	palBoxGeometry::Init(pos,width,height,depth,mass);
	memset (&odeGeom ,0,sizeof(odeGeom));
	odeGeom = dCreateBox (g_space,m_fWidth,m_fHeight,m_fDepth);
	SetPosition(pos);

//	printf("trying: makin box geom\n");
	if (m_pBody) {
		palODEBody *pob=dynamic_cast<palODEBody *>(m_pBody);
		if (pob) {
			if (pob->odeBody) {
			dGeomSetBody(odeGeom,pob->odeBody);
//			printf("made geom with b:%d\n",pob->odeBody);
			}
		}
	}
}

palODESphereGeometry::palODESphereGeometry() {
}

void palODESphereGeometry::Init(palMatrix4x4 &pos, Float radius, Float mass) {
	palSphereGeometry::Init(pos,radius,mass);
	memset (&odeGeom ,0,sizeof(odeGeom));
	odeGeom = dCreateSphere(g_space, m_fRadius);
	SetPosition(pos);
	if (m_pBody) {
		palODEBody *pob=dynamic_cast<palODEBody *>(m_pBody);
		if (pob) {
			if (pob->odeBody) {
			dGeomSetBody(odeGeom,pob->odeBody);
			}
		}
	}
}

palODECapsuleGeometry::palODECapsuleGeometry() {
}

void palODECapsuleGeometry::Init(palMatrix4x4 &pos, Float radius, Float length, Float mass) {
	#pragma message("todo: fix cyl geom")
	palCapsuleGeometry::Init(pos,radius,length,mass);
	memset (&odeGeom ,0,sizeof(odeGeom));
	odeGeom = dCreateCCylinder (g_space, m_fRadius, m_fLength+m_fRadius);
	//mat_set_rotation(&pos,1,0,0);
	
	
	SetPosition(pos);
	if (m_pBody) {
		palODEBody *pob=dynamic_cast<palODEBody *>(m_pBody);
		if (pob) {
			if (pob->odeBody) {
			dGeomSetBody(odeGeom,pob->odeBody);
			palMatrix4x4 m;
			mat_identity(&m);
			mat_rotate(&m,90,1,0,0);
			//mat_set_rotation(&m,1,0,0);
			dReal pos[3];
			dReal R[12];
			convODEFromPAL(pos,R,m);
			dGeomSetOffsetRotation(odeGeom,R);
			}
		}
	}
	//mat_rotate(&pos,90,1,0,0);

}

palMatrix4x4& palODECapsuleGeometry::GetLocationMatrix() {
	if (odeGeom) {
		const dReal *pos = dGeomGetPosition (odeGeom);
		const dReal *R = dGeomGetRotation (odeGeom);
		convODEToPAL(pos,R,m_mLoc);
		mat_rotate(&m_mLoc,-90,1,0,0);
	}
	return m_mLoc;
}


palODEConvexGeometry::palODEConvexGeometry() {
}

#include "hull.h"

void palODEConvexGeometry::Init(palMatrix4x4 &pos, const Float *pVertices, int nVertices, Float mass) {
	
	palConvexGeometry::Init(pos,pVertices,nVertices,mass);
	int i;

	HullDesc desc;
	desc.SetHullFlag(QF_TRIANGLES);
	desc.mVcount       = nVertices;
	desc.mVertices     = new double[desc.mVcount*3];
	for (  i=0; i<desc.mVcount; i++)
	{
		desc.mVertices[i*3+0] = pVertices[i*3+0];
		desc.mVertices[i*3+1] = pVertices[i*3+1];
		desc.mVertices[i*3+2] = pVertices[i*3+2];
	}

	desc.mVertexStride = sizeof(double)*3;

	HullResult dresult;
	HullLibrary hl;
	HullError ret = hl.CreateConvexHull(desc,dresult);

	odeGeom = CreateTriMesh(pVertices,nVertices,(int*)dresult.mIndices,dresult.mNumFaces*3);

	palODEBody *pob = 0;
	if (m_pBody) 
		pob = dynamic_cast<palODEBody *>(m_pBody);
	if (!pob)
		return;
	if (pob->odeBody == 0) {
		return;
	}

	dGeomSetBody(odeGeom,pob->odeBody);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
palODEConvex::palODEConvex() {
}

void palODEConvex::Init(Float x, Float y, Float z, const Float *pVertices, int nVertices, Float mass) {
	memset (&odeBody ,0,sizeof(odeBody));
	odeBody = dBodyCreate (g_world);

	palConvex::Init(x,y,z,pVertices,nVertices,mass);

	palODEConvexGeometry *png=dynamic_cast<palODEConvexGeometry *> (m_Geometries[0]);
	png->SetMass(mass);
	
	dMass m;
	m_fMass=mass;
#pragma message("todo: mass set in convex geom")
	dMassSetSphereTotal(&m,1,1);
	dBodySetMass(odeBody,&m);
}

palODECompoundBody::palODECompoundBody() {
	

}

void palODECompoundBody::Finalize() {

	SumInertia();

	odeBody = dBodyCreate (g_world);
	
	for (int i=0;i<m_Geometries.size();i++) {
		palODEGeometry *pog=dynamic_cast<palODEGeometry *> (m_Geometries[i]);
		
		dReal pos[3];
		dReal R[12];

		convODEFromPAL(pos,R,pog->GetOffsetMatrix());
		if (pog->odeGeom,odeBody) {
		dGeomSetBody(pog->odeGeom,odeBody);
		dGeomSetOffsetPosition(pog->odeGeom,pos[0],pos[1],pos[2]);
		dGeomSetOffsetRotation(pog->odeGeom,R);
		} else {
			//error
		}
	}
	dMass m;
	dMassSetSphereTotal(&m,1,1);
	dBodySetMass(odeBody,&m);

	SetPosition(m_mLoc);

}

palODEBox::palODEBox() {
}

void palODEBox::Init(Float x, Float y, Float z, Float width, Float height, Float depth, Float mass) {
	memset (&odeBody ,0,sizeof(odeBody));
	odeBody = dBodyCreate (g_world);

	palBox::Init(x,y,z,width,height,depth,mass); //create geom

	SetMass(mass);
	SetPosition(x,y,z);
//	printf("made box %d, b:%d",this,odeBody);
};

void palODEBox::SetMass(Float mass) {
	m_fMass=mass;
	//denisty == 5.0f //how this relates to mass i dont know. =( desnity = mass/volume ?
	dMass m;
//	dMassSetBox (&m,5.0f,m_fWidth,m_fHeight,m_fDepth);
	palBoxGeometry *m_pBoxGeom = dynamic_cast<palBoxGeometry *>(m_Geometries[0]);
	dMassSetBoxTotal(&m,mass,m_pBoxGeom->m_fWidth,m_pBoxGeom->m_fHeight,m_pBoxGeom->m_fDepth);
	dBodySetMass(odeBody,&m);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

palODESphere::palODESphere() {
}

void palODESphere::Init(Float x, Float y, Float z, Float radius, Float mass) {
	memset (&odeBody ,0,sizeof(odeBody));
	odeBody = dBodyCreate (g_world);

	palSphere::Init(x,y,z,radius,mass);

	SetMass(mass);
	SetPosition(x,y,z);
}

void palODESphere::SetMass(Float mass) {
	m_fMass=mass;
	dMass m;
	palSphereGeometry *m_pSphereGeom = dynamic_cast<palSphereGeometry *>(m_Geometries[0]);
	dMassSetSphereTotal(&m,m_fMass,m_pSphereGeom->m_fRadius);
	dBodySetMass(odeBody,&m);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

palODECylinder::palODECylinder() {
}

void palODECylinder::Init(Float x, Float y, Float z, Float radius, Float length, Float mass) {
	memset (&odeBody ,0,sizeof(odeBody));
	odeBody = dBodyCreate (g_world);

	palCapsule::Init(x,y,z,radius,length,mass);
	
	SetPosition(x,y,z);
	SetMass(mass);
}

void palODECylinder::SetMass(Float mass) {
	m_fMass=mass;
	dMass m;
//	dMassSetSphereTotal(&m,m_fMass,m_fRadius);
	palCapsuleGeometry *m_pCylinderGeom = dynamic_cast<palCapsuleGeometry *> (m_Geometries[0]);
	dMassSetCappedCylinderTotal(&m,m_fMass,2,m_pCylinderGeom->m_fRadius,m_pCylinderGeom->m_fLength);
	//dMassSetParameters 
	dBodySetMass(odeBody,&m);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
palODELink::palODELink() {
	odeJoint = 0;
	odeMotorJoint = 0;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

palODESphericalLink::palODESphericalLink(){
}

void palODESphericalLink::InitMotor() {
	if (odeMotorJoint == 0) {
	odeMotorJoint = dJointCreateAMotor (g_world,0);
	palODEBody *body0 = dynamic_cast<palODEBody *> (m_pParent);
	palODEBody *body1 = dynamic_cast<palODEBody *> (m_pChild);
    dJointAttach (odeMotorJoint ,body0->odeBody ,body1->odeBody );
	dJointSetAMotorNumAxes (odeMotorJoint,3);
    dJointSetAMotorAxis (odeMotorJoint,0,1, 0,0,1);
    dJointSetAMotorAxis (odeMotorJoint,2,2, 1,0,0); //i may need to check this?
	dJointSetAMotorMode (odeMotorJoint,dAMotorEuler);
	}
	if (odeMotorJoint == 0) {
		printf("OH FUCK!!!!!!!! on line %d\n",__LINE__);
	}
}
/*
void palODESphericalLink::SetLimits(Float lower_limit_rad, Float upper_limit_rad) {
	palSphericalLink::SetLimits(lower_limit_rad,upper_limit_rad);
	InitMotor();

	dJointSetAMotorParam(odeMotorJoint,dParamLoStop,m_fLowerLimit);
	dJointSetAMotorParam(odeMotorJoint,dParamHiStop,m_fUpperLimit);

	dJointSetAMotorParam(odeMotorJoint,dParamLoStop2,m_fLowerLimit);
	dJointSetAMotorParam(odeMotorJoint,dParamHiStop2,m_fUpperLimit);

	//twist:
	//dJointSetAMotorParam(odeMotorJoint,dParamLoStop3,m_fLowerLimit);
	//dJointSetAMotorParam(odeMotorJoint,dParamHiStop3,m_fUpperLimit);
}

void palODESphericalLink::SetTwistLimits(Float lower_limit_rad, Float upper_limit_rad) {
	palSphericalLink::SetTwistLimits(lower_limit_rad,upper_limit_rad);
	InitMotor();

	dJointSetAMotorParam(odeMotorJoint,dParamLoStop3,m_fLowerTwistLimit);
	dJointSetAMotorParam(odeMotorJoint,dParamHiStop3,m_fUpperTwistLimit);
}
*/
void palODESphericalLink::Init(palBodyBase *parent, palBodyBase *child, Float x, Float y, Float z) {
	palSphericalLink::Init(parent,child,x,y,z);
	palODEBody *body0 = dynamic_cast<palODEBody *> (parent);
	palODEBody *body1 = dynamic_cast<palODEBody *> (child);
//	printf("%d and %d\n",body0,body1);

	odeJoint = dJointCreateBall(g_world,0);
	dJointAttach (odeJoint,body0->odeBody ,body1->odeBody );

	SetAnchor(x,y,z);
}

void palODESphericalLink::SetAnchor(Float x, Float y, Float z) {
	dJointSetBallAnchor (odeJoint, x, y, z);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

palODERevoluteLink::palODERevoluteLink() {
}

void palODERevoluteLink::AddTorque(Float torque) {
	dJointAddHingeTorque(odeJoint,torque);
}
/*
Float palODERevoluteLink::GetAngle() {
	return dJointGetHingeAngle(odeJoint);
}*/

void palODERevoluteLink::SetLimits(Float lower_limit_rad, Float upper_limit_rad) {
	palRevoluteLink::SetLimits(lower_limit_rad,upper_limit_rad);
	dJointSetHingeParam(odeJoint,dParamLoStop,m_fLowerLimit);
	dJointSetHingeParam(odeJoint,dParamHiStop,m_fUpperLimit);
}

void palODERevoluteLink::Init(palBodyBase *parent, palBodyBase *child, Float x, Float y, Float z, Float axis_x, Float axis_y, Float axis_z) {
	palRevoluteLink::Init(parent,child,x,y,z,axis_x,axis_y,axis_z);
	palODEBody *body0 = dynamic_cast<palODEBody *> (parent);
	palODEBody *body1 = dynamic_cast<palODEBody *> (child);
//	printf("%d and %d\n",body0,body1);

	odeJoint = dJointCreateHinge(g_world,0);
	dJointAttach (odeJoint,body0->odeBody ,body1->odeBody );

	SetAnchorAxis(x,y,z,axis_x,axis_y,axis_z);
}

void palODERevoluteLink::SetAnchorAxis(Float x, Float y, Float z, Float axis_x, Float axis_y, Float axis_z) {
	dJointSetHingeAnchor(odeJoint,x,y,z);
	dJointSetHingeAxis(odeJoint,axis_x,axis_y,axis_z);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

palODEPrismaticLink::palODEPrismaticLink() {
}

void palODEPrismaticLink::Init(palBodyBase *parent, palBodyBase *child, Float x, Float y, Float z, Float axis_x, Float axis_y, Float axis_z) {
	palPrismaticLink::Init(parent,child,x,y,z,axis_x,axis_y,axis_z);
	palODEBody *body0 = dynamic_cast<palODEBody *> (parent);
	palODEBody *body1 = dynamic_cast<palODEBody *> (child);
//	printf("%d and %d\n",body0,body1);

	odeJoint = dJointCreateSlider (g_world,0);
	dJointAttach (odeJoint,body0->odeBody ,body1->odeBody );

	SetAnchorAxis(x,y,z,axis_x,axis_y,axis_z);
}

void palODEPrismaticLink::SetAnchorAxis(Float x, Float y, Float z, Float axis_x, Float axis_y, Float axis_z) {
	dJointSetSliderAxis (odeJoint,axis_x,axis_y,axis_z);
//	dJointSetHingeAnchor(odeJoint,x,y,z);
//	dJointSetHingeAxis(odeJoint,axis_x,axis_y,axis_z);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
palODETerrain::palODETerrain() {
	odeGeom = 0;
}

void palODETerrain::SetMaterial(palMaterial *material) {
	if (odeGeom)
		palODEMaterials::InsertIndex(odeGeom, material);
}

palMatrix4x4& palODETerrain::GetLocationMatrix() {
	memset(&m_mLoc,0,sizeof(m_mLoc));
	m_mLoc._11=1;m_mLoc._22=1;m_mLoc._33=1;m_mLoc._44=1;
	m_mLoc._41=m_fPosX;
	m_mLoc._42=m_fPosY;
	m_mLoc._43=m_fPosZ;
	return m_mLoc;
}


palODETerrainPlane::palODETerrainPlane() {
}

palMatrix4x4& palODETerrainPlane::GetLocationMatrix() {
	memset(&m_mLoc,0,sizeof(m_mLoc));
	m_mLoc._11=1;m_mLoc._22=1;m_mLoc._33=1;m_mLoc._44=1;
	return m_mLoc;
}

void palODETerrainPlane::Init(Float x, Float y, Float z, Float size) {
	palTerrainPlane::Init(x,y,z,size);
	odeGeom=dCreatePlane (g_space,0,1,0,0);
}

palODEOrientatedTerrainPlane::palODEOrientatedTerrainPlane() {
}

void palODEOrientatedTerrainPlane::Init(Float x, Float y, Float z, Float nx, Float ny, Float nz, Float min_size) {
	palOrientatedTerrainPlane::Init(x,y,z,nx,ny,nz,min_size);
	odeGeom=dCreatePlane (g_space,nx,ny,nz,CalculateD());
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

palODETerrainHeightmap::palODETerrainHeightmap() {
}

void palODETerrainHeightmap::Init(Float px, Float py, Float pz, Float width, Float depth, int terrain_data_width, int terrain_data_depth, const Float *pHeightmap) {
#if 0
	palTerrainHeightmap::Init(px,py,pz,width,depth,terrain_data_width,terrain_data_depth,pHeightmap);
	int iTriIndex;
	float fTerrainX, fTerrainZ;
	int x,z;

		dVector3 *vertices; // vertex array for trimesh geom
		int *indices; // index array for trimesh geom
		int vertexcount; // number of vertices in the vertex array
		int indexcount; // number of indices in the index array

		int nv=m_iDataWidth*m_iDataDepth;
		int ni=(m_iDataWidth-1)*(m_iDataDepth-1)*2*3;

		vertexcount=nv;
		indexcount=ni;

		vertices = new dVector3[nv];
		indices = new int[ni];

		// Set the vertex values
		fTerrainZ = -m_fDepth/2;
		for (z=0; z<m_iDataDepth; z++)
		{
			fTerrainX = -m_fWidth/2;
			for (x=0; x<m_iDataWidth; x++)
			{
				//triVertices[x + z*m_iDataWidth].Set(fTerrainX, gfTerrainHeights[x][z], fTerrainZ);
				vertices[x + z*m_iDataWidth][0]=fTerrainX;
				vertices[x + z*m_iDataWidth][1]=pHeightmap[x+z*m_iDataWidth]; 
				vertices[x + z*m_iDataWidth][2]=fTerrainZ;
				printf("(%d,%d),%f\n",x,z,pHeightmap[x+z*m_iDataWidth]);
				fTerrainX += (m_fWidth / (m_iDataWidth-1));
			}
			fTerrainZ += (m_fDepth / (m_iDataDepth-1));
		}
	
	int xDim=m_iDataWidth;
	int yDim=m_iDataDepth;
	int y;
	for (y=0;y < yDim-1;y++)
	for (x=0;x < xDim-1;x++) {
		/*
//		SetIndex(((x+y*(xDim-1))*2)+0,(y*xDim)+x,(y*xDim)+xDim+x,(y*xDim)+x+1);
		indices[(((x+y*(xDim-1))*2)+0)*3+0]=(y*xDim)+x;
		indices[(((x+y*(xDim-1))*2)+0)*3+1]=(y*xDim)+xDim+x;
		indices[(((x+y*(xDim-1))*2)+0)*3+2]=(y*xDim)+x+1;

//		SetIndex(((x+y*(xDim-1))*2)+1,(y*xDim)+x+1,(y*xDim)+xDim+x,(y*xDim)+x+xDim+1);

		indices[(((x+y*(xDim-1))*2)+1)*3+0]=(y*xDim)+x+1;
		indices[(((x+y*(xDim-1))*2)+1)*3+1]=(y*xDim)+xDim+x;
		indices[(((x+y*(xDim-1))*2)+1)*3+2]=(y*xDim)+x+xDim+1;
		*/
		indices[iTriIndex*3+0]=(y*xDim)+x;
		indices[iTriIndex*3+1]=(y*xDim)+xDim+x;
		indices[iTriIndex*3+2]=(y*xDim)+x+1;
		// Move to the next triangle in the array
		iTriIndex += 1;
		
		indices[iTriIndex*3+0]=(y*xDim)+x+1;
		indices[iTriIndex*3+1]=(y*xDim)+xDim+x;
		indices[iTriIndex*3+2]=(y*xDim)+x+xDim+1;
		// Move to the next triangle in the array
		iTriIndex += 1;
	}

		// build the trimesh data
		dTriMeshDataID data=dGeomTriMeshDataCreate();
		dGeomTriMeshDataBuildSimple(data,(dReal*)vertices,vertexcount,indices,indexcount);
		// build the trimesh geom 
		odeGeom=dCreateTriMesh(g_space,data,0,0,0);
		// set the geom position 
		dGeomSetPosition(odeGeom,m_fPosX,m_fPosY,m_fPosZ);
		// in our application we don't want geoms constructed with meshes (the terrain) to have a body
		dGeomSetBody(odeGeom,0); 
#else
	palTerrainHeightmap::Init(px,py,pz,width,depth,terrain_data_width,terrain_data_depth,pHeightmap);
	int iTriIndex;
	float fTerrainX, fTerrainZ;
	int x,z;

	int nv=m_iDataWidth*m_iDataDepth;
	int ni=(m_iDataWidth-1)*(m_iDataDepth-1)*2*3;

	Float *v = new Float[nv*3];
	int *ind = new int[ni];

	// Set the vertex values
	fTerrainZ = -m_fDepth/2;
	for (z=0; z<m_iDataDepth; z++)
	{
		fTerrainX = -m_fWidth/2;
		for (x=0; x<m_iDataWidth; x++)
		{
			v[(x + z*m_iDataWidth)*3+0]=fTerrainX + m_fPosX;
			v[(x + z*m_iDataWidth)*3+1]=pHeightmap[x+z*m_iDataWidth] + m_fPosY;
			v[(x + z*m_iDataWidth)*3+2]=fTerrainZ + m_fPosZ;

		fTerrainX += (m_fWidth / (m_iDataWidth-1));
		}
		fTerrainZ += (m_fDepth / (m_iDataDepth-1));
	}

	iTriIndex = 0;
	int xDim=m_iDataWidth;
	int yDim=m_iDataDepth;
	int y;
	for (y=0;y < yDim-1;y++)
	for (x=0;x < xDim-1;x++) {
		ind[iTriIndex*3+0]=(y*xDim)+x;
		ind[iTriIndex*3+1]=(y*xDim)+xDim+x;
		ind[iTriIndex*3+2]=(y*xDim)+x+1;
		// Move to the next triangle in the array
		iTriIndex += 1;
		
		ind[iTriIndex*3+0]=(y*xDim)+x+1;
		ind[iTriIndex*3+1]=(y*xDim)+xDim+x;
		ind[iTriIndex*3+2]=(y*xDim)+x+xDim+1;
		// Move to the next triangle in the array
		iTriIndex += 1;
	}
	palODETerrainMesh::Init(px,py,pz,v,nv,ind,ni);

	delete [] v;
	delete [] ind;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
palODETerrainMesh::palODETerrainMesh() {
}
/*
palMatrix4x4& palODETerrainMesh::GetLocationMatrix() {
	memset(&m_mLoc,0,sizeof(m_mLoc));
	m_mLoc._11=1;m_mLoc._22=1;m_mLoc._33=1;m_mLoc._44=1;
	m_mLoc._41=m_fPosX;
	m_mLoc._42=m_fPosY;
	m_mLoc._43=m_fPosZ;
	return m_mLoc;
}
*/

void palODETerrainMesh::Init(Float px, Float py, Float pz, const Float *pVertices, int nVertices, const int *pIndices, int nIndices) {
	palTerrainMesh::Init(px,py,pz,pVertices,nVertices,pIndices,nIndices);

	odeGeom = CreateTriMesh(pVertices,nVertices,pIndices,nIndices);
	// set the geom position 
	dGeomSetPosition(odeGeom,m_fPosX,m_fPosY,m_fPosZ);
	// in our application we don't want geoms constructed with meshes (the terrain) to have a body
	dGeomSetBody(odeGeom,0); 

	//delete [] spacedvert;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////