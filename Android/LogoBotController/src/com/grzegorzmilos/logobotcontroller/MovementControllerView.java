package com.grzegorzmilos.logobotcontroller;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class MovementControllerView extends View implements View.OnTouchListener {

	private static final float ORIGIN_X = 0.5f;
	private static final float ORIGIN_Y = 0.66f;
	private MovementControllerController controller;
	private Paint circlePaint = new Paint();
	private Paint innerCirclePaint = new Paint();
	private Paint linePaint = new Paint();
	private float x;
	private float y;
	private static final float MIN_CIRCLE_RADIUS = 50;
	private float circleRadius;
	
	public MovementControllerView(Context context, MovementControllerController controller) {
		this(context, (AttributeSet)null);
		this.controller = controller;
	}

	public MovementControllerView(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
	}

	public MovementControllerView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		
		circlePaint.setColor(Color.RED);
		circlePaint.setStrokeWidth(5);
		circlePaint.setStyle(Style.STROKE);
		circlePaint.setAntiAlias(true);
		innerCirclePaint.setColor(Color.GRAY);
		innerCirclePaint.setStrokeWidth(5);
		innerCirclePaint.setStyle(Paint.Style.FILL);
		innerCirclePaint.setAntiAlias(true);
		linePaint.setColor(Color.GREEN);
		linePaint.setStrokeWidth(5);
		linePaint.setStyle(Style.STROKE);
		linePaint.setAntiAlias(true);

		setBackgroundColor(Color.BLACK);
		this.setOnTouchListener(this);
	}

	public boolean onTouch(View v, MotionEvent event) {
		// TODO: pay attention to event.getPointerCount()
		//System.out.println("==> " + event.getAction() +  ": " + event.getPointerCount());
		int pointerIndex = 0;
		float x = event.getX(pointerIndex);
		float y = event.getY(pointerIndex);
		this.x = x;
		this.y = y;
		/* Convert x and y to relative coordinates, for the Controller/Model. */
		float height = this.getHeight();
		float width = this.getWidth();
		x -= ORIGIN_X * width;
		x /= ORIGIN_X * width;
		y -= ORIGIN_Y * height;
		y /= ORIGIN_Y * height;
		y *= -1;
		
		boolean downEvent = true;
		if (event.getAction() == MotionEvent.ACTION_UP) {
			downEvent = false;
		} else {
			if (event.getAction() != MotionEvent.ACTION_DOWN &&
				event.getAction() != MotionEvent.ACTION_MOVE) {
				System.out.println("Event type nethinge DOWN or UP or MOVE\n");
			}
		}
			
		controller.touchEventAt(downEvent, x, y);
		this.invalidate();
		return true;
	}
	
	public void centerToOrigin() {
		float height = this.getHeight();
		float width = this.getWidth();
		this.x = ORIGIN_X * width;
		this.y = ORIGIN_Y * height;
		setDesiredSpeed(0);
		this.invalidate();
	}
	
	public int setDesiredSpeed(double desiredSpeed) {
		double desiredColor = 1 - desiredSpeed;
		float[] hsvColor = new float[3];
		hsvColor[0] = (float) (360.0 * 3/4 * desiredColor);
		hsvColor[1] = (float)1.0; 
		hsvColor[2] = (float)1.0; 
		int color = Color.HSVToColor(hsvColor);
		circlePaint.setColor(color);
		circleRadius = (float) (MIN_CIRCLE_RADIUS + desiredSpeed * MIN_CIRCLE_RADIUS); 

		return color;
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);
		float height = this.getHeight();
		float width = this.getWidth();

		float relative_x = this.x - ORIGIN_X * width;
		float relative_y = this.y - ORIGIN_Y * height;
		float circleDistance = (float)Math.sqrt(relative_x * relative_x + relative_y * relative_y);
		double alpha = Math.asin(this.circleRadius / circleDistance);
		double beta = Math.asin(relative_y / circleDistance);
		float tangentLength = (float)Math.sqrt(circleDistance * circleDistance - this.circleRadius * this.circleRadius);
		float deltaX1 = (float) (Math.cos(beta - alpha) * tangentLength) * (relative_x > 0 ? 1 : -1);
		float deltaY1 = (float) (Math.sin(beta - alpha) * tangentLength);
		float deltaX2 = (float) (Math.cos(beta + alpha) * tangentLength) * (relative_x > 0 ? 1 : -1);
		float deltaY2 = (float) (Math.sin(beta + alpha) * tangentLength);
		canvas.drawLine(ORIGIN_X * width + deltaX1, ORIGIN_Y * height + deltaY1, ORIGIN_X * width, ORIGIN_Y * height, this.circlePaint);
		canvas.drawLine(ORIGIN_X * width + deltaX2, ORIGIN_Y * height + deltaY2, ORIGIN_X * width, ORIGIN_Y * height, this.circlePaint);
		canvas.drawCircle(this.x, this.y, this.circleRadius, circlePaint);
		canvas.drawCircle(this.x, this.y, 5.0f, innerCirclePaint);
	}

}
