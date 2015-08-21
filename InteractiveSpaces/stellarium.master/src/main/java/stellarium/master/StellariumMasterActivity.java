package stellarium.master;

import interactivespaces.activity.impl.BaseActivity;
import interactivespaces.activity.impl.ros.BaseRoutableRosActivity;
import interactivespaces.activity.binary.NativeActivityRunner;
import interactivespaces.activity.impl.BaseActivity;
import interactivespaces.util.process.NativeApplicationRunner;
import interactivespaces.util.process.NativeApplicationRunnerCollection;

import org.zeromq.ZMQ;
import java.lang.Object;
import java.io.ObjectInputStream;
import java.io.ByteArrayInputStream;
import java.io.IOException;

import com.google.common.collect.Maps;

import java.util.Map;

import java.net.URL;
import java.net.URLClassLoader;

/**
 * The master activity for Stellarium on Liquid Galaxy
 */
public class StellariumMasterActivity extends BaseRoutableRosActivity {

	private String bindTo;
	private ZMQ.Context context;
	private ZMQ.Socket socket;
	private Thread stellariumListener;
	public static final String CONFIGURATION_APPLICATION_EXECUTABLE = "space.nativeapplication.executable";
	public static final String CONFIGURATION_APPLICATION_EXECUTABLE_FLAGS = "space.nativeapplication.executable.flags";
	private NativeApplicationRunnerCollection nativeAppRunnerCollection;

    @Override
    public void onActivitySetup() {
        getLog().info("Setting up Stellarium Master Activity");
	ClassLoader cl = ClassLoader.getSystemClassLoader();
	URL[] urls = ((URLClassLoader)cl).getURLs();
	for(URL url: urls){
		System.out.println(url.getFile());
	}

	nativeAppRunnerCollection = new NativeApplicationRunnerCollection(getSpaceEnvironment(), getLog());
	addManagedResource(nativeAppRunnerCollection);

	// Set up socket!
	bindTo = "tcp://127.0.0.1:5000"; //ip:port for masterActivity <-> Stellarium
	context = ZMQ.context(1);
	socket = context.socket(ZMQ.PAIR);

	// Initialize thread which listens to Stellarium
	stellariumListener = new Thread(new Runnable() {
				                        public void run() {
								getLog().info("Started listening to Stellarium...");
								try {
									int i=0;
									String Params="";
									while(true)
									{
										byte [] req = socket.recv(0);
										Params = new String(req);

										//when Stellarium communicates an update, send it to all the clients:
										Map<String, Object> message = Maps.newHashMap();
										message.put("s", Params);
										sendOutputJson("MasterToClientChannel", message);
									}
								} catch (Exception e) {e.printStackTrace();} 
							}
						});
    }

    @Override
    public void onActivityStartup() {
        getLog().info("Stellarium Master Activity started !");

	Map<String, Object> config = Maps.newHashMap();
	config.put(NativeApplicationRunner.EXECUTABLE_PATHNAME,getConfiguration().getRequiredPropertyString(CONFIGURATION_APPLICATION_EXECUTABLE));
	config.put(NativeApplicationRunner.EXECUTABLE_FLAGS,getConfiguration().getRequiredPropertyString(CONFIGURATION_APPLICATION_EXECUTABLE_FLAGS));
	NativeApplicationRunner runner = nativeAppRunnerCollection.newNativeApplicationRunner();
	runner.configure(config);

	nativeAppRunnerCollection.addNativeApplicationRunner(runner);

	// Bind the socket
	socket.bind(bindTo);

	// Start Stellarium Listener
	stellariumListener.start();
    }

    @Override
    public void onActivityShutdown() {
        getLog().info("Stellarium Master Activity shutting down !");

	//Unbind Socket
	socket.unbind(bindTo);
    }

}

