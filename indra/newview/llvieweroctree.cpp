/** 
 * @file llvieweroctree.cpp
 * @brief LLViewerOctreeGroup class implementation and supporting functions
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llvieweroctree.h"
#include "llviewerregion.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llglslshader.h"
#include "llviewershadermgr.h"

//-----------------------------------------------------------------------------------
//static variables definitions
//-----------------------------------------------------------------------------------
U32 LLViewerOctreeEntryData::sCurVisible = 0;
BOOL LLViewerOctreeDebug::sInDebug = FALSE;

//-----------------------------------------------------------------------------------
//some global functions definitions
//-----------------------------------------------------------------------------------
typedef enum
{
	b000 = 0x00,
	b001 = 0x01,
	b010 = 0x02,
	b011 = 0x03,
	b100 = 0x04,
	b101 = 0x05,
	b110 = 0x06,
	b111 = 0x07,
} eLoveTheBits;

//contact Runitai Linden for a copy of the SL object used to write this table
//basically, you give the table a bitmask of the look-at vector to a node and it
//gives you a triangle fan index array
static U16 sOcclusionIndices[] =
{
	 //000
		b111, b110, b010, b011, b001, b101, b100, b110,
	 //001 
		b011, b010, b000, b001, b101, b111, b110, b010,
	 //010
		b101, b100, b110, b111, b011, b001, b000, b100,
	 //011 
		b001, b000, b100, b101, b111, b011, b010, b000,
	 //100 
		b110, b000, b010, b011, b111, b101, b100, b000,
	 //101 
		b010, b100, b000, b001, b011, b111, b110, b100,
	 //110
		b100, b010, b110, b111, b101, b001, b000, b010,
	 //111
		b000, b110, b100, b101, b001, b011, b010, b110,
};

U32 get_box_fan_indices(LLCamera* camera, const LLVector4a& center)
{
	LLVector4a origin;
	origin.load3(camera->getOrigin().mV);

	S32 cypher = center.greaterThan(origin).getGatheredBits() & 0x7;
	
	return cypher*8;
}

U8* get_box_fan_indices_ptr(LLCamera* camera, const LLVector4a& center)
{
	LLVector4a origin;
	origin.load3(camera->getOrigin().mV);

	S32 cypher = center.greaterThan(origin).getGatheredBits() & 0x7;
	
	return (U8*) (sOcclusionIndices+cypher*8);
}

//create a vertex buffer for efficiently rendering cubes
LLVertexBuffer* ll_create_cube_vb(U32 type_mask, U32 usage)
{
	LLVertexBuffer* ret = new LLVertexBuffer(type_mask, usage);

	ret->allocateBuffer(8, 64, true);

	LLStrider<LLVector3> pos;
	LLStrider<U16> idx;

	ret->getVertexStrider(pos);
	ret->getIndexStrider(idx);

	pos[0] = LLVector3(-1,-1,-1);
	pos[1] = LLVector3(-1,-1, 1);
	pos[2] = LLVector3(-1, 1,-1);
	pos[3] = LLVector3(-1, 1, 1);
	pos[4] = LLVector3( 1,-1,-1);
	pos[5] = LLVector3( 1,-1, 1);
	pos[6] = LLVector3( 1, 1,-1);
	pos[7] = LLVector3( 1, 1, 1);

	for (U32 i = 0; i < 64; i++)
	{
		idx[i] = sOcclusionIndices[i];
	}

	ret->flush();

	return ret;
}


#define LL_TRACK_PENDING_OCCLUSION_QUERIES 0

const F32 SG_OCCLUSION_FUDGE = 0.25f;
#define SG_DISCARD_TOLERANCE 0.01f


S32 AABBSphereIntersect(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &rad)
{
	return AABBSphereIntersectR2(min, max, origin, rad*rad);
}

S32 AABBSphereIntersectR2(const LLVector3& min, const LLVector3& max, const LLVector3 &origin, const F32 &r)
{
	F32 d = 0.f;
	F32 t;
	
	if ((min-origin).magVecSquared() < r &&
		(max-origin).magVecSquared() < r)
	{
		return 2;
	}

	for (U32 i = 0; i < 3; i++)
	{
		if (origin.mV[i] < min.mV[i])
		{
			t = min.mV[i] - origin.mV[i];
			d += t*t;
		}
		else if (origin.mV[i] > max.mV[i])
		{
			t = origin.mV[i] - max.mV[i];
			d += t*t;
		}

		if (d > r)
		{
			return 0;
		}
	}

	return 1;
}


S32 AABBSphereIntersect(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &rad)
{
	return AABBSphereIntersectR2(min, max, origin, rad*rad);
}

S32 AABBSphereIntersectR2(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &r)
{
	F32 d = 0.f;
	F32 t;
	
	LLVector4a origina;
	origina.load3(origin.mV);

	LLVector4a v;
	v.setSub(min, origina);
	
	if (v.dot3(v) < r)
	{
		v.setSub(max, origina);
		if (v.dot3(v) < r)
		{
			return 2;
		}
	}


	for (U32 i = 0; i < 3; i++)
	{
		if (origin.mV[i] < min[i])
		{
			t = min[i] - origin.mV[i];
			d += t*t;
		}
		else if (origin.mV[i] > max[i])
		{
			t = origin.mV[i] - max[i];
			d += t*t;
		}

		if (d > r)
		{
			return 0;
		}
	}

	return 1;
}

//-----------------------------------------------------------------------------------
//class LLViewerOctreeEntry definitions
//-----------------------------------------------------------------------------------
LLViewerOctreeEntry::LLViewerOctreeEntry() 
	: mGroup(NULL),
	  mBinRadius(0.f),
	  mBinIndex(-1)
{
	mPositionGroup.clear();
	mExtents[0].clear();
	mExtents[1].clear();	

	for(S32 i = 0; i < NUM_DATA_TYPE; i++)
	{
		mData[i] = NULL;
	}
}

LLViewerOctreeEntry::~LLViewerOctreeEntry()
{
	llassert(!mGroup);
}

void LLViewerOctreeEntry::addData(LLViewerOctreeEntryData* data)
{
	//llassert(mData[data->getDataType()] == NULL);	
	llassert(data != NULL);

	mData[data->getDataType()] = data;
}

void LLViewerOctreeEntry::removeData(LLViewerOctreeEntryData* data)
{
	//llassert(data->getDataType() != LLVOCACHEENTRY); //can not remove VOCache entry

	if(!mData[data->getDataType()])
	{
		return;
	}

	mData[data->getDataType()] = NULL;
	
	if(mGroup != NULL && !mData[LLDRAWABLE])
	{
		LLviewerOctreeGroup* group = mGroup;
		mGroup = NULL;
		group->removeFromGroup(data);

		llassert(mBinIndex == -1);				
	}
}

//called by group handleDestruction() ONLY when group is destroyed by octree.
void LLViewerOctreeEntry::nullGroup()
{
	mGroup = NULL;
}

void LLViewerOctreeEntry::setGroup(LLviewerOctreeGroup* group)
{
	if(mGroup == group)
	{
		return;
	}

	if(mGroup)
	{
		LLviewerOctreeGroup* group = mGroup;
		mGroup = NULL;
		group->removeFromGroup(this);

		llassert(mBinIndex == -1);
	}

	mGroup = group;
}

//-----------------------------------------------------------------------------------
//class LLViewerOctreeEntryData definitions
//-----------------------------------------------------------------------------------
LLViewerOctreeEntryData::~LLViewerOctreeEntryData()
{
	if(mEntry)
	{
		mEntry->removeData(this);
	}
}

LLViewerOctreeEntryData::LLViewerOctreeEntryData(LLViewerOctreeEntry::eEntryDataType_t data_type)
	: mDataType(data_type),
	  mEntry(NULL)
{
}

//virtual
void LLViewerOctreeEntryData::setOctreeEntry(LLViewerOctreeEntry* entry)
{
	if(mEntry.notNull())
	{
		return; 
	}

	if(!entry)
	{
		mEntry = new LLViewerOctreeEntry();
	}
	else
	{
		mEntry = entry;
	}
	mEntry->addData(this);
}

void LLViewerOctreeEntryData::setSpatialExtents(const LLVector3& min, const LLVector3& max)
{ 
	mEntry->mExtents[0].load3(min.mV); 
	mEntry->mExtents[1].load3(max.mV);
}

void LLViewerOctreeEntryData::setSpatialExtents(const LLVector4a& min, const LLVector4a& max)
{ 
	mEntry->mExtents[0] = min; 
	mEntry->mExtents[1] = max;
}

void LLViewerOctreeEntryData::setPositionGroup(const LLVector4a& pos)
{
	mEntry->mPositionGroup = pos;
}

const LLVector4a* LLViewerOctreeEntryData::getSpatialExtents() const 
{
	return mEntry->getSpatialExtents();
}

//virtual
void LLViewerOctreeEntryData::setGroup(LLviewerOctreeGroup* group)
{
	mEntry->setGroup(group);
}

void LLViewerOctreeEntryData::shift(const LLVector4a &shift_vector)
{
	mEntry->mExtents[0].add(shift_vector);
	mEntry->mExtents[1].add(shift_vector);
	mEntry->mPositionGroup.add(shift_vector);
}

LLviewerOctreeGroup* LLViewerOctreeEntryData::getGroup()const        
{
	return mEntry.notNull() ? mEntry->mGroup : NULL;
}

const LLVector4a& LLViewerOctreeEntryData::getPositionGroup() const  
{
	return mEntry->getPositionGroup();
}	

//virtual
bool LLViewerOctreeEntryData::isVisible() const
{
	if(mEntry)
	{
		return mEntry->mVisible == sCurVisible;
	}
	return false;
}

//virtual
bool LLViewerOctreeEntryData::isRecentlyVisible() const
{
	if(!mEntry)
	{
		return false;
	}

	if(isVisible())
	{
		return true;
	}
	if(getGroup() && getGroup()->isRecentlyVisible())
	{
		setVisible();
		return true;
	}

	return (sCurVisible - mEntry->mVisible < getMinFrameRange());
}

void LLViewerOctreeEntryData::setVisible() const
{
	if(mEntry)
	{
		mEntry->mVisible = sCurVisible;
	}
}

//-----------------------------------------------------------------------------------
//class LLviewerOctreeGroup definitions
//-----------------------------------------------------------------------------------

LLviewerOctreeGroup::~LLviewerOctreeGroup()
{
	//empty here
}

LLviewerOctreeGroup::LLviewerOctreeGroup(OctreeNode* node) :
	mOctreeNode(node),
	mState(CLEAN)
{
	LLVector4a tmp;
	tmp.splat(0.f);
	mExtents[0] = mExtents[1] = mObjectBounds[0] = mObjectBounds[0] = mObjectBounds[1] = 
		mObjectExtents[0] = mObjectExtents[1] = tmp;
	
	mBounds[0] = node->getCenter();
	mBounds[1] = node->getSize();

	mOctreeNode->addListener(this);
}

bool LLviewerOctreeGroup::hasElement(LLViewerOctreeEntryData* data) 
{ 
	if(!data->getEntry())
	{
		return false;
	}
	return std::find(getDataBegin(), getDataEnd(), data->getEntry()) != getDataEnd(); 
}

bool LLviewerOctreeGroup::removeFromGroup(LLViewerOctreeEntryData* data)
{
	return removeFromGroup(data->getEntry());
}

bool LLviewerOctreeGroup::removeFromGroup(LLViewerOctreeEntry* entry)
{
	llassert(entry != NULL);
	llassert(!entry->getGroup());

	unbound();
	setState(OBJECT_DIRTY);

	if (mOctreeNode)
	{
		if (!mOctreeNode->remove(entry)) //this could cause *this* pointer to be destroyed, so no more function calls after this.
		{
			OCT_ERRS << "Could not remove LLVOCacheEntry from LLVOCacheOctreeGroup" << LL_ENDL;
			return false;
		}
	}	

	return true;
}

//virtual 
void LLviewerOctreeGroup::unbound()
{
	if (isDirty())
	{
		return;
	}

	setState(DIRTY);
	
	//all the parent nodes need to rebound this child
	if (mOctreeNode)
	{
		OctreeNode* parent = (OctreeNode*) mOctreeNode->getParent();
		while (parent != NULL)
		{
			LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) parent->getListener(0);
			if (!group || group->isDirty())
			{
				return;
			}
			
			group->setState(DIRTY);
			parent = (OctreeNode*) parent->getParent();
		}
	}
}
	
//virtual 
void LLviewerOctreeGroup::rebound()
{
	if (!isDirty())
	{	
		return;
	}
	
	if (mOctreeNode->getChildCount() == 1 && mOctreeNode->getElementCount() == 0)
	{
		LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->rebound();
		
		//copy single child's bounding box
		mBounds[0] = group->mBounds[0];
		mBounds[1] = group->mBounds[1];
		mExtents[0] = group->mExtents[0];
		mExtents[1] = group->mExtents[1];
		
		group->setState(SKIP_FRUSTUM_CHECK);
	}
	else if (mOctreeNode->isLeaf())
	{ //copy object bounding box if this is a leaf
		boundObjects(TRUE, mExtents[0], mExtents[1]);
		mBounds[0] = mObjectBounds[0];
		mBounds[1] = mObjectBounds[1];
	}
	else
	{
		LLVector4a& newMin = mExtents[0];
		LLVector4a& newMax = mExtents[1];
		LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) mOctreeNode->getChild(0)->getListener(0);
		group->clearState(SKIP_FRUSTUM_CHECK);
		group->rebound();
		//initialize to first child
		newMin = group->mExtents[0];
		newMax = group->mExtents[1];

		//first, rebound children
		for (U32 i = 1; i < mOctreeNode->getChildCount(); i++)
		{
			group = (LLviewerOctreeGroup*) mOctreeNode->getChild(i)->getListener(0);
			group->clearState(SKIP_FRUSTUM_CHECK);
			group->rebound();
			const LLVector4a& max = group->mExtents[1];
			const LLVector4a& min = group->mExtents[0];

			newMax.setMax(newMax, max);
			newMin.setMin(newMin, min);
		}

		boundObjects(FALSE, newMin, newMax);
		
		mBounds[0].setAdd(newMin, newMax);
		mBounds[0].mul(0.5f);
		mBounds[1].setSub(newMax, newMin);
		mBounds[1].mul(0.5f);
	}
	
	clearState(DIRTY);

	return;
}

//virtual 
void LLviewerOctreeGroup::handleInsertion(const TreeNode* node, LLViewerOctreeEntry* obj)
{
	obj->setGroup(this);	
	unbound();
	setState(OBJECT_DIRTY);
}
	
//virtual 
void LLviewerOctreeGroup::handleRemoval(const TreeNode* node, LLViewerOctreeEntry* obj)
{
	unbound();
	setState(OBJECT_DIRTY);

	obj->setGroup(NULL); //this could cause *this* pointer to be destroyed. So no more function calls after this.	
}
	
//virtual 
void LLviewerOctreeGroup::handleDestruction(const TreeNode* node)
{
	for (OctreeNode::element_iter i = mOctreeNode->getDataBegin(); i != mOctreeNode->getDataEnd(); ++i)
	{
		LLViewerOctreeEntry* obj = *i;
		if (obj && obj->getGroup() == this)
		{
			obj->nullGroup();
			//obj->setGroup(NULL);
		}
	}
	mOctreeNode = NULL;
}
	
//virtual 
void LLviewerOctreeGroup::handleStateChange(const TreeNode* node)
{
	//drop bounding box upon state change
	if (mOctreeNode != node)
	{
		mOctreeNode = (OctreeNode*) node;
	}
	unbound();
}
	
//virtual 
void LLviewerOctreeGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child)
{
	if (child->getListenerCount() == 0)
	{
		new LLviewerOctreeGroup(child);
	}
	else
	{
		OCT_ERRS << "LLviewerOctreeGroup redundancy detected." << LL_ENDL;
	}

	unbound();
	
	((LLviewerOctreeGroup*)child->getListener(0))->unbound();
}
	
//virtual 
void LLviewerOctreeGroup::handleChildRemoval(const OctreeNode* parent, const OctreeNode* child)
{
	unbound();
}

LLviewerOctreeGroup* LLviewerOctreeGroup::getParent()
{
	if (isDead())
	{
		return NULL;
	}

	if(!mOctreeNode)
	{
		return NULL;
	}
	
	OctreeNode* parent = mOctreeNode->getOctParent();

	if (parent)
	{
		return (LLviewerOctreeGroup*) parent->getListener(0);
	}

	return NULL;
}

//virtual 
bool LLviewerOctreeGroup::boundObjects(BOOL empty, LLVector4a& minOut, LLVector4a& maxOut)
{
	const OctreeNode* node = mOctreeNode;

	if (node->isEmpty())
	{	//don't do anything if there are no objects
		if (empty && mOctreeNode->getParent())
		{	//only root is allowed to be empty
			OCT_ERRS << "Empty leaf found in octree." << LL_ENDL;
		}
		return false;
	}

	LLVector4a& newMin = mObjectExtents[0];
	LLVector4a& newMax = mObjectExtents[1];
	
	if (hasState(OBJECT_DIRTY))
	{ //calculate new bounding box
		clearState(OBJECT_DIRTY);

		//initialize bounding box to first element
		OctreeNode::const_element_iter i = node->getDataBegin();
		LLViewerOctreeEntry* entry = *i;
		const LLVector4a* minMax = entry->getSpatialExtents();

		newMin = minMax[0];
		newMax = minMax[1];

		for (++i; i != node->getDataEnd(); ++i)
		{
			entry = *i;
			minMax = entry->getSpatialExtents();
			
			update_min_max(newMin, newMax, minMax[0]);
			update_min_max(newMin, newMax, minMax[1]);
		}
		
		mObjectBounds[0].setAdd(newMin, newMax);
		mObjectBounds[0].mul(0.5f);
		mObjectBounds[1].setSub(newMax, newMin);
		mObjectBounds[1].mul(0.5f);
	}
	
	if (empty)
	{
		minOut = newMin;
		maxOut = newMax;
	}
	else
	{
		minOut.setMin(minOut, newMin);
		maxOut.setMax(maxOut, newMax);
	}
		
	return TRUE;
}

//virtual 
BOOL LLviewerOctreeGroup::isVisible() const
{
	return mVisible[LLViewerCamera::sCurCameraID] >= LLViewerOctreeEntryData::getCurrentFrame() ? TRUE : FALSE;
}

//virtual 
BOOL LLviewerOctreeGroup::isRecentlyVisible() const 
{
	return FALSE;
}

void LLviewerOctreeGroup::setVisible()
{
	mVisible[LLViewerCamera::sCurCameraID] = LLViewerOctreeEntryData::getCurrentFrame();
}

void LLviewerOctreeGroup::checkStates()
{
#if LL_OCTREE_PARANOIA_CHECK
	//LLOctreeStateCheck checker;
	//checker.traverse(mOctreeNode);
#endif
}

//-------------------------------------------------------------------------------------------
//occulsion culling functions and classes
//-------------------------------------------------------------------------------------------
std::set<U32> LLOcclusionCullingGroup::sPendingQueries;
class LLOcclusionQueryPool : public LLGLNamePool
{
public:
	LLOcclusionQueryPool()
	{
		mCurQuery = 1;
	}

protected:

	std::list<GLuint> mAvailableName;
	GLuint mCurQuery;
		
	virtual GLuint allocateName()
	{
		GLuint ret = 0;

		if (!mAvailableName.empty())
		{
			ret = mAvailableName.front();
			mAvailableName.pop_front();
		}
		else
		{
			ret = mCurQuery++;
		}

		return ret;
	}

	virtual void releaseName(GLuint name)
	{
#if LL_TRACK_PENDING_OCCLUSION_QUERIES
		LLSpatialGroup::sPendingQueries.erase(name);
#endif
		llassert(std::find(mAvailableName.begin(), mAvailableName.end(), name) == mAvailableName.end());
		mAvailableName.push_back(name);
	}
};

static LLOcclusionQueryPool sQueryPool;
U32 LLOcclusionCullingGroup::getNewOcclusionQueryObjectName()
{
	return sQueryPool.allocate();
}

void LLOcclusionCullingGroup::releaseOcclusionQueryObjectName(GLuint name)
{
	sQueryPool.release(name);
}

//=====================================
//		Occlusion State Set/Clear
//=====================================
class LLSpatialSetOcclusionState : public OctreeTraveler
{
public:
	U32 mState;
	LLSpatialSetOcclusionState(U32 state) : mState(state) { }
	virtual void visit(const OctreeNode* branch) { ((LLOcclusionCullingGroup*) branch->getListener(0))->setOcclusionState(mState); }	
};

class LLSpatialSetOcclusionStateDiff : public LLSpatialSetOcclusionState
{
public:
	LLSpatialSetOcclusionStateDiff(U32 state) : LLSpatialSetOcclusionState(state) { }

	virtual void traverse(const OctreeNode* n)
	{
		LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*) n->getListener(0);
		
		if (!group->isOcclusionState(mState))
		{
			OctreeTraveler::traverse(n);
		}
	}
};


LLOcclusionCullingGroup::LLOcclusionCullingGroup(OctreeNode* node, LLViewerOctreePartition* part) : 
	LLviewerOctreeGroup(node),
	mSpatialPartition(part)
{
	part->mLODSeed = (part->mLODSeed+1)%part->mLODPeriod;
	mLODHash = part->mLODSeed;

	OctreeNode* oct_parent = node->getOctParent();
	LLOcclusionCullingGroup* parent = oct_parent ? (LLOcclusionCullingGroup*) oct_parent->getListener(0) : NULL;

	for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
	{
		mOcclusionQuery[i] = 0;
		mOcclusionIssued[i] = 0;
		mOcclusionState[i] = parent ? SG_STATE_INHERIT_MASK & parent->mOcclusionState[i] : 0;
		mVisible[i] = 0;
	}
}

LLOcclusionCullingGroup::~LLOcclusionCullingGroup()
{
	releaseOcclusionQueryObjectNames();
}

BOOL LLOcclusionCullingGroup::needsUpdate()
{
	return (LLDrawable::getCurrentFrame() % mSpatialPartition->mLODPeriod == mLODHash) ? TRUE : FALSE;
}

//virtual 
void LLOcclusionCullingGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child)
{
	if (child->getListenerCount() == 0)
	{
		new LLOcclusionCullingGroup(child, mSpatialPartition);
	}
	else
	{
		OCT_ERRS << "LLOcclusionCullingGroup redundancy detected." << LL_ENDL;
	}

	unbound();
	
	((LLviewerOctreeGroup*)child->getListener(0))->unbound();
}

void LLOcclusionCullingGroup::releaseOcclusionQueryObjectNames()
{
	if (gGLManager.mHasOcclusionQuery)
	{
		for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; ++i)
		{
			if (mOcclusionQuery[i])
			{
				releaseOcclusionQueryObjectName(mOcclusionQuery[i]);
				mOcclusionQuery[i] = 0;
			}
		}
	}
}

void LLOcclusionCullingGroup::setOcclusionState(U32 state, S32 mode) 
{
	if (mode > STATE_MODE_SINGLE)
	{
		if (mode == STATE_MODE_DIFF)
		{
			LLSpatialSetOcclusionStateDiff setter(state);
			setter.traverse(mOctreeNode);
		}
		else if (mode == STATE_MODE_BRANCH)
		{
			LLSpatialSetOcclusionState setter(state);
			setter.traverse(mOctreeNode);
		}
		else
		{
			for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
			{
				mOcclusionState[i] |= state;

				if ((state & DISCARD_QUERY) && mOcclusionQuery[i])
				{
					releaseOcclusionQueryObjectName(mOcclusionQuery[i]);
					mOcclusionQuery[i] = 0;
				}
			}
		}
	}
	else
	{
		mOcclusionState[LLViewerCamera::sCurCameraID] |= state;
		if ((state & DISCARD_QUERY) && mOcclusionQuery[LLViewerCamera::sCurCameraID])
		{
			releaseOcclusionQueryObjectName(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
			mOcclusionQuery[LLViewerCamera::sCurCameraID] = 0;
		}
	}
}

class LLSpatialClearOcclusionState : public OctreeTraveler
{
public:
	U32 mState;
	
	LLSpatialClearOcclusionState(U32 state) : mState(state) { }
	virtual void visit(const OctreeNode* branch) { ((LLOcclusionCullingGroup*) branch->getListener(0))->clearOcclusionState(mState); }
};

class LLSpatialClearOcclusionStateDiff : public LLSpatialClearOcclusionState
{
public:
	LLSpatialClearOcclusionStateDiff(U32 state) : LLSpatialClearOcclusionState(state) { }

	virtual void traverse(const OctreeNode* n)
	{
		LLOcclusionCullingGroup* group = (LLOcclusionCullingGroup*) n->getListener(0);
		
		if (group->isOcclusionState(mState))
		{
			OctreeTraveler::traverse(n);
		}
	}
};

void LLOcclusionCullingGroup::clearOcclusionState(U32 state, S32 mode)
{
	if (mode > STATE_MODE_SINGLE)
	{
		if (mode == STATE_MODE_DIFF)
		{
			LLSpatialClearOcclusionStateDiff clearer(state);
			clearer.traverse(mOctreeNode);
		}
		else if (mode == STATE_MODE_BRANCH)
		{
			LLSpatialClearOcclusionState clearer(state);
			clearer.traverse(mOctreeNode);
		}
		else
		{
			for (U32 i = 0; i < LLViewerCamera::NUM_CAMERAS; i++)
			{
				mOcclusionState[i] &= ~state;
			}
		}
	}
	else
	{
		mOcclusionState[LLViewerCamera::sCurCameraID] &= ~state;
	}
}

static LLFastTimer::DeclareTimer FTM_OCCLUSION_READBACK("Readback Occlusion");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_WAIT("Occlusion Wait");

BOOL LLOcclusionCullingGroup::earlyFail(LLCamera* camera, const LLVector4a* bounds)
{
	if (camera->getOrigin().isExactlyZero())
	{
		return FALSE;
	}

	const F32 vel = SG_OCCLUSION_FUDGE*2.f;
	LLVector4a fudge;
	fudge.splat(vel);

	const LLVector4a& c = bounds[0];
	LLVector4a r;
	r.setAdd(bounds[1], fudge);

	/*if (r.magVecSquared() > 1024.0*1024.0)
	{
		return TRUE;
	}*/

	LLVector4a e;
	e.load3(camera->getOrigin().mV);
	
	LLVector4a min;
	min.setSub(c,r);
	LLVector4a max;
	max.setAdd(c,r);
	
	S32 lt = e.lessThan(min).getGatheredBits() & 0x7;
	if (lt)
	{
		return FALSE;
	}

	S32 gt = e.greaterThan(max).getGatheredBits() & 0x7;
	if (gt)
	{
		return FALSE;
	}

	return TRUE;
}

