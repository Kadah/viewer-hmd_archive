/** 
 * @file lllog.cpp
 * @author Don
 * @date 2007-11-27
 * @brief  Class to log messages to syslog for streambase to process.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"
#include "lllog.h"

#include "llapp.h"
#include "llsd.h"
#include "llsdserialize.h"


class LLLogImpl
{
public:
	LLLogImpl(LLApp* app) : mApp(app) {}
	~LLLogImpl() {}

	void log(const std::string message, LLSD& info);
	bool useLegacyLogMessage(const std::string message);

private:
	LLApp* mApp;
};


//@brief Function to log a message to syslog for streambase to collect.
void LLLogImpl::log(const std::string message, LLSD& info)
{
	static S32 sequence = 0;
    LLSD log_config = mApp->getOption("log-messages");
	if (log_config.has(message))
	{
		LLSD message_config = log_config[message];
		if (message_config.has("use-syslog"))
		{
			if (! message_config["use-syslog"].asBoolean())
			{
				return;
			}
		}
	}
	llinfos << "LLLOGMESSAGE (" << (sequence++) << ") " << message << " " << LLSDXMLStreamer(info) << llendl;
}

//@brief Function to check if specified legacy log message should be sent.
bool LLLogImpl::useLegacyLogMessage(const std::string message)
{
    LLSD log_config = mApp->getOption("log-messages");
	if (log_config.has(message))
	{
		LLSD message_config = log_config[message];
		if (message_config.has("use-legacy"))
		{
			return message_config["use-legacy"].asBoolean();
		}
	}
	return true;
}


LLLog::LLLog(LLApp* app)
{
	mImpl = new LLLogImpl(app);
}

LLLog::~LLLog()
{
	delete mImpl;
	mImpl = NULL;
}

void LLLog::log(const std::string message, LLSD& info)
{
	if (mImpl) mImpl->log(message, info);
}

bool LLLog::useLegacyLogMessage(const std::string message)
{
	if (mImpl)
	{
		return mImpl->useLegacyLogMessage(message);
	}
	return true;
}

