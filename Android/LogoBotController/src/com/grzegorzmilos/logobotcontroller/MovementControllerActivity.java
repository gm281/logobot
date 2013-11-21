package com.grzegorzmilos.logobotcontroller;

import android.os.Bundle;
import android.app.Activity;

public class MovementControllerActivity extends Activity {
	
	private MovementControllerModel movementModel;
	private MovementControllerView movementView;

	public MovementControllerActivity() {
		movementModel = new MovementControllerModel();
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.setTitle(R.string.title);
		this.movementView = new MovementControllerView(this);
		setContentView(this.movementView);
	}
	
	public void touchEventAt(float x, float y) {
		movementModel.touchEventAt(x,y);
	}
}
