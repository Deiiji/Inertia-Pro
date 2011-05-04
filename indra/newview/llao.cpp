// <edit>

#include "llviewerprecompiledheaders.h"
#include "llao.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llfilepicker.h"
#include "llsdserialize.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "llinventorymodel.h"
#include <boost/foreach.hpp>

//static variables
std::list<LLUUID> LLAO::mStandOverrides;
std::map<LLUUID,LLUUID> LLAO::mOverrides;
LLFloaterAO* LLFloaterAO::sInstance;
BOOL LLAO::mEnabled = FALSE;
F32 LLAO::mPeriod;
LLAOStandTimer* LLAO::mTimer = NULL;
std::vector< AOLineEditor* > LLFloaterAO::sLineEditorDrop;
//local file only
static const AO_Pair ao_pair_list[] = {AO_Pair("line_walking",LLUUID("6ed24bd8-91aa-4b12-ccc7-c97c857ab4e0")),
	AO_Pair("line_running",LLUUID("05ddbff8-aaa9-92a1-2b74-8fe77a29b445")),
	AO_Pair("line_crouchwalk",LLUUID("47f5f6fb-22e5-ae44-f871-73aaaf4a6022")),
	AO_Pair("line_flying",LLUUID("aec4610c-757f-bc4e-c092-c6e9caf18daf")),
	AO_Pair("line_turn_left",LLUUID("56e0ba0d-4a9f-7f27-6117-32f2ebbf6135")),
	AO_Pair("line_turn_right",LLUUID("2d6daa51-3192-6794-8e2e-a15f8338ec30")),
	AO_Pair("line_jumping",LLUUID("2305bd75-1ca9-b03b-1faa-b176b8a8c49e")),
	AO_Pair("line_fly_up",LLUUID("62c5de58-cb33-5743-3d07-9e4cd4352864")),
	AO_Pair("line_crouching",LLUUID("201f3fdf-cb1f-dbec-201f-7333e328ae7c")),
	AO_Pair("line_fly_down",LLUUID("20f063ea-8306-2562-0b07-5c853b37b31e")),
	AO_Pair("line_hover",LLUUID("4ae8016b-31b9-03bb-c401-b1ea941db41d")),
	AO_Pair("line_sitting",LLUUID("1a5fe8ac-a804-8a5d-7cbd-56bd83184568")),
	AO_Pair("line_prejump",LLUUID("7a4e87fe-de39-6fcb-6223-024b00893244")),
	AO_Pair("line_falling",LLUUID("666307d9-a860-572d-6fd4-c3ab8865c094")),
	AO_Pair("line_stride",LLUUID("1cb562b0-ba21-2202-efb3-30f82cdf9595")),
	AO_Pair("line_soft_landing",LLUUID("7a17b059-12b2-41b1-570a-186368b6aa6f")),
	AO_Pair("line_medium_landing",LLUUID("f4f00d6e-b9fe-9292-f4cb-0ae06ea58d57")),
	AO_Pair("line_hard_landing",LLUUID("3da1d753-028a-5446-24f3-9c9b856d9422")),
	AO_Pair("line_flying_slow",LLUUID("2b5a38b2-5e00-3a97-a495-4c826bc443e6")),
	AO_Pair("line_sitting_on_ground",LLUUID("1a2bd58e-87ff-0df8-0b4c-53e047b0bb6e"))};

LLAOStandTimer::LLAOStandTimer(F32 period) : LLEventTimer(period)
{
}
BOOL LLAOStandTimer::tick()
{
	if(!mPaused && LLAO::isEnabled() && !LLAO::mStandOverrides.empty())
	{
#ifdef AO_DEBUG
		llinfos << "tick" << llendl;
#endif
		LLVOAvatar* avatarp = gAgent.getAvatarObject();
		if (avatarp)
		{
			for ( LLVOAvatar::AnimIterator anim_it =
					  avatarp->mPlayingAnimations.begin();
				  anim_it != avatarp->mPlayingAnimations.end();
				  anim_it++)
			{
				if(LLAO::isStand(anim_it->first))
				{
					//back is always last played, front is next
					avatarp->stopMotion(LLAO::mStandOverrides.back());
#ifdef AO_DEBUG
					llinfos << "Stopping " << LLAO::mStandOverrides.back().asString() << llendl;
#endif
					avatarp->startMotion(LLAO::mStandOverrides.front());
#ifdef AO_DEBUG
					llinfos << "Starting " << LLAO::mStandOverrides.front().asString() << llendl;
#endif
					LLAO::mStandOverrides.push_back(LLAO::mStandOverrides.front());
					LLAO::mStandOverrides.pop_front();
					LLFloaterAO* ao = LLFloaterAO::sInstance;
					if(ao)
					{
						ao->mStandsCombo->setSimple(LLStringExplicit(LLAO::mStandOverrides.back().asString()));
					}
					break;
				}
			}
		}
	}
	return FALSE;
}

