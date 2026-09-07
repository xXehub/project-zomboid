package mod.debug;

public class Debug {
	public static boolean active = false;
	
	public static void Log(String message) {
		if(active) {
			System.out.println(message);
		}
	}
}
