#include "LGCommunicate.hpp"

void ListenerThread::run()
{
	Communicate::instance().listen();
}

Communicate::Communicate (std::string _prefix) {
/*	MAX_EVENTS = 300;
	current = 0;
	prefix = _prefix;
	comm = false;
*/
	fov = 60;
	viewchanged = false;
	listening = true;
	port="5000";
/*
	vd1 = false;
	f1 = false;
*/
	ctx = new zmq::context_t(1); 
	s = new zmq::socket_t(*ctx, ZMQ_PAIR);
	Listener = new ListenerThread;

/*
	// Create thread for listen

	//zmq::message_t rep;
	//s->recv (&rep, 0);
	//cout << "Received " << string((char*)rep.data(), rep.size()) << endl;
*/
}

Communicate::~Communicate () { 
	listening = false;
	s->disconnect ("tcp://127.0.0.1:"+port);
	s->close();
}

void Communicate::connect (int _offset, std::string _port="5000") {
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

void Communicate::write (double f) {
///	std::cout << "Mode is " << mode << std::endl;
	if (mode==SERVER) {
		fov = f;
///		std::cout << "Setting fov = " << fov << " == " << f << std::endl;
	}
}

void Communicate::write (Vec3d v) {
	if (mode==SERVER) {
		viewdirection = v;
	}
}

///int sc = 0;

void Communicate::send () {
	if (mode!=SERVER)
		return;
/*	unsigned char data [34];
	sc = (sc+1)%255;
	data[0] = 1;
	data[33] = sc;
	data[1] = viewdirection[0] & 0xff;
	data[2] = (viewdirection[0] >> 8) & 0xff;
	data[3] = (viewdirection[0] >> 16) & 0xff;
	data[4] = (viewdirection[0] >> 24)& 0xff;
	data[5] = (viewdirection[0] >> 32)& 0xff;
	data[6] = (viewdirection[0] >> 40)& 0xff;
	data[7] = (viewdirection[0] >> 48)& 0xff;
	data[8] = (viewdirection[0] >> 56)& 0xff;
	data[9] = viewdirection[1] & 0xff;
	data[10] = (viewdirection[1] >> 8) & 0xff;
	data[11] = (viewdirection[1] >> 16) & 0xff;
	data[12] = (viewdirection[1] >> 24)& 0xff;
	data[13] = (viewdirection[1] >> 32)& 0xff;
	data[14] = (viewdirection[1] >> 40)& 0xff;
	data[15] = (viewdirection[1] >> 48)& 0xff;
	data[16] = (viewdirection[1] >> 56)& 0xff;
	data[17] = viewdirection[2] & 0xff;
	data[18] = (viewdirection[2] >> 8) & 0xff;
	data[19] = (viewdirection[2] >> 16) & 0xff;
	data[20] = (viewdirection[2] >> 24)& 0xff;
	data[21] = (viewdirection[2] >> 32)& 0xff;
	data[22] = (viewdirection[2] >> 40)& 0xff;
	data[23] = (viewdirection[2] >> 48)& 0xff;
	data[24] = (viewdirection[2] >> 56)& 0xff;
	data[25] = fov & 0xff;
	data[26] = (fov >> 8)& 0xff;
	data[27] = (fov >> 16)& 0xff;
	data[28] = (fov >> 24)& 0xff;
	data[29] = (fov >> 32)& 0xff;
	data[30] = (fov >> 40)& 0xff;
	data[31] = (fov >> 48)& 0xff;
	data[32] = (fov >> 56)& 0xff;
*/
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
	std::stringstream datass;
	datass << "1 " << viewdirection[0] << " " << viewdirection[1] << " " << viewdirection[2] << " " << fov << " ";
/*	datass << "1 " << viewdirection[0] << " " << viewdirection[1] << " " << viewdirection[2] << " ";
	if (vd1) {
		datass << "1 " << viewdirection1[0] << " " << viewdirection1[1] << " " << viewdirection1[2] << " ";
		vd1 = false;
	}
	if (f1) {
		datass << "2 " << fov1 << " ";
		f1 = false;
	}
	datass << "3 " << fov << " ";
*/
///	std::cout << "100 " << viewdirection[0] << " " << viewdirection[1] << " " << viewdirection[2] << " " << fov << std::endl;
	
	zmq::message_t mssg (datass.str().length());
	memcpy ((void*)mssg.data(), datass.str().c_str(), datass.str().length());
	s->send (mssg);

	// if (fov > 0);
	// create prime message 00000002
	// else
	// create prime message 00000001
}

void Communicate::listen () {
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
///		std::cout << "INDEX:OFF " << offset << std::endl;
//		switch(((unsigned char*)mssg.data())[0]) {
		switch(mid) {
			case 1:
				mtx.lock();

/*				viewdirection[0] = data[1] + (data[2] << 8) + (data[3] << 16) + (data[4] << 24) + (data[5] << 32) + (data[6] << 40) + (data[7] << 48) + (data[8] << 56);
				viewdirection[1] = data[9] + (data[10] << 8) + (data[11] << 16) + (data[12] << 24) + (data[13] << 32) + (data[14] << 40) + (data[15] << 48) + (data[16] << 56);
				viewdirection[2] = data[17] + (data[18] << 8) + (data[19] << 16) + (data[20] << 24) + (data[21] << 32) + (data[22] << 40) + (data[23] << 48) + (data[24] << 56);
				fov = data[24] + (data[26] << 8) + (data[27] << 16) + (data[28] << 24) + (data[29] << 32) + (data[30] << 40) + (data[31] << 48) + (data[32] << 56);
*/
/*
				memcpy(&viewdirection[0], &data[1], sizeof(double));
				memcpy(&viewdirection[1], &data[9], sizeof(double));
				memcpy(&viewdirection[2], &data[17], sizeof(double));
				memcpy(&fov, &data[25], sizeof(double));
*/
				datass >> viewdirection[0] >> viewdirection[1] >> viewdirection[2] >> fov;
/*				int id;
				datass >> id;
				datass >> viewdirection[0] >> viewdirection[1] >> viewdirection[2] >> id;
				if (id==1) {
					vd1 = true;
					datass >> viewdirection[0] >> viewdirection[1] >> viewdirection[2] >> id;
				}
				if (id==2) {
					f1 = true;
					datass >> fov >> id;
				}
				if (id==3)
					datass >> fov;
*/
				viewchanged = true;
				mtx.unlock();
//				std::cout << "DOUBLE: " << sizeof(double) << std::endl;
//				for (int i=0; i<34; i++)
//					std::cout << (int)data[i] << ", ";
//				std::cout << std::endl;
///				std::cout<<"Listening: "<< viewdirection[0] << ", "<< viewdirection[1] << ", " << viewdirection[2] << " ... " << fov << std::endl;
				break;
			case 2:
				bool atmosvis;
				datass >> atmosvis;
				GETSTELMODULE(Atmosphere)->setFlagShow(atmosvis);
				break;
		}
	}
}

