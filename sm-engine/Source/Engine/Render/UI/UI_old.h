#pragma once

#include "Engine/Core/Types.h"

namespace UI
{
	enum MsgType : U8
	{
		kPersistent,
		kFrame
	};

	void Init();
	void BeginFrame();
	void Render();

	void LogMsg(MsgType msgType, const char* msg);
	void LogMsgFmt(MsgType msgType, const char* fmt, ...);
	void ClearMsgLog(MsgType msgType);
}
