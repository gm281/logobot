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
	
	public void setDesiredSpeed(double desiredSpeed) {
		if (movementView != null) {
			movementView.setDesiredSpeed(desiredSpeed);
		}
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
	

}
