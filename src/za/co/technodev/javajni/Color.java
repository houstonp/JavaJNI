package za.co.technodev.javajni;

public class Color {
	private int mColor;
	
	public Color(String pColor) {
		super();
		mColor = android.graphics.Color.parseColor(pColor);
	}
	
	@Override
	public String toString() {
		return String.format("#%06X", mColor);
	}
	
	@Override
	public int hashCode() {
		return mColor;
	}
	
	@Override
	public boolean equals(Object pOther) {
		if (this == pOther) {return true;}
		if (pOther == null) {return false;}
		if (getClass() != pOther.getClass()) {return false;}
		Color pColor = (Color) pOther;
		return (mColor == pColor.mColor);
	}
}