U32 LLOcclusionCullingGroup::getLastOcclusionIssuedTime()
{
	return mOcclusionIssued[LLViewerCamera::sCurCameraID];
}

void LLOcclusionCullingGroup::checkOcclusion()
{
	if (LLPipeline::sUseOcclusion > 1)
	{
		LLFastTimer t(FTM_OCCLUSION_READBACK);
		LLOcclusionCullingGroup* parent = (LLOcclusionCullingGroup*)getParent();
		if (parent && parent->isOcclusionState(LLOcclusionCullingGroup::OCCLUDED))
		{	//if the parent has been marked as occluded, the child is implicitly occluded
			clearOcclusionState(QUERY_PENDING | DISCARD_QUERY);
		}
		else if (isOcclusionState(QUERY_PENDING))
		{	//otherwise, if a query is pending, read it back

			GLuint available = 0;
			if (mOcclusionQuery[LLViewerCamera::sCurCameraID])
			{
				glGetQueryObjectuivARB(mOcclusionQuery[LLViewerCamera::sCurCameraID], GL_QUERY_RESULT_AVAILABLE_ARB, &available);

				static LLCachedControl<bool> wait_for_query(gSavedSettings, "RenderSynchronousOcclusion");

				if (wait_for_query && mOcclusionIssued[LLViewerCamera::sCurCameraID] < gFrameCount)
				{ //query was issued last frame, wait until it's available
					S32 max_loop = 1024;
					LLFastTimer t(FTM_OCCLUSION_WAIT);
					while (!available && max_loop-- > 0)
					{
						//do some usefu work while we wait
						F32 max_time = llmin(gFrameIntervalSeconds.value()*10.f, 1.f);
						LLAppViewer::instance()->updateTextureThreads(max_time);
						
						glGetQueryObjectuivARB(mOcclusionQuery[LLViewerCamera::sCurCameraID], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
					}
				}
			}
			else
			{
				available = 1;
			}

			if (available)
			{ //result is available, read it back, otherwise wait until next frame
				GLuint res = 1;
				if (!isOcclusionState(DISCARD_QUERY) && mOcclusionQuery[LLViewerCamera::sCurCameraID])
				{
					glGetQueryObjectuivARB(mOcclusionQuery[LLViewerCamera::sCurCameraID], GL_QUERY_RESULT_ARB, &res);	
#if LL_TRACK_PENDING_OCCLUSION_QUERIES
					sPendingQueries.erase(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
#endif
				}
				else if (mOcclusionQuery[LLViewerCamera::sCurCameraID])
				{ //delete the query to avoid holding onto hundreds of pending queries
					releaseOcclusionQueryObjectName(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
					mOcclusionQuery[LLViewerCamera::sCurCameraID] = 0;
				}
				
				if (isOcclusionState(DISCARD_QUERY))
				{
					res = 2;
				}

				if (res > 0)
				{
					assert_states_valid(this);
					clearOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
					assert_states_valid(this);
				}
				else
				{
					assert_states_valid(this);
					
					setOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
					
					assert_states_valid(this);
				}

				clearOcclusionState(QUERY_PENDING | DISCARD_QUERY);
			}
		}
		else if (mSpatialPartition->isOcclusionEnabled() && isOcclusionState(LLOcclusionCullingGroup::OCCLUDED))
		{	//check occlusion has been issued for occluded node that has not had a query issued
			assert_states_valid(this);
			clearOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
			assert_states_valid(this);
		}
	}
}

static LLFastTimer::DeclareTimer FTM_PUSH_OCCLUSION_VERTS("Push Occlusion");
static LLFastTimer::DeclareTimer FTM_SET_OCCLUSION_STATE("Occlusion State");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_EARLY_FAIL("Occlusion Early Fail");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_ALLOCATE("Allocate");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_BUILD("Build");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_BEGIN_QUERY("Begin Query");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_END_QUERY("End Query");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_SET_BUFFER("Set Buffer");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_DRAW_WATER("Draw Water");
static LLFastTimer::DeclareTimer FTM_OCCLUSION_DRAW("Draw");

void LLOcclusionCullingGroup::doOcclusion(LLCamera* camera, const LLVector3* region_agent)
{
	if (mSpatialPartition->isOcclusionEnabled() && LLPipeline::sUseOcclusion > 1)
	{
		//move mBounds to the agent space if necessary
		LLVector4a bounds[2];
		bounds[0] = mBounds[0];
		bounds[1] = mBounds[1];
		if(region_agent != NULL)
		{
			LLVector4a shift((*region_agent)[0], (*region_agent)[1], (*region_agent)[2]);
			bounds[0].sub(shift);
			bounds[1].sub(shift);
		}

		// Don't cull hole/edge water, unless we have the GL_ARB_depth_clamp extension
		if (earlyFail(camera, bounds))
		{
			LLFastTimer t(FTM_OCCLUSION_EARLY_FAIL);
			setOcclusionState(LLOcclusionCullingGroup::DISCARD_QUERY);
			assert_states_valid(this);
			clearOcclusionState(LLOcclusionCullingGroup::OCCLUDED, LLOcclusionCullingGroup::STATE_MODE_DIFF);
			assert_states_valid(this);
		}
		else
		{
			if (!isOcclusionState(QUERY_PENDING) || isOcclusionState(DISCARD_QUERY))
			{
				{ //no query pending, or previous query to be discarded
					LLFastTimer t(FTM_RENDER_OCCLUSION);

					if (!mOcclusionQuery[LLViewerCamera::sCurCameraID])
					{
						LLFastTimer t(FTM_OCCLUSION_ALLOCATE);
						mOcclusionQuery[LLViewerCamera::sCurCameraID] = getNewOcclusionQueryObjectName();
					}

					// Depth clamp all water to avoid it being culled as a result of being
					// behind the far clip plane, and in the case of edge water to avoid
					// it being culled while still visible.
					bool const use_depth_clamp = gGLManager.mHasDepthClamp &&
												(mSpatialPartition->mDrawableType == LLDrawPool::POOL_WATER ||						
												mSpatialPartition->mDrawableType == LLDrawPool::POOL_VOIDWATER);

					LLGLEnable clamp(use_depth_clamp ? GL_DEPTH_CLAMP : 0);				
						
#if !LL_DARWIN					
					U32 mode = gGLManager.mHasOcclusionQuery2 ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED_ARB;
#else
					U32 mode = GL_SAMPLES_PASSED_ARB;
#endif
					
#if LL_TRACK_PENDING_OCCLUSION_QUERIES
					sPendingQueries.insert(mOcclusionQuery[LLViewerCamera::sCurCameraID]);
#endif

					{
						LLFastTimer t(FTM_PUSH_OCCLUSION_VERTS);
						
						//store which frame this query was issued on
						mOcclusionIssued[LLViewerCamera::sCurCameraID] = gFrameCount;

						{
							LLFastTimer t(FTM_OCCLUSION_BEGIN_QUERY);
							glBeginQueryARB(mode, mOcclusionQuery[LLViewerCamera::sCurCameraID]);					
						}
					
						LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
						llassert(shader);

						shader->uniform3fv(LLShaderMgr::BOX_CENTER, 1, bounds[0].getF32ptr());
						shader->uniform3f(LLShaderMgr::BOX_SIZE, bounds[1][0]+SG_OCCLUSION_FUDGE, 
																 bounds[1][1]+SG_OCCLUSION_FUDGE, 
																 bounds[1][2]+SG_OCCLUSION_FUDGE);

						if (!use_depth_clamp && mSpatialPartition->mDrawableType == LLDrawPool::POOL_VOIDWATER)
						{
							LLFastTimer t(FTM_OCCLUSION_DRAW_WATER);

							LLGLSquashToFarClip squash(glh_get_current_projection(), 1);
							if (camera->getOrigin().isExactlyZero())
							{ //origin is invalid, draw entire box
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, 0);
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, b111*8);				
							}
							else
							{
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, bounds[0]));
							}
						}
						else
						{
							LLFastTimer t(FTM_OCCLUSION_DRAW);
							if (camera->getOrigin().isExactlyZero())
							{ //origin is invalid, draw entire box
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, 0);
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, b111*8);				
							}
							else
							{
								gPipeline.mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, bounds[0]));
							}
						}


						{
							LLFastTimer t(FTM_OCCLUSION_END_QUERY);
							glEndQueryARB(mode);
						}
					}
				}

				{
					LLFastTimer t(FTM_SET_OCCLUSION_STATE);
					setOcclusionState(LLOcclusionCullingGroup::QUERY_PENDING);
					clearOcclusionState(LLOcclusionCullingGroup::DISCARD_QUERY);
				}
			}
		}
	}
}
//-------------------------------------------------------------------------------------------
//end of occulsion culling functions and classes
//-------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//class LLViewerOctreePartition definitions
//-----------------------------------------------------------------------------------
LLViewerOctreePartition::LLViewerOctreePartition() : 
	mRegionp(NULL), 
	mOcclusionEnabled(TRUE), 
	mDrawableType(0),
	mLODSeed(0),
	mLODPeriod(1)
{
	LLVector4a center, size;
	center.splat(0.f);
	size.splat(1.f);

	mOctree = new OctreeRoot(center,size, NULL);
}
	
