package za.co.technodev.exception;

@SuppressWarnings("serial")
public class InvalidTypeException extends Exception {
	public InvalidTypeException(String pDetailMessage) {
		super(pDetailMessage);
	}
}