void LLAOStandTimer::pause()
{
	if(mPaused) return;
#ifdef AO_DEBUG
	llinfos << "Pausing AO Timer...." << llendl;
#endif
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
#ifdef AO_DEBUG
		llinfos << "Stopping " << LLAO::mStandOverrides.back().asString() << llendl;
#endif
		gAgent.sendAnimationRequest(LLAO::mStandOverrides.back(), ANIM_REQUEST_STOP);
		avatarp->stopMotion(LLAO::mStandOverrides.back());
	}
	mEventTimer.reset();
	mEventTimer.stop();
	mPaused = TRUE;
}

void LLAOStandTimer::resume()
{
	if(!mPaused) return;
#ifdef AO_DEBUG
	llinfos << "Unpausing AO Timer...." << llendl;
#endif
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
#ifdef AO_DEBUG
		llinfos << "Starting " << LLAO::mStandOverrides.back().asString() << llendl;
#endif
		gAgent.sendAnimationRequest(LLAO::mStandOverrides.back(), ANIM_REQUEST_START);
		avatarp->startMotion(LLAO::mStandOverrides.back());
	}
	mEventTimer.reset();
	mEventTimer.start();
	mPaused = FALSE;
}

void LLAOStandTimer::reset()
{
	mEventTimer.reset();
}

AOLineEditor::AOLineEditor(const std::string& name, const LLRect& rect,
						  void (*callback)(LLViewerInventoryItem*,const char*),const char* field) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mCallback(callback),
	mField(field)
{
}

AOLineEditor::~AOLineEditor()
{
}

BOOL AOLineEditor::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	if(getParent())
	{
		LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
		if(gInventory.getItem(inv_item->getUUID()))
		{
			*accept = ACCEPT_YES_COPY_SINGLE;
			if(drop)
			{
				mCallback(inv_item,mField);
			}
		}
		else
		{
			*accept = ACCEPT_NO;
		}
		return TRUE;
	}
	return FALSE;
}

// -------------------------------------------------------

//static
void LLAO::setup()
{
	mEnabled = gSavedSettings.getBOOL("AO.Enabled");
	mPeriod = gSavedSettings.getF32("AO.Period");
	mTimer = new LLAOStandTimer(mPeriod);
	gSavedSettings.getControl("AO.Enabled")->getSignal()->connect(boost::bind(&handleAOEnabledChanged, _1));
	gSavedSettings.getControl("AO.Period")->getSignal()->connect(boost::bind(&handleAOPeriodChanged, _1));
}
//static
void LLAO::runAnims(BOOL enabled)
{
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		for ( LLVOAvatar::AnimIterator anim_it =
				  avatarp->mPlayingAnimations.begin();
			  anim_it != avatarp->mPlayingAnimations.end();
			  anim_it++)
		{
			if(LLAO::mOverrides.find(anim_it->first) != LLAO::mOverrides.end())
			{
				LLUUID anim_id = mOverrides[anim_it->first];
				// this is an override anim
				if(enabled)
				{
					// make override start
					avatarp->startMotion(anim_id);
				}
				else
				{
					avatarp->stopMotion(anim_id);
					gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_STOP);
				}
			}
		}
		if(mTimer)
		{
			if(enabled)
				mTimer->resume();
			else
				mTimer->pause();
		}
	}
}
//static
bool LLAO::handleAOPeriodChanged(const LLSD& newvalue)
{
	F32 value = (F32)newvalue.asReal();
	mPeriod = value;
	return true;
}
//static
bool LLAO::handleAOEnabledChanged(const LLSD& newvalue)
{
	BOOL value = newvalue.asBoolean();
	mEnabled = value;
	runAnims(value);
	return true;
}
//static
BOOL LLAO::isStand(LLUUID _id)
{
	std::string id = _id.asString();
	//ALL KNOWN STANDS
	if(id == "2408fe9e-df1d-1d7d-f4ff-1384fa7b350f") return TRUE;
	if(id == "15468e00-3400-bb66-cecc-646d7c14458e") return TRUE;
	if(id == "370f3a20-6ca6-9971-848c-9a01bc42ae3c") return TRUE;
	if(id == "42b46214-4b44-79ae-deb8-0df61424ff4b") return TRUE;
	if(id == "f22fed8b-a5ed-2c93-64d5-bdd8b93c889f") return TRUE;
	return FALSE;
}
//static
void LLAO::refresh()
{
	mOverrides.clear();
	mStandOverrides.clear();
	LLSD settings = gSavedPerAccountSettings.getLLSD("AO.Settings");
	//S32 version = (S32)settings["version"].asInteger();
	LLSD overrides = settings["overrides"];
	LLSD::map_iterator sd_it = overrides.beginMap();
	LLSD::map_iterator sd_end = overrides.endMap();
	for( ; sd_it != sd_end; sd_it++)
	{
		if(sd_it->first == "stands")
			for(LLSD::array_iterator itr = sd_it->second.beginArray();
				itr != sd_it->second.endArray(); ++itr)
			{
					//list of listness
				if(itr->asUUID().notNull())
					mStandOverrides.push_back(itr->asUUID());
			}
		// ignore if override is null key...
		if(sd_it->second.asUUID().isNull() 
			// don't allow override to be used as a trigger
			|| mOverrides.find(sd_it->second.asUUID()) != mOverrides.end())
			continue;
		else if(LLAO::isStand(LLUUID(sd_it->first)))
			//list of listness
			mStandOverrides.push_back(sd_it->second.asUUID());
		else
			//add to the list 
			mOverrides[LLUUID(sd_it->first)] = sd_it->second.asUUID();
	}
}