LLViewerOctreePartition::~LLViewerOctreePartition()
{
	delete mOctree;
	mOctree = NULL;
}

BOOL LLViewerOctreePartition::isOcclusionEnabled()
{
	return mOcclusionEnabled || LLPipeline::sUseOcclusion > 2;
}

//-----------------------------------------------------------------------------------
//class LLViewerOctreeCull definitions
//-----------------------------------------------------------------------------------

//virtual 
bool LLViewerOctreeCull::earlyFail(LLviewerOctreeGroup* group)
{	
	return false;
}
	
//virtual 
void LLViewerOctreeCull::traverse(const OctreeNode* n)
{
	LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) n->getListener(0);

	if (earlyFail(group))
	{
		return;
	}
		
	if (mRes == 2 || 
		(mRes && group->hasState(LLviewerOctreeGroup::SKIP_FRUSTUM_CHECK)))
	{	//fully in, just add everything
		OctreeTraveler::traverse(n);
	}
	else
	{
		mRes = frustumCheck(group);
				
		if (mRes)
		{ //at least partially in, run on down
			OctreeTraveler::traverse(n);
		}

		mRes = 0;
	}
}
	
//------------------------------------------
//agent space group culling
S32 LLViewerOctreeCull::AABBInFrustumNoFarClipGroupBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustumNoFarClip(group->mBounds[0], group->mBounds[1]);
}

