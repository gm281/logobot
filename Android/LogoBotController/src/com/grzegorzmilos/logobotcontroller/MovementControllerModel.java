package com.grzegorzmilos.logobotcontroller;

import android.util.Pair;

public class MovementControllerModel {

	private static final int MAX_STEPS_PER_SECOND = 300;
	private static final long ACCELERATION_TIME_MS_TO_MAX_STEPS = 1000;
    private static final double STEERING_WHEEL_DISTANCE_M = 1;
    private static final double DRIVE_WHEEL_SEPARATION_M = 0.2;
    private static final double MAX_EVENT_INTERARRIVAL_TIME_MS = 200;
    
    private double desiredAngle;
    private double desiredSpeed;
    private long lastDesiredTimestamp;
    private double currentRightWheelSpeed; /* In steps per second */
    private double currentLeftWheelSpeed;  /* In steps per second */
    private long lastCommandTimestamp;
    
	public MovementControllerModel() {
		this.lastDesiredTimestamp = 0;
		this.desiredAngle = 0;
		this.desiredSpeed = 0;
		this.lastCommandTimestamp = 0;
		this.currentRightWheelSpeed = 0;
		this.currentLeftWheelSpeed = 0;
	}
	
	private Pair<Integer, Integer> getStepsForAngleAndTotalSteps(double desiredAngle, double desiredSteps) {
		double tan = Math.tan(desiredAngle);
		double radius;
		if (tan != 0) {
			radius = STEERING_WHEEL_DISTANCE_M / tan;
		} else {
			radius = Double.POSITIVE_INFINITY;
		}

		double rightWheelRadius = radius - DRIVE_WHEEL_SEPARATION_M / 2;
		double leftWheelRadius = radius + DRIVE_WHEEL_SEPARATION_M / 2;
		
		double stepsPerRadiusUnit =  desiredSteps / (Math.abs(rightWheelRadius) + Math.abs(leftWheelRadius));
		
		double f = desiredAngle > 0 ? 1.0 : -1.0;
		double rightWheelSteps = f * stepsPerRadiusUnit * rightWheelRadius;
		double leftWheelSteps = f * stepsPerRadiusUnit * leftWheelRadius;
		
		return new Pair<Integer, Integer>((int)rightWheelSteps, (int)leftWheelSteps);
	}

	public void touchEventAt(float x, float y) {
		this.lastDesiredTimestamp = System.currentTimeMillis();

		double desiredSpeed = Math.sqrt(x*x + y*y);
		this.desiredSpeed = desiredSpeed < 1 ? desiredSpeed : 1;

		double desiredAngle;

		if (x != 0) {
			double tan = x / y;
			desiredAngle = Math.atan(tan);
			if (y < 0) {
				desiredAngle = desiredAngle + (x > 0 ? Math.PI : -Math.PI);
			}
		} else {
			desiredAngle = 0;
		}
		this.desiredAngle = desiredAngle;
		
		//Pair<Integer, Integer> steps = getStepsForAngleAndTotalSteps(desiredAngle, MAX_STEPS_PER_SECOND);
		//System.out.println("l: "+steps.second + ", r: " + steps.first);
	}
	
	private double getDesiredAngle() {
		return this.desiredAngle;
	}
	
	private double getDesiredSpeed() {
		long currentTime = System.currentTimeMillis();
		
		if (currentTime - this.lastDesiredTimestamp > MAX_EVENT_INTERARRIVAL_TIME_MS) {
			return 0;
		}
		
		if (this.desiredSpeed < 0.05) {
			return 0;
		}
		
		return this.desiredSpeed;		
	}
	
	// TODO: this should be replaced with a model based on certain amount of power being available (sqrt model)
	private double maxAccelerationSpeed(double currentSpeed, double desiredSpeed, long accelerationTimeMS) {
		double requestedSpeedDelta = Math.abs(desiredSpeed - currentSpeed);

		double maxSpeedDelta = (double)MAX_STEPS_PER_SECOND * accelerationTimeMS / ACCELERATION_TIME_MS_TO_MAX_STEPS;
		if (requestedSpeedDelta <= maxSpeedDelta) {
			return desiredSpeed;
		}

		return currentSpeed + (desiredSpeed > currentSpeed ? maxSpeedDelta : -maxSpeedDelta);
	}
	
	public Pair<Integer, Integer> getMovementCommand(int commandDurationMS) {
		long currentTime = System.currentTimeMillis();
		long timeSinceLastCommand = currentTime - this.lastCommandTimestamp; 
		this.lastCommandTimestamp = currentTime; 
		if (timeSinceLastCommand > 500) {
			this.currentLeftWheelSpeed = 0;
			this.currentRightWheelSpeed = 0;
			timeSinceLastCommand = 100;
		}

		double desiredSpeed = getDesiredSpeed();
		double desiredSteps = 2 * desiredSpeed * MAX_STEPS_PER_SECOND * commandDurationMS / 1000; 
		double desiredAngle = getDesiredAngle();
		System.out.println("Desired speed: "+desiredSpeed + ", desiredSteps: " + desiredSteps + ", desiredAngle: " + desiredAngle);
		
		Pair<Integer, Integer> steps = getStepsForAngleAndTotalSteps(desiredAngle, desiredSteps);
		double desiredRightWheelSpeed = 1000 * steps.first / commandDurationMS;
		double desiredLeftWheelSpeed = 1000 * steps.second / commandDurationMS;
		System.out.println("Desired rightSteps: " + steps.first + "("+ desiredRightWheelSpeed+"), leftSteps: " + steps.second+"(" + desiredLeftWheelSpeed+")");

		this.currentRightWheelSpeed = maxAccelerationSpeed(this.currentRightWheelSpeed, desiredRightWheelSpeed, timeSinceLastCommand);
		this.currentLeftWheelSpeed = maxAccelerationSpeed(this.currentLeftWheelSpeed, desiredLeftWheelSpeed, timeSinceLastCommand);

		int rightWheelSteps = (int)(this.currentRightWheelSpeed * commandDurationMS / 1000);
		int leftWheelSteps = (int)(this.currentLeftWheelSpeed * commandDurationMS / 1000);
		
		return new Pair<Integer, Integer>(rightWheelSteps, leftWheelSteps);
	}
}
