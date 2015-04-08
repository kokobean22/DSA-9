#include "BoundingBoxManagerSingleton.h"

//  BoundingBoxManagerSingleton
BoundingBoxManagerSingleton* BoundingBoxManagerSingleton::m_pInstance = nullptr;
void BoundingBoxManagerSingleton::Init(void)
{
	m_nBoxs = 0;
}
void BoundingBoxManagerSingleton::Release(void)
{
	//Clean the list of Boxs
	for(int n = 0; n < m_nBoxs; n++)
	{
		//Make sure to release the memory of the pointers
		if(m_lBox[n] != nullptr)
		{
			delete m_lBox[n];
			m_lBox[n] = nullptr;
		}
	}
	m_lBox.clear();
	m_lMatrix.clear();
	m_lColor.clear();
	m_nBoxs = 0;
}
BoundingBoxManagerSingleton* BoundingBoxManagerSingleton::GetInstance()
{
	if(m_pInstance == nullptr)
	{
		m_pInstance = new BoundingBoxManagerSingleton();
	}
	return m_pInstance;
}
void BoundingBoxManagerSingleton::ReleaseInstance()
{
	if(m_pInstance != nullptr)
	{
		delete m_pInstance;
		m_pInstance = nullptr;
	}
}
//The big 3
BoundingBoxManagerSingleton::BoundingBoxManagerSingleton(){Init();}
BoundingBoxManagerSingleton::BoundingBoxManagerSingleton(BoundingBoxManagerSingleton const& other){ }
BoundingBoxManagerSingleton& BoundingBoxManagerSingleton::operator=(BoundingBoxManagerSingleton const& other) { return *this; }
BoundingBoxManagerSingleton::~BoundingBoxManagerSingleton(){Release();};
//Accessors
int BoundingBoxManagerSingleton::GetBoxTotal(void){ return m_nBoxs; }

//--- Non Standard Singleton Methods
void BoundingBoxManagerSingleton::GenerateBoundingBox(matrix4 a_mModelToWorld, String a_sInstanceName)
{
	MeshManagerSingleton* pMeshMngr = MeshManagerSingleton::GetInstance();
	//Verify the instance is loaded
	if(pMeshMngr->IsInstanceCreated(a_sInstanceName))
	{//if it is check if the Box has already been created
		int nBox = IdentifyBox(a_sInstanceName);
		if(nBox == -1)
		{
			//Create a new bounding Box
			BoundingBoxClass* pBB = new BoundingBoxClass();
			//construct its information out of the instance name
			pBB->GenerateOrientedBoundingBox(a_sInstanceName);
			//Push the Box back into the list
			m_lBox.push_back(pBB);
			//Push a new matrix into the list
			m_lMatrix.push_back(matrix4(IDENTITY));
			//Specify the color the Box is going to have
			m_lColor.push_back(vector3(0.0f));
			//Increase the number of Boxes
			m_nBoxs++;
		}
		else //If the box has already been created you will need to check its global orientation
		{
			m_lBox[nBox]->GenerateAxisAlignedBoundingBox(a_mModelToWorld);
		}
		nBox = IdentifyBox(a_sInstanceName);
		m_lMatrix[nBox] = a_mModelToWorld;
	}
}

void BoundingBoxManagerSingleton::SetBoundingBoxSpace(matrix4 a_mModelToWorld, String a_sInstanceName)
{
	int nBox = IdentifyBox(a_sInstanceName);
	//If the Box was found
	if(nBox != -1)
	{
		//Set up the new matrix in the appropriate index
		m_lMatrix[nBox] = a_mModelToWorld;
	}
}

int BoundingBoxManagerSingleton::IdentifyBox(String a_sInstanceName)
{
	//Go one by one for all the Boxs in the list
	for(int nBox = 0; nBox < m_nBoxs; nBox++)
	{
		//If the current Box is the one we are looking for we return the index
		if(a_sInstanceName == m_lBox[nBox]->GetName())
			return nBox;
	}
	return -1;//couldn't find it return with no index
}

void BoundingBoxManagerSingleton::AddBoxToRenderList(String a_sInstanceName)
{
	//If I need to render all
	if(a_sInstanceName == "ALL")
	{
		for(int nBox = 0; nBox < m_nBoxs; nBox++)
		{
			m_lBox[nBox]->AddBoxToRenderList(m_lMatrix[nBox], m_lColor[nBox], true);
		}
	}
	else
	{
		int nBox = IdentifyBox(a_sInstanceName);
		if(nBox != -1)
		{
			m_lBox[nBox]->AddBoxToRenderList(m_lMatrix[nBox], m_lColor[nBox], true);
		}
	}
}

void BoundingBoxManagerSingleton::CalculateCollision(void)
{
	//Create a placeholder for all center points
	std::vector<vector3> lCentroid;
	//for all Boxs...
	for(int nBox = 0; nBox < m_nBoxs; nBox++)
	{
		//Make all the Boxs black
		m_lColor[nBox] = vector3(0.0f);
		//Place all the centroids of Boxs in global space
		lCentroid.push_back(static_cast<vector3>(m_lMatrix[nBox] * vector4(m_lBox[nBox]->GetCentroid(), 1.0f)));
		// this makes the outside box default to white
		m_lBox[nBox]->aabbColor = vector3(1.0f);
	}

	//Now the actual check
	for(int i = 0; i < m_nBoxs - 1; i++)
	{
		/*
		here I use seperating axis theorem
		you might use your own collision detection here
		here are some resources about SAT:
		http://www.metanetsoftware.com/technique/tutorialA.html
		http://gamedevelopment.tutsplus.com/tutorials/collision-detection-using-the-separating-axis-theorem--gamedev-169
		http://www.dyn4j.org/2010/01/sat/
		*/
		auto box1HalfSize = m_lBox[i]->aabbSize / 2.0f;
		auto trans1 = vector3(m_lMatrix[i][3]);
		auto box1center = trans1 + m_lBox[i]->aabbCenter;

		for(int j = i + 1; j < m_nBoxs; j++)
		{
			auto box2halfsize = m_lBox[j]->aabbSize / 2.0f;
			auto trans2 = vector3(m_lMatrix[j][3]);
			auto box2center = trans2 + m_lBox[j]->aabbCenter;

			auto dist = glm::abs(box1center - box2center);

			auto sumHalfSizes = box1HalfSize + box2halfsize;

			auto bColliding = true;

			for(int k = 0; k < 3; k++)
			{
				if(dist[k] > sumHalfSizes[k])
				{
					bColliding = false;
					break;
				}
			}

			if(bColliding)
			{
				m_lColor[i] = m_lColor[j] = MECYAN; // make the inside box of both models cyan
				m_lBox[i]->aabbColor = m_lBox[j]->aabbColor = MERED; //We make the outside box of both models red
			}
		}
	}
}