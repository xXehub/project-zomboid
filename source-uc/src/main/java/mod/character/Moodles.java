package mod.character;

import net.bytebuddy.asm.Advice;
import zombie.scripting.objects.MoodleType;

public class Moodles {

	@Advice.OnMethodExit
	public static void getMoodleLevel(@Advice.Argument(0) MoodleType moodleType,
			@Advice.Return(readOnly = false) int ogReturn) {		
		if(moodleType == MoodleType.ENDURANCE || moodleType == MoodleType.HEAVY_LOAD 
				|| moodleType == MoodleType.PANIC || moodleType == MoodleType.STRESS || moodleType == MoodleType.BORED 
				|| moodleType == MoodleType.INJURED || moodleType == MoodleType.UNHAPPY || moodleType == MoodleType.NOXIOUS_SMELL 
				|| moodleType == MoodleType.CANT_SPRINT || moodleType == MoodleType.UNCOMFORTABLE) {			
			ogReturn = 0;
		}
		
	}
	
	public static class updateMoodleLevel{
		@Advice.OnMethodExit
		public static void run(@Advice.FieldValue(value = "moodleType", readOnly = false) MoodleType moodleType,
				@Advice.FieldValue(value = "moodleLevel", readOnly = false) int moodleLevel) {
			if(moodleType.equals(MoodleType.HEAVY_LOAD)) {
				moodleLevel = 0;
			}
		}
	}
}
