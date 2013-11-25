package com.grzegorzmilos.logobotcontroller;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import android.app.Activity;
import android.app.Fragment;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.util.Pair;

public class MovementControllerController extends Fragment {

	private static final String ROBOT_ADDRESS = "192.168.0.108";
	private static final int ROBOT_PORT = 12348;
	private static final int SINGLE_COMMAND_DURATION_MS = 1000;

	private MovementControllerActivity currentActivity;
	private MovementControllerModel movementModel;

	private Handler timerHandler;
	private boolean noEvent;
	private boolean moving;
	private Socket connectionSocket;
	
	public MovementControllerController() {
		movementModel = new MovementControllerModel();
		timerHandler = new Handler();
		noEvent = true;
		moving = false;
	}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
 
        /* Retain this fragment across configuration changes. */
        setRetainInstance(true);

    	new OpenRobotConnection().execute();
		timerHandler.postDelayed(timerRunnable, 50);
    }
 
    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        currentActivity = (MovementControllerActivity)activity;
    }

	public void touchEventAt(float x, float y) {
		noEvent = false;
		double desiredSpeed = movementModel.touchEventAt(x,y);
		currentActivity.setDesiredSpeed(desiredSpeed);
	}
    
    private class OpenRobotConnection extends AsyncTask<Void, Void, Boolean> {

		@Override
		protected Boolean doInBackground(Void... params) {
            try {
                InetAddress serverAddr = InetAddress.getByName(ROBOT_ADDRESS);
                connectionSocket = new Socket(serverAddr , ROBOT_PORT);
            } catch (UnknownHostException e1) {
            	return false;
            } catch (IOException e1) {
            	return false;
            }
            return true;
		}
    }

    class MovementCommand {
    	int rightWheelSteps;
    	int leftWheelSteps;
    	int duriationMS;
    	
    	public MovementCommand(int rightWheelSteps, int leftWheelSteps, int durationMS) {
    		this.rightWheelSteps = rightWheelSteps;
    		this.leftWheelSteps = leftWheelSteps;
    		this.duriationMS = durationMS;
    	}
    }

    private class SendMovementCommandTask extends AsyncTask<MovementCommand, Void, Boolean> {

		@Override
		protected Boolean doInBackground(MovementCommand... movementCommands) {
			MovementCommand movementCommand = movementCommands[0];
    		//System.out.println("Moving by: l:" + movementCommand.leftWheelSteps + ", r:" + movementCommand.rightWheelSteps);
    		String commandString = "m" + movementCommand.rightWheelSteps + "," + movementCommand.leftWheelSteps + "," + movementCommand.duriationMS + "$";
    		byte[] commandBytes = commandString.getBytes();
    		try {
    			OutputStream out = connectionSocket.getOutputStream();
    			out.write(commandBytes);
    			out.flush();
    		} catch (Exception e) {
    			return false;
    		}
			return true;
		}
    }
    
    private Runnable timerRunnable = new Runnable () {
    	@Override
    	public void run() {
    		if (noEvent && currentActivity != null) {
    			currentActivity.centerToOrigin();
    		}    		
    		noEvent = true;
    		Pair<Integer, Integer> steps = movementModel.getMovementCommand(SINGLE_COMMAND_DURATION_MS);

    		if (steps.first == 0 && steps.second == 0) {
    			if (moving == false) {
    				timerHandler.postDelayed(timerRunnable, 100);
    				return;
    			}
    			moving = false;
    		} else {
    			moving = true;
    		}
    		MovementCommand command = new MovementCommand(steps.first, steps.second, SINGLE_COMMAND_DURATION_MS);
    		new SendMovementCommandTask().execute(command);

    		timerHandler.postDelayed(timerRunnable, 100);
    	}
    };
}
