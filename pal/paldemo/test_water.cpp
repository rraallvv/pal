#define NOMINMAX 
#include "test_water.h"

#include "../pal/palFluid.h"

FACTORY_CLASS_IMPLEMENTATION(Test_Water);
// Fluid globals


VECTOR<palFluid *> g_Fluids;

void Test_Water::AdditionalRender() {
	if (g_Fluids.size()==0) return;
	
	int i,j;

	glColor3f(1,1,1);
	glBegin(GL_POINTS);
	
	for (j=0; j<g_Fluids.size();j++) {
		palVector3 *pp = g_Fluids[j]->GetParticlePositions();
		for (i=0; i<g_Fluids[j]->GetNumParticles(); i++)
		{
			glVertex3f(	pp[i].x,
				pp[i].y,
				pp[i].z);
		}
	}
	glEnd();
}

void Test_Water::Init(int terrain_type) {
		CreateTerrain(terrain_type);
}

void Test_Water::Update() 
 {
	
	 int i;
	 for (i=0;i<act.size();i++)
		 act[i]->Apply();
	 for (i=0;i<bodies.size();i++)
		 bodies[i]->SetActive(true);

	};	


void Test_Water::Input(SDL_Event E) {
		int i,j,k;
		palBodyBase *pb= NULL;
		switch(E.type) {
		case SDL_KEYDOWN:
			switch (E.key.keysym.sym) {
			case SDLK_w:
				{
					int size = 5;
					float d = 0.1f;
					palFluid *pf = dynamic_cast<palFluid * >(PF->CreateObject("palFluid"));
					if (!pf) return;
					pf->Init();
					for (k=0;k<size;k++) {
						for (j=0;j<size*16;j++) {
							for (i=0;i<size;i++) {
								pf->AddParticle(i*d-(size*d)*0.5f,j*d+4,k*d-(size*d)*0.5f,0,0,0);
					}
					}
					}
					pf->Finalize();
					g_Fluids.push_back(pf);
				}
				break;

			case SDLK_1:
				{
				pb = CreateBody("palBox",sfrand()*3,sfrand()*0.5f+0.1f,sfrand()*3,ufrand()+0.1f,ufrand()+0.1f,ufrand()+0.1f,1);

				palFakeBuoyancy *pfb = new palFakeBuoyancy;//dynamic_cast<palFakeBuoyancy *>( PF->CreateObject("palFakeBuoyancy") );
				pfb->Init(dynamic_cast<palBody*>(pb));
				act.push_back(pfb);

				if (pb == NULL) {
					printf("Error: Could not create a box\n");
				} 
				break;
				}
			case SDLK_2:
				palSphere *ps;
				ps = NULL;
				ps=dynamic_cast<palSphere *>(PF->CreateObject("palSphere"));
				if (ps) {
					float r = 0.5f*ufrand()+0.05f;
					float v = r*r*r*4/3.0f*3.14;
					ps->Init(sfrand()*1,sfrand()*2+5.0f,sfrand()*1,r,v*0.1f);
					BuildGraphics(ps);
				} else {
					printf("Error: Could not create a sphere\n");
				} 
				pb = ps;
				break;
			case SDLK_5:
				{
					float mult;
					mult = 0.5f;
					float r = mult*0.25;
					float v = r*r*r*(4/3.0f)*3.14;
					pb = CreateBody("palSphere",sfrand()*0.01f,4.0f,0,r,mult,mult,v*1000);
				}
				break;
			case SDLK_4:
				{
				int dist;
				dist = 1;
				float mult;
				mult = 0.7f;
				float ymult=1.5f;
				for (j=-dist;j<=dist;j++)
					for (i=-dist;i<=dist;i++) {
						if ((i==0) && (j==0))
							;//pb = CreateBody("palSphere",0,5.0f,0,mult*0.25f,mult,mult,1000.0f);
						else
							pb = CreateBody("palStaticBox",i*mult,ymult*0.5f,j*mult,mult,ymult,mult,0,true);
					}
				
				}
				pb = 0;
				break;
			case SDLK_8:
				{
					if (bodies.size()>0) {
						int r= rand() % bodies.size();
						bodies[r]->SetPosition(sfrand()*3,sfrand()*2+5.0f,sfrand()*3,ufrand()*M_PI,ufrand()*M_PI,ufrand()*M_PI);
						bodies[r]->SetActive(true);
					}
				}
				break;
			case SDLK_9:
				if (bodies.size()>0) {
				DeleteGraphics(bodies[0]);
				delete bodies[0];
				
				bodies.erase(bodies.begin());
				}
				break;
			} 
			if (pb) {
				bodies.push_back(dynamic_cast<palBody*>(pb));
			}
			break;
		}
	}