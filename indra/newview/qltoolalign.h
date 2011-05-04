/** 
 * @file qltoolalign.h
 * @brief A tool to align objects
 */

#ifndef QL_QLTOOLALIGN_H
#define QL_QLTOOLALIGN_H

#include "lltool.h"
#include "llbbox.h"

class LLViewerObject;
class LLPickInfo;
class LLToolSelectRect;

class QLToolAlign
:	public LLTool, public LLSingleton<QLToolAlign>
{
public:
	QLToolAlign();
	virtual ~QLToolAlign();

	virtual void	handleSelect();
	virtual void	handleDeselect();
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL    handleHover(S32 x, S32 y, MASK mask);
	virtual void	render();

	static void pickCallback(const LLPickInfo& pick_info);

private:
	void            align();
	void            computeManipulatorSize();
	void            renderManipulators();
	BOOL            findSelectedManipulator(S32 x, S32 y);
	
	LLBBox          mBBox;
	F32             mManipulatorSize;
	S32             mHighlightedAxis;
	F32             mHighlightedDirection;
	BOOL            mForce;
};

#endif // QL_QLTOOLALIGN_H
