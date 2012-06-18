/**
 * @file floaterhop.cpp
 * @brief floater for gridhopping
 *
 * Copyright (C) 2012 arminweatherwax (at) lavabit.com
 * floaterhop.cpp is partially a drivate work of:
 *	llfloaterwebcontent.cpp
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
 * Any part that isn't derivate work of llfloaterwebcontent.cpp
 * can also be used under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 *
 */

#include "llviewerprecompiledheaders.h"

#include "floaterhop.h"
#include "llappviewer.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "llpluginclassmedia.h"
#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llweb.h"
#include "llwindow.h"
#include "teapot.h"
#include "lleventtimer.h"

class HopTimer : LLEventTimer
{
public:
	HopTimer(const std::string& cookie, void* userdata, int timeout = 30) : LLEventTimer(1),
		mCookie(cookie),
		mUserdata(userdata),
		mTimeout(timeout)
	{
		std::string c = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,mCookie);
		c.append(".relogcookie");
		LLFILE* fp = LLFile::fopen(c, "wb");
		LLFile::close(fp);
		//llwarns << "hello " << this << llendl;

	}

	~HopTimer()
	{
		//llwarns << "bye " << this << llendl;
	}

	virtual BOOL tick()
	{
		llstat file_stat;
		std::string c = mCookie;
		c.append(".relogcookie");
		if(LLFile::stat(c, &file_stat))
		{
			FloaterHop::onHopFinished(mUserdata);
			//llwarns << "end of spam" << llendl;
			return TRUE;
		}
		else
		{
			if (0 >= mTimeout--)
			{
				llstat file_stat;
				if(!LLFile::stat(c, &file_stat))
				{
					LLFile::remove(c);
				}
				FloaterHop::onHopFailed(mUserdata);
				return TRUE;
			}
			//llwarns << "spam" << llendl;
			return FALSE;
		}
	}

private:
	std::string mCookie;
	void* mUserdata;
	int mTimeout;
};

FloaterHop::_Params::_Params()
:	url("url"),
	target("target"),
	id("id"),
	window_class("grid_hop"),
	show_chrome("show_chrome", false),
	allow_address_entry("allow_address_entry", false),
	preferred_media_size("preferred_media_size"),
	trusted_content("trusted_content", false),
	show_page_title("show_page_title", true)
{}

FloaterHop::FloaterHop( const Params& params )
:	LLFloater( params ),
	LLInstanceTracker<FloaterHop, std::string>(params.id()),
	mWebBrowser(NULL),
	mUUID(params.id()),
	mShowPageTitle(params.show_page_title)
{
	mSLURL = LLSLURL(params.url);
}

BOOL FloaterHop::postBuild()
{

	LLButton* hop_button = getChild<LLButton>("hop_btn");
	hop_button->setClickedCallback(onClickHop, this);
	LLButton* cancel_button = getChild<LLButton>("cancel_btn");
	cancel_button->setClickedCallback(onClickCancel, this);

	mWebBrowser        = getChild< LLMediaCtrl >( "thumbnail_login_html" );

	// observe browser events
	mWebBrowser->addObserver( this );


	return TRUE;
}

//static
void FloaterHop::closeRequest(const std::string &uuid)
{
	FloaterHop* floaterp = instance_tracker_t::getInstance(uuid);
	if (floaterp)
	{
		floaterp->closeFloater(false);
	}
}

//static
void FloaterHop::geometryChanged(const std::string &uuid, S32 x, S32 y, S32 width, S32 height)
{
	FloaterHop* floaterp = instance_tracker_t::getInstance(uuid);
	if (floaterp)
	{
		floaterp->geometryChanged(x, y, width, height);
	}
}

void FloaterHop::geometryChanged(S32 x, S32 y, S32 width, S32 height)
{
	// Make sure the layout of the browser control is updated, so this calculation is correct.
	LLLayoutStack::updateClass();

	// TODO: need to adjust size and constrain position to make sure floaters aren't moved outside the window view, etc.
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);

	// Adjust width and height for the size of the chrome on the web Browser window.
	LLRect browser_rect;
	mWebBrowser->localRectToOtherView(mWebBrowser->getLocalRect(), &browser_rect, this);

	S32 requested_browser_bottom = window_size.mY - (y + height);
	LLRect geom;
	geom.setOriginAndSize(x - browser_rect.mLeft, 
						requested_browser_bottom - browser_rect.mBottom, 
						width + getRect().getWidth() - browser_rect.getWidth(), 
						height + getRect().getHeight() - browser_rect.getHeight());

	lldebugs << "geometry change: " << geom << llendl;
	
	LLRect new_rect;
	getParent()->screenRectToLocal(geom, &new_rect);
	setShape(new_rect);	
}

