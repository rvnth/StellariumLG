#include "LGCommunicate.hpp"

void ListenerThread::run()
{
	lgc->listen();
}

LGCommunicate::LGCommunicate (StelCore* _core, StelMovementMgr* _mmgr, MODE m, int _offset, std::string _port) {
	core = _core;
	mmgr = _mmgr;
	
	mode = m;

	fov = 60;
	viewchanged = false;
	listening = true;
	atmosvis = true;


	ctx = new zmq::context_t(1); 
	s = new zmq::socket_t(*ctx, ZMQ_PAIR);
	Listener = new ListenerThread(this);

	if (mode!=NONE)
		connectSocket (_offset, _port);

	if (mode==SERVER) {
		QObject::connect ((const QObject*)core, SIGNAL(timeRateChanged(double)), this, SLOT(sendTimeRate(double)));
		QObject::connect ((const QObject*)core, SIGNAL(timeReset()), this, SLOT(sendTimeReset()));
	}
				

/*
	// Create thread for listen

	//zmq::message_t rep;
	//s->recv (&rep, 0);
	//cout << "Received " << string((char*)rep.data(), rep.size()) << endl;
*/
}

LGCommunicate::~LGCommunicate () { 
	listening = false;
	s->disconnect ("tcp://127.0.0.1:"+port);
	s->close();
}

void LGCommunicate::connectSocket (int _offset, std::string _port) {
	offset = _offset;
	port = _port;
	s->connect ("tcp://127.0.0.1:"+port);

	std::stringstream data;
	data<<"Stellarium Connection Established";
	zmq::message_t mssg (data.str().length()+1);
	memcpy ((void*)mssg.data(), (void *)data.str().c_str(), data.str().length()+1);
	s->send (mssg);
	Listener->start();
}

void LGCommunicate::write (double f) {
	if (mode==SERVER) {
		fov = f;
	}
}

void LGCommunicate::write (Vec3d v) {
	if (mode==SERVER) {
		viewdirection = v;
	}
}

void LGCommunicate::send () {
	if (mode!=SERVER)
		return;
/*	memcpy(&data[1], &viewdirection[0], sizeof(double));
	memcpy(&data[9], &viewdirection[1], sizeof(double));
	memcpy(&data[17], &viewdirection[2], sizeof(double));
	memcpy(&data[25], &fov, sizeof(double));

				for (int i=0; i<34; i++)
					std::cout << (int)data[i] << ", ";
				std::cout << std::endl;


	Vec3d v3;
	double fv;
	memcpy(&v3[0], &data[1], sizeof(double));
	memcpy(&v3[1], &data[9], sizeof(double));
	memcpy(&v3[2], &data[17], sizeof(double));
	memcpy(&fv, &data[25], sizeof(double));

	std::cout << "SENDING: " << viewdirection[0] << ", " << viewdirection[1] << ", " << viewdirection[2] << " ... " << fov << std::endl;
	std::cout << "SENT:    " << v3[0] << ", " << v3[1] << ", " << v3[2] << " ... " << fv << std::endl;


	zmq::message_t mssg (34);
	memcpy ((void*)mssg.data(), data, 34);
	s->send (mssg);
*/
	{
		std::stringstream datass;
		datass << "1 " << viewdirection[0] << " " << viewdirection[1] << " " << viewdirection[2] << " " << fov << " ";
		zmq::message_t mssg (datass.str().length());
		memcpy ((void*)mssg.data(), datass.str().c_str(), datass.str().length());
		s->send (mssg);
	}

	bool currentatmosvis = GETSTELMODULE(LandscapeMgr)->getFlagAtmosphere();
	if (atmosvis != currentatmosvis) {
		atmosvis = currentatmosvis;
		std::stringstream datass;
		datass << "2 " << atmosvis << " ";
		zmq::message_t mssg (datass.str().length());
		memcpy ((void*)mssg.data(), datass.str().c_str(), datass.str().length());
		s->send (mssg);
	}

	// if (fov > 0);
	// create prime message 00000002
	// else
	// create prime message 00000001
}

void LGCommunicate::sendTimeRate (double rate) {
	std::stringstream datass;
	datass << "3 " << rate << " ";
	zmq::message_t mssg (datass.str().length());
	memcpy ((void*)mssg.data(), datass.str().c_str(), datass.str().length());
	s->send (mssg);
	std::cout << "SENDING  " << datass.str() << std::endl;
}

void LGCommunicate::sendTimeReset () {
	std::stringstream datass;
	datass << "4 "; 
	zmq::message_t mssg (datass.str().length());
	memcpy ((void*)mssg.data(), datass.str().c_str(), datass.str().length());
	s->send (mssg);
	std::cout << "SENDINGR " << datass.str() << std::endl;
}

void LGCommunicate::listen () {
	if (mode!=CLIENT)
		return;
	while (listening) {
		zmq::message_t mssg;
		s->recv (&mssg, 0);
		char* data = (char*)mssg.data();
		std::string datas(data);
		std::stringstream datass(datas);
		int mid=0;
		datass >> mid;
		switch(mid) {
			case 1:
				mtx.lock();

/*
				memcpy(&viewdirection[0], &data[1], sizeof(double));
				memcpy(&viewdirection[1], &data[9], sizeof(double));
				memcpy(&viewdirection[2], &data[17], sizeof(double));
				memcpy(&fov, &data[25], sizeof(double));
*/
				datass >> viewdirection[0] >> viewdirection[1] >> viewdirection[2] >> fov;
				viewchanged = true;
				mtx.unlock();
				break;
			case 2:
				{
					bool atmosvis;
					datass >> atmosvis;
					LandscapeMgr* lmr = (LandscapeMgr*)GETSTELMODULE(LandscapeMgr);
					if (lmr)
						lmr->setFlagAtmosphere(atmosvis);
				}
				break;
			case 3:
				{
					double rate;
					datass >> rate;
					core->setTimeRate(rate);
					std::cout << "RECEIVED " << rate << std::endl;
				}
				break;
			case 4:
				core->setTimeNow();
				break;
		}
	}
}

bool LGCommunicate::read () {
	if (mode!=CLIENT)
		return false;

	if(viewchanged && mtx.try_lock()) {
		//			mtx.lock();
		mmgr->setViewDirectionJ2000(viewdirection);
		mmgr->setCFov(fov);
		viewchanged = false;
		mtx.unlock();
		return true;
	} else {
		std::cout << "VIEW DIR NOT AVAILABLE !!!" << std::endl;
		return false;
	}
}


/* END OF USEFUL CODE */

