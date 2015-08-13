#include "LGCommunicate.hpp"

Communicate::Communicate (std::string _prefix) {
	MAX_EVENTS = 300;
	current = 0;
	prefix = _prefix;
	comm = false;

	fov = 60;
	viewchanged = false;
	listening = true;

	ctx = new zmq::context_t(1); 
	s = new zmq::socket_t(*ctx, ZMQ_PAIR);


	// Create thread for listen

	//zmq::message_t rep;
	//s->recv (&rep, 0);
	//cout << "Received " << string((char*)rep.data(), rep.size()) << endl;
}

Communicate::~Communicate () { 
	listening = false;
	s->disconnect ("tcp://127.0.0.1:5050");
	s->close();
}

void Communicate::connect (MODE m) {
	mode = m;
	s->connect ("tcp://127.0.0.1:5050");

	std::stringstream data;
	data<<"Stellarium Connection Established";
	zmq::message_t mssg (data.str().length()+1);
	memcpy ((void*)mssg.data(), (void *)data.str().c_str(), data.str().length()+1);
	s->send (mssg);
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

void Communicate::write (double f) {
	if (mode==SERVER)
		fov = f;
}

void Communicate::write (Vec3d v) {
	if (mode==SERVER)
		viewdirection = v;
}

void Communicate::send () {
	if (mode!=SERVER)
		return;
	unsigned char data [33];
	data[0] = 1;
	memcpy(&data[1], &viewdirection[0], sizeof(double));
	memcpy(&data[9], &viewdirection[1], sizeof(double));
	memcpy(&data[17], &viewdirection[2], sizeof(double));
	memcpy(&data[25], &fov, sizeof(double));
	zmq::message_t mssg (33);
	memcpy ((void*)mssg.data(), data, 33);
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
		switch(((char*)mssg.data())[0]) {
			case 0:
				char* data = (char*)mssg.data();
				mtx.lock();
				memcpy(&data[1], &viewdirection[0], sizeof(double));
				memcpy(&data[9], &viewdirection[1], sizeof(double));
				memcpy(&data[17], &viewdirection[2], sizeof(double));
				memcpy(&data[25], &fov, sizeof(double));
				viewchanged = true;
				mtx.unlock();
				break;
		}
	}
}

bool Communicate::read (StelMovementMgr* smm) {
	if (mode!=CLIENT)
		return false;
	if (mtx.try_lock()) {
		if(viewchanged) {
			smm->setViewDirectionJ2000(viewdirection);
			smm->setCFov(fov);
			smm->setViewDirectionJ2000WithOffset(1);
			viewchanged = false;
			mtx.unlock();
			return true;
		} else {
			mtx.unlock();
			return false;
		}
	} else
		return false;
}

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
		smm->setViewDirectionJ2000WithOffset(1);
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