void FloaterHop::open_media(const Params& p)
{
	// Specifying a mime type of text/html here causes the plugin system to skip the MIME type probe and just open a browser plugin.
	LLViewerMedia::proxyWindowOpened(p.target(), p.id());

	std::string grid = mSLURL.getGrid();
	LLSD grid_info;
	LLGridManager::getInstance()->getGridData(grid, grid_info);
	std::string url = grid_info[GRID_LOGIN_PAGE_VALUE].asString();

	mWebBrowser->setHomePageUrl(url, "text/html");
	mWebBrowser->setTarget(p.target);
	mWebBrowser->navigateTo(url, "text/html");
	
	set_current_url(url);

	getChild<LLLayoutPanel>("status_bar")->setVisible(p.show_chrome);
	getChild<LLLayoutPanel>("nav_controls")->setVisible(p.show_chrome);
	bool address_entry_enabled = p.allow_address_entry && !p.trusted_content;
	getChildView("address")->setEnabled(address_entry_enabled);
	getChildView("popexternal")->setEnabled(address_entry_enabled);

	if (!address_entry_enabled)
	{
		mWebBrowser->setFocus(TRUE);
	}

	if (!p.show_chrome)
	{
		setResizeLimits(100, 100);
	}

	if (!p.preferred_media_size().isEmpty())
	{
		LLLayoutStack::updateClass();
		LLRect browser_rect = mWebBrowser->calcScreenRect();
		LLCoordWindow window_size;
		getWindow()->getSize(&window_size);
		
		geometryChanged(browser_rect.mLeft, window_size.mY - browser_rect.mTop, p.preferred_media_size().getWidth(), p.preferred_media_size().getHeight());
	}

}

void FloaterHop::onOpen(const LLSD& key)
{
	Params params(key);

	if (!params.validateBlock())
	{
		closeFloater();
		return;
	}

	mWebBrowser->setTrustedContent(params.trusted_content);

	// tell the browser instance to load the specified URL
	open_media(params);
}

//virtual
void FloaterHop::onClose(bool app_quitting)
{
	LLViewerMedia::proxyWindowClosed(mUUID);
	destroy();
}

// virtual
void FloaterHop::draw()
{

	LLFloater::draw();
}

//static
void FloaterHop::onHopFinished(void* userdata)
{
	FloaterHop* instance = (FloaterHop*) userdata;
	instance->closeFloater();
	LLAppViewer::instance()->forceQuit();
}

//static
void FloaterHop::onHopFailed(void* userdata)
{
	FloaterHop* instance = (FloaterHop*) userdata;
	instance->closeFloater();
	llwarns << "something went wrong" << llendl;
}

//static
void FloaterHop::onClickHop(void* userdata)
{
	FloaterHop* instance = (FloaterHop*) userdata;

	std::vector<std::string> args;
	if(gSavedSettings.getBOOL("HopAutoLogin"))
	{
		args.push_back("-autologin");
	}

	bool keepalive = gSavedSettings.getBOOL("HopKeepOldSessionAlive");
	if(keepalive)
	{
		args.push_back("-multiple");
	}
	else
	{
		LLUUID cookie;
		cookie.generate();
		instance->mHopTimer = new HopTimer(cookie.asString(), instance);
		args.push_back("-isrelogsession");
		args.push_back(cookie.asString());
	}

	args.push_back(instance->mSLURL.getSLURLString());

	// this is under development and likely changing
	if(Teapot::getInstance()->launchNewViewer(args))
	{
		if(keepalive)
		{
			instance->closeFloater();
		}
	}
}

//static
void FloaterHop::onClickCancel(void* userdata)
{
	FloaterHop* instance = (FloaterHop*) userdata;
	instance->closeFloater();
}

// virtual
void FloaterHop::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	if(event == MEDIA_EVENT_LOCATION_CHANGED)
	{
		const std::string url = self->getLocation();

		set_current_url( url );
	}
	else if(event == MEDIA_EVENT_NAVIGATE_BEGIN)
	{

	}
	else if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{

	}
	else if(event == MEDIA_EVENT_CLOSE_REQUEST)
	{
		// The browser instance wants its window closed.
		closeFloater();
	}
	else if(event == MEDIA_EVENT_GEOMETRY_CHANGE)
	{
		geometryChanged(self->getGeometryX(), self->getGeometryY(), self->getGeometryWidth(), self->getGeometryHeight());
	}
	else if(event == MEDIA_EVENT_STATUS_TEXT_CHANGED )
	{

	}
	else if(event == MEDIA_EVENT_PROGRESS_UPDATED )
	{

	}
	else if(event == MEDIA_EVENT_NAME_CHANGED )
	{
		std::string page_title = self->getMediaName();
		// simulate browser behavior - title is empty, use the current URL
		if (mShowPageTitle)
		{
			if ( page_title.length() > 0 )
				setTitle( page_title );
			else
				setTitle( mCurrentURL );
		}
	}
	else if(event == MEDIA_EVENT_LINK_HOVERED )
	{

	}
}

void FloaterHop::set_current_url(const std::string& url)
{
	mCurrentURL = url;
}
