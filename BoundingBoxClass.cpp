#include "BoundingBoxClass.h"
//  BoundingBoxClass
void BoundingBoxClass::Init(void)
{
	m_bInitialized = false;
	m_v3Min = vector3(0.0f);
	m_v3Max = vector3(0.0f);
	m_v3Centroid = vector3(0.0f);
	m_sName = "NULL";
}
void BoundingBoxClass::Swap(BoundingBoxClass& other)
{
	std::swap(m_bInitialized, other.m_bInitialized);
	std::swap(m_v3Min, other.m_v3Min);
	std::swap(m_v3Max, other.m_v3Max);
	std::swap(m_v3Centroid, other.m_v3Centroid);
	std::swap(m_sName, other.m_sName);
}
void BoundingBoxClass::Release(void)
{
	//No pointers to release
}
//The big 3
BoundingBoxClass::BoundingBoxClass(){Init();}
BoundingBoxClass::BoundingBoxClass(BoundingBoxClass const& other)
{
	m_bInitialized = other.m_bInitialized;
	m_v3Min = other.m_v3Min;
	m_v3Max = other.m_v3Max;
	m_v3Centroid = other.m_v3Centroid;
	m_sName = other.m_sName;
}
BoundingBoxClass& BoundingBoxClass::operator=(BoundingBoxClass const& other)
{
	if(this != &other)
	{
		Release();
		Init();
		BoundingBoxClass temp(other);
		Swap(temp);
	}
	return *this;
}
BoundingBoxClass::~BoundingBoxClass(){Release();};
//Accessors
bool BoundingBoxClass::IsInitialized(void){ return m_bInitialized; }
vector3 BoundingBoxClass::GetMinimumOBB(void){ return m_v3Min; }
vector3 BoundingBoxClass::GetMaximumOBB(void){ return m_v3Max; }
vector3 BoundingBoxClass::GetCentroid(void){ return m_v3Centroid; }
String BoundingBoxClass::GetName(void){return m_sName;}
//Methods
void BoundingBoxClass::GenerateOrientedBoundingBox(String a_sInstanceName)
{
	//If this has already been initialized there is nothing to do here
	if(m_bInitialized)
		return;
	MeshManagerSingleton* pMeshMngr = MeshManagerSingleton::GetInstance();
	if(pMeshMngr->IsInstanceCreated(a_sInstanceName))
	{
		m_sName = a_sInstanceName;
		
		std::vector<vector3> lVertices = pMeshMngr->GetVertices(m_sName);
		unsigned int nVertices = lVertices.size();
		m_v3Centroid = lVertices[0];
		m_v3Max = lVertices[0];
		m_v3Min = lVertices[0];
		for(unsigned int nVertex = 1; nVertex < nVertices; nVertex++)
		{
			//m_v3Centroid += lVertices[nVertex];
			if(m_v3Min.x > lVertices[nVertex].x)
				m_v3Min.x = lVertices[nVertex].x;
			else if(m_v3Max.x < lVertices[nVertex].x)
				m_v3Max.x = lVertices[nVertex].x;
			
			if(m_v3Min.y > lVertices[nVertex].y)
				m_v3Min.y = lVertices[nVertex].y;
			else if(m_v3Max.y < lVertices[nVertex].y)
				m_v3Max.y = lVertices[nVertex].y;

			if(m_v3Min.z > lVertices[nVertex].z)
				m_v3Min.z = lVertices[nVertex].z;
			else if(m_v3Max.z < lVertices[nVertex].z)
				m_v3Max.z = lVertices[nVertex].z;
		}
		m_v3Centroid = (m_v3Min + m_v3Max) / 2.0f;

		m_v3Size.x = glm::distance(vector3(m_v3Min.x, 0.0f, 0.0f), vector3(m_v3Max.x, 0.0f, 0.0f));
		m_v3Size.y = glm::distance(vector3(0.0f, m_v3Min.y, 0.0f), vector3(0.0f, m_v3Max.y, 0.0f));
		m_v3Size.z = glm::distance(vector3(0.0f, 0.0f, m_v3Min.z), vector3(0.0f, 0.0f, m_v3Max.z));

		m_bInitialized = true;
	}
}
void BoundingBoxClass::GenerateAxisAlignedBoundingBox(matrix4 a_m4ModeltoWorld)
{
	//Generate the Axis Aligned Bounding Box here based on the Oriented Bounding Box
	
	// NOTE ON AUTO: http://thbecker.net/articles/auto_and_decltype/section_02.html
	
	/*
	 the first thing we need to do, is to recreate the rectangular prism (RP) that represents the OOBB
	*/
	auto transMaxOBB = this->GetMaximumOBB(); 
	auto transMinOBB = this->GetMinimumOBB();

	/*
		obviously this finds the center of the OBB,
		we need it because we need to find:
			- the half width (x-axis)
			- the half height (y-axis)
			- the half depth (z-axis)
		these will be used to recreate each corner of the RP
		sizes has each component equal to the half-size along that axis (so a width of 2 would have a component of 1)
	*/
	auto obbCenter = (transMaxOBB + transMinOBB) / 2.0f;
	auto halfSizes = transMaxOBB - obbCenter;
	
	// set up vectors for the maximium along each axis and the minimium along each axis
	auto max = vector3();
	auto min = vector3();

	/*
		this gives us a matrix that transforms our points WITHOUT translating them
		the following matrix shows a translation matrix:
		[[ 1, 0, 0, x],
		 [ 0, 1, 0, y],
		 [ 0, 0, 1, z],
		 [ 0, 0, 0, 1]]

		 cutting off the last row and last column gives us a matrix that keeps the other transformations, but ignores translations
	*/
	auto noTrans = glm::mat3(a_m4ModeltoWorld);

	/*
		so now we need to loop through all the corners of the RP
		each corner lies at one of two points of any axis:
		 -- +x or -x
		 -- +y or -y
		 -- +z or -z

		 the loops go kind of like this:
		 1) -1 = -1
		 2) -1 + 2 = 1
		 3) 1 + 2 = 3 (since 3 > 2 we never actually perform this)
		 we do this for x, y, and z

	*/
	for(int x = -1; x < 2; x += 2) {
		for(int y = -1; y < 2; y += 2) {
			for(int z = -1; z < 2; z += 2) {
				/*
					the new point is at the axis times the size of that axis
					OFFSET by the center of the obb
				*/
				vector3 np;
				np.x = x * halfSizes.x;
				np.y = y * halfSizes.y;
				np.z = z * halfSizes.z;
				np += obbCenter;
				/*
					so now we rotate the point with the no translation to get where the point is in the world
				*/
				np = noTrans * np;
				/*
					now that we have the rotated point, we just find the max and min along each axis
					this is the same as the previous algorithm used to find the OBB
				*/ 
				max = glm::max(max, np);
				min = glm::min(min, np);
			}
		}
	}

	// finding the center to translate the box
	aabbCenter = (min + max) / 2.0f;

	// find the sizes along each axis
	aabbSize.x = glm::distance(vector3(min.x, 0.0f, 0.0f), vector3(max.x, 0.0f, 0.0f));
	aabbSize.y = glm::distance(vector3(0.0f, min.y, 0.0f), vector3(0.0f, max.y, 0.0f));
	aabbSize.z = glm::distance(vector3(0.0f, 0.0f, min.z), vector3(0.0f, 0.0f, max.z));

	/*
		now we need to translate the box to where it actually is
		to do that we first grab the identity matrix, and bring the last column of the orignal matrix
		to transform everything
	*/
	auto tranOnly = glm::mat4(1.0f);
	tranOnly[3] = a_m4ModeltoWorld[3];

	// adding everything to be rendered
	MeshManagerSingleton* pMeshMngr = MeshManagerSingleton::GetInstance();
	pMeshMngr->AddAxisToQueue(tranOnly * glm::translate(aabbCenter));
	pMeshMngr->AddCubeToQueue(tranOnly * glm::translate(aabbCenter) * glm::scale(aabbSize), aabbColor, MERENDER::WIRE);
	

}
void BoundingBoxClass::AddBoxToRenderList(matrix4 a_m4ModelToWorld, vector3 a_vColor, bool a_bRenderCentroid)
{
	if(!m_bInitialized)
		return;
	MeshManagerSingleton* pMeshMngr = MeshManagerSingleton::GetInstance();
	if(a_bRenderCentroid)
		pMeshMngr->AddAxisToQueue(a_m4ModelToWorld * glm::translate(m_v3Centroid));
	pMeshMngr->AddCubeToQueue(a_m4ModelToWorld * glm::translate(m_v3Centroid) * glm::scale(m_v3Size), a_vColor, MERENDER::WIRE);
}