/** 
 * @file llpreviewanim.cpp
 * @brief LLPreviewAnim class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpreviewanim.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llinventory.h"
#include "llinventoryview.h"
#include "llvoavatar.h"
#include "llagent.h"          // gAgent
#include "llkeyframemotion.h"
#include "llfilepicker.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "lluictrlfactory.h"
#include "llviewermenufile.h"
#include "llviewerwindow.h" // for alert
#include "llappviewer.h" // gStaticVFS
#include "llassetstorage.h"
#include "tsbvhexporter.h"
// </edit>

extern LLAgent gAgent;

LLPreviewAnim::LLPreviewAnim(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const S32& activate, const LLUUID& object_uuid )	:
	LLPreview( name, rect, title, item_uuid, object_uuid)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_preview_animation.xml");

	childSetAction("Anim play btn",playAnim,this);
	childSetAction("Anim audition btn",auditionAnim,this);
	// <edit>
	childSetAction("Anim copy uuid btn", copyAnimID, this);
	// </edit>
	childSetAction("Anim remake btn",dupliAnim,this);
	childSetAction("Anim export btn",exportAnim,this);

	childSetAction("Anim .anim btn",exportasdotAnim,this);

	childSetEnabled("Anim remake btn", FALSE);
	childSetEnabled("Anim export btn", TRUE);
	childSetEnabled("Anim .anim btn", TRUE);
	mAnimBuffer = NULL;

	const LLInventoryItem* item = getItem();
	
	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);
	
	setTitle(title);

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}

	mAnimBuffer = NULL;
	// preload the animation
	if(item)
	{
		gAgent.getAvatarObject()->createMotion(item->getAssetUUID());
			gAssetStorage->getAssetData(item->getAssetUUID(), LLAssetType::AT_ANIMATION, downloadCompleteCallback, (void *)(new LLHandle<LLFloater>(this->getHandle())), TRUE);
	}
	
	switch ( activate ) 
	{
		case 1:
		{
			playAnim( (void *) this );
			break;
		}
		case 2:
		{
			auditionAnim( (void *) this );
			break;
		}
		default:
		{
		//do nothing
		}
	}
}

// static
void LLPreviewAnim::endAnimCallback( void *userdata )
{
	LLHandle<LLFloater>* handlep = ((LLHandle<LLFloater>*)userdata);
	LLFloater* self = handlep->get();
	delete handlep; // done with the handle
	if (self)
	{
		self->childSetValue("Anim play btn", FALSE);
		self->childSetValue("Anim audition btn", FALSE);
	}
}
// static
void LLPreviewAnim::downloadCompleteCallback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type, void *userdata, S32 result, LLExtStat extstat)
{
	LLHandle<LLFloater>* handlep = ((LLHandle<LLFloater>*)userdata);
	LLPreviewAnim* self = (LLPreviewAnim*)handlep->get();
	delete handlep; // done with the handle
	if (self)
	{
		if(result == LL_ERR_NOERR) {
			self->childSetEnabled("Anim remake btn", TRUE);
			self->childSetEnabled("Anim export btn", TRUE);
			self->childSetEnabled("Anim .anim btn", TRUE);
			self->mAnimBufferSize = vfs->getSize(uuid, type);
			self->mAnimBuffer = new U8[self->mAnimBufferSize];
			vfs->getData(uuid, type, self->mAnimBuffer, 0, self->mAnimBufferSize);
		}
	}
}

// static
void LLPreviewAnim::exportAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();
	if(item)
	{
		if(self->mAnimBuffer == NULL) return;

		LLUUID assetID=item->getAssetUUID();
        std::string filename = item->getName() + ".bvh";
        LLFilePicker& picker = LLFilePicker::instance();

        if( !picker.getSaveFile( LLFilePicker::FFSAVE_ALL, filename.c_str() ) )
        {
         // User canceled save.
            return;
        }

		TSBVHExporter exporter;
		LLDataPackerBinaryBuffer dp(self->mAnimBuffer, self->mAnimBufferSize);

		if(exporter.deserialize(dp)) {
			exporter.exportBVHFile(picker.getFirstFile().c_str());
		}
	}
}

// static
void LLPreviewAnim::playAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();

	if(item)
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = self->getChild<LLButton>("Anim play btn");
		if (btn)
		{
			btn->toggleState();
		}
		
		if (self->childGetValue("Anim play btn").asBoolean() ) 
		{
			self->mPauseRequest = NULL;
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_START);
			
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			LLMotion*   motion = avatar->findMotion(itemID);
			
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback, (void *)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgent.getAvatarObject()->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}

// static
void LLPreviewAnim::auditionAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();

	if(item)
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = self->getChild<LLButton>("Anim audition btn");
		if (btn)
		{
			btn->toggleState();
		}
		
		if (self->childGetValue("Anim audition btn").asBoolean() ) 
		{
			self->mPauseRequest = NULL;
			gAgent.getAvatarObject()->startMotion(item->getAssetUUID());
			
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			LLMotion*   motion = avatar->findMotion(itemID);
			
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback, (void *)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgent.getAvatarObject()->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}

void LLPreviewAnim::dupliAnim( void *userdata )
{


		LLPreviewAnim* self = (LLPreviewAnim*) userdata;

		//if(!self->childGetValue("Anim play btn").asBoolean())
		//{
		//	printchat("anim must be playing to copy by this method; please try again");
		//	LLPreviewAnim::playAnim( userdata );
		//	return;
		//}

		const LLInventoryItem *item = self->getItem();
		
		if(item)
		{
			if(self->mAnimBuffer == NULL) 
			{
			
				return;
			}
			
		
			LLKeyframeMotion* motionp = NULL;
			//LLBVHLoader* loaderp = NULL;

			LLAssetID			xMotionID;
			LLTransactionID		xTransactionID;

			// generate unique id for this motion
			xTransactionID.generate();
			xMotionID = xTransactionID.makeAssetID(gAgent.getSecureSessionID());
			motionp = (LLKeyframeMotion*)gAgent.getAvatarObject()->createMotion(xMotionID);
/*
			// pass animation data through memory buffer
			//loaderp->serialize(dp);
			gAgent.getAvatarObject()->startMotion(item->getAssetUUID());	
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			LLMotion*   motion = avatar->findMotion(item->getAssetUUID());
			LLKeyframeMotion* tmp = (LLKeyframeMotion*)motion;

			S32 file_size = tmp->getFileSize();
			U8* buffer = new U8[file_size];

			LLDataPackerBinaryBuffer dp(buffer, file_size);*/
			LLDataPackerBinaryBuffer dp(self->mAnimBuffer, self->mAnimBufferSize);
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			LLMotion*   motion = avatar->findMotion(item->getAssetUUID());
			LLKeyframeMotion* tmp = (LLKeyframeMotion*)motion;
			tmp->serialize(dp);
			dp.reset();
			BOOL success = motionp && motionp->deserialize(dp);

			//delete []buffer;

			if (success)
			{
				motionp->setName(item->getName());
				gAgent.getAvatarObject()->startMotion(xMotionID);

	////////////////////////////////////////////////////////////////////
			/*LLKeyframeMotion* */motionp = (LLKeyframeMotion*)gAgent.getAvatarObject()->findMotion(xMotionID);

			S32 file_size = motionp->getFileSize();
			U8* buffer = new U8[file_size];

			LLDataPackerBinaryBuffer dp(buffer, file_size);
			if (motionp->serialize(dp))
			{
				LLVFile file(gVFS, motionp->getID(), LLAssetType::AT_ANIMATION, LLVFile::APPEND);

				S32 size = dp.getCurrentSize();
				file.setMaxSize(size);
				if (file.write((U8*)buffer, size))
				{
					std::string name = item->getName();
					std::string desc = item->getDescription();
					upload_new_resource(xTransactionID, // tid
										LLAssetType::AT_ANIMATION,
										name,
										desc,
										0,
										LLAssetType::AT_NONE,
										LLInventoryType::IT_ANIMATION,
										PERM_NONE,PERM_NONE,PERM_NONE,
										name,0,10,0);
				}
				else
				{
					llwarns << "Failure writing animation data." << llendl;
					LLNotifications::instance().add("WriteAnimationFail");
				}
			}

			delete [] buffer;
			// clear out cache for motion data
			gAgent.getAvatarObject()->removeMotion(xMotionID);
			LLKeyframeDataCache::removeKeyframeData(xMotionID);
	////////////////////////////////////////////////////////////////////
			}

		}

}
void LLPreviewAnim::copyAnimID(void *userdata)
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();

	if(item)
	{
		gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(item->getAssetUUID().asString()));
	}
}
// </edit>

