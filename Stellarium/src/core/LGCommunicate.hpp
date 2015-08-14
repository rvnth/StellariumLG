#include "StelMovementMgr.hpp"
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
		Vec3d viewdirection;
		Vec3d v3;
		double fov;
		bool viewchanged;
		bool listening;
		std::mutex mtx;

		MODE mode;
		int offset;

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
		void connect (int offset);
	
		void write (double f);
		void write (Vec3d v);
		bool read (StelMovementMgr* smm);
	
		void send ();
		void listen ();
		
		void read (int i, StelMovementMgr* smm);
		void write (int i, Vec3d v);
		void write (int i, double f);
};


