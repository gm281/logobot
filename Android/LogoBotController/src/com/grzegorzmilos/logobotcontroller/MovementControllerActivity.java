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

	public MovementControllerActivity() {
		movementModel = new MovementControllerModel();
		timerHandler = new Handler();
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
		movementModel.touchEventAt(x,y);
	}
    
	private Runnable timerRunnable = new Runnable () {
    	@Override
    	public void run() {
    		System.out.println("TH");
    		
    		Pair<Integer, Integer> movementCommand = movementModel.getMovementCommand(SINGLE_COMMAND_DURATION_MS);
    		System.out.println("Moving by: l:" + movementCommand.first + ", r:" + movementCommand.second);
    		timerHandler.postDelayed(timerRunnable, 100);
    	}
    };
	
}