//static
void LLFloaterAO::show()
{
	if(sInstance)
		sInstance->open();
	else
		(new LLFloaterAO())->open();
}

LLFloaterAO::LLFloaterAO()
:	LLFloater()
{
	sInstance = this;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_ao.xml");
}

LLFloaterAO::~LLFloaterAO()
{
	sInstance = NULL;
	while(!sLineEditorDrop.empty()) delete sLineEditorDrop.back(), sLineEditorDrop.pop_back();
}

BOOL LLFloaterAO::postBuild(void)
{
	childSetAction("btn_save", onClickSave, this);
	childSetAction("btn_load", onClickLoad, this);
	//empty local list line_editor_drop
	while(!sLineEditorDrop.empty()) delete sLineEditorDrop.back(), sLineEditorDrop.pop_back();
	BOOST_FOREACH(AO_Pair pair,ao_pair_list)
	{
		LLLineEditor* editor = getChild<LLLineEditor>(pair.field);
		sLineEditorDrop.push_back(new AOLineEditor(std::string("drop target ").append(pair.field), editor->getRect(), LLFloaterAO::onAnimDrop, pair.field));
		addChild(sLineEditorDrop.back());
		childSetCommitCallback(pair.field, onCommitAnim, this);
	}
	LLComboBox* combo = getChild<LLComboBox>( "combo_stands");
	combo->setAllowTextEntry(TRUE,36,TRUE);
	combo->setCommitCallback(onCommitStands);
	combo->setCallbackUserData(this);
	sLineEditorDrop.push_back(new AOLineEditor(std::string("drop target stands"), combo->getRect(), LLFloaterAO::onAnimDrop, "stands"));
	addChild(sLineEditorDrop.back());
	mStandsCombo = combo;
	
	childSetAction("combo_stands_add", onClickStandAdd, this);
	childSetAction("combo_stands_delete", onClickStandRemove, this);
	refresh();
	return TRUE;
}
void LLFloaterAO::onAnimDrop(LLViewerInventoryItem* item, const char* field)
{
	LLUUID uuid = LLUUID::null;
	if(item && item->getIsLinkType())
		uuid = item->getLinkedItem()->getAssetUUID();
	else
		uuid = item->getAssetUUID();
	if(sInstance){
		if(strcmp(field,"stands") == 0){
			sInstance->mStandsCombo->setLabel(uuid.asString());
			onClickStandAdd(sInstance);
		}
		else{
			sInstance->childSetText(field, uuid.asString());
			onCommitAnim(NULL,sInstance);
		}
	}
}
std::string LLFloaterAO::idstr(LLUUID id)
{
	if(id.notNull()) return id.asString();
	else return "";
}