S32 LLViewerOctreeCull::AABBSphereIntersectGroupExtents(const LLviewerOctreeGroup* group)
{
	return AABBSphereIntersect(group->mExtents[0], group->mExtents[1], mCamera->getOrigin(), mCamera->mFrustumCornerDist);
}

S32 LLViewerOctreeCull::AABBInFrustumGroupBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustum(group->mBounds[0], group->mBounds[1]);
}
//------------------------------------------

//------------------------------------------
//agent space object set culling
S32 LLViewerOctreeCull::AABBInFrustumNoFarClipObjectBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustumNoFarClip(group->mObjectBounds[0], group->mObjectBounds[1]);
}

S32 LLViewerOctreeCull::AABBSphereIntersectObjectExtents(const LLviewerOctreeGroup* group)
{
	return AABBSphereIntersect(group->mObjectExtents[0], group->mObjectExtents[1], mCamera->getOrigin(), mCamera->mFrustumCornerDist);
}

S32 LLViewerOctreeCull::AABBInFrustumObjectBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInFrustum(group->mObjectBounds[0], group->mObjectBounds[1]);
}
//------------------------------------------

//------------------------------------------
//local regional space group culling
S32 LLViewerOctreeCull::AABBInRegionFrustumNoFarClipGroupBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustumNoFarClip(group->mBounds[0], group->mBounds[1]);
}

