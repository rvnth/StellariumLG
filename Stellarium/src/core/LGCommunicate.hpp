#include "StelMovementMgr.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "modules/Atmosphere.hpp"
#include <zmq.hpp>
#include <mutex>
#include <QThread>

#include <iostream>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <fstream>

class ListenerThread : public QThread
{
	Q_OBJECT
	protected:
		void run();
};

class Communicate {
	public:
		enum MODE {
			NONE,
			SERVER,
			CLIENT
		};

	private:
		Vec3d viewdirection, viewdirection1;
		Vec3d v3;
		double fov, fov1;
		bool atmosvis;


		bool viewchanged;
		bool listening;
		std::mutex mtx;

		bool vd1, f1;

		MODE mode;
		int offset;
		std::string port;

		zmq::context_t* ctx;; 
		zmq::socket_t* s;
		ListenerThread* Listener;

		int current, MAX_EVENTS;
		std::string prefix;
		std::ofstream file;
		std::ifstream ifile;
		int t;
		bool comm;
		std::string filename;


	public:
		Communicate (std::string _prefix);
		~Communicate ();
		
		static Communicate& instance () {
			static Communicate c("rstream");
			return c;
		}
		void setMode (MODE m) { mode = m; }
		void connect (int _offset, std::string _port);
	
		void write (double f);
		void write (Vec3d v);
		bool read (StelMovementMgr* smm);
	
		void send ();
		void listen ();
		
		void read (int i, StelMovementMgr* smm);
		void write (int i, Vec3d v);
		void write (int i, double f);
///		void write1 (double f);
///		void write1 (Vec3d v);
};