void LLFloaterAO::refresh()
{
	BOOST_FOREACH(AO_Pair pair,ao_pair_list)
	{
		childSetText(pair.field, idstr(LLAO::mOverrides[pair.uuid]));
	}
	mStandsCombo->clearRows();
	for(std::list<LLUUID>::iterator itr = LLAO::mStandOverrides.begin();itr != LLAO::mStandOverrides.end();
		itr++)
	{
		mStandsCombo->add((*itr).asString());
	}
	mStandsCombo->setSimple(LLStringExplicit(LLAO::mStandOverrides.back().asString()));
}
// static
void LLFloaterAO::onCommitStands(LLUICtrl* ctrl, void* user_data)
{
	//LLFloaterAO* floater = (LLFloaterAO*)user_data;
	LLUUID id = ctrl->getValue().asUUID();
	std::list<LLUUID>::iterator itr = std::find(LLAO::mStandOverrides.begin(),LLAO::mStandOverrides.end(),id);
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if(id.notNull() && itr != LLAO::mStandOverrides.end())
	{
		//back is always last played
		if(LLAO::isEnabled()){
			avatarp->stopMotion(LLAO::mStandOverrides.back());
			avatarp->startMotion(id);
		}
		LLAO::mStandOverrides.push_back(id);
		LLAO::mStandOverrides.erase(itr);

		LLAO::mTimer->reset();
	}
	onCommitAnim(NULL,user_data);
}
// static
void LLFloaterAO::onCommitAnim(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;

	LLSD overrides;
	BOOST_FOREACH(AO_Pair pair,ao_pair_list){
		LLUUID id = LLUUID(floater->childGetValue(pair.field).asString());
		if(id.notNull()) overrides[pair.uuid.asString()] = id;
	}
	for(std::list<LLUUID>::iterator itr = LLAO::mStandOverrides.begin();itr != LLAO::mStandOverrides.end();
		itr++)
	{
		overrides["stands"].append((*itr));
	}
	LLSD settings;
	settings["version"] = 2;
	settings["overrides"] = overrides;
	gSavedPerAccountSettings.setLLSD("AO.Settings", settings);
	LLAO::refresh();
	floater->refresh();
}

//static
void LLFloaterAO::onClickStandRemove(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	LLUUID id = floater->mStandsCombo->getValue().asUUID();
	std::list<LLUUID>::iterator itr = std::find(LLAO::mStandOverrides.begin(),LLAO::mStandOverrides.end(),id);
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if(id.notNull() && itr != LLAO::mStandOverrides.end())
	{
		//back is always last played, front is next
		if(LLAO::isEnabled()){
			avatarp->stopMotion(id);
			avatarp->startMotion(LLAO::mStandOverrides.front());
		}
		LLAO::mStandOverrides.erase(itr);
		LLAO::mStandOverrides.push_back(LLAO::mStandOverrides.front());
		LLAO::mStandOverrides.pop_front();

		floater->refresh();
		LLAO::mTimer->reset();
	}
	onCommitAnim(NULL,user_data);
}
//static
void LLFloaterAO::onClickStandAdd(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;
	LLUUID id = floater->mStandsCombo->getValue().asUUID();
	std::list<LLUUID>::iterator itr = std::find(LLAO::mStandOverrides.begin(),LLAO::mStandOverrides.end(),id);
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if(id.notNull() && itr == LLAO::mStandOverrides.end())
	{
		//back is always last played
		if(LLAO::isEnabled()){
			avatarp->stopMotion(LLAO::mStandOverrides.back());
			avatarp->startMotion(id);
		}
		LLAO::mStandOverrides.push_back(id);

		floater->refresh();
		LLAO::mTimer->reset();
	}
	onCommitAnim(NULL,user_data);
}
//static
void LLFloaterAO::onClickSave(void* user_data)
{
	LLFilePicker& file_picker = LLFilePicker::instance();
	if(file_picker.getSaveFile( LLFilePicker::FFSAVE_AO, LLDir::getScrubbedFileName("untitled.ao")))
	{
		std::string file_name = file_picker.getFirstFile();
		llofstream export_file(file_name);
		LLSDSerialize::toPrettyXML(gSavedPerAccountSettings.getLLSD("AO.Settings"), export_file);
		export_file.close();
	}
}

//static
void LLFloaterAO::onClickLoad(void* user_data)
{
	LLFloaterAO* floater = (LLFloaterAO*)user_data;

	LLFilePicker& file_picker = LLFilePicker::instance();
	if(file_picker.getOpenFile(LLFilePicker::FFLOAD_AO))
	{
		std::string file_name = file_picker.getFirstFile();
		llifstream xml_file(file_name);
		if(!xml_file.is_open()) return;
		LLSD data;
		if(LLSDSerialize::fromXML(data, xml_file) >= 1)
		{
			if(LLAO::isEnabled())
				LLAO::runAnims(FALSE);

			gSavedPerAccountSettings.setLLSD("AO.Settings", data);
			LLAO::refresh();

			if(LLAO::isEnabled())
				LLAO::runAnims(TRUE);

			floater->refresh();
		}
		xml_file.close();
	}
}

// </edit>