// <edit>
// virtual
BOOL LLPreviewAnim::canSaveAs() const
{
	return TRUE;
}

// virtual
void LLPreviewAnim::saveAs()
{
	const LLInventoryItem *item = getItem();

	if(item)
	{
		// Some animations aren't hosted on the servers
		// I guess they're in this static vfs thing
		bool static_vfile = false;
		LLVFile* anim_file = new LLVFile(gStaticVFS, item->getAssetUUID(), LLAssetType::AT_ANIMATION);
		if (anim_file && anim_file->getSize())
		{
			//S32 anim_file_size = anim_file->getSize();
			//U8* anim_data = new U8[anim_file_size];
			//if(anim_file->read(anim_data, anim_file_size))
			//{
			//	static_vfile = true;
			//}
			static_vfile = true; // for method 2
			LLPreviewAnim::gotAssetForSave(gStaticVFS, item->getAssetUUID(), LLAssetType::AT_ANIMATION, this, 0, 0);
		}
		delete anim_file;
		anim_file = NULL;

		if(!static_vfile)
		{
			gAssetStorage->getAssetData(item->getAssetUUID(), LLAssetType::AT_ANIMATION, LLPreviewAnim::gotAssetForSave, this, TRUE);
		}
	}
}

// static
void LLPreviewAnim::gotAssetForSave(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLPreviewAnim* self = (LLPreviewAnim*) user_data;
	//const LLInventoryItem *item = self->getItem();

	LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
	S32 size = file.getSize();

	char* buffer = new char[size];
	if (buffer == NULL)
	{
		llerrs << "Memory Allocation Failed" << llendl;
		return;
	}

	file.read((U8*)buffer, size);

	// Write it back out...

	LLFilePicker& file_picker = LLFilePicker::instance();
	if( !file_picker.getSaveFile( LLFilePicker::FFSAVE_ANIMATN, LLDir::getScrubbedFileName(self->getItem()->getName())) )
	{
		// User canceled or we failed to acquire save file.
		return;
	}
	// remember the user-approved/edited file name.
	std::string filename = file_picker.getFirstFile();

	std::ofstream export_file(filename.c_str(), std::ofstream::binary);
	export_file.write(buffer, size);
	export_file.close();
	
	delete[] buffer;
	buffer = NULL;
}

