/*
 * Stellarium LG
 * Copyright (C) 2015 Revanth N R
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _LGCOMMUNICATE_HPP_
#define _LGCOMMUNICATE_HPP_

#include <iostream>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <fstream>
#include <zmq.hpp>
#include <mutex>
#include <QThread>

class StelMovementMgr;
class LGCommunicate;

#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "modules/LandscapeMgr.hpp"

class LGListenerThread : public QThread
{
	Q_OBJECT
	public:
		LGListenerThread (LGCommunicate* _lgc) { lgc=_lgc; }
		~LGListenerThread () { }
	protected:
		LGCommunicate* lgc;
		void run();
};

class LGCommunicate : public QObject
{
	Q_OBJECT
	public:
		enum MODE {
			NONE,
			SERVER,
			CLIENT
		};

	private:
		Vec3d viewdirection;
		double fov;
		bool atmosvis;

		bool viewchanged;
		bool listening;
		std::mutex mtx;

		MODE mode;
		int offset;
		std::string port;

		zmq::context_t* ctx;; 
		zmq::socket_t* s;
		LGListenerThread* Listener;

		StelCore* core;
		StelMovementMgr* mmgr;

	public slots:
		//! Send new time rate to clients
		void sendTimeRate (double rate);
		//! Send time reset to clients
		void sendTimeReset ();

	public:
		LGCommunicate (StelCore* _core, StelMovementMgr* _mmgr, MODE m=NONE, int _offset=0, std::string _port="5000");
		~LGCommunicate ();
		
		void setMode (MODE m) { mode = m; }
		void connectSocket (int _offset, std::string _port="5000");
	
		void write (double f);
		void write (Vec3d v);
		bool read ();
	
		void send ();
		void listen ();
};

#endif // _LGCOMMUNICATE_HPP_
