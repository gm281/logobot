package com.grzegorzmilos.logobotcontroller;

import android.os.Bundle;
import android.view.Window;
import android.app.Activity;
import android.app.FragmentManager;


public class MovementControllerActivity extends Activity {
	
	private MovementControllerView movementView;
	private MovementControllerController controller;

	public MovementControllerActivity() {
		movementView = null;
	}
	
	public int setDesiredSpeed(double desiredSpeed) {
		if (movementView != null) {
			return movementView.setDesiredSpeed(desiredSpeed);
		}
		return 0; 
	}
	public void centerToOrigin() {
		if (movementView != null) {
			movementView.centerToOrigin();
		}
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.requestWindowFeature(Window.FEATURE_NO_TITLE);
		this.setTitle(R.string.title);
        FragmentManager fm = getFragmentManager();
        controller = (MovementControllerController) fm.findFragmentByTag("controller");
 
        // If the Fragment is not null, then it is currently being
        // retained when a configuration change occurs.
        if (controller == null) {
            controller = new MovementControllerController();
            fm.beginTransaction().add(controller, "controller").commit();
        }	
		this.movementView = new MovementControllerView(this, controller);
		setContentView(this.movementView);
	}

	@Override
	public void onStart() {
		super.onStart();
		System.out.println("======== STARTING =========");
	}
		
	@Override
	public void onStop() {
		super.onStop();
		System.out.println("======== STOPPING =========");
	}

	@Override
	public void onPause() {
		super.onPause();
		System.out.println("======== PAUSING =========");
	}
	
	@Override
	public void onRestart() {
		super.onRestart();
		System.out.println("======== RESTARTING =========");
	}

	@Override
	public void onResume() {
		super.onResume();
		System.out.println("======== RESUMING =========");
	}
}