// virtual
LLUUID LLPreviewAnim::getItemID()
{
	const LLViewerInventoryItem* item = getItem();
	if(item)
	{
		return item->getUUID();
	}
	return LLUUID::null;
}
void LLPreviewAnim::exportasdotAnim( void *userdata )
{

		LLPreviewAnim* self = (LLPreviewAnim*) userdata;
		
		const LLInventoryItem *item = self->getItem();
		
		//LLVOAvatar* avatar = gAgent.getAvatarObject();
		//LLMotion*   motion = avatar->findMotion(item->getAssetUUID());
		//LLKeyframeMotion* motionp = (LLKeyframeMotion*)motion;
		//if (motionp)
		{
			
			//U32 size = motionp->getFileSize();
			//U8* buffer = new U8[size];
			
			//LLDataPackerBinaryBuffer dp(buffer, size);
			//if(motionp->serialize(dp))
			{
				
				std::string filename = item->getName() + ".animatn";
				LLFilePicker& picker = LLFilePicker::instance();
				if(!picker.getSaveFile( LLFilePicker::FFSAVE_ALL, filename.c_str() ) )
				{
					// User canceled save.
					return;
				}
				std::string name = picker.getFirstFile();
				std::string save_filename(name);
				LLAPRFile infile ;
				infile.open(save_filename.c_str(), LL_APR_WB, LLAPRFile::local);
				apr_file_t *fp = infile.getFileHandle();
				if(fp)infile.write(self->mAnimBuffer, self->mAnimBufferSize);
				
				infile.close();
			}
			//delete[] buffer;
		}
}
// </edit>

void LLPreviewAnim::onClose(bool app_quitting)
{
	const LLInventoryItem *item = getItem();

	if(item)
	{
		gAgent.getAvatarObject()->stopMotion(item->getAssetUUID());
		gAgent.sendAnimationRequest(item->getAssetUUID(), ANIM_REQUEST_STOP);
					
		LLVOAvatar* avatar = gAgent.getAvatarObject();
		LLMotion*   motion = avatar->findMotion(item->getAssetUUID());
		
		if (motion)
		{
			// *TODO: minor memory leak here, user data is never deleted (Use real callbacks)
			motion->setDeactivateCallback(NULL, (void *)NULL);
		}
	}
	if(mAnimBuffer) {
		delete mAnimBuffer;
		mAnimBuffer = NULL;
	}
	destroy();
}
