package mod.iso;

import mod.debug.Debug;
import mod.lua.LuaEventManager;
import net.bytebuddy.asm.Advice;
import zombie.core.textures.ColorInfo;

public class IsoGridSquare {
	public static class GetLightInfo{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) ColorInfo ogValue) {			 
			if(LuaEventManager.toggleNightVision) {
				ColorInfo colorInfo = (ColorInfo) ogValue;
				colorInfo.r = 1.0f;
				colorInfo.g = 1.0f;
				colorInfo.b = 1.0f;
			}
		}
	}
	public static class isCanSee{
		public static void run(@Advice.Argument(0) int playerIndex, @Advice.Return(readOnly = false) boolean canSee) {
			if(playerIndex > 0) {
				canSee = true;
			}
		}		
	}
}
