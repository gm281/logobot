package com.grzegorzmilos.logobotcontroller;

import android.os.Bundle;
import android.os.Handler;
import android.util.Pair;
import android.app.Activity;

public class MovementControllerActivity extends Activity {
	
	private static final int SINGLE_COMMAND_DURATION_MS = 1000;
	private MovementControllerModel movementModel;
	private MovementControllerView movementView;
	private Handler timerHandler;
	private boolean noEvent;

	public MovementControllerActivity() {
		movementModel = new MovementControllerModel();
		movementView = null;
		timerHandler = new Handler();
		noEvent = true;
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.setTitle(R.string.title);
		this.movementView = new MovementControllerView(this);
		setContentView(this.movementView);
		timerHandler.postDelayed(timerRunnable, 50);
	}
	
	public void touchEventAt(float x, float y) {
		noEvent = false;
		double desiredSpeed = movementModel.touchEventAt(x,y);
		movementView.setDesiredSpeed(desiredSpeed);
	}
    
	private Runnable timerRunnable = new Runnable () {
    	@Override
    	public void run() {
    		System.out.println("TH");

    		if (noEvent && movementView != null) {
    			movementView.centerToOrigin();
    		}    		
    		noEvent = true;
    		Pair<Integer, Integer> movementCommand = movementModel.getMovementCommand(SINGLE_COMMAND_DURATION_MS);

    		System.out.println("Moving by: l:" + movementCommand.first + ", r:" + movementCommand.second);
    		timerHandler.postDelayed(timerRunnable, 100);
    	}
    };
	
}