S32 LLViewerOctreeCull::AABBInRegionFrustumGroupBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustum(group->mBounds[0], group->mBounds[1]);
}

S32 LLViewerOctreeCull::AABBRegionSphereIntersectGroupExtents(const LLviewerOctreeGroup* group, const LLVector3& shift)
{
	return AABBSphereIntersect(group->mExtents[0], group->mExtents[1], mCamera->getOrigin() - shift, mCamera->mFrustumCornerDist);
}
//------------------------------------------

//------------------------------------------
//local regional space object culling
S32 LLViewerOctreeCull::AABBInRegionFrustumObjectBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustum(group->mObjectBounds[0], group->mObjectBounds[1]);
}

S32 LLViewerOctreeCull::AABBInRegionFrustumNoFarClipObjectBounds(const LLviewerOctreeGroup* group)
{
	return mCamera->AABBInRegionFrustumNoFarClip(group->mObjectBounds[0], group->mObjectBounds[1]);
}

S32 LLViewerOctreeCull::AABBRegionSphereIntersectObjectExtents(const LLviewerOctreeGroup* group, const LLVector3& shift)
{
	return AABBSphereIntersect(group->mObjectExtents[0], group->mObjectExtents[1], mCamera->getOrigin() - shift, mCamera->mFrustumCornerDist);
}
//------------------------------------------

