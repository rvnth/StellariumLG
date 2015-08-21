package stellarium.client;

import interactivespaces.activity.impl.BaseActivity;
import interactivespaces.activity.impl.ros.BaseRoutableRosActivity;
import interactivespaces.activity.binary.NativeActivityRunner;
import interactivespaces.activity.impl.BaseActivity;
import interactivespaces.util.process.NativeApplicationRunner;
import interactivespaces.util.process.NativeApplicationRunnerCollection;

import java.lang.Object;
import java.io.ObjectOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

import com.google.common.collect.Maps;

import java.util.Map;

import java.net.URL;
import java.net.URLClassLoader;

import org.zeromq.ZMQ;
/**
 * The client activity for Stellarium on Liquid Galaxy
 */
public class StellariumClientActivity extends BaseRoutableRosActivity {
	
	private String bindTo;
	private ZMQ.Context context;
	private ZMQ.Socket socket;
	public static final String CONFIGURATION_APPLICATION_EXECUTABLE = "space.nativeapplication.executable";
	public static final String CONFIGURATION_APPLICATION_EXECUTABLE_FLAGS = "space.nativeapplication.executable.flags";
	private NativeApplicationRunnerCollection nativeAppRunnerCollection;

    @Override
    public void onActivitySetup() {
        getLog().info("Setting up Stellarium Client Activity");
       	ClassLoader cl = ClassLoader.getSystemClassLoader();
	URL[] urls = ((URLClassLoader)cl).getURLs();
	for(URL url: urls){
		System.out.println(url.getFile());
	}

	nativeAppRunnerCollection = new NativeApplicationRunnerCollection(getSpaceEnvironment(), getLog());
	addManagedResource(nativeAppRunnerCollection);

	// Set up socket
	bindTo = "tcp://127.0.0.1:5000"; //ip:port for clientActivity <-> Stellarium
	context = ZMQ.context(1);
	socket = context.socket(ZMQ.PAIR);
    }

    @Override
    public void onActivityStartup() {
        getLog().info("Stellarium Client Activity started !");
	Map<String, Object> config = Maps.newHashMap();
	config.put(NativeApplicationRunner.EXECUTABLE_PATHNAME,getConfiguration().getRequiredPropertyString(CONFIGURATION_APPLICATION_EXECUTABLE));
	config.put(NativeApplicationRunner.EXECUTABLE_FLAGS,getConfiguration().getRequiredPropertyString(CONFIGURATION_APPLICATION_EXECUTABLE_FLAGS));
	NativeApplicationRunner runner = nativeAppRunnerCollection.newNativeApplicationRunner();
	runner.configure(config);

	nativeAppRunnerCollection.addNativeApplicationRunner(runner);

	// Bind the socket
	socket.bind(bindTo);
    }

    @Override
    public void onNewInputJson(String channelName, Map<String, Object> message) {
        getLog().debug("Received message on input channel '" + channelName + "': " + message);

	String msg = message.get("s").toString();

	//Send message to Stellarium
	socket.send(msg, 0);
    }

    @Override
    public void onActivityShutdown() {
        getLog().info("Stellarium Client Activity shutting down !");
   	
	//Unbind Socket
	socket.unbind(bindTo);
    }
}

