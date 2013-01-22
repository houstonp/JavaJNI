package za.co.technodev.javajni;

import za.co.technodev.exception.InvalidTypeException;
import za.co.technodev.exception.NotExistingKeyException;
import android.os.Handler;

/*
 * A Handler is a message queue associated with the thread it was created on (here, it will be
 * the UI thread). Any message posted from whatever thread is received in an internal queue processed
 * magically on the initial thread. Handlers are a popular and easy inter=thread communication
 * technique on Android.
 * 
 * Declare a delegate StoreListener to which messages (that is, a method call) received from the
 * watcher thread are going to be posted. This will be the StoreActivity.
 */

public class Store implements StoreListener {
	static {
		System.loadLibrary("store");
	}

	private Handler mHandler;
	private StoreListener mDelegateListener;

	public Store(StoreListener pListener) {
		mHandler = new Handler();
		mDelegateListener = pListener;
	}
	
	@Override
	public void onAlert(final int pValue) {
		mHandler.post(new Runnable() {
			public void run() {
				mDelegateListener.onAlert(pValue);
			}
		});
	}
	
	@Override
	public void onAlert(final String pValue) {
		mHandler.post(new Runnable() {
			public void run() {
				mDelegateListener.onAlert(pValue);
			}
		});
	}
	
	@Override
	public void onAlert(final Color pValue) {
		mHandler.post(new Runnable() {
			public void run() {
				mDelegateListener.onAlert(pValue);
			}
		});
	}	
	
	public native void initializeStore();
	public native void finalizeStore();
	
	public native synchronized int getInteger(String pKey) throws NotExistingKeyException, InvalidTypeException;
	public native synchronized void setInteger(String pKey, int pInt);
	
	public native synchronized String getString(String pKey) throws NotExistingKeyException, InvalidTypeException;
	public native synchronized void setString(String pKey, String pString);
	
	public native synchronized Color getColor(String pKey) throws NotExistingKeyException, InvalidTypeException;
	public native synchronized void setColor(String pKey, Color pColor);
	
	public native synchronized int[] getIntegerArray(String pKey) throws NotExistingKeyException;
	public native synchronized void setIntegerArray(String pKey, int[] pIntArray);
	
	public native synchronized Color[] getColorArray(String pKey) throws NotExistingKeyException;
	public native synchronized void setColorArray(String pKey, Color[] pColorArray);
}