package mod.weather;

import net.bytebuddy.asm.Advice;
import net.bytebuddy.implementation.bind.annotation.RuntimeType;
import zombie.scripting.objects.CharacterTrait;

public class ClimateManager {
	
	public static class GetNightStrength{
		@Advice.OnMethodExit
		public static void run(@Advice.Return(readOnly = false) float ogValue) {
			if(ogValue > 0.0f) {
				ogValue = ogValue / 3;				
			}
		}
	}
}
