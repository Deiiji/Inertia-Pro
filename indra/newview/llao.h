// <edit>
#ifndef LL_LLAO_H
#define LL_LLAO_H

//this is for debugging ;D
//#define AO_DEBUG

#include "llfloater.h"
#include "llcombobox.h"
#include "llviewerinventory.h"

class LLAOStandTimer : public LLEventTimer
{
public:
	LLAOStandTimer(F32 period);
	BOOL tick();
	void pause();
	void resume();
	void reset();
private:
	BOOL mPaused;
};
typedef struct aopair
{
	const char* field;
	const LLUUID uuid;
	aopair(const char* f,const LLUUID u):
		field(f),
		uuid(u)
	{
	}
} AO_Pair;
class AOLineEditor : public LLView
{
public:
	AOLineEditor(const std::string& name, const LLRect& rect, void (*callback)(LLViewerInventoryItem*,const char*),const char*);
	~AOLineEditor();
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
protected:
	void	(*mCallback)(LLViewerInventoryItem*,const char*);
	const char* mField;
};
class LLAO
{
public:
	static void setup();
	static std::map<LLUUID,LLUUID> mOverrides;
	static std::list<LLUUID> mStandOverrides;
	static BOOL isEnabled(){ return mEnabled; }
	static BOOL isStand(LLUUID _id);
	static void refresh();
	static void runAnims(BOOL enabled);
	static bool handleAOEnabledChanged(const LLSD& newvalue);
	static bool handleAOPeriodChanged(const LLSD& newvalue);
	static LLAOStandTimer* mTimer;
private:
	static BOOL mEnabled;
	static F32 mPeriod;
};

class LLFloaterAO : public LLFloater
{
public:
	static LLFloaterAO* sInstance;
	static void show();
	LLFloaterAO();
	BOOL postBuild(void);
	void refresh();
	static void onAnimDrop(LLViewerInventoryItem* item, const char* field);
	static void onCommitAnim(LLUICtrl* ctrl, void* user_data);
	static void onCommitStands(LLUICtrl* ctrl,void* user_data);
	static void onClickStandRemove(void* user_data);
	static void onClickStandAdd(void* user_data);
	static void onClickSave(void* user_data);
	static void onClickLoad(void* user_data);
private:
	virtual ~LLFloaterAO();
	std::string idstr(LLUUID id); // silly utility
	static std::vector< AOLineEditor* > sLineEditorDrop;
public:
	LLComboBox* mStandsCombo;
};

#endif
// </edit>
