-------------------------------------------------------------------------------
Installation
-------------------------------------------------------------------------------
Stellarium on Liquid Galaxy requires two basic components to be installed:
- LG version of Stellarium
- Interactive Spaces

1. LG version of Stellarium (core - 0.13.3)

   Dependencies :
     - ZeroMQ. Download and install from http://zeromq.org
     - jzmq - Java binding for ZeroMQ. 
              Download and install from http://zeromq.org/bindings:java
     - Qt 5.3

   Installation :
   - Download and install ZeroMQ and jzmq.
   - Download Stellarium from https://github.com/rvnth/StellariumLG
   - Follow the installation instructions provided with Stellarium
      $ cd StellariumLG/Stellarium
      $ mkdir build
      $ cd build
      $ cmake ..
      $ make
      $ sudo make install

2. Interactive Spaces
   
   Installation :
   - Download and install Interactive Spaces from
     http://www.interactive-spaces.org
   - Follow the isntallation instructions provided on the website.
   - On Master node, install and configure the interactive spaces master,
     controller and workbench.
   - On Slave nodes, install and configure the interactive spaces controller.
   - Use the workbench compiler on the master node to create OSGi bundles of
     zmq.jar and zmq-perf.jar files
     (usually found at /usr/local/share/java/ and /usr/local/share/perf)
   - Copy the OSGi bundles to the bootstrap directory of every individual
     controller. More information about creating OSGi bundles is provided in the
     Interactive Spaces documentation.

--------------------------------------------------------------------------------
Configuration
--------------------------------------------------------------------------------

- The path to stellarium (generally /usr/local/bin/stellarium) should be updated
  in the project.xml file in "StellariumLG/InteractiveSpaces/stellarium.master"
  and "StellariumLG/InteractiveSpaces/stellarium.client" directories.
- Build both these activities using the utilities provided by Interactive Spaces
- Start the Interactive Spaces Master on the master node.
- Start the Interactive Spaces Controller on all the nodes including the master.
- Upload the Stellarium Master and Client activities using the admin console of
  Interactive Spaces on the master node.
- Create and deploy a Stellarium Master live activity on the controller of the
  master node.
- Edit the configuration flags on Master using the edit config window:
   space.nativeapplication.executable.flags=-f yes -lg SERVER --lg-offset 0
- Create and deploy Stellarium Client live activities on the client controllers.
- Edit the configuration flags on Clients using the edit config window:
   space.nativeapplication.executable.flags=-f yes -lg CLIENT --lg-offset -1
  "--lg-offset -1" marks the client node to the left of the master.
  "--lg-offset -2" marks the second client node to the left of the master.
  "--lg-offset 1" marks the client node to the right of the master. And so on
- Make sure to click the "Configure" button after editing the configuration
  flags.
- Start the Stellarium Live activities on all the controllers using the
  Interactive Spaces admin console to have a multiview Stellarium.
- You should be able to control the views and modes from master node with an
  immersive experience.


