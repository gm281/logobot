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

	private MovementControllerActivity controller;
	private Paint testPaint = new Paint();
	private float x;
	private float y;
	
	public MovementControllerView(MovementControllerActivity controller) {
		this(controller, null);
		this.controller = controller;
	}

	public MovementControllerView(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
	}

	public MovementControllerView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		
		testPaint.setColor(Color.RED);
		testPaint.setStrokeWidth(5);
		testPaint.setStyle(Style.STROKE);
		testPaint.setAntiAlias(true);

		setBackgroundColor(Color.MAGENTA);
		this.setOnTouchListener(this);
	}

	public boolean onTouch(View v, MotionEvent event) {
		controller.touchEventAt(event.getX(0), event.getY(0));
		this.invalidate();
		return true;
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		super.onDraw(canvas);
		canvas.drawCircle(x, y, 20.0f, testPaint);
	}

}
