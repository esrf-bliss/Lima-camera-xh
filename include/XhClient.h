//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
/*
 * XhClient.cpp
 * Created on: Apr 10, 2013
 * Author: gm
 */

#ifndef XHCLIENT_CPP_
#define XHCLIENT_CPP_

#include <netinet/in.h>
#include "Debug.h"

using namespace std;

namespace lima {
namespace Xh {

const int RD_BUFF = 1000;	// Read buffer for more efficient recv

class XhClient {
DEB_CLASS_NAMESPC(DebModCamera, "XhClient", "Xh");

public:
	XhClient();
	~XhClient();

	void sendNowait(string cmd);
	void sendWait(string cmd);
	void sendWait(string cmd, int& value);
	void sendWait(string cmd, double& value);
	void sendWait(string cmd, string& value);

	int waitForResponse(string& value);
	int waitForResponse(double& value);
	int waitForResponse(int& value);

	int connectToServer (const string hostname, int port);
	void disconnectFromServer();
	int initServerDataPort();
	void getData(void* bptr, int num);
	string getErrorMessage() const;
	vector<string> getDebugMessages() const;

private:
	mutable Cond m_cond;
	bool m_valid;						// true if connected
	int m_skt;							// socket for commands */
	struct sockaddr_in m_remote_addr;	// address of remote server */
	int m_data_port;					// our data port
	int m_data_listen_skt;				// data socket we listen on
	int m_prompts;						// counts # of prompts received
	int m_num_read, m_cur_pos;
	char m_rd_buff[RD_BUFF];
	int m_just_read;
	string m_errorMessage;
	vector<string> m_debugMessages;

	enum ServerResponse {
		CLN_NEXT_PROMPT,		// '> ': at prompt
		CLN_NEXT_ERRMSG, 		// '! ': read error message
		CLN_NEXT_DEBUGMSG,		// '# ': read debugging message
		CLN_NEXT_TIMEBAR,		// '@ ': time-bar message
		CLN_NEXT_UNKNOWN,		// unknown line
		CLN_NEXT_INTRET,		// '* ': read integer ret value
		CLN_NEXT_DBLRET,		// '* ': read double ret value
		CLN_NEXT_STRRET			// '* ': read string ret value
	};
	void sendCmd(const string cmd);
	int waitForPrompt();
	int nextLine(string *errmsg, int *ivalue, double *dvalue, string *svalue, int *done, int *outoff);
	int getChar();

	void errmsg_handler(const string errmsg);
	void debugmsg_handler(const string msg);
	void timebar_handler(int done, int outoff, const string errmsg);
	void error_handler(const string errmsg);
};

} // namespace Xh
} // namespace lima

#endif /* XHCLIENT_CPP_ */
