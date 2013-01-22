package za.co.technodev.javajni;

/*
 * Define methods through which native code is going to communicate with Java code
 */

public interface StoreListener {
	public void onAlert(int pValue);
	
	public void onAlert(String pValue);
	
	public void onAlert(Color pValue);
}
