/**
 * @file floaterhop.h
 * @brief floater for gridhopping
 *
 * Copyright (C) 2012 arminweatherwax (at) lavabit.com
 * floaterhop.h is partially a drivate work of:
 *	llfloaterwebcontent.h
 *	Copyright (C) 2010, Linden Research, Inc.
 *	licensed under GNU Lesser General Public License
 *	as published by the Free Software Foundation;
 *	version 2.1 of the License only.
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
 *
 * Any part that isn't derivate work of llfloaterwebcontent.h
 * can also be used under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 *
 */

#ifndef FLOATER_HOP_H
#define FLOATER_HOP_H

#include "llfloater.h"
#include "llmediactrl.h"
#include "llsdparam.h"
#include "llslurl.h"

class LLMediaCtrl;
class LLTextBox;
class HopTimer;

class FloaterHop :
	public LLFloater,
	public LLViewerMediaObserver,
	public LLInstanceTracker<FloaterHop, std::string>
{
public:
	typedef LLInstanceTracker<FloaterHop, std::string> instance_tracker_t;
    LOG_CLASS(FloaterHop);

	struct _Params : public LLInitParam::Block<_Params>
	{
		Optional<std::string>	url,
								target,
								window_class,
								id;
		Optional<bool>			show_chrome,
								allow_address_entry,
								trusted_content,
								show_page_title;
		Optional<LLRect>		preferred_media_size;

		_Params();
	};

	typedef LLSDParamAdapter<_Params> Params;

	FloaterHop(const Params& params);

	static void closeRequest(const std::string &uuid);
	static void geometryChanged(const std::string &uuid, S32 x, S32 y, S32 width, S32 height);
	void geometryChanged(S32 x, S32 y, S32 width, S32 height);

	/* virtual */ BOOL postBuild();
	/* virtual */ void onOpen(const LLSD& key);
	/* virtual */ void onClose(bool app_quitting);
	/* virtual */ void draw();

	static void onHopFinished(void* userdata);
	static void onHopFailed(void* userdata);
	static void onClickHop(void* userdata);
	static void onClickCancel(void* userdata);


protected:
	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	void open_media(const Params& );
	void set_current_url(const std::string& url);

	LLMediaCtrl*	mWebBrowser;

	LLSLURL mSLURL;
	std::string		mCurrentURL;
	std::string		mUUID;
	bool			mShowPageTitle;
	HopTimer* mHopTimer;
};

#endif  // FLOATER_HOP_H