//virtual 
bool LLViewerOctreeCull::checkObjects(const OctreeNode* branch, const LLviewerOctreeGroup* group)
{
	if (branch->getElementCount() == 0) //no elements
	{
		return false;
	}
	else if (branch->getChildCount() == 0) //leaf state, already checked tightest bounding box
	{
		return true;
	}
	else if (mRes == 1 && !frustumCheckObjects(group)) //no objects in frustum
	{
		return false;
	}
		
	return true;
}

//virtual 
void LLViewerOctreeCull::preprocess(LLviewerOctreeGroup* group)
{		
}
	
//virtual 
void LLViewerOctreeCull::processGroup(LLviewerOctreeGroup* group)
{
}
	
//virtual 
void LLViewerOctreeCull::visit(const OctreeNode* branch) 
{	
	LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) branch->getListener(0);

	preprocess(group);
		
	if (checkObjects(branch, group))
	{
		processGroup(group);
	}
}

//--------------------------------------------------------------
//class LLViewerOctreeDebug
//virtual 
void LLViewerOctreeDebug::visit(const OctreeNode* branch)
{
#if 0
	LL_INFOS() << "Node: " << (U32)branch << " # Elements: " << branch->getElementCount() << " # Children: " << branch->getChildCount() << LL_ENDL;
	for (U32 i = 0; i < branch->getChildCount(); i++)
	{
		LL_INFOS() << "Child " << i << " : " << (U32)branch->getChild(i) << LL_ENDL;
	}
#endif
	LLviewerOctreeGroup* group = (LLviewerOctreeGroup*) branch->getListener(0);
	processGroup(group);	
}

//virtual 
void LLViewerOctreeDebug::processGroup(LLviewerOctreeGroup* group)
{
#if 0
	const LLVector4a* vec4 = group->getBounds();
	LLVector3 vec[2];
	vec[0].set(vec4[0].getF32ptr());
	vec[1].set(vec4[1].getF32ptr());
	LL_INFOS() << "Bounds: " << vec[0] << " : " << vec[1] << LL_ENDL;

	vec4 = group->getExtents();
	vec[0].set(vec4[0].getF32ptr());
	vec[1].set(vec4[1].getF32ptr());
	LL_INFOS() << "Extents: " << vec[0] << " : " << vec[1] << LL_ENDL;

	vec4 = group->getObjectBounds();
	vec[0].set(vec4[0].getF32ptr());
	vec[1].set(vec4[1].getF32ptr());
	LL_INFOS() << "ObjectBounds: " << vec[0] << " : " << vec[1] << LL_ENDL;

	vec4 = group->getObjectExtents();
	vec[0].set(vec4[0].getF32ptr());
	vec[1].set(vec4[1].getF32ptr());
	LL_INFOS() << "ObjectExtents: " << vec[0] << " : " << vec[1] << LL_ENDL;
#endif
}
//--------------------------------------------------------------
