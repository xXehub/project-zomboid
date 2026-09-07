package mod.lua;

import se.krka.kahlua.vm.KahluaThread;

public class LuaUtil {

	public static boolean isStackFromLua() {
		StackTraceElement[] stackTrace = Thread.currentThread().getStackTrace();

        boolean fromLua = false;
        String suspiciousLuaFile = "Unknown";

        for (int i = 0; i < stackTrace.length; i++) {
            String className = stackTrace[i].getClassName();

            if (className.contains("se.krka.kahlua")) {
                fromLua = true;
                suspiciousLuaFile = stackTrace[i].toString();
                break;
            }
        }

        return fromLua;
	}
}