bool Communicate::read (StelMovementMgr* smm) {
	if (mode!=CLIENT)
		return false;

	if (true) {
		mtx.lock();
		if(viewchanged) {
			smm->setViewDirectionJ2000(viewdirection);
/*			if (f1) {
//				smm->setCFov(fov1);
//				f1 = false;
//			}
//			if (vd1) {
//				smm->setViewDirectionJ2000(viewdirection);
//				vd1=false;
//			}
*/			smm->setCFov(fov);
			smm->setViewDirectionJ2000WithOffset(offset);
			viewchanged = false;
			mtx.unlock();
			return true;
		} else {
			std::cout << "VIEW DIR NOT AVAILABLE !!!" << std::endl;
			mtx.unlock();
			return false;
		}
	} else
		return false;
}


/* END OF USEFUL CODE */




void Communicate::read (int i, StelMovementMgr* smm) {

	//The following part needs to replace the file-read code 
	zmq::message_t rep;
	s->recv (&rep, 0);
	std::string received((char*)rep.data());
	std::stringstream recvss(received);
	int a;

	//cout << "Recvd JSON : " << received << endl;
	recvss >> a;

	if(a==0)
	{
		recvss >> v3[0] >> v3[1] >> v3[2];
		smm->setViewDirectionJ2000(v3);
		//cout << "Recvd0 " << v3[0] << " " << v3[1] << " " << v3[2] << endl;
	} else
		return;

	s->recv (&rep, 0);
	received = std::string((char*)rep.data(), rep.size());
	//cout << "Recvd JSON1 : " << received << endl;
	recvss.ignore(256);
	recvss.clear();
	recvss.str("");
	recvss << received;
	recvss >> a;
	//cout << "After Recvd JSON1 : " << a << endl;
	if(a==1)
	{	
		recvss >> fov;
		smm->setCFov(fov);			

		s->recv (&rep, 0);
		received = std::string((char*)rep.data());
		recvss.str("");
		recvss.clear();
		recvss.str(received);
		recvss >> a;
	}
	if(a==2)
	{
		recvss >> v3[0] >> v3[1] >> v3[2];
		smm->setViewDirectionJ2000(v3);

		s->recv (&rep, 0);
		received = std::string((char*)rep.data());
		recvss.str("");
		recvss.clear();
		recvss.str(received);
		recvss >> a;
	}
	if(a==3)
	{
		recvss >> fov;

		smm->setCFov(fov);
//		smm->setViewDirectionJ2000WithOffset(1);
		//cout << "Recvd1 " << fov << endl;
	}

	/*if (i==0) {
	  std::stringstream ssr;
	  ssr << prefix << "_" << current << ".str";
	  struct stat buffer;
	  if (stat (ssr.str().c_str(), &buffer) == 0) {
	  cout << "READING " << current << endl;
	  current = (current+1)%MAX_EVENTS;
	  filename = ssr.str();
	  ifile.open (ssr.str().c_str());
	  comm=true;
	  ifile >> t >> v3[0] >> v3[1] >> v3[2];
	  smm->setViewDirectionJ2000(v3);
	  }
	  } else if (comm==true) {
	  ifile >> t;
	  if (t==1) {
	  ifile >> fov;
	  smm->setCFov(fov);
	  ifile >> t;
	  }
	  if (t==2) {
	  ifile >> v3[0] >> v3[1] >> v3[2];
	  smm->setViewDirectionJ2000(v3);
	  ifile >> t;
	  }
	  if (t==3) {
	  ifile >> fov;
	  ifile.close ();
	//std::stringstream ssr;
	//ssr << "rm " << filename;
	//system(ssr.str().c_str());
	comm=false;

	smm->setCFov(fov);
	smm->setViewDirectionJ2000WithOffset(1);
	}
	}*/
}


