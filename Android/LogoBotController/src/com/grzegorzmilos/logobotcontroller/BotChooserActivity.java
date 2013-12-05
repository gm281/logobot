package com.grzegorzmilos.logobotcontroller;

import java.io.IOException;

import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceEvent;
import javax.jmdns.ServiceListener;

public class BotChooserActivity extends Activity {

    WifiManager.MulticastLock lock;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_bot_chooser);
		new DiscoverZeroConfBots().execute();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.bot_chooser, menu);
		return true;
	}

    private static final String type = "_fish._tcp.local.";
    private ServiceListener listener;
    private JmDNS jmdns;

    private class DiscoverZeroConfBots extends AsyncTask<Void, Void, Void> {

		@Override
		protected Void doInBackground(Void... params) {
	        WifiManager wifi = (WifiManager) getSystemService(android.content.Context.WIFI_SERVICE);
	        lock = wifi.createMulticastLock("logobotlock");
	        lock.setReferenceCounted(true);
	        lock.acquire();
	        try {
	            jmdns = JmDNS.create();
	            jmdns.addServiceListener(type, listener = new ServiceListener() {

	                @Override
	                public void serviceResolved(ServiceEvent ev) {
	                    String additions = "";
	                    if (ev.getInfo().getInetAddresses() != null && ev.getInfo().getInetAddresses().length > 0) {
	                        additions = ev.getInfo().getInetAddresses()[0].getHostAddress();
	                    }
	                    System.out.println("Service resolved: " + ev.getInfo().getQualifiedName() + " port:" + ev.getInfo().getPort() + " :" + additions);
	                }

	                @Override
	                public void serviceRemoved(ServiceEvent ev) {
	                    System.out.println("Service removed: " + ev.getName());
	                }

	                @Override
	                public void serviceAdded(ServiceEvent event) {
	                    // Required to force serviceResolved to be called again (after the first search)
	                    jmdns.requestServiceInfo(event.getType(), event.getName(), 1);
	                }
	            });
	        } catch (IOException e) {
	        	System.out.println("jmDNS discovery failed with an exception: " + e);
	            e.printStackTrace();
	        }

			return null;
		}
    }
    
    @Override
    protected void onStop()
    {
        if (jmdns != null) {
        	if (listener != null) {
        		jmdns.removeServiceListener(type, listener);
        		listener = null;
        	}
        	jmdns.unregisterAllServices();
        	try {
        		jmdns.close();
        	} catch (IOException e) {
        		// TODO Auto-generated catch block
        		e.printStackTrace();
        	}
        	jmdns = null;
        }
        lock.release();
        super.onStop();
    }
}
 