//static Communicate& Communicate::instance () {
//	static Communicate c("rstream", Communicate::SERVER);
//	return c;
//}

void Communicate::write (int i, Vec3d v) {
	if (mode!=SERVER)
		return;
	//viewdirection = v;
	if (i==0) {
		//cout << "WRITING " << current << endl;
		std::stringstream ssw;
		ssw << prefix << "_" << current << ".str";
		current = (current+1)%MAX_EVENTS;
		//file.open (ssw.str().c_str());
	}
	//file << i << " " << v[0] << " " << v[1] << " " << v[2] << endl;

	std::stringstream reqss;
	reqss << i << " " << v[0] << " " << v[1] << " " << v[2];
	zmq::message_t req (reqss.str().length()+1);
	memcpy ((void*)req.data(), (void*)reqss.str().c_str(), reqss.str().length());
	s->send(req);


	//zmq::message_t rep;
	//s->recv (&rep, 0);
	//cout << "Received " << string((char*)rep.data(), rep.size()) << endl;
}

void Communicate::write (int i, double f) {
	if (mode!=SERVER)
		return;
	//fov = f;
	//file << i << " " << f << endl;
	if (i==3) {
		//file.close();
		//cout << "WRITTEN " << current << endl;
	}

	std::stringstream reqss;
	reqss << i << " " << f;
	zmq::message_t req (reqss.str().length()+1);
	memcpy ((void*)req.data(), reqss.str().c_str(), reqss.str().length());
	s->send (req);
}

/*
void Communicate::write1 (double f) {
	if (mode==SERVER) {
		fov1 = f;
		f1 = true;
		std::cout << "Setting fov1 = " << fov1 << std::endl;
	}
}

void Communicate::write1 (Vec3d v) {
	if (mode==SERVER) {
		viewdirection1 = v;
		vd1 = true;
	}
}
